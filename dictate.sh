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

LOCK_FILE="/tmp/chung.lock"
PID_FILE="/tmp/chung.pid"

# Check if already recording
if [ -f "$LOCK_FILE" ]; then
  # Stop recording
  if [ -f "$PID_FILE" ]; then
    RECORD_PID=$(cat "$PID_FILE")
    kill "$RECORD_PID" 2>/dev/null || true
    rm -f "$PID_FILE"
  fi

  # Wait a moment for the file to finalize
  sleep 0.2

  AUDIO_FILE=$(cat "$LOCK_FILE")
  rm -f "$LOCK_FILE"

  # Transcribe with Groq Whisper API
  TRANSCRIPTION=$(curl -s -X POST "https://api.groq.com/openai/v1/audio/transcriptions" \
    -H "Authorization: Bearer $GROQ_API_KEY" \
    -H "Content-Type: multipart/form-data" \
    -F "file=@$AUDIO_FILE" \
    -F "model=whisper-large-v3-turbo" \
    | jq -r '.text')

  # Type the transcription using ydotool (toggle keyd to normal QWERTY layer with Rightalt)
  ydotool key 100:1 100:0  # Press Rightalt to toggle to tweaks layer
  printf '%s' "$TRANSCRIPTION" | ydotool type --file - --key-delay 0
  ydotool key 100:1 100:0  # Press Rightalt again to toggle back

  rm -f "$AUDIO_FILE"
else
  # Start recording
  AUDIO_FILE="/tmp/chung-recording-$$.wav"

  echo "$AUDIO_FILE" > "$LOCK_FILE"

  pw-record --channels=1 --rate=16000 "$AUDIO_FILE" &
  RECORD_PID=$!
  echo "$RECORD_PID" > "$PID_FILE"
fi
