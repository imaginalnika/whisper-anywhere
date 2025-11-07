#!/bin/bash
# Test single-character paste for both ASCII and international characters

set -e

# Parse arguments
CHUNK_SIZE=1
while getopts "n:" opt; do
    case $opt in
        n) CHUNK_SIZE=$OPTARG ;;
        *) echo "Usage: $0 [-n chunk_size]"; exit 1 ;;
    esac
done

# Check if paste exists
if [ ! -f ./paste ]; then
    echo "Error: paste not found. Run ./build.sh first."
    exit 1
fi

# Detect clipboard tool
if command -v wl-copy &> /dev/null; then
    CLIP_COPY="wl-copy"
elif command -v xclip &> /dev/null; then
    CLIP_COPY() { xclip -selection clipboard; }
else
    echo "Error: No clipboard tool found. Install wl-clipboard or xclip."
    exit 1
fi

echo "=== Testing paste with chunk size: $CHUNK_SIZE ==="
echo ""
echo "Make sure pasted daemon is running: ./pasted &"
echo ""
echo "Focus a text editor and press Enter in 3 seconds..."
sleep 3

echo ""
echo "--- Testing ASCII characters ---"
ASCII="abcABC123 !?"
echo "Testing: $ASCII"
for ((i=0; i<${#ASCII}; i+=CHUNK_SIZE)); do
    chunk="${ASCII:$i:$CHUNK_SIZE}"
    echo -n "$chunk" | $CLIP_COPY
    ./paste
done

sleep 0.5

echo ""
echo "--- Testing international/Unicode characters ---"
INTL="éàèùçäöüß你好世界مرحباМосква日本語ひらがなカタカナ한글สวัสดีשลוםΕλληνικά😊🎉✨"
echo "Testing: $INTL"
for ((i=0; i<${#INTL}; i+=CHUNK_SIZE)); do
    chunk="${INTL:$i:$CHUNK_SIZE}"
    echo -n "$chunk" | $CLIP_COPY
    ./paste
    sleep 0.1
done

echo ""
echo "=== Test complete ==="
echo "Expected ASCII: $ASCII"
echo "Expected International: $INTL"
echo ""
echo "Note: ASCII single chars may not paste (environment-specific),"
echo "but international/Unicode chars should paste successfully."
