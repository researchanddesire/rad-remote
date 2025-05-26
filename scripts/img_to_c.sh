#!/bin/bash

# Check if ImageMagick is installed
if ! command -v convert &> /dev/null; then
    echo "Error: ImageMagick is not installed. Please install it using:"
    echo "brew install imagemagick"
    exit 1
fi

# Check if input file is provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <input_image>"
    exit 1
fi

INPUT_IMAGE="$1"

# Check if input file exists
if [ ! -f "$INPUT_IMAGE" ]; then
    echo "Error: Input file '$INPUT_IMAGE' does not exist"
    exit 1
fi

# Get image dimensions
WIDTH=$(identify -format "%w" "$INPUT_IMAGE")
HEIGHT=$(identify -format "%h" "$INPUT_IMAGE")

# Create temporary file
TEMP_RAW="./temp.bmp"

# Convert image to RGB565 format and save as raw data with maximum compression
magick "$INPUT_IMAGE" -flip -type truecolor -define bmp:subtype=rgb565 -quality 1 -strip -sampling-factor 4:2:0 -compress none BMP3:"$TEMP_RAW"

# Create the C string output
C_STRING="const uint16_t IMAGE_WIDTH = $WIDTH;\nconst uint16_t IMAGE_HEIGHT = $HEIGHT;\n// clang-format off\nconst uint16_t image_data[] PROGMEM = {\n$(hexdump -v -s 54 -e '2/2 "0x%04x, "' "$TEMP_RAW" | sed 's/, $//')\n};\n// clang-format on"

# Output to terminal
echo -e "$C_STRING"

# Copy to clipboard
echo -e "$C_STRING" | pbcopy

# Clean up
rm "$TEMP_RAW"

echo -e "\nC string has been copied to clipboard!"


