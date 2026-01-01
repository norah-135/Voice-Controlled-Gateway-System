#include <WiFiS3.h>
#include <ArduinoJson.h>

const char* ssid = "Nader-4G";
const char* password = "00447711";

WiFiServer localServer(80);
const int ledPin = 5;

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  Serial.begin(115200);
  while (!Serial); 
  
  Serial.println("\n--- System Ready ---");
  showMenu();
}

void loop() {
  if (Serial.available()) {
    char input = Serial.read();
    switch (input) {
      case '0':
        showMenu();
        break;
      case '1':
        connectToWiFi();
        break;
      case '2':
        showSystemInfo();
        break;
      default:
        if (input != '\n' && input != '\r') {
          Serial.println("Invalid command. Press 0 for menu.");
        }
        break;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client = localServer.available();
    if (client) {
      handleIncomingClient(client);
    }
  }
}

void showMenu() {
  Serial.println("\n*******************************");
  Serial.println("Control Menu (Arduino R4):");
  Serial.println(" [0] -> Show Menu");
  Serial.println(" [1] -> Connect to WiFi");
  Serial.println(" [2] -> Show System Info & IP");
  Serial.println("*******************************");
}

void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Already connected!");
    return;
  }
  
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter < 10) {
    delay(1000);
    Serial.print(".");
    counter++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected successfully!");
    localServer.begin();
    showSystemInfo();
  } else {
    Serial.println("\nConnection failed.");
  }
}

void showSystemInfo() {
  Serial.println("\n===============================");
  Serial.println("System Status:");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi: Connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("WiFi: Disconnected");
  }
  
  Serial.print("LED Status (Pin 5): ");
  Serial.println(digitalRead(ledPin) == HIGH ? "ON" : "OFF");
  Serial.println("===============================");
}

void handleIncomingClient(WiFiClient client) {
  Serial.println("\n[R4] Receiving data...");
  String fullRequest = "";
  unsigned long timeout = millis();
  
  while (client.connected() && millis() - timeout < 2000) {
    while (client.available()) {
      fullRequest += (char)client.read();
      timeout = millis();
    }
  }

  Serial.println("Full Request:");
  Serial.println(fullRequest); 

  int jsonStart = fullRequest.indexOf('{');
  int jsonEnd = fullRequest.lastIndexOf('}');
  if (jsonStart != -1 && jsonEnd != -1) {
    String finalJson = fullRequest.substring(jsonStart, jsonEnd + 1);
    Serial.println("JSON Detected: " + finalJson);
    processJson(finalJson);
  } else {
    Serial.println("No JSON found.");
  }
  
  client.println("HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n");
  client.stop();
}

void processJson(String json) {
  StaticJsonDocument<512> doc;
  deserializeJson(doc, json);
  
  String transcription = doc["transcription"] | "";
  String action = doc["action"] | "";
  if (transcription.indexOf("ولع") >= 0 || action == "on") {
      digitalWrite(ledPin, HIGH);
      Serial.println("\nCommand: [LED ON]");
  } else if (transcription.indexOf("طفي") >= 0 || action == "off") {
      digitalWrite(ledPin, LOW);
      Serial.println("\nCommand: [LED OFF]");
  }
}