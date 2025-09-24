#include "displayUtils.h"

#include "services/display.h"

static const int scrollWidth = 6;
static const int scrollHeight = Display::HEIGHT - Display::StatusbarHeight;

void drawScrollBar(int currentOption, int numOptions) {
    // Always use direct drawing to avoid memory allocation
    float scrollPercent = (float)currentOption / (numOptions);
    int scrollPosition = scrollPercent * (scrollHeight - 20);

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        tft.fillRect(Display::WIDTH - scrollWidth, Display::StatusbarHeight,
                     scrollWidth, scrollHeight, Colors::black);

        // Track
        tft.drawFastVLine(Display::WIDTH - (scrollWidth / 2),
                          Display::StatusbarHeight + Display::Padding::P0,
                          scrollHeight - Display::Padding::P1,
                          Colors::bgGray900);

        // Thumb
        tft.fillRoundRect(Display::WIDTH - scrollWidth,
                          Display::StatusbarHeight + scrollPosition,
                          scrollWidth, 20, 3, Colors::white);
        xSemaphoreGive(displayMutex);
    }
}

void clearScreen() {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        tft.fillScreen(Colors::black);
        xSemaphoreGive(displayMutex);
    }
}

void clearPage(bool clearStatusbar) {
    if (clearStatusbar) {
        return clearScreen();
    }

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        tft.fillRect(0, Display::PageY, Display::WIDTH, Display::PageHeight,
                     Colors::black);
        // Also clear top left and top right corners to remove buttons
        tft.fillRect(0, 0, 75, Display::StatusbarHeight, Colors::black);
        tft.fillRect(Display::WIDTH - 75, 0, 75, Display::StatusbarHeight,
                     Colors::black);

        xSemaphoreGive(displayMutex);
    }
}

void wrapText(Adafruit_GFX &gfx, const String &text,
              const WrapTextProps &props = WrapTextProps()) {
    const int16_t availableWidth = gfx.width() - props.x - props.rightPadding;

    // Ensure cursor starts at provided coordinates
    gfx.setCursor(props.x, props.y);

    String currentLine;

    auto printAndNewline = [&](const String &line) {
        if (line.length() > 0) {
            gfx.print(line);
        }
        gfx.println("");
        gfx.setCursor(props.x, gfx.getCursorY());
    };

    // Helper to measure width of a string
    auto measureWidth = [&](const String &s) -> uint16_t {
        int16_t x1, y1;
        uint16_t w, h;
        gfx.getTextBounds(s.c_str(), 0, 0, &x1, &y1, &w, &h);
        return w;
    };

    String word;
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];

        // Explicit newline: flush current word and line
        if (c == '\n') {
            // Place pending word if any
            if (word.length() > 0) {
                String trial = currentLine;
                if (trial.length() > 0) trial += ' ';
                trial += word;
                if (measureWidth(trial) <= availableWidth ||
                    currentLine.length() == 0) {
                    currentLine = trial;
                } else {
                    // Wrap before placing the word
                    printAndNewline(currentLine);
                    currentLine = word;
                }
                word = "";
            }
            printAndNewline(currentLine);
            currentLine = "";
            continue;
        }

        if (c == ' ') {
            // End of a word; place it respecting width
            if (word.length() > 0) {
                String trial = currentLine;
                if (trial.length() > 0) trial += ' ';
                trial += word;

                if (measureWidth(trial) <= availableWidth ||
                    currentLine.length() == 0) {
                    currentLine = trial;
                } else {
                    // Wrap before placing the word
                    printAndNewline(currentLine);
                    currentLine = word;
                }
                word = "";
            }
            // Collapse multiple spaces to a single separator (already handled)
        } else {
            // Build the word
            word += c;

            // Handle very long word that exceeds line width on its own
            // If currentLine is empty and word itself exceeds width, hard-wrap
            // the word
            if (currentLine.length() == 0 &&
                measureWidth(word) > availableWidth) {
                // Split word by characters to fit
                String chunk;
                for (size_t j = 0; j < word.length(); ++j) {
                    String trial = chunk;
                    trial += word[j];
                    if (measureWidth(trial) > availableWidth &&
                        chunk.length() > 0) {
                        gfx.print(chunk);
                        printAndNewline("");
                        chunk = "";
                    }
                    chunk += word[j];
                }
                // Start new line with leftover chunk
                currentLine = chunk;
                word = "";
            }
        }
    }

    // Place any remaining word
    if (word.length() > 0) {
        String trial = currentLine;
        if (trial.length() > 0) trial += ' ';
        trial += word;
        if (measureWidth(trial) <= availableWidth ||
            currentLine.length() == 0) {
            currentLine = trial;
        } else {
            printAndNewline(currentLine);
            currentLine = word;
        }
    }

    // Flush any remaining line
    if (currentLine.length() > 0) {
        gfx.print(currentLine);
    }
}

int drawQRCode(Adafruit_GFX &gfx, const String &qrValue,
               const DrawQRCodeProps &props = DrawQRCodeProps()) {
    // Initialize QR code (Version 7, ECC Low = 1)
    QRCode qrcode;
    static constexpr uint8_t QR_VERSION = 7;
    static constexpr uint8_t QR_ECC_LOW = 1;

    // Buffer sized for the selected version
    uint8_t qrcodeData[qrcode_getBufferSize(QR_VERSION)];

    qrcode_initText(&qrcode, qrcodeData, QR_VERSION, QR_ECC_LOW,
                    qrValue.c_str());

    // Determine scale to fit within the drawable area, treating props.x as the
    // RIGHT edge and props.y as the TOP edge. Respect maxWidth/maxHeight.
    const int16_t widthLimit = std::min<int16_t>(props.x, props.maxWidth);
    const int16_t heightLimit =
        std::min<int16_t>(gfx.height() - props.y, props.maxHeight);
    const int16_t availableWidth = std::max<int16_t>(0, widthLimit);
    const int16_t availableHeight = std::max<int16_t>(0, heightLimit);
    const int16_t maxModuleScaleW =
        availableWidth > 0 ? (availableWidth / qrcode.size) : 0;
    const int16_t maxModuleScaleH =
        availableHeight > 0 ? (availableHeight / qrcode.size) : 0;
    int16_t scale = std::max<int16_t>(
        1, std::min<int16_t>(maxModuleScaleW, maxModuleScaleH));

    // If there is absolutely no space, fallback to scale 1 drawing at props
    if (scale <= 0) {
        scale = 1;
    }

    // Right-align: position so the QR's right edge sits at props.x
    const int16_t totalSizePx = qrcode.size * scale;
    const int16_t xOffset = props.x - totalSizePx;
    const int16_t yOffset = props.y;

    // Draw modules
    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                gfx.fillRect(xOffset + x * scale, yOffset + y * scale, scale,
                             scale, Colors::white);
            }
        }
    }

    return totalSizePx;
}