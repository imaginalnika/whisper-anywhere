#!/bin/bash

# Chung v1.0
# A dictation script that writes to the cursor.
# - Transcription via Groq Whisper
# - Enhancement via Anthropic Haiku 4.5 (for longer recordings)
# - Elegant processing message at the cursor

# Configuration:
# - LONG_RECORDING_THRESHOLD (threshold for using large model + enhancement)
# - TRANSCRIPTION_PROMPT (context for Whisper)
# - ENHANCEMENT_PROMPT (instructions for Haiku)

# Requirements (Fedora):
# - sudo dnf install -y pipewire pipewire-utils ydotool jq curl ffmpeg
# - dotool: https://git.sr.ht/~geb/dotool (run dotoold)

if [ -f "$HOME/.env" ]; then
  source "$HOME/.env"
fi

RECORDING="/tmp/chung.wav"
LOGFILE="/tmp/chung.log"
PROCESS_PATTERN="pw-record.*$RECORDING"
LONG_RECORDING_THRESHOLD=10 # s
TRANSCRIPTION_PROMPT="Programming dictation with terms like left paren, right paren, left bracket, right bracket."
ENHANCEMENT_PROMPT="You are a text processor in a voice dictation pipeline. Your job is to clean up transcribed speech: convert spoken code to symbols ('left paren' → '(', 'dot' → '.', etc.), fix grammar and formatting. Return ONLY the corrected text on a single line with no added newlines, no markdown formatting (no bold, italics, code blocks), no asterisks, no extra punctuation. Some keywords: clojure, emacs, vim, EdgeQL. DO NOT respond like chat."

paste() {
  # rightalt is specific to my setup with dvorak and keyd
  echo key rightalt | dotoolc
  echo type "$1" | dotoolc
  echo key rightalt | dotoolc
}

delete_n_chars() {
  local n="$1"
  for ((i=0; i<n; i++)); do
    echo key backspace | dotoolc
  done
}

get_duration() {
  local recording="$1"
  ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "$recording" 2>/dev/null || echo "0"
}

logging_end_and_write_to_logfile() {
  local title="$1"
  local result="$2"
  local logging_start="$3"

  local logging_end=$(date +%s%N)
  local time=$(echo "scale=3; ($logging_end - $logging_start) / 1000000000" | bc)

  echo "=== $title ===" >> "$LOGFILE"
  echo "Result: [$result]" >> "$LOGFILE"
  echo "Time: ${time}s" >> "$LOGFILE"
}

transcribe() {
  local recording="$1"
  local logging_start=$(date +%s%N)

  # Use large model for longer recordings, turbo for short ones
  local is_long_recording=$(echo "$(get_duration "$recording") > $LONG_RECORDING_THRESHOLD" | bc -l)
  local model=$([[ $is_long_recording -eq 1 ]] && echo "whisper-large-v3" || echo "whisper-large-v3-turbo")

  local transcription=$(curl -s -X POST "https://api.groq.com/openai/v1/audio/transcriptions" \
    -H "Authorization: Bearer $GROQ_API_KEY" \
    -H "Content-Type: multipart/form-data" \
    -F "file=@$recording" \
    -F "model=$model" \
    -F "prompt=$TRANSCRIPTION_PROMPT" \
    | jq -r '.text' | sed 's/^ //') # Transcription always returns a leading space, so remove it via sed

  logging_end_and_write_to_logfile "Transcription" "$transcription" "$logging_start"

  echo "$transcription"
}

enhance() {
  local text="$1"
  local logging_start=$(date +%s%N)

  local enhanced=$(curl -s -X POST "https://api.anthropic.com/v1/messages" \
    -H "x-api-key: $ANTHROPIC_API_KEY" \
    -H "anthropic-version: 2023-06-01" \
    -H "content-type: application/json" \
    -d "{
      \"model\": \"claude-haiku-4-5-20251001\",
      \"max_tokens\": 1024,
      \"system\": \"$ENHANCEMENT_PROMPT\",
      \"messages\": [{
        \"role\": \"user\",
        \"content\": \"$text\\n\\nCorrect the above.\"
      }]
    }" | jq -r '.content[0].text')

  logging_end_and_write_to_logfile "Enhancement" "$enhanced" "$logging_start"

  echo "$enhanced"
}

# Main

# Find recording process, if so then kill
if pgrep -f "$PROCESS_PATTERN" > /dev/null; then
  pkill -f "$PROCESS_PATTERN"; sleep 0.2 # Buffer for flush
  delete_n_chars 14 # "(recording...)"

  paste "(transcribing...)"
  TRANSCRIPTION=$(transcribe "$RECORDING")
  delete_n_chars 17 # "(transcribing...)"

  # Enhance longer recordings
  if (( $(echo "$(get_duration "$RECORDING") > $LONG_RECORDING_THRESHOLD" | bc -l) )); then
    paste "(enhancing...)"
    ENHANCED=$(enhance "$TRANSCRIPTION")
    delete_n_chars 15 # "(enhancing...)"
    paste "$ENHANCED"
  else
    paste "$TRANSCRIPTION"
  fi

  rm -f "$RECORDING"
else
  # No recording running, so start
  sleep 0.3
  paste "(recording...)"
  pw-record --channels=1 --rate=16000 "$RECORDING"
fi
