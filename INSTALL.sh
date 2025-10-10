#!/bin/bash

set -e

# Check if running on Fedora
if [ ! -f /etc/fedora-release ]; then
    echo "Sorry, this install script only supports Fedora."
    exit 1
fi

echo "Installing Chung dependencies for Fedora..."
echo ""

# Install system dependencies
echo "Installing system packages..."
sudo dnf install -y pipewire pipewire-utils ydotool jq curl nvidia-driver
echo ""

# Install uv if not present
if ! command -v uv &> /dev/null; then
    echo "Installing uv..."
    curl -LsSf https://astral.sh/uv/install.sh | sh
    export PATH="$HOME/.cargo/bin:$PATH"
fi

# Install Python 3.10 (required for av dependency compatibility)
echo "Installing Python 3.10..."
uv python install 3.10
echo ""

# Create virtual environment with Python 3.10
if [ -d ".venv" ]; then
    VENV_PYTHON_VERSION=$(.venv/bin/python --version 2>&1 | grep -oP '3\.\d+')
    if [ "$VENV_PYTHON_VERSION" != "3.10" ]; then
        echo "Existing venv is Python $VENV_PYTHON_VERSION, recreating with 3.10..."
        rm -rf .venv
        uv venv --python 3.10
    else
        echo "Virtual environment already exists (Python 3.10), skipping..."
    fi
else
    echo "Creating virtual environment..."
    uv venv --python 3.10
fi
echo ""

# Install Python dependencies
echo "Installing Python dependencies..."
source .venv/bin/activate
uv pip install -e .
echo ""

# Make dictate.sh executable
chmod +x ./dictate.sh

echo ""
echo "Installation complete!"
echo ""

# Check if GROQ_API_KEY is set
if [ -f "$HOME/.env" ] && grep -q "GROQ_API_KEY" "$HOME/.env" 2>/dev/null; then
    echo "✓ GROQ_API_KEY configured"
else
    echo "✗ GROQ_API_KEY not found - add to ~/.env"
fi

echo "✓ dictate.sh is executable"
echo ""
echo "Don't forget to bind dictate.sh to a keyboard shortcut"
