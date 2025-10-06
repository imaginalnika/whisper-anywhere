# Whisper Wayland

STT using Groq.

## Setup

Install dependencies:
```bash
# Fedora
sudo dnf install pipewire-utils curl jq wtype

# Ubuntu/Debian
sudo apt install pipewire curl jq wtype

# Arch
sudo pacman -S pipewire curl jq wtype
```

Add [Groq API key](https://console.groq.com/) to `~/.env`:
```bash
echo "GROQ_API_KEY=your_key_here" > ~/.env
```

## Usage

```bash
chmod +x dictate.sh
./dictate.sh
```

Press `Ctrl+C` to stop recording.
