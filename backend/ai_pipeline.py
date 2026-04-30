"""
AI Buddy — AI Pipeline
========================
Pluggable LLM integration: Gemini (default), OpenAI, or Ollama.
Manages conversation memory and personality context.
"""

import os
from datetime import datetime
from personality import get_personality, extract_emotion


class AIPipeline:
    def __init__(self, provider: str = "gemini", personality_name: str = "snarky"):
        """
        Initialize the AI pipeline.
        
        Args:
            provider: "gemini", "openai", or "ollama"
            personality_name: Key from PERSONALITIES dict
        """
        self.provider = provider
        self.personality = get_personality(personality_name)
        self.conversation_history = []
        self.max_history = 10  # Keep last N turns for context

        print(f"[AI] Provider: {provider}")
        print(f"[AI] Personality: {self.personality['name']}")

        self._init_provider()
        self.memory_facts = self._load_memory()

    def _load_memory(self):
        import json
        import os
        self.memory_file = os.path.join(os.path.dirname(__file__), "memory.json")
        if os.path.exists(self.memory_file):
            try:
                with open(self.memory_file, "r") as f:
                    return json.load(f)
            except Exception:
                return []
        return []

    def _save_memory(self, fact):
        import json
        memory = self._load_memory()
        memory.append(fact)
        with open(self.memory_file, "w") as f:
            json.dump(memory, f, indent=4)

    def _init_provider(self):
        """Initialize the chosen LLM provider."""
        if self.provider == "gemini":
            import google.generativeai as genai
            api_key = os.getenv("GEMINI_API_KEY")
            if not api_key or api_key == "your_gemini_api_key_here":
                raise ValueError("Set GEMINI_API_KEY in .env file! "
                                 "Get one at https://aistudio.google.com/apikey")
            genai.configure(api_key=api_key)
            self.model = genai.GenerativeModel("gemini-2.0-flash-lite")
            self.chat = self.model.start_chat(history=[])
            print("[AI] Gemini initialized (2.0-flash-lite)")

        elif self.provider == "openai":
            from openai import OpenAI
            api_key = os.getenv("OPENAI_API_KEY")
            if not api_key:
                raise ValueError("Set OPENAI_API_KEY in .env file!")
            self.client = OpenAI(api_key=api_key)
            print("[AI] OpenAI initialized")

        elif self.provider == "ollama":
            import requests
            host = os.getenv("OLLAMA_HOST", "http://localhost:11434")
            self.ollama_host = host
            self.ollama_model = os.getenv("OLLAMA_MODEL", "llama3.2")
            # Test connection
            try:
                resp = requests.get(f"{host}/api/tags", timeout=5)
                resp.raise_for_status()
                print(f"[AI] Ollama connected ({self.ollama_model})")
            except Exception as e:
                raise ConnectionError(f"Can't connect to Ollama at {host}: {e}")

        else:
            raise ValueError(f"Unknown AI provider: {self.provider}")

    def generate_response(self, user_text: str) -> tuple[str, int]:
        """
        Generate AI response from user's transcribed speech.
        
        Args:
            user_text: What the user said (from STT)
            
        Returns:
            Tuple of (response_text, emotion_id)
        """
        if not user_text.strip():
            return "I didn't catch that. Say again?", 0

        # Check for local computer actions first!
        action_result = self._check_for_actions(user_text)
        if action_result:
            response_text, emotion_id = action_result
            print(f"[AI] Action Executed: \"{response_text}\" (emotion: {emotion_id})")
            return response_text, emotion_id

        # Add time context and memory to the system prompt
        now = datetime.now()
        time_context = f"\nCurrent time: {now.strftime('%I:%M %p')}, {now.strftime('%A, %B %d')}."
        
        memory_context = ""
        self.memory_facts = self._load_memory()
        if self.memory_facts:
            memory_context = "\n\nFacts you must remember about the user:\n" + "\n".join([f"- {fact}" for fact in self.memory_facts])

        system_prompt = self.personality["system_prompt"] + time_context + memory_context

        print(f"[AI] User said: \"{user_text}\"")

        try:
            if self.provider == "gemini":
                response_text = self._gemini_generate(system_prompt, user_text)
            elif self.provider == "openai":
                response_text = self._openai_generate(system_prompt, user_text)
            elif self.provider == "ollama":
                response_text = self._ollama_generate(system_prompt, user_text)
            else:
                response_text = "Something broke. [sad]"
        except Exception as e:
            print(f"[AI] Error: {e}")
            response_text = "Ugh, my brain glitched. Try again? [sad]"

        # Extract emotion from response
        clean_text, emotion_id = extract_emotion(response_text)

        # Update conversation history
        self.conversation_history.append({
            "user": user_text,
            "assistant": clean_text
        })
        if len(self.conversation_history) > self.max_history:
            self.conversation_history.pop(0)

        print(f"[AI] Response: \"{clean_text}\" (emotion: {emotion_id})")
        return clean_text, emotion_id

    def _gemini_generate(self, system_prompt: str, user_text: str) -> str:
        """Generate response using Google Gemini."""
        # Build the prompt with conversation context
        full_prompt = f"{system_prompt}\n\n"

        # Add conversation history
        for turn in self.conversation_history[-5:]:
            full_prompt += f"User: {turn['user']}\nBuddy: {turn['assistant']}\n"

        full_prompt += f"User: {user_text}\nBuddy:"

        response = self.model.generate_content(full_prompt)
        return response.text.strip()

    def _openai_generate(self, system_prompt: str, user_text: str) -> str:
        """Generate response using OpenAI GPT."""
        messages = [{"role": "system", "content": system_prompt}]

        for turn in self.conversation_history[-5:]:
            messages.append({"role": "user", "content": turn["user"]})
            messages.append({"role": "assistant", "content": turn["assistant"]})

        messages.append({"role": "user", "content": user_text})

        response = self.client.chat.completions.create(
            model="gpt-4o-mini",
            messages=messages,
            max_tokens=150,
            temperature=0.8,
        )
        return response.choices[0].message.content.strip()

    def _ollama_generate(self, system_prompt: str, user_text: str) -> str:
        """Generate response using local Ollama."""
        import requests

        messages = [{"role": "system", "content": system_prompt}]

        for turn in self.conversation_history[-5:]:
            messages.append({"role": "user", "content": turn["user"]})
            messages.append({"role": "assistant", "content": turn["assistant"]})

        messages.append({"role": "user", "content": user_text})

        response = requests.post(
            f"{self.ollama_host}/api/chat",
            json={
                "model": self.ollama_model,
                "messages": messages,
                "stream": False,
                "options": {"num_predict": 40, "temperature": 0.8},
            },
            timeout=30,
        )
        response.raise_for_status()
        return response.json()["message"]["content"].strip()

    def _check_for_actions(self, text: str):
        """Check if the user is asking to perform local tasks."""
        import re
        import subprocess
        import webbrowser
        import os
        import urllib.parse
        import psutil
        
        text_lower = text.lower()
        responses = []
        final_emotion = 0
        
        # 0. LIVE SYSTEM TOOLS (Battery / CPU)
        if 'battery' in text_lower:
            battery = psutil.sensors_battery()
            if battery:
                plugged = "plugged in" if battery.power_plugged else "on battery power"
                responses.append(f"Battery is at {battery.percent}% and {plugged}.")
            else:
                responses.append("I cannot read the battery sensors right now.")
                final_emotion = 2
            
        if 'cpu' in text_lower or 'system status' in text_lower or 'system running' in text_lower:
            cpu = psutil.cpu_percent(interval=0.5)
            responses.append(f"System CPU usage is at {cpu}%.")
            
        # 0.5 LONG TERM MEMORY INTERCEPT
        mem_match = re.search(r'(?:remember that|memorize that|save the fact that)\s+(.+)', text_lower)
        if mem_match:
            fact = mem_match.group(1).strip()
            self._save_memory(fact)
            responses.append("I have committed that to memory.")
            final_emotion = 1
        
        # 1. DICTATION / NOTE-TAKING
        note_match = re.search(r'(?:take a note|write down|list down|note that)(?:\s+saying)?\s+(.+)', text_lower)
        if note_match:
            note_content = note_match.group(1).strip()
            desktop_path = os.path.join(os.path.expanduser("~"), "Desktop")
            note_file = os.path.join(desktop_path, "AI_Buddy_Notes.txt")
            
            try:
                with open(note_file, "a", encoding="utf-8") as f:
                    f.write(f"- {note_content}\n")
                if os.name == 'nt':
                    subprocess.Popen(['notepad.exe', note_file])
                responses.append("Note taken and opened.")
                final_emotion = 1
            except Exception as e:
                print(f"[ACTION ERR] Failed to take note: {e}")
                
        # 2. YOUTUBE — DIRECT VIDEO PLAYBACK
        yt_match = re.search(r'(?:play|search for)\s+(.+?)\s+(?:on youtube|in youtube)', text_lower)
        if not yt_match:
            yt_match = re.search(r'(?:youtube|play on youtube)\s+(.+)', text_lower)
            
        if yt_match:
            query = yt_match.group(1).strip()
            if query != "some music" and query != "music":
                import urllib.request
                encoded_query = urllib.parse.quote(query)
                search_url = f"https://www.youtube.com/results?search_query={encoded_query}"
                try:
                    html = urllib.request.urlopen(search_url).read().decode()
                    video_ids = re.findall(r'watch\?v=([a-zA-Z0-9_-]{11})', html)
                    if video_ids:
                        webbrowser.open(f"https://www.youtube.com/watch?v={video_ids[0]}")
                        responses.append(f"Playing {query} on YouTube.")
                        final_emotion = 1
                    else:
                        webbrowser.open(search_url)
                        responses.append(f"Searching for {query} on YouTube.")
                        final_emotion = 1
                except Exception:
                    webbrowser.open(search_url)
                    responses.append(f"Searching for {query} on YouTube.")
                    final_emotion = 1
            else:
                webbrowser.open("https://www.youtube.com/watch?v=jfKfPfyJRdk")
                responses.append("Playing some relaxing beats!")
                final_emotion = 1
            
        # 3. SPECIFIC OS FOLDERS
        if 'open' in text_lower and 'folder' in text_lower:
            target_dir = None
            folder_name = ""
            user_dir = os.path.expanduser("~")
            
            if 'screenshot' in text_lower:
                target_dir = os.path.join(user_dir, "Pictures", "Screenshots")
                folder_name = "Screenshots"
            elif 'download' in text_lower:
                target_dir = os.path.join(user_dir, "Downloads")
                folder_name = "Downloads"
            elif 'document' in text_lower:
                target_dir = os.path.join(user_dir, "Documents")
                folder_name = "Documents"
                
            if target_dir and os.path.exists(target_dir):
                if os.name == 'nt':
                    subprocess.Popen(f'explorer "{target_dir}"')
                responses.append(f"Opening {folder_name} folder.")
                final_emotion = 1
                
        # 4. NEW SPECIFIC SITES (LinkedIn, GitHub, Mail)
        if 'linkedin' in text_lower:
            webbrowser.open('https://www.linkedin.com/')
            responses.append("Opening LinkedIn.")
            final_emotion = 1
            
        if 'github' in text_lower:
            webbrowser.open('https://github.com/')
            responses.append("Opening GitHub.")
            final_emotion = 1
            
        if 'mail' in text_lower or 'email' in text_lower:
            webbrowser.open('https://mail.google.com/')
            responses.append("Checking your emails.")
            final_emotion = 1
            
        # Compile response if any actions were taken
        if responses:
            final_text = "Right away! " + " ".join(responses)
            return final_text, final_emotion
            
        return None
