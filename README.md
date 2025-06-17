# EVIL-orchestrator
Application for M5 cardputer to control one or many esp32s.


Prerequisites & Hardware: An M5Stack Cardputer (ESP32-S3) for the Master, and one or more ESP32-based boards for Slaves. Both should support Arduino firmware. Connect Cardputer via USB (install M5Stack boards in Arduino IDE) and each ESP32 via USB. The Cardputer’s screen will display status.

Libraries: In Arduino IDE, install the M5Unified library (the M5Stack Cardputer driver) via Library Manager ￼. This also installs the required M5GFX library. No external library is needed for ESP-NOW; just include <esp_now.h> and <WiFi.h> (part of the ESP32 Arduino core).

Flashing:
	•	Cardputer Master: In Arduino IDE select “M5StampS3” (Cardputer) board. To enter bootloader mode, hold the G0 key on the Cardputer while powering on ￼. Then upload the Master sketch.
	•	ESP32 Slave: Select your ESP32 dev board (e.g. “ESP32 Dev Module”) and upload the Slave sketch normally (boot/flash mode as usual for that board).

Usage Example: Power on the Master and Slaves. The Slaves will auto-broadcast a “PAIR” request. When the Master pairs with them (you’ll see “Slave paired” on the Cardputer screen), the Master will automatically send a SCAN command. Slaves will scan and report networks; the Master will display lines l


Commands:
	•	SCAN: Master → Slaves to start Wi-Fi scan. Slaves reply with SSIDs.
	•	CHANNEL_HOP: Instruct slaves to change channel (placeholder, not needed if scanning all channels).
	•	JOIN: (Optional) could be used to have slaves join a specific AP. Not implemented here.

Extending: The code is modular: message types are distinguished by msgType, so new commands can be added by defining a new type and handling it in the callbacks.

Troubleshooting: Ensure all devices use the same Wi-Fi channel. The Master’s AP channel (set via WiFi.softAP) must match the channel Slaves listen on (they try all channels until they find a response ￼). ESP-NOW has long range, but heavy obstructions or distance may require closer placement ￼. Use serial print for debugging if needed. If the Cardputer fails to enter download mode, check that G0 is held before USB power.
