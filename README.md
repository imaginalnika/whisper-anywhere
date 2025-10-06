# Whisper Wayland

Voice-to-text dictation for my Wayland setup using Groq's Whisper API.
(Highly specific to Nika's setup with keyd)

## Setup

Install dependencies:
```bash
sudo dnf install pipewire-utils curl jq ydotool
```

Add Groq API key to `~/.env`:
```bash
echo "GROQ_API_KEY=gsk_..." >> ~/.env
```

Bind to `dictate.sh` to hotkey in your favorite compositor (Niri).

## Usage

- Press keybind → starts recording
- Press again → transcribes and types text

## Notes

- Uses `ydotool` for typing (toggles keyd Rightalt layer for QWERTY)
- Audio: 16kHz mono via `pw-record`
- Model: `whisper-large-v3-turbo`
