import os
import logging
from flask import Flask, request, jsonify, send_file
import speech_recognition as sr
from pydub import AudioSegment
import json
import requests
import time
import hmac
import hashlib

app = Flask(__name__)

EXECUTE_COMMANDS = True 
DEBUG_AUDIO_FILE = "last_recorded_audio.wav"

CLIENT_ID = "ctjhe3nkj3rqmwcqac85"
CLIENT_SECRET = "9c8f473221914a25a6ed8171a4c093a2"
DEVICE_ID = "bf9f74lawzj7ulxy"
REGION_URL = "https://openapi.tuyaeu.com"

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

try:
    recognizer = sr.Recognizer()
except:
    recognizer = None

def get_timestamp():
    return str(int(time.time() * 1000))

def calculate_sign(method, url, body_str, token=""):
    if not body_str:
        content_hash = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    else:
        content_hash = hashlib.sha256(body_str.encode('utf-8')).hexdigest().lower()

    string_to_sign = f"{method}\n{content_hash}\n\n{url}"
    t = get_timestamp()
    str_to_sign_final = CLIENT_ID + token + t + string_to_sign
    sign = hmac.new(
        CLIENT_SECRET.encode('utf-8'),
        msg=str_to_sign_final.encode('utf-8'),
        digestmod=hashlib.sha256
    ).hexdigest().upper()
    return sign, t

def get_access_token():
    url = "/v1.0/token?grant_type=1"
    sign, t = calculate_sign("GET", url, "")
    headers = {
        "client_id": CLIENT_ID, "sign": sign, "t": t, "sign_method": "HMAC-SHA256",
        "nonce": "", "stringToSign": ""
    }
    try:
        r = requests.get(REGION_URL + url, headers=headers)
        return r.json()['result']['access_token']
    except:
        return None

def control_device_tuya(action):
    if not EXECUTE_COMMANDS:
        return {"success": True, "message": f"Simulation: Device {action}"}
    token = get_access_token()
    if not token: return {"success": False, "error": "Token Failed"}
    url = f"/v1.0/devices/{DEVICE_ID}/commands"
    switch_val = True if action == "on" else False
    commands = {"commands": [{"code": "switch_led", "value": switch_val}]}
    body_str = json.dumps(commands, separators=(',', ':'))
    sign, t = calculate_sign("POST", url, body_str, token)
    headers = {
        "client_id": CLIENT_ID, "access_token": token, "sign": sign, "t": t,
        "sign_method": "HMAC-SHA256", "Content-Type": "application/json"
    }
    try:
        r = requests.post(REGION_URL + url, headers=headers, data=body_str)
        return r.json()
    except Exception as e:
        return {"success": False, "error": str(e)}

def analyze_text(text):
    if any(w in text for w in ['شغل', 'فتح', 'تشغيل', 'ولع', 'on']):
        return 'on'
    if any(w in text for w in ['طفي', 'سكر', 'اطفاء', 'إيقاف', 'off']):
        return 'off'
    return "unknown"

def process_audio_data(raw_data):
    if not raw_data: return {"success": False, "error": "No data"}
    temp_raw = f"temp_{os.urandom(8).hex()}.raw"
    try:
        with open(temp_raw, 'wb') as f:
            f.write(raw_data)
        audio = AudioSegment.from_raw(
            file=temp_raw,
            sample_width=2, 
            frame_rate=16000, 
            channels=1
        )
        audio.export(DEBUG_AUDIO_FILE, format="wav")
        with sr.AudioFile(DEBUG_AUDIO_FILE) as source:
            recognizer.adjust_for_ambient_noise(source, duration=0.1)
            audio_data = recognizer.record(source)
            text = recognizer.recognize_google(audio_data, language="ar-SA")
            action = analyze_text(text)
            execution_result = {}
            if action != "unknown":
                execution_result = control_device_tuya(action)
            return {
                "success": True, 
                "transcription": text, 
                "action": action,
                "execution": execution_result
            }
    except sr.UnknownValueError:
        return {"success": False, "error": "Google could not understand audio"}
    except Exception as e:
        return {"success": False, "error": str(e)}
    finally:
        if os.path.exists(temp_raw): os.remove(temp_raw)

@app.route("/transcribe", methods=["POST"])
def transcribe():
    try:
        raw_data = request.get_data()
        result = process_audio_data(raw_data)
        return jsonify(result)
    except Exception as e:
        return jsonify({"success": False, "error": str(e)}), 500

@app.route('/listen', methods=['GET'])
def listen():
    if os.path.exists(DEBUG_AUDIO_FILE):
        return send_file(DEBUG_AUDIO_FILE, mimetype="audio/wav", as_attachment=False)
    return "No audio recorded yet.", 404

@app.route('/')
def home():
    return "Server Ready"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)