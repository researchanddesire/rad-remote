# TextOverlay System Summary

### 1. TextOverlay.h - Core Component
- `TextOverlay` class extending `DisplayObject`
- Support for 4 text alignments: LEFT, CENTER, RIGHT, JUSTIFIED
- Support for 2 fonts: NORMAL (FreeSans9pt7b) and BOLD (FreeSansBold12pt7b)
- Automatic word wrapping and line breaking
- Customizable colors and background clearing

### 2. Device.h/cpp - Extended Base Class
- Added `textOverlays` map for tracking overlays by ID
- `drawText()` method with full parameter control
- `drawText()` overload with default parameters
- `clearText()` and `clearAllText()` methods
- `hasTextOverlay()` utility method

### 3. Generic device support
- OSSM example pattern text overlays


## API Usage

### Basic Text Display
```cpp
// Simple text with defaults (left-aligned, normal font, white text)
device->drawText("status", 10, 50, 300, 80, "Ready");
```

### Advanced Text Display
```cpp
// Full control over appearance and positioning
device->drawText("title", 
                 50, 60, 270, 90,           // Rectangle coordinates
                 "Pattern Name",            // Text content
                 TEXT_ALIGN_CENTER,         // Alignment
                 TEXT_FONT_BOLD,           // Font
                 0xFFFF,                   // White text
                 0x0000,                   // Black background
                 true);                    // Clear background
```

### Text Management
```cpp
// Update existing overlay
device->drawText("status", 10, 50, 300, 80, "Connected");

// Clear specific overlay
device->clearText("status");

// Clear all overlays
device->clearAllText();
```

## Future Extensibility

The system is designed for extension to support images