#!/bin/bash

# sudo dnf install -y pipewire pipewire-utils ydotool jq curl
# also follow instructions on https://git.sr.ht/~geb/dotool

# Load environment variables from .env
if [ -f "$HOME/.env" ]; then
  source "$HOME/.env"
fi

RECORDING="/tmp/chung.wav"
LOGFILE="/tmp/chung.log"
PROCESS_PATTERN="pw-record.*$RECORDING"

paste() {
  # rightalt is specific to my setup with dvorak and keyd
  echo key rightalt | dotool
  echo type "$1" | dotool
  echo key rightalt | dotool
}

transcribe() {
  local recording="$1"
  local start=$(date +%s%N)

  # Transcription always returns a leading space, so remove it
  local result=$(curl -s -X POST "https://api.groq.com/openai/v1/audio/transcriptions" \
    -H "Authorization: Bearer $GROQ_API_KEY" \
    -H "Content-Type: multipart/form-data" \
    -F "file=@$recording" \
    -F "model=whisper-large-v3-turbo" \
    -F "prompt=Programming dictation with terms like left paren, right paren, left bracket, right bracket." \
    | jq -r '.text' | sed 's/^ //')

  local end=$(date +%s%N)
  local time=$(echo "scale=3; ($end - $start) / 1000000000" | bc)

  # If transcription took less than 0.2 seconds, likely too short
  if (( $(echo "$time < 0.2" | bc -l) )); then
    echo "=== Transcription ===" >> "$LOGFILE"
    echo "Result: [$result] (rejected: too short)" >> "$LOGFILE"
    echo "Time: ${time}s" >> "$LOGFILE"
    echo ""
    return
  fi

  echo "=== Transcription ===" >> "$LOGFILE"
  echo "Result: [$result]" >> "$LOGFILE"
  echo "Time: ${time}s" >> "$LOGFILE"

  echo "$result"
}

enhance() {
  local text="$1"

  # If input is empty, return empty
  if [ -z "$text" ]; then
    echo "=== Enhancement ===" >> "$LOGFILE"
    echo "Result: (skipped: empty input)" >> "$LOGFILE"
    echo ""
    return
  fi

  local start=$(date +%s%N)

  local result=$(curl -s -X POST "https://api.anthropic.com/v1/messages" \
    -H "x-api-key: $ANTHROPIC_API_KEY" \
    -H "anthropic-version: 2023-06-01" \
    -H "content-type: application/json" \
    -d "{
      \"model\": \"claude-haiku-4-5-20251001\",
      \"max_tokens\": 1024,
      \"system\": \"You are a programming assistant processing voice dictation. Understand the user's intent and convert spoken code to proper syntax. Common patterns: 'left/right paren/bracket/brace' become their symbols, 'dot' becomes '.', 'equals' becomes '=', etc. Fix grammar, formatting, and make the output natural for the context (code, commands, or prose). Be flexible in interpretation. Return only the corrected text.\",
      \"messages\": [{
        \"role\": \"user\",
        \"content\": \"$text\"
      }]
    }" | jq -r '.content[0].text')

  local end=$(date +%s%N)
  local time=$(echo "scale=3; ($end - $start) / 1000000000" | bc)

  echo "=== Enhancement ===" >> "$LOGFILE"
  echo "Result: [$result]" >> "$LOGFILE"
  echo "Time: ${time}s" >> "$LOGFILE"

  echo "$result"
}

# Main

# Find recording process, if so then kill
if pgrep -f "$PROCESS_PATTERN" > /dev/null; then
  pkill -f "$PROCESS_PATTERN"; sleep 0.2 # Buffer for flush

  paste "$(enhance "$(transcribe "$RECORDING")")"

  rm -f "$RECORDING"
else
  # No recording running, so start
  pw-record --channels=1 --rate=16000 "$RECORDING"
fi
