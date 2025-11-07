#!/bin/bash
# Build script for yell

set -e

echo "Building pasted (daemon)..."
gcc -O2 -Wall -Wextra pasted.c -o pasted

echo "Building paste (client)..."
gcc -O2 -Wall -Wextra paste.c -o paste

echo "Building typed (daemon)..."
gcc -O2 -Wall -Wextra typed.c -o typed

echo "Building typer (client)..."
gcc -O2 -Wall -Wextra typer.c -o typer

echo ""
echo "Done! Binaries: ./pasted ./paste ./typed ./typer"
echo ""
echo "To use:"
echo "  1. Start daemons: ./pasted & ./typed &"
echo "  2. Paste via clipboard: ./paste"
echo "  3. Type ASCII chars: ./typer a"
echo "  4. Backspace: ./typer backspace"
echo ""
echo "Make sure your user is in the 'input' group:"
echo "  sudo usermod -aG input \$USER"
echo "  (then log out and back in)"
