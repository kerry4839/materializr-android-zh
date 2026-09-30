#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$PROJECT_DIR/dist"
ARCH=$(uname -m)

echo "=== Building Materializr AppImage (${ARCH}) ==="

cd "$PROJECT_DIR"

# Build in Docker and extract the AppImage
DOCKER_BUILDKIT=1 docker build --output type=local,dest="$OUTPUT_DIR" .

echo ""
echo "=== Build complete ==="
echo "AppImage: $OUTPUT_DIR/Materializr-${ARCH}.AppImage"
echo ""
echo "To run:"
echo "  chmod +x $OUTPUT_DIR/Materializr-${ARCH}.AppImage"
echo "  $OUTPUT_DIR/Materializr-${ARCH}.AppImage"
