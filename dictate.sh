#!/bin/bash

set -e

# Load environment variables from .env
if [ -f "$HOME/.env" ]; then
  source "$HOME/.env"
fi

mkdir -p ./tmp
AUDIO_FILE="./tmp/whisper-recording-$$.wav"

echo "🎤 Recording... Press Ctrl+C to stop"
pw-record --channels=1 --rate=16000 "$AUDIO_FILE" &
RECORD_PID=$!

trap "kill $RECORD_PID 2>/dev/null; echo" INT
wait $RECORD_PID 2>/dev/null || true

# Transcribe with Groq Whisper API
TRANSCRIPTION=$(curl -s -X POST "https://api.groq.com/openai/v1/audio/transcriptions" \
  -H "Authorization: Bearer $GROQ_API_KEY" \
  -H "Content-Type: multipart/form-data" \
  -F "file=@$AUDIO_FILE" \
  -F "model=whisper-large-v3-turbo" \
  | jq -r '.text')

echo "$TRANSCRIPTION"

# Type the transcription using wtype
wtype "$TRANSCRIPTION"

rm -f "$AUDIO_FILE"
