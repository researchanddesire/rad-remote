#include "pages/genericPages.h"

#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <components/TextButton.h>
#include <constants/Colors.h>
#include <constants/Sizes.h>
#include <qrcode.h>
#include <services/display.h>
#include <utils/stringFormat.h>

void drawPageTask(void *pvParameters) {
    const TextPage *params = static_cast<const TextPage *>(pvParameters);
    // No delete needed - params points to static const objects in flash memory

    int16_t width = Display::WIDTH;
    int16_t height = Display::PageHeight;
    GFXcanvas16 *canvas = new GFXcanvas16(width, height);
    if (canvas == nullptr) {
        vTaskDelete(NULL);
        return;
    }

    canvas->fillScreen(Colors::black);

    // Draw title with large bold font
    canvas->setFont(&FreeSansBold12pt7b);
    canvas->setTextColor(Colors::white);

    // Get title bounds for centering
    int16_t titleX1, titleY1;
    uint16_t titleWidth, titleHeight;
    canvas->getTextBounds(params->title.c_str(), 0, 0, &titleX1, &titleY1,
                          &titleWidth, &titleHeight);

    // Center title horizontally
    int16_t titleX = (width - titleWidth) / 2;
    int16_t titleY = Display::Padding::P3 - titleY1;  // Top padding
    canvas->setCursor(titleX, titleY);
    canvas->print(params->title);

    // Draw description with smaller font
    canvas->setFont(&FreeSans9pt7b);
    canvas->setTextColor(Colors::lightGray);

    // Optionally draw QR code on the right side if provided
    if (params->qrValue.length() > 0 && params->qrValue != EMPTY_STRING) {
        QRCode qrcode;
        const int qrVersion = 7;  // reasonable size
        const int qrScale = 2;
        // NOLINTBEGIN(modernize-avoid-c-arrays)
        uint8_t qrcodeData[qrcode_getBufferSize(qrVersion)];
        // NOLINTEND(modernize-avoid-c-arrays)
        qrcode_initText(&qrcode, qrcodeData, qrVersion, 1,
                        params->qrValue.c_str());

        int qrWidthPixels = qrcode.size * qrScale;
        int qrHeightPixels = qrcode.size * qrScale;

        int xOffset = width - qrWidthPixels - Display::Padding::P2;
        int yOffset = (Display::PageHeight - qrHeightPixels) / 2;
        if (yOffset < 0) yOffset = 0;

        for (uint8_t y = 0; y < qrcode.size; y++) {
            for (uint8_t x = 0; x < qrcode.size; x++) {
                if (qrcode_getModule(&qrcode, x, y)) {
                    canvas->fillRect(xOffset + x * qrScale,
                                     yOffset + y * qrScale, qrScale, qrScale,
                                     Colors::white);
                }
            }
        }
    }

    int16_t descY = titleY + titleHeight + Display::Padding::P2;
    wrapText(*canvas, params->description, Display::Padding::P2, descY);

    // Draw buttons if text is provided (using TextButton styling)
    const int buttonWidth = 80;
    const int buttonHeight = 30;
    const int buttonY = height - buttonHeight - Display::Padding::P2;

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        tft.drawRGBBitmap(0, Display::PageY, canvas->getBuffer(), width,
                          height);
        xSemaphoreGive(displayMutex);
    }

    delete canvas;
    canvas = nullptr;

    // Left button
    if (params->leftButtonText.length() > 0 &&
        params->leftButtonText != EMPTY_STRING) {
        TextButton *leftButton = new TextButton(
            params->leftButtonText, pins::BTN_UNDER_L, 0, Display::HEIGHT - 25);
        leftButton->tick();
        delete leftButton;
        leftButton = nullptr;
    }

    // Right button
    if (params->rightButtonText.length() > 0 &&
        params->rightButtonText != EMPTY_STRING) {
        TextButton *rightButton =
            new TextButton(params->rightButtonText, pins::BTN_UNDER_R,
                           Display::WIDTH - 60, Display::HEIGHT - 25);
        rightButton->tick();
        delete rightButton;
        rightButton = nullptr;
    }

    vTaskDelete(NULL);
}