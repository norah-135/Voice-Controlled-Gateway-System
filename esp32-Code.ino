#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>

const char* ssid = "";
const char* password = "";
const char* serverHost = "(yourServer).pythonanywhere.com"; 

const char* arduinoAddress = "http://192.168.x.xxx"; 

#define I2S_SD 25 
#define I2S_WS 26
#define I2S_SCK 27
#define I2S_PORT I2S_NUM_0

const int sample_rate = 16000;
const int record_time = 3;
const int waveDataSize = record_time * sample_rate * 2; 

String systemStatus = "IDLE"; 
const int VAD_THRESHOLD = 1500; 
unsigned long lastStatusPrint = 0;

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = sample_rate,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK, .ws_io_num = I2S_WS, .data_out_num = I2S_PIN_NO_CHANGE, .data_in_num = I2S_SD
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
  Serial.begin(115200);
  setupI2S();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n[ESP32] Connected and Ready");
  systemStatus = "LISTENING";
}

void sendToArduinoLocal(String jsonPayload) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(arduinoAddress);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(jsonPayload); 
    
    if (httpCode > 0) {
      Serial.printf("[ESP32] Sent, Arduino response: %d\n", httpCode);
    } else {
      Serial.printf("[ESP32] Send error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
}

void recordAndSend() {
  systemStatus = "RECORDING";
  Serial.println("\n[STATUS: " + systemStatus + "] Recording...");
  
  uint8_t *buffer = (uint8_t*) malloc(waveDataSize);
  if (!buffer) return;

  size_t bytesRead;
  int32_t tempSample;
  int16_t *audioPtr = (int16_t*)buffer;
  
  for (int i = 0; i < waveDataSize/2; i++) {
    i2s_read(I2S_PORT, &tempSample, 4, &bytesRead, portMAX_DELAY);
    audioPtr[i] = (int16_t)(tempSample >> 14); 
  }
  
  systemStatus = "SENDING";
  Serial.println("[STATUS: " + systemStatus + "] Sending to Cloud...");

  HTTPClient http;
  http.begin("http://" + String(serverHost) + "/transcribe");
  http.addHeader("Content-Type", "application/octet-stream");
  
  int code = http.POST(buffer, waveDataSize);
  
  if (code == 200) {
    String response = http.getString();
    Serial.println("[STATUS: SUCCESS] Response received.");
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
        Serial.println("Response: " + response);

        if (doc.containsKey("stats")) {
            Serial.println("\n--- [ Server Performance ] ---");
            Serial.print("Memory Ops: "); 
            Serial.print(doc["stats"]["memory_ms"].as<int>()); Serial.println(" ms");
            
            Serial.print("Google AI: "); 
            Serial.print(doc["stats"]["speech_ai_ms"].as<int>()); Serial.println(" ms");
            
            Serial.print("Total Server: "); 
            Serial.print(doc["stats"]["total_server_ms"].as<int>()); Serial.println(" ms");
            Serial.println("------------------------------");
        }

        sendToArduinoLocal(response); 
    } else {
        Serial.println("JSON Parse Error");
    }
    
  } else {
    Serial.println("[STATUS: ERROR] Cloud Connection Failed");
  }
  
  http.end();
  free(buffer);
  
  systemStatus = "LISTENING"; 
  Serial.println("[STATUS: " + systemStatus + "] Waiting for voice...");
}

void loop() {
  size_t bytesRead;
  int32_t sample = 0;
  
  i2s_read(I2S_PORT, &sample, 4, &bytesRead, 10);
  int currentVolume = abs(sample >> 14);

  if (millis() - lastStatusPrint > 1000 && systemStatus == "LISTENING") {
    Serial.print("."); 
    lastStatusPrint = millis();
  }

  if (currentVolume > VAD_THRESHOLD && systemStatus == "LISTENING") {
    Serial.println("\nVoice detected: " + String(currentVolume));
    recordAndSend();
  }
}
