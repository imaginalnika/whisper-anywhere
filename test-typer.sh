#!/bin/bash
# Test ASCII typing via typer (direct key simulation)

set -e

# Check if typer exists
if [ ! -f ./typer ]; then
    echo "Error: typer not found. Run ./build.sh first."
    exit 1
fi

echo "=== Testing typer (ASCII direct key typing) ==="
echo ""
echo "Make sure typed daemon is running: ./typed &"
echo ""
echo "Focus a text editor and press Enter in 3 seconds..."
sleep 3

echo ""
echo "--- Testing ASCII printable characters (32-126) ---"

# Test basic alphabet
echo "Typing: abcdefghijklmnopqrstuvwxyz"
for char in {a..z}; do
    ./typer "$char"
done

sleep 0.2
./typer " "  # space separator

# Test uppercase
echo "Typing: ABCDEFGHIJKLMNOPQRSTUVWXYZ"
for char in {A..Z}; do
    ./typer "$char"
done

sleep 0.2
./typer " "  # space separator

# Test numbers
echo "Typing: 0123456789"
for i in {0..9}; do
    ./typer "$i"
done

sleep 0.2
./typer " "  # space separator

# Test common punctuation and symbols
echo "Typing: !@#\$%^&*()_+-=[]{}\\|;:'\"<>,.?/\`~"
SYMBOLS='!@#$%^&*()_+-=[]{}\\|;:'"'"'"<>,.?/`~'
for ((i=0; i<${#SYMBOLS}; i++)); do
    char="${SYMBOLS:$i:1}"
    ./typer "$char"
done

echo ""
sleep 0.5

echo "--- Testing backspace ---"
echo "Typing 'test' then deleting 4 chars with backspace"
./typer "t"
./typer "e"
./typer "s"
./typer "t"
sleep 0.3
./typer backspace
./typer backspace
./typer backspace
./typer backspace

sleep 0.5

echo "--- Testing sentence ---"
echo "Typing: Hello, World! Testing 123."
TEXT="Hello, World! Testing 123."
for ((i=0; i<${#TEXT}; i++)); do
    char="${TEXT:$i:1}"
    ./typer "$char"
done

echo ""
echo "=== Test complete ==="
echo ""
echo "Expected output:"
echo "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 !@#\$%^&*()_+-=[]{}\\|;:'\"<>,.?/\`~ Hello, World! Testing 123."
echo ""
echo "Note: All printable ASCII characters (32-126) should type correctly."
