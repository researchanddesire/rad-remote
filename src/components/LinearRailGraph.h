#pragma once

#ifndef LINEARRAILGRAPH_H
#define LINEARRAILGRAPH_H

#include <Adafruit_MCP23X17.h>
#include <Adafruit_ST77xx.h>
#include <constants.h>

#include <map>

#include "DisplayObject.h"
#include "esp_log.h"
#include "pins.h"
#include "services/display.h"
#include "utils/buzzer.h"
#include "utils/vibrator.h"

class LinearRailGraph : public DisplayObject {
  private:
    float *stroke;
    float *depth;
    int lastStrokePercent = -1;
    int lastDepthPercent = -1;
    // Track last drawn geometry to avoid unnecessary clearing
    int lastStrokeWidth = -1;
    int lastDepthOffset = -1;

  public:
    LinearRailGraph(float *stroke, float *depth, int16_t x = -1, int16_t y = -1,
                    int16_t width = 90, int16_t height = 30)
        : DisplayObject((x == -1) ? (DISPLAY_WIDTH / 2 - width / 2) : x,
                        (y == -1) ? (DISPLAY_HEIGHT / 2 - height / 2) : y,
                        width, height),
          stroke(stroke),
          depth(depth) {
        // No further adjustment needed; handled in initializer list
    }

    bool shouldDraw() override {
        int currentStrokePercent = 0;
        int currentDepthPercent = 0;

        if (stroke != nullptr) {
            currentStrokePercent = constrain((int)roundf(*stroke), 0, 100);
        }
        if (depth != nullptr) {
            currentDepthPercent = constrain((int)roundf(*depth), 0, 100);
        }

        if (lastStrokePercent != currentStrokePercent) {
            return true;
        }
        if (lastDepthPercent != currentDepthPercent) {
            return true;
        }

        return false;
    }

    void draw() override {
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            // Calculate visualization based on:
            // - depth determines the right edge position of the bar
            // - stroke determines how far left from that right edge the bar
            // extends

            int borderMargin =
                3;  // 1px border + 2px for rounded corner protection
            int innerWidth = width - (2 * borderMargin);

            int depthRightEdge = int(innerWidth * (*this->depth) / 100.0f);

            int strokeExtent = int(depthRightEdge * (*this->stroke) / 100.0f);

            // Calculate actual fill coordinates
            int fillEnd = depthRightEdge;  // Right edge at depth position
            int fillStart =
                depthRightEdge -
                strokeExtent;  // Left edge extends back by stroke amount
            int actualFillWidth = max(1, strokeExtent);
            int actualFillStart = max(0, fillStart);  // Relative to inner area

            // For tracking changes, use the fill parameters
            int currentStrokeWidth = actualFillWidth;
            int currentFillStart = actualFillStart;

            // First time drawing - set up the border and initial state
            if (lastStrokeWidth == -1) {
                tft.fillRect(x, y, width, height, ST77XX_BLACK);
                tft.drawRoundRect(x, y, width, height, 4, COLOR_WHITE);

                // Draw the initial fill bar
                if (actualFillWidth > 0) {
                    int screenFillStart = x + borderMargin + actualFillStart;
                    tft.fillRect(screenFillStart, y + 1, actualFillWidth,
                                 height - 2, COLOR_WHITE);
                }

                lastStrokeWidth = currentStrokeWidth;
                lastDepthOffset = currentFillStart;
            } else if (lastStrokeWidth != currentStrokeWidth ||
                       lastDepthOffset != currentFillStart) {
                // Differential drawing to prevent flicker - only change what
                // needs to change
                int oldStart = lastDepthOffset;
                int oldEnd = oldStart + lastStrokeWidth;
                int newStart = actualFillStart;
                int newEnd = newStart + actualFillWidth;

                // Convert to screen coordinates
                int screenOldStart = x + borderMargin + oldStart;
                int screenOldEnd = x + borderMargin + oldEnd;
                int screenNewStart = x + borderMargin + newStart;
                int screenNewEnd = x + borderMargin + newEnd;

                // Ensure we stay within bounds
                int innerLeft = x + borderMargin;
                int innerRight = x + width - borderMargin;
                screenOldStart = max(screenOldStart, innerLeft);
                screenOldEnd = min(screenOldEnd, innerRight);
                screenNewStart = max(screenNewStart, innerLeft);
                screenNewEnd = min(screenNewEnd, innerRight);

                // Clear areas that should no longer be filled
                if (lastStrokeWidth > 0) {
                    // Clear left overhang (old bar extends further left than
                    // new bar)
                    if (screenOldStart < screenNewStart) {
                        int clearWidth =
                            min(screenNewStart, screenOldEnd) - screenOldStart;
                        if (clearWidth > 0) {
                            tft.fillRect(screenOldStart, y + 1, clearWidth,
                                         height - 2, ST77XX_BLACK);
                        }
                    }

                    // Clear right overhang (old bar extends further right than
                    // new bar)
                    if (screenOldEnd > screenNewEnd) {
                        int clearStart = max(screenNewEnd, screenOldStart);
                        int clearWidth = screenOldEnd - clearStart;
                        if (clearWidth > 0) {
                            tft.fillRect(clearStart, y + 1, clearWidth,
                                         height - 2, ST77XX_BLACK);
                        }
                    }
                }

                // Fill areas that should now be filled
                if (actualFillWidth > 0) {
                    // Fill left extension (new bar extends further left than
                    // old bar)
                    if (screenNewStart < screenOldStart) {
                        int fillWidth =
                            min(screenOldStart, screenNewEnd) - screenNewStart;
                        if (fillWidth > 0) {
                            tft.fillRect(screenNewStart, y + 1, fillWidth,
                                         height - 2, COLOR_WHITE);
                        }
                    }

                    // Fill right extension (new bar extends further right than
                    // old bar)
                    if (screenNewEnd > screenOldEnd) {
                        int fillStart = max(screenOldEnd, screenNewStart);
                        int fillWidth = screenNewEnd - fillStart;
                        if (fillWidth > 0) {
                            tft.fillRect(fillStart, y + 1, fillWidth,
                                         height - 2, COLOR_WHITE);
                        }
                    }
                }

                // Update cached geometry
                lastStrokeWidth = currentStrokeWidth;
                lastDepthOffset = currentFillStart;
            }

            xSemaphoreGive(displayMutex);
        }

        // Update cached values
        int currentStrokePercent = 0;
        int currentDepthPercent = 0;
        if (stroke != nullptr) {
            currentStrokePercent = constrain((int)roundf(*stroke), 0, 100);
        }
        if (depth != nullptr) {
            currentDepthPercent = constrain((int)roundf(*depth), 0, 100);
        }
        lastStrokePercent = currentStrokePercent;
        lastDepthPercent = currentDepthPercent;
    }
};

// Initialize static member
#endif