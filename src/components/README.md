## Components: DisplayObject and Children

This folder contains lightweight UI components that render to the TFT via the display service. All UI widgets extend `DisplayObject` and are owned/managed by a `Device`.

### TL;DR

- **Register once**: Devices create and register components inside their `drawControls()` method. Registration happens once per page/device.
- **Update via tick**: Each component’s `tick()` decides whether to redraw based on `shouldDraw()` and internal change detection.
- **Draw directly to TFT**: Render straight to `tft` while holding `displayMutex` (no canvas due to memory constraints).
- **Detect changes**: Pass external values into components by reference so they can detect when to redraw.

---

## Device Integration

Inside a `Device`, implement `drawControls()` to register your controls once. Example:

```cpp
void drawControls() override {
    // Top bumpers - positioned with margin to prevent border cutoff
    draw<TextButton>("<-", pins::BTN_L_SHOULDER, 5, 0);
    draw<TextButton>("->", pins::BTN_R_SHOULDER, DISPLAY_WIDTH - 75, 0);
}
```

`drawControls()` is called exactly once to register components for the device.
After that, the device’s `drawControlsTask` manages ongoing UI/UX updates by iterating components and calling `tick()` on each.

```cpp
for (auto &displayObject : device->displayObjects) {
    displayObject->tick();
    vTaskDelay(1 / portTICK_PERIOD_MS);
}
```

---

## DisplayObject Lifecycle

`DisplayObject` defines the minimal lifecycle for all UI widgets:

- `shouldDraw()`

  - Default: redraw on first draw or every 2000 ms.
  - Override to implement precise change detection (recommended).

- `draw()`

  - Perform the actual rendering. Must be thread-safe (use `displayMutex`).
  - Draw directly to `tft` within a short critical section.

- `tick()`
  - Calls `shouldDraw()`; if true, calls `draw()` and updates internal timing/flags.

Key data provided by `DisplayObject`:

- `x`, `y`, `width`, `height` layout fields
- `isFirstDraw` and internal `lastDrawTime`

---

## Rendering Rules (Direct to TFT)

To avoid tearing and maintain thread safety:

- Draw UI content directly using the Adafruit_GFX API on `tft`.
- Always acquire `displayMutex` before drawing to the display and release it after.
- Prefer the smallest region necessary; clear only what you need.

Minimal pattern:

```cpp
if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    // Clear/redraw only the necessary region
    tft.fillRect(x, y, width, height, Colors::black);

    // Draw your UI directly to tft
    tft.setTextColor(Colors::white);
    tft.setCursor(x + 2, y + height - 4);
    tft.print("Hello");

    xSemaphoreGive(displayMutex);
}
```

---

## Creating a New Component

Steps:

1. Create a new header file in `components/`.
2. Extend `DisplayObject`.
3. Pass external, changing values by reference (e.g., `const std::string&`, numeric refs) so the component can detect changes.
4. Override `shouldDraw()` for your change conditions.
5. Implement `draw()` with direct `tft` drawing while holding `displayMutex`.

---

## Template: MyDisplayObject

```cpp
#ifndef MYDISPLAYOBJECT_H
#define MYDISPLAYOBJECT_H

#include "DisplayObject.h"
#include "constants/Colors.h"
#include "services/display.h" // provides tft and displayMutex

class MyDisplayObject : public DisplayObject {
  private:
    // Example external value passed by reference for change detection
    const int& valueRef;

    // Cache of last drawn value to detect changes
    int lastValue = -1;

  public:
    // Width/height should match your drawing region. Choose minimal region.
    MyDisplayObject(const int& valueRef, int16_t x, int16_t y, int16_t width, int16_t height)
      : DisplayObject(x, y, width, height), valueRef(valueRef) {
        lastValue = valueRef; // initialize cache
    }

    bool shouldDraw() override {
        // Redraw on first draw or when the observed value changes
        return isFirstDraw || (valueRef != lastValue);
    }

    void draw() override {
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            // Clear/redraw only this component's region
            tft.fillRect(x, y, width, height, Colors::black);

            // Draw directly to the TFT
            tft.setTextColor(Colors::white);
            tft.setCursor(x + 2, y + height - 4);
            tft.print("Val: ");
            tft.print(valueRef);

            xSemaphoreGive(displayMutex);
        }

        // Update cache
        lastValue = valueRef;
    }
};

#endif // MYDISPLAYOBJECT_H
```

Usage in a `Device`:

```cpp
// Inside drawControls():
draw<MyDisplayObject>(someChangingIntRef, 10, 30, 120, 20);
```

---

## Example: DynamicText

`DynamicText` is a simple text component that updates when the observed string changes. It demonstrates value-by-reference, simple change detection, and direct drawing to `tft` under `displayMutex`.

Key ideas you can borrow:

- Store a cached copy of the last drawn value and compare in `shouldDraw()`.
- Measure text bounds on the drawing surface to compute layout.
- Use the smallest possible region for redraws.

For advanced components, follow the same approach but prefer the canvas-first rendering pattern outlined above to minimize flicker and ensure smooth updates.

---

## Best Practices

- **Pass references** to changing inputs; avoid copying large strings/objects.
- **Minimize redraw regions** to reduce memory bandwidth and flicker.
- **Guard direct `tft` drawing with `displayMutex`**; keep critical sections short.
- **Keep `shouldDraw()` cheap**; do heavier work in `draw()`.
- **Respect timing**; if your component updates rapidly, ensure your region is small and work is minimal.
