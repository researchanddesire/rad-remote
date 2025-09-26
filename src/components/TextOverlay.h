#ifndef TEXT_OVERLAY_H
#define TEXT_OVERLAY_H

#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <vector>

#include "../constants/Colors.h"
#include "../services/display.h"
#include "DisplayObject.h"

extern Adafruit_ST7789 tft;
extern SemaphoreHandle_t displayMutex;

// Text alignment options
#define TEXT_ALIGN_LEFT 0
#define TEXT_ALIGN_CENTER 1
#define TEXT_ALIGN_RIGHT 2
#define TEXT_ALIGN_JUSTIFIED 3

typedef int TextAlign;

// Default fonts for convenience
extern const GFXfont* const TEXT_FONT_NORMAL;
extern const GFXfont* const TEXT_FONT_BOLD;

// Additional future default font options
extern const GFXfont* const TEXT_FONT_SMALL;
extern const GFXfont* const TEXT_FONT_LARGE;

struct TextOverlayProps {
    String text;
    TextAlign alignment = TEXT_ALIGN_LEFT;
    const GFXfont* font = TEXT_FONT_NORMAL;
    uint16_t textColor = 0xFFFF;        // White
    uint16_t backgroundColor = 0x0000;  // Black
    bool clearBackground = true;
    uint8_t padding = 4;  // Internal padding from rectangle edges
};

class TextOverlay : public DisplayObject {
  private:
    TextOverlayProps props;
    String lastText;
    bool needsRedraw;

    void setFont() { tft.setFont(props.font); }

    int16_t getLineHeight() {
        // Calculate line height based on the font
        if (props.font == nullptr) {
            // Built-in system font is small - approximately 8 pixels tall
            return 10;  // 8px font height + 2px spacing
        }

        // Get font metrics to calculate proper line height for custom fonts
        int16_t x1, y1;
        uint16_t w, h;
        tft.getTextBounds("Ag", 0, 0, &x1, &y1, &w, &h);

        // Add some spacing between lines (typically 1.2x font height)
        return h + (h / 5);
    }

    void drawJustifiedText(const String& text, int16_t rectX, int16_t rectY,
                           int16_t rectWidth, int16_t rectHeight) {
        // Split text into words
        std::vector<String> words;
        String word = "";

        for (int i = 0; i < text.length(); i++) {
            char c = text[i];
            if (c == ' ' || c == '\n') {
                if (word.length() > 0) {
                    words.push_back(word);
                    word = "";
                }
                if (c == '\n') {
                    words.push_back("\n");  // Mark line breaks
                }
            } else {
                word += c;
            }
        }
        if (word.length() > 0) {
            words.push_back(word);
        }

        int16_t currentY = rectY + props.padding;
        int16_t lineHeight = getLineHeight();

        std::vector<String> currentLine;
        int16_t currentLineWidth = 0;

        auto flushLine = [&]() {
            if (currentLine.empty()) return;

            if (currentLine.size() == 1) {
                // Single word, center it
                int16_t x1, y1;
                uint16_t w, h;
                tft.getTextBounds(currentLine[0].c_str(), 0, 0, &x1, &y1, &w,
                                  &h);
                int16_t textX = rectX + (rectWidth - w) / 2;
                tft.setCursor(textX, currentY);
                tft.print(currentLine[0]);
            } else {
                // Multiple words, justify
                int totalTextWidth = 0;
                for (const String& w : currentLine) {
                    int16_t x1, y1;
                    uint16_t ww, hh;
                    tft.getTextBounds(w.c_str(), 0, 0, &x1, &y1, &ww, &hh);
                    totalTextWidth += ww;
                }

                int availableWidth = rectWidth - (2 * props.padding);
                int spaceWidth = (availableWidth - totalTextWidth) /
                                 (currentLine.size() - 1);

                int16_t currentX = rectX + props.padding;
                for (int i = 0; i < currentLine.size(); i++) {
                    tft.setCursor(currentX, currentY);
                    tft.print(currentLine[i]);

                    int16_t x1, y1;
                    uint16_t w, h;
                    tft.getTextBounds(currentLine[i].c_str(), 0, 0, &x1, &y1,
                                      &w, &h);
                    currentX += w;

                    if (i < currentLine.size() - 1) {
                        currentX += spaceWidth;
                    }
                }
            }

            currentLine.clear();
            currentLineWidth = 0;
            currentY += lineHeight;
        };

        for (const String& word : words) {
            if (word == "\n") {
                flushLine();
                continue;
            }

            int16_t x1, y1;
            uint16_t w, h;
            tft.getTextBounds(word.c_str(), 0, 0, &x1, &y1, &w, &h);

            int availableWidth = rectWidth - (2 * props.padding);

            if (currentLineWidth + w > availableWidth && !currentLine.empty()) {
                flushLine();
            }

            currentLine.push_back(word);
            currentLineWidth += w;

            if (!currentLine.empty() && currentLine.size() > 1) {
                currentLineWidth += 8;  // Approximate space width
            }
        }

        if (!currentLine.empty()) {
            flushLine();
        }
    }

  public:
    TextOverlay(int16_t x, int16_t y, int16_t width, int16_t height,
                const TextOverlayProps& overlayProps)
        : DisplayObject(x, y, width, height),
          props(overlayProps),
          lastText(""),
          needsRedraw(true) {}

    void updateText(const String& newText) {
        if (newText != lastText) {
            lastText = newText;
            props.text = newText;
            needsRedraw = true;
            isDirty = true;
        }
    }

    void updateProps(const TextOverlayProps& newProps) {
        props = newProps;
        needsRedraw = true;
        isDirty = true;
    }

    bool shouldDraw() override {
        return needsRedraw || isDirty || DisplayObject::shouldDraw();
    }

    void draw() override {
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            // Clear the area if requested
            if (props.clearBackground) {
                tft.fillRect(x, y, width, height, props.backgroundColor);
            }

            // If text is empty, just clear and return
            if (props.text.length() == 0) {
                xSemaphoreGive(displayMutex);
                needsRedraw = false;
                return;
            }

            // Set font and color
            setFont();
            tft.setTextColor(props.textColor);

            // Calculate available area for text (minus padding)
            int16_t textAreaX = x + props.padding;
            int16_t textAreaY = y + props.padding;
            int16_t textAreaWidth = width - (2 * props.padding);
            int16_t textAreaHeight = height - (2 * props.padding);

            // Handle different alignment types
            switch (props.alignment) {
                case TEXT_ALIGN_JUSTIFIED:
                    drawJustifiedText(props.text, x, y, width, height);
                    break;

                case TEXT_ALIGN_CENTER:
                case TEXT_ALIGN_LEFT:
                case TEXT_ALIGN_RIGHT:
                default: {
                    // For non-justified text, use simple word wrapping
                    String word = "";
                    String currentLine = "";
                    int16_t currentY = textAreaY;
                    int16_t lineHeight = getLineHeight();

                    auto flushLine = [&]() {
                        if (currentLine.length() == 0) return;

                        int16_t x1, y1;
                        uint16_t w, h;
                        tft.getTextBounds(currentLine.c_str(), 0, 0, &x1, &y1,
                                          &w, &h);

                        int16_t textX;
                        switch (props.alignment) {
                            case TEXT_ALIGN_CENTER:
                                textX = textAreaX + (textAreaWidth - w) / 2;
                                break;
                            case TEXT_ALIGN_RIGHT:
                                textX = textAreaX + textAreaWidth - w;
                                break;
                            case TEXT_ALIGN_LEFT:
                            default:
                                textX = textAreaX;
                                break;
                        }

                        tft.setCursor(textX, currentY);
                        tft.print(currentLine);
                        currentLine = "";
                        currentY += lineHeight;
                    };

                    for (int i = 0; i < props.text.length(); i++) {
                        char c = props.text[i];

                        if (c == ' ' || c == '\n') {
                            if (word.length() > 0) {
                                String testLine = currentLine;
                                if (testLine.length() > 0) testLine += " ";
                                testLine += word;

                                int16_t x1, y1;
                                uint16_t w, h;
                                tft.getTextBounds(testLine.c_str(), 0, 0, &x1,
                                                  &y1, &w, &h);

                                if (w <= textAreaWidth ||
                                    currentLine.length() == 0) {
                                    currentLine = testLine;
                                } else {
                                    flushLine();
                                    currentLine = word;
                                }
                                word = "";
                            }

                            if (c == '\n') {
                                flushLine();
                            }
                        } else {
                            word += c;
                        }
                    }

                    // Handle remaining word
                    if (word.length() > 0) {
                        String testLine = currentLine;
                        if (testLine.length() > 0) testLine += " ";
                        testLine += word;

                        int16_t x1, y1;
                        uint16_t w, h;
                        tft.getTextBounds(testLine.c_str(), 0, 0, &x1, &y1, &w,
                                          &h);

                        if (w <= textAreaWidth || currentLine.length() == 0) {
                            currentLine = testLine;
                        } else {
                            flushLine();
                            currentLine = word;
                        }
                    }

                    if (currentLine.length() > 0) {
                        flushLine();
                    }
                    break;
                }
            }

            xSemaphoreGive(displayMutex);
        }

        needsRedraw = false;
    }
};

#endif  // TEXT_OVERLAY_H