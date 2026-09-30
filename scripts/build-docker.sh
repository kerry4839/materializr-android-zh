#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="$PROJECT_DIR/dist"

echo "=== Building Materializr in Docker ==="

cd "$PROJECT_DIR"

# Build the Docker image
docker build -t materializr-builder .

# Extract the packaged binary + libs
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Create a container and copy the package out
CONTAINER_ID=$(docker create materializr-builder)
docker cp "$CONTAINER_ID:/package" "$OUTPUT_DIR/materializr"
docker rm "$CONTAINER_ID"

# Create a tarball for distribution
cd "$OUTPUT_DIR"
tar czf materializr-linux-$(uname -m).tar.gz materializr/

echo ""
echo "=== Build complete ==="
echo "Portable package: $OUTPUT_DIR/materializr-linux-$(uname -m).tar.gz"
echo ""
echo "To run:"
echo "  tar xzf materializr-linux-$(uname -m).tar.gz"
echo "  cd materializr"
echo "  ./materializr.sh"
