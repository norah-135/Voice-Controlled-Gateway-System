Project Name: Voice-Controlled Gateway System

Description: This project is an IoT framework that bridges cloud-based voice recognition with physical actuators. The system is designed to distribute tasks between two main components: an ESP32 and an Arduino Uno R4 WiFi, both communicating seamlessly over the same router.

System Architecture: The project is built on the principle of dividing workloads between two specialized wireless nodes:

Voice Gateway (ESP32):

Role: Acts as the system’s "ear" and command translator.

Functions:

Captures high-quality audio using an I2S microphone.

Sends audio data to a cloud server for processing.

Receives processed commands in JSON format.

Dispatches commands wirelessly and directly to the Arduino on the same network.

Universal Actuator (Arduino Uno R4 WiFi):

Role: Acts as the system’s "hand" and execution unit.

Functions:

Runs a local web server to receive incoming commands from the ESP32.

Parses command logic (e.g., toggling pins based on transcription keywords).

Executes physical actions (such as controlling an LED).

Networking and Communication: Both devices are wirelessly connected to the same router (Local WiFi Network). The ESP32 sends commands directly to the Arduino’s local IP address. This ensures fast and stable command execution without requiring an external internet intermediary for the final hardware action.

Technical Components:

Hardware: ESP32, Arduino Uno R4 WiFi, I2S Microphone (INMP441).

Communication Protocols: HTTP (JSON), I2S, WiFi.

Cloud Integration: Google Cloud Speech-to-Text.

Future Scalability: The architecture is designed to be actuator-agnostic. This means a single ESP32 gateway can control multiple Arduino units or different platforms connected to the same router simultaneously by targeting their specific local IP addresses.
