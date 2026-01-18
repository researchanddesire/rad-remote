#!/bin/bash

set -e

DOCS_V_PATH="$(dirname "$0")/../docs/v"
ALL_VERSIONS_JSON_PATH="$(dirname "$0")/../docs/all-versions.json"

# Build parallel arrays: old_versions and old_datetimes
old_versions=()
old_datetimes=()
if [ -s "$ALL_VERSIONS_JSON_PATH" ]; then
    if command -v jq >/dev/null 2>&1; then
        while IFS=$'\t' read -r version datetime; do
            old_versions+=("$version")
            old_datetimes+=("$datetime")
        done < <(jq -r '.[] | select(.version != null) | "\(.version)\t\(.datetime)"' "$ALL_VERSIONS_JSON_PATH")
    else
        old_versions=($(grep '"version"' "$ALL_VERSIONS_JSON_PATH" | sed 's/.*: *"\([^"]*\)".*/\1/'))
        old_datetimes=($(grep '"datetime"' "$ALL_VERSIONS_JSON_PATH" | sed 's/.*: *"\([^"]*\)".*/\1/'))
    fi
fi

# Helper: get old datetime for a version
get_old_datetime() {
    local search_version="$1"
    local i=0
    while [ $i -lt ${#old_versions[@]} ]; do
        if [ "${old_versions[$i]}" = "$search_version" ]; then
            echo "${old_datetimes[$i]}"
            return
        fi
        i=$((i+1))
    done
    echo ""
}

# Start JSON array
echo '[' > "$ALL_VERSIONS_JSON_PATH.tmp"
first=1

for version in $(ls -1 "$DOCS_V_PATH" | sort); do
    # Skip the 'Latest' directory (case-insensitive, POSIX compatible)
    if [ "$(echo "$version" | tr '[:upper:]' '[:lower:]')" = "latest" ]; then
        continue
    fi
    version_path="$DOCS_V_PATH/$version"
    if [ -d "$version_path" ]; then
        version_json="$version_path/version.json"
        firmware_bin="$version_path/firmware.bin"
        if [ -f "$version_json" ] && [ -f "$firmware_bin" ]; then
            # Try to parse fields from version.json
            if command -v jq >/dev/null 2>&1; then
                v_version=$(jq -r '.version // "'"$version"'"' "$version_json")
                v_major=$(jq -r '.major // empty' "$version_json")
                v_minor=$(jq -r '.minor // empty' "$version_json")
                v_patch=$(jq -r '.patch // empty' "$version_json")
            else
                v_version=$(grep -o '"version"[ ]*:[ ]*"[^"]*"' "$version_json" | head -n1 | sed 's/.*: *"\([^"]*\)"/\1/')
                v_major=$(grep -o '"major"[ ]*:[ ]*[0-9]*' "$version_json" | head -n1 | sed 's/.*: *\([0-9]*\)/\1/')
                v_minor=$(grep -o '"minor"[ ]*:[ ]*[0-9]*' "$version_json" | head -n1 | sed 's/.*: *\([0-9]*\)/\1/')
                v_patch=$(grep -o '"patch"[ ]*:[ ]*[0-9]*' "$version_json" | head -n1 | sed 's/.*: *\([0-9]*\)/\1/')
            fi
            # Fallbacks if fields are missing
            [ -z "$v_version" ] && v_version="$version"
            [ -z "$v_major" ] && v_major=0
            [ -z "$v_minor" ] && v_minor=0
            [ -z "$v_patch" ] && v_patch=0
            # Determine datetime: preserve if present, else use now
            v_datetime="$(get_old_datetime "$v_version")"
            if [ -z "$v_datetime" ]; then
                v_datetime=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
            fi
            # Add comma if not the first entry
            if [ $first -eq 0 ]; then
                echo ',' >> "$ALL_VERSIONS_JSON_PATH.tmp"
            fi
            first=0
            # Write JSON object
            echo '    {' >> "$ALL_VERSIONS_JSON_PATH.tmp"
            echo "        \"version\": \"$v_version\"," >> "$ALL_VERSIONS_JSON_PATH.tmp"
            echo "        \"major\": $v_major," >> "$ALL_VERSIONS_JSON_PATH.tmp"
            echo "        \"minor\": $v_minor," >> "$ALL_VERSIONS_JSON_PATH.tmp"
            echo "        \"patch\": $v_patch," >> "$ALL_VERSIONS_JSON_PATH.tmp"
            echo "        \"url\": \"./v/$version/version.json\"," >> "$ALL_VERSIONS_JSON_PATH.tmp"
            echo "        \"firmware\": \"./v/$version/firmware.bin\"," >> "$ALL_VERSIONS_JSON_PATH.tmp"
            echo "        \"datetime\": \"$v_datetime\"" >> "$ALL_VERSIONS_JSON_PATH.tmp"
            echo -n '    }' >> "$ALL_VERSIONS_JSON_PATH.tmp"
        fi
    fi
done

echo '' >> "$ALL_VERSIONS_JSON_PATH.tmp"
echo ']' >> "$ALL_VERSIONS_JSON_PATH.tmp"
mv "$ALL_VERSIONS_JSON_PATH.tmp" "$ALL_VERSIONS_JSON_PATH"
