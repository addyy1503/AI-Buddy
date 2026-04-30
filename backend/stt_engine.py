"""
AI Buddy — Speech-to-Text Engine
==================================
Uses faster-whisper for local, fast speech recognition.
"""

import io
import os
import wave
import numpy as np
from pathlib import Path

# Suppress harmless HuggingFace Windows symlink warning
os.environ["HF_HUB_DISABLE_SYMLINKS_WARNING"] = "1"

from faster_whisper import WhisperModel


class STTEngine:
    def __init__(self, model_size: str = "base"):
        """
        Initialize Whisper model.
        
        Args:
            model_size: "tiny", "base", "small", "medium", or "large"
                        tiny = fastest (< 1s), base = good balance
        """
        print(f"[STT] Loading Whisper model '{model_size}'...")
        self.model = WhisperModel(
            model_size,
            device="cpu",      # Use "cuda" if you have an NVIDIA GPU
            compute_type="int8"  # Fastest on CPU
        )
        print("[STT] Model loaded!")

    def transcribe(self, pcm_data: bytes, sample_rate: int = 16000) -> str:
        """
        Transcribe raw PCM audio bytes to text.
        
        Args:
            pcm_data: Raw 16-bit signed PCM audio data
            sample_rate: Sample rate (should be 16000)
            
        Returns:
            Transcribed text string
        """
        if len(pcm_data) < 1000:
            return ""

        # Save raw audio for debugging
        debug_path = Path(__file__).parent / "debug_audio.wav"
        with wave.open(str(debug_path), 'wb') as dbg:
            dbg.setnchannels(1)
            dbg.setsampwidth(2)
            dbg.setframerate(sample_rate)
            dbg.writeframes(pcm_data)
        print(f"[STT] Debug audio saved to {debug_path}")

        # Normalize audio — INMP441 mic can be quiet
        samples = np.frombuffer(pcm_data, dtype=np.int16).copy()
        
        # Remove DC offset
        samples = samples - np.mean(samples).astype(np.int16)
        
        # Calculate RMS and boost if too quiet
        rms = np.sqrt(np.mean(samples.astype(np.float32) ** 2))
        print(f"[STT] Audio RMS: {rms:.0f}, samples: {len(samples)}")
        
        if rms < 100:
            print("[STT] Audio too quiet — boosting 8x")
            samples = np.clip(samples.astype(np.int32) * 8, -32768, 32767).astype(np.int16)
        elif rms < 500:
            print("[STT] Audio quiet — boosting 4x")
            samples = np.clip(samples.astype(np.int32) * 4, -32768, 32767).astype(np.int16)
        elif rms < 1000:
            print("[STT] Audio moderate — boosting 2x")
            samples = np.clip(samples.astype(np.int32) * 2, -32768, 32767).astype(np.int16)

        # Convert to WAV in memory
        pcm_data = samples.tobytes()
        wav_buffer = io.BytesIO()
        with wave.open(wav_buffer, 'wb') as wav_file:
            wav_file.setnchannels(1)        # Mono
            wav_file.setsampwidth(2)         # 16-bit
            wav_file.setframerate(sample_rate)
            wav_file.writeframes(pcm_data)
        wav_buffer.seek(0)

        # Transcribe — VAD filter DISABLED (INMP441 audio confuses it)
        segments, info = self.model.transcribe(
            wav_buffer,
            beam_size=3,
            language="en",
            vad_filter=False,
        )

        # Collect all segment texts
        text = " ".join(segment.text.strip() for segment in segments)
        
        if text:
            print(f"[STT] Transcribed: \"{text}\"")
        else:
            print("[STT] No speech detected")

        return text
