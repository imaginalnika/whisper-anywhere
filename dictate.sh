#!/bin/bash

# TODO:
# - App-awareness
# - Take current context in (via select all)
# - Enhance via llm and system prompt (with faster stt)
# - Command mode
# - Command mode in highlighted region
# - Fix leading single whitespace
# - Add visual indicator to Niri top bar

# Load environment variables from .env
if [ -f "$HOME/.env" ]; then
  source "$HOME/.env"
fi

RECORDING="/tmp/chung.wav"
LOGFILE="/tmp/chung.log"
RIGHTALT="100:1 100:0"  # Right alt toggles keyd layer

trim() {
  echo "$1" | sed 's/^[[:space:]]*//'
}

paste() {
  ydotool key $RIGHTALT
  printf '%s' "$1" | ydotool type --file - --key-delay 0
  ydotool key $RIGHTALT
}

transcribe() {
  local system_prompt="$1"

  curl -s -X POST "https://api.groq.com/openai/v1/audio/transcriptions" \
    -H "Authorization: Bearer $GROQ_API_KEY" \
    -H "Content-Type: multipart/form-data" \
    -F "file=@$RECORDING" \
    -F "model=whisper-large-v3-turbo" \
    -F "prompt=$system_prompt" \
    | jq -r '.text'
}

# Main

# Check if pw-record is running
if pgrep -f "pw-record.*$RECORDING" > /dev/null; then
  # Killing the process flushes to file
  pkill -f "pw-record.*$RECORDING"; sleep 0.2 # Buffer for flush

  SYSTEM_PROMPT=""
  TRANSCRIPTION=$(trim "$(transcribe "$SYSTEM_PROMPT")")

  echo "=== Transcription Debug ===" >> "$LOGFILE"
  echo "Raw transcription: [$TRANSCRIPTION]" >> "$LOGFILE"
  echo "Length: ${#TRANSCRIPTION}" >> "$LOGFILE"

  paste "$TRANSCRIPTION"

  rm -f "$RECORDING"
else
  # No recording running, so start
  TIMEOUT=60 # seconds
  timeout $TIMEOUT pw-record --channels=1 --rate=16000 "$RECORDING"
fi
