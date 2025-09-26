# Text Overlay System

## Overview

The text overlay system provides a flexible way to display text information on any device's control screen without affecting the underlying UI elements. It's designed to work generically across all devices (OSSM, Domi, etc.) while in the `device_draw_control` state.

## Features

- **Rectangle-based overlays**: Specify exact screen coordinates for text placement
- **Multiple alignment options**: Left, center, right, and justified text
- **Font options**: Default (FreeSans9pt7b) and Bold (FreeSansBold12pt7b)  
- **Color customization**: Custom text and background colors
- **Automatic word wrapping**: Text automatically wraps within specified bounds
- **Non-destructive**: Overlays only affect their specified rectangular areas
- **Persistent**: Text remains until explicitly cleared or updated

## API Reference

### `drawText(overlayId, xStart, yStart, xEnd, yEnd, text, [options])`

Creates or updates a text overlay with the specified parameters.

**Parameters:**
- `overlayId` (String): Unique identifier for this overlay
- `xStart`, `yStart` (int16_t): Top-left corner coordinates  
- `xEnd`, `yEnd` (int16_t): Bottom-right corner coordinates
- `text` (String): Text content to display
- `alignment` (TextAlign): Text alignment (LEFT, CENTER, RIGHT, JUSTIFIED)
- `font` (TextFont): Font style (DEFAULT, BOLD)
- `textColor` (uint16_t): Text color (default: Colors::white)
- `backgroundColor` (uint16_t): Background color (default: ST77XX_BLACK)
- `clearBackground` (bool): Whether to clear background before drawing (default: true)

**Returns:** `TextOverlay*` - Pointer to the created or updated overlay

### `clearText(overlayId)`

Clears the specified text overlay by setting its text to empty.

**Parameters:**
- `overlayId` (String): ID of the overlay to clear

### `clearAllText()`

Clears all text overlays for this device.

### `hasTextOverlay(overlayId)`

Checks if a text overlay with the specified ID exists.

**Parameters:**
- `overlayId` (String): ID to check

**Returns:** `bool` - True if overlay exists

## Usage Examples

### Basic Text Display

```cpp
// Display centered title
drawText("title", 50, 60, 270, 90, "Pattern Name", 
         TextAlign::CENTER, TextFont::BOLD);

// Display left-aligned description
drawText("description", 50, 100, 270, 150, 
         "This is a longer description that will wrap automatically", 
         TextAlign::LEFT, TextFont::DEFAULT);
```

### OSSM Control Screen Example

The OSSM device demonstrates the system with text overlays positioned between the encoder dials:

```cpp
void drawControls() override {
    // ... existing control setup ...
    
    // Pattern title overlay (centered, bold)
    drawText("pattern_title", 
             90, Display::PageY + 30, 
             DISPLAY_WIDTH - 90, Display::PageY + 60,
             "Gentle Waves",
             TextAlign::CENTER, TextFont::BOLD);

    // Pattern description overlay (justified text with wrapping)
    drawText("pattern_description",
             90, Display::PageY + 70,
             DISPLAY_WIDTH - 90, Display::PageY + 120,
             "A smooth, wave-like motion that builds slowly",
             TextAlign::JUSTIFIED, TextFont::DEFAULT);
}

void onRightBumperClick() override {
    // Update text dynamically
    drawText("pattern_title", 90, Display::PageY + 30, 
             DISPLAY_WIDTH - 90, Display::PageY + 60,
             "New Pattern Name", TextAlign::CENTER, TextFont::BOLD);
}

void onLeftBumperClick() override {
    // Clear specific overlay
    clearText("pattern_title");
}
```

### Justified Text Example

```cpp
// Justified text automatically distributes words evenly across lines
drawText("justified_text", 20, 80, 300, 140,
         "This text will be justified across multiple lines with even spacing",
         TextAlign::JUSTIFIED, TextFont::DEFAULT);
```

### Custom Styling

```cpp
// Red warning text with custom background
drawText("warning", 10, 10, 310, 50,
         "WARNING: High Temperature", 
         TextAlign::CENTER, TextFont::BOLD,
         Colors::red, Colors::yellow, true);
```

### Dynamic Updates

```cpp
// Update existing overlay with new content
void updateStatus(const String& status) {
    drawText("status", 0, 200, 320, 240, 
             status, TextAlign::CENTER, TextFont::DEFAULT);
}

// Clear when no longer needed
void hideStatus() {
    clearText("status");
}
```

## Screen Layout Considerations

### OSSM Device Layout (320x240)
- Status bar: y=0-30
- Available area: y=30-240
- Left encoder: x=0-90
- Right encoder: x=230-320  
- Center area: x=90-230 (good for text overlays)

### General Guidelines
- Always use `Display::PageY` for y-coordinates to avoid status bar
- Leave margins around existing UI elements
- Test text wrapping with longest expected content
- Use consistent overlay IDs for easy management

## Thread Safety

The system uses the global `displayMutex` to ensure thread-safe access to the display. All drawing operations automatically acquire and release this mutex.

## Memory Management

- Overlays are managed by the Device's `displayObjects` vector
- Text overlay pointers are tracked in `textOverlays` map for easy access
- Memory is automatically cleaned up when device is destroyed
- No manual memory management required

## Future Enhancements

The system is designed to be easily extensible for future image support:

- Replace `TextOverlayProps` with generic `OverlayProps`
- Add `OverlayType` enum (TEXT, IMAGE, etc.)  
- Extend drawing logic to handle different content types
- Maintain same rectangular coordinate system