#!/bin/bash

# Simple YAML to JSON converter with registry generation
# Converts .yml files to JSON and creates registry.json in one pass

set -e

# Check dependencies
command -v yq >/dev/null 2>&1 || { echo "Error: yq is required but not installed. Run: brew install yq"; exit 1; }
command -v jq >/dev/null 2>&1 || { echo "Error: jq is required but not installed. Run: brew install jq"; exit 1; }

# Create directories
mkdir -p ./data
mkdir -p ./data/protocols

# Create temp file for registry mappings
TEMP_REGISTRY=$(mktemp)

echo "Converting YAML files to JSON and collecting registry data..."

# Process each YAML file
for yaml_file in ./ButtplugIO/protocols/*.yml; do
    if [ -f "$yaml_file" ]; then
        filename=$(basename "$yaml_file" .yml)
        json_file="./data/protocols/${filename}.json"
        
        echo "  Converting: $filename"
        
        # Convert YAML to JSON
        yq eval -o=json "$yaml_file" > "$json_file"
        
        # Extract service UUIDs from communication.btle.services path
        # This is more intentional about the exact path structure
        jq -r '.communication[]?.btle?.services | keys[]?' "$json_file" 2>/dev/null | while read -r uuid; do
            if [ -n "$uuid" ] && [ "$uuid" != "null" ]; then
                echo "{\"$uuid\": \"/protocols/${filename}.json\"}" >> "$TEMP_REGISTRY"
            fi
        done
    fi
done

# Generate final registry JSON
echo "Generating registry.json..."

# Build registry using jq, handling duplicates by merging into arrays
# This ensures all UUIDs are captured and properly formatted as arrays
jq -s '
  reduce .[] as $mapping ({}; 
    reduce ($mapping | to_entries[]) as $entry (.;
      if has($entry.key) then
        .[$entry.key] = (.[$entry.key] + [$entry.value] | unique)
      else
        .[$entry.key] = [$entry.value]
      end
    )
  ) | to_entries | sort_by(.key) | from_entries
' "$TEMP_REGISTRY" > ./data/registry.json

# Check for duplicate UUIDs in source files before cleanup
echo "Checking for duplicate service UUIDs..."
duplicates=$(cat "$TEMP_REGISTRY" | jq -s 'group_by(. | keys[0]) | map(select(length > 1)) | flatten' 2>/dev/null || echo "[]")
if [ "$duplicates" != "[]" ]; then
    echo "  Found duplicate service UUIDs (now handled as arrays):"
    echo "$duplicates" | jq -s 'group_by(. | keys[0]) | map(select(length > 1)) | .[] | "    \(.[0] | keys[0]): \([.[] | .[keys[0]]] | unique)"'
fi

# Generate metadata.json with MD5 hash of protocols directory
echo "Generating metadata.json..."
protocols_md5=$(find ./data/protocols -name "*.json" -type f -exec md5sum {} \; | sort | md5sum | cut -d' ' -f1)
echo "{\"protocols_md5\": \"$protocols_md5\"}" > ./data/metadata.json

# Cleanup
rm "$TEMP_REGISTRY"

# Show results
json_count=$(ls ./data/protocols/*.json | wc -l)
registry_count=$(jq 'keys | length' ./data/registry.json)

echo "Done!"
echo "  Converted: $json_count JSON files"
echo "  Registry: $registry_count service UUIDs"
echo "  Protocols MD5: $protocols_md5"
