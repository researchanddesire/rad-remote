#!/bin/bash

NEXT_VERSION=$(./scripts/get_next_version.sh "$@")
echo "Next version: $NEXT_VERSION"
