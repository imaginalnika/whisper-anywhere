#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10,<3.11"
# dependencies = ["faster-whisper>=1.0.0"]
# ///
"""Local transcription using faster-whisper."""

import sys
import argparse
from faster_whisper import WhisperModel

def transcribe(audio_file, model_size="medium"):
    # Load model (cached after first run)
    model = WhisperModel(model_size, device="cuda", compute_type="float16")

    segments, info = model.transcribe(audio_file, beam_size=5)
    text = " ".join([segment.text for segment in segments])
    return text.strip()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Transcribe audio using faster-whisper")
    parser.add_argument("audio_file", help="Path to audio file")
    parser.add_argument("-m", "--model",
                        choices=["base", "small", "medium"],
                        default="medium",
                        help="Model size (default: medium)")

    args = parser.parse_args()

    result = transcribe(args.audio_file, args.model)
    print(result)
