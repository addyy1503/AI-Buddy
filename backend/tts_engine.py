"""
AI Buddy — Text-to-Speech Engine
===================================
Uses pyttsx3 for fully offline text-to-speech.
Outputs raw PCM audio at 16kHz 16-bit mono for the ESP32.
"""

import io
import os
import asyncio
import pyttsx3
from pydub import AudioSegment


class TTSEngine:
    def __init__(self, voice: str = "en"):
        """
        Initialize offline TTS engine using pyttsx3.
        """
        self.voice = voice
        print(f"[TTS] Using OFFLINE pyttsx3 Engine")

    async def synthesize_async(self, text: str) -> tuple[bytes, bytes]:
        """
        Convert text to raw PCM audio bytes (async wrapper).
        """
        return await asyncio.to_thread(self.synthesize, text)

    def synthesize(self, text: str) -> tuple[bytes, bytes]:
        """
        Convert text to raw PCM audio bytes (sync).
        """
        if not text:
            return b"", b""

        print(f"[TTS] Synthesizing: \"{text[:60]}{'...' if len(text) > 60 else ''}\"")

        # Initialize inside thread to avoid Windows COM apartment threading errors
        engine = pyttsx3.init()
        # Set voice rate (speed)
        engine.setProperty('rate', 160)
        
        # Set voice to female
        voices = engine.getProperty('voices')
        for v in voices:
            if "female" in v.name.lower() or "zira" in v.name.lower():
                engine.setProperty('voice', v.id)
                break
        else:
            if len(voices) > 1:
                engine.setProperty('voice', voices[1].id)
        
        # Save to temp file since pyttsx3 only outputs to files natively
        temp_wav = "temp_tts.wav"
        engine.save_to_file(text, temp_wav)
        engine.runAndWait()

        # Load the generated WAV file
        audio = AudioSegment.from_wav(temp_wav)
        
        # Export a copy to a memory buffer for Pygame to play locally
        wav_buffer = io.BytesIO()
        audio.export(wav_buffer, format="wav")
        wav_data = wav_buffer.getvalue()

        # Convert to 16kHz, 16-bit, mono for the ESP32
        audio = audio.set_frame_rate(16000).set_channels(1).set_sample_width(2)
        pcm_data = audio.raw_data

        print(f"[TTS] Generated {len(pcm_data)} bytes of PCM audio "
              f"({len(pcm_data) / 32000:.1f} sec)")

        # Cleanup temp file
        try:
            if os.path.exists(temp_wav):
                os.remove(temp_wav)
        except Exception:
            pass

        return pcm_data, wav_data
