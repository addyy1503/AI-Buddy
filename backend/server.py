"""
AI Buddy — HTTP + WebSocket Server
====================================
Receives audio from ESP32 via HTTP POST or WebSocket,
processes through AI pipeline (STT -> LLM -> TTS),
sends back response.

Endpoints:
  POST /talk    - Send raw PCM audio, get JSON + base64 audio back
  GET  /ping    - Health check
  WS   /ws      - WebSocket (optional, for future use)

Usage:
  1. Copy .env.example to .env and add your GEMINI_API_KEY
  2. pip install -r requirements.txt
  3. python server.py
"""

import os
import sys
import json
import base64
import asyncio
import io
import pygame
from pathlib import Path

# Initialize pygame mixer for local audio playback
pygame.mixer.init()

# Fix Windows console encoding
if sys.platform == "win32":
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    sys.stderr.reconfigure(encoding='utf-8', errors='replace')

from dotenv import load_dotenv
from fastapi import FastAPI, Request, WebSocket, WebSocketDisconnect
from fastapi.responses import JSONResponse
import uvicorn

from stt_engine import STTEngine
from tts_engine import TTSEngine
from ai_pipeline import AIPipeline

# Load environment variables
env_path = Path(__file__).parent / ".env"
load_dotenv(env_path)

# ── Initialize AI components ──
print("=" * 50)
print("  AI Buddy -- Backend Server")
print("=" * 50)

stt = STTEngine(model_size=os.getenv("WHISPER_MODEL", "base"))
tts = TTSEngine(voice=os.getenv("TTS_VOICE", "en"))
ai = AIPipeline(
    provider=os.getenv("AI_PROVIDER", "gemini"),
    personality_name=os.getenv("PERSONALITY", "snarky"),
)

# ── FastAPI App ──
app = FastAPI()


@app.get("/ping")
async def ping():
    """Health check endpoint."""
    return {"status": "ok", "device": "ai-buddy-server"}


@app.post("/talk")
async def talk(request: Request):
    """
    Receive raw PCM audio, process through AI pipeline,
    return JSON with emotion, text, and base64-encoded audio.
    """
    # Read raw PCM audio from body
    pcm_data = await request.body()
    print(f"\n[SERVER] >> Received {len(pcm_data)} bytes of audio via HTTP")

    if len(pcm_data) == 0:
        return JSONResponse({"emotion": 0, "text": "", "audio_b64": ""})

    try:
        # Step 1: Speech-to-Text
        print("[SERVER] Step 1/3: Transcribing...")
        text = stt.transcribe(pcm_data, sample_rate=16000)

        if not text or not text.strip():
            print("[SERVER] No speech detected")
            return JSONResponse({"emotion": 0, "text": "I didn't catch that", "audio_b64": ""})

        print(f'[SERVER] Heard: "{text}"')

        # Step 2: Generate AI response
        print("[SERVER] Step 2/3: Generating response...")
        response_text, emotion = ai.generate_response(text)
        short = response_text[:60] + "..." if len(response_text) > 60 else response_text
        print(f'[SERVER] Response: "{short}" (emotion={emotion})')

        # Step 3: Text-to-Speech
        print("[SERVER] Step 3/3: Synthesizing speech...")
        response_audio, wav_data = await tts.synthesize_async(response_text)

        print("[SERVER] Playing audio on laptop speaker...")
        try:
            wav_fp = io.BytesIO(wav_data)
            pygame.mixer.music.load(wav_fp)
            pygame.mixer.music.play()
        except Exception as e:
            print(f"[SERVER] Failed to play audio locally: {e}")

        # Send empty audio data to the ESP32 so it doesn't try to play it
        # It will just show the matching facial expression
        audio_b64 = ""

        print(f"[SERVER] [OK] Done! emotion={emotion}, audio={len(response_audio)} bytes")

        audio_duration_ms = int((len(response_audio) / 32000.0) * 1000)

        return JSONResponse({
            "emotion": emotion,
            "text": response_text[:100],
            "audio_b64": audio_b64,
            "duration_ms": audio_duration_ms
        })

    except Exception as e:
        print(f"[SERVER] [ERR] Pipeline error: {e}")
        import traceback
        traceback.print_exc()
        return JSONResponse({
            "emotion": 2,
            "text": "Something went wrong",
            "audio_b64": ""
        })


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """WebSocket endpoint (kept for compatibility)."""
    await websocket.accept()
    print("[SERVER] ESP32 connected via WebSocket!")

    audio_buffer = bytearray()
    try:
        while True:
            msg = await websocket.receive()
            if "bytes" in msg:
                audio_buffer.extend(msg["bytes"])
            elif "text" in msg:
                data = json.loads(msg["text"])
                if data.get("type") == "audio_end" and len(audio_buffer) > 0:
                    print(f"[SERVER] WS: Received {len(audio_buffer)} bytes")
                    # Process same pipeline
                    text = stt.transcribe(bytes(audio_buffer))
                    if text and text.strip():
                        response_text, emotion = ai.generate_response(text)
                        response_audio, mp3_data = await tts.synthesize_async(response_text)
                        
                        try:
                            mp3_fp = io.BytesIO(mp3_data)
                            pygame.mixer.music.load(mp3_fp)
                            pygame.mixer.music.play()
                        except Exception as e:
                            pass
                        
                        await websocket.send_text(json.dumps({
                            "emotion": emotion, "text": response_text[:100],
                            "audio_follows": False
                        }))
                        # Skip sending audio bytes over WS
                        # await websocket.send_bytes(response_audio)
                    audio_buffer.clear()
    except WebSocketDisconnect:
        print("[SERVER] ESP32 disconnected from WebSocket")


if __name__ == "__main__":
    print("\n[SERVER] Starting on http://0.0.0.0:8765")
    print("[SERVER] POST /talk  - Send audio, get response")
    print("[SERVER] GET  /ping  - Health check")
    print("[SERVER] Waiting for ESP32...\n")
    uvicorn.run(app, host="0.0.0.0", port=8765, log_level="info")
