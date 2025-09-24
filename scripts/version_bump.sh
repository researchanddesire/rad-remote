#!/bin/bash

# Validate argument
if [[ -z "$1" ]]; then
  echo "Error: Version argument required (format: X.Y.Z)"
  exit 1
fi

version_regex='^[0-9]+\.[0-9]+\.[0-9]+$'
if [[ ! "$1" =~ $version_regex ]]; then
  echo "Error: Version argument must be in format X.Y.Z (e.g., 1.2.3)"
  exit 1
fi

new_version_string="$1"
IFS='.' read -r major minor patch <<< "$new_version_string"

# Check that the version was parsed correctly
if [[ -z "$major" || -z "$minor" || -z "$patch" ]]; then
    echo "Error: Failed to parse version '$new_version_string' into major, minor, patch components"
    exit 1
fi

# Validate that all components are numeric
if ! [[ "$major" =~ ^[0-9]+$ ]] || ! [[ "$minor" =~ ^[0-9]+$ ]] || ! [[ "$patch" =~ ^[0-9]+$ ]]; then
    echo "Error: Version components must be numeric. Got: major=$major, minor=$minor, patch=$patch"
    exit 1
fi
# Update src/constants/Version.h
cat > src/constants/Version.h << EOF
#ifndef OSSM_REMOTE_VERSION_H
#define OSSM_REMOTE_VERSION_H

#define VERSION "$new_version_string"
#define MAJOR_VERSION $major
#define MINOR_VERSION $minor
#define PATCH_VERSION $patch

#endif  // OSSM_REMOTE_VERSION_H
EOF

# Now, pip install platformio and build the project for production.
pip install -U platformio

# Build the project
platformio run -e production

# Create versioned and latest directories
mkdir -p ./docs/v/$new_version_string
mkdir -p ./docs/v/latest

cp ./.pio/build/production/firmware.bin ./docs/v/$new_version_string/firmware.bin
cp ./.pio/build/production/firmware.bin ./docs/v/latest/firmware.bin

# Create version.json for versioned and latest directories
cat > ./docs/v/$new_version_string/version.json << JSON
{
  "version": "$new_version_string",
  "major": $major,
  "minor": $minor,
  "patch": $patch
}
JSON

cp ./docs/v/$new_version_string/version.json ./docs/v/latest/version.json

# and finally, update the docs site
./scripts/update_docs_site.sh