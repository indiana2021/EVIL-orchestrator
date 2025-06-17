#include <M5Unified.h>
#include <WiFi.h>
#include <esp_now.h>

// Message types
#define MSG_PAIR        0
#define MSG_CMD         1
#define MSG_SCAN_RESULT 2
#define MSG_SCAN_DONE   3
#define MSG_PAIR_ACK    4

// Command strings
#define CMD_SCAN      "SCAN"
#define CMD_JOIN      "JOIN"
#define CMD_CHANNEL   "CHANNEL_HOP"

// Pairing request (from slave)
typedef struct {
    uint8_t msgType;
    uint8_t mac[6];    // Slave MAC
} PairRequest;

// Pairing acknowledgement (from master)
typedef struct {
    uint8_t msgType;
    uint8_t mac[6];    // Master MAC
    uint8_t channel;   // Master Wi-Fi channel
} PairAck;

// Command message (from master)
typedef struct {
    uint8_t msgType;
    char cmd[12];
} CommandMsg;

// Wi-Fi scan result (from slave)
typedef struct {
    uint8_t msgType;
    int32_t rssi;
    uint8_t channel;
    char ssid[32];
} ScanResultMsg;

// Scan completion notice (from slave)
typedef struct {
    uint8_t msgType;
    uint8_t count;
} ScanDoneMsg;

uint8_t wifiChannel = 1;  // Fixed Wi-Fi channel for the AP

// Callback: handle incoming ESP-NOW data
void onReceive(const uint8_t * mac, const uint8_t *incomingData, int len) {
    uint8_t type = incomingData[0];
    if (type == MSG_PAIR) {
        // Slave pairing request
        PairRequest req;
        memcpy(&req, incomingData, sizeof(req));
        // Add slave as peer for future messaging
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, req.mac, 6);
        peerInfo.channel = wifiChannel;
        peerInfo.encrypt = false;
        esp_now_add_peer(&peerInfo);

        // Reply with master MAC and channel
        PairAck ack;
        ack.msgType = MSG_PAIR_ACK;
        esp_read_mac(ack.mac, ESP_MAC_WIFI_STA);
        ack.channel = wifiChannel;
        esp_err_t res = esp_now_send(req.mac, (uint8_t *)&ack, sizeof(ack));
        // Display pairing info
        char buf[32];
        sprintf(buf, "Slave %02X%02X paired", req.mac[4], req.mac[5]);
        M5.Display.println(buf);
    }
    else if (type == MSG_SCAN_RESULT) {
        // A Wi-Fi network found by a slave
        ScanResultMsg resMsg;
        memcpy(&resMsg, incomingData, sizeof(resMsg));
        M5.Display.printf("%s  RSSI%d  CH%d\n", resMsg.ssid, resMsg.rssi, resMsg.channel);
    }
    else if (type == MSG_SCAN_DONE) {
        // Slave finished scanning
        ScanDoneMsg doneMsg;
        memcpy(&doneMsg, incomingData, sizeof(doneMsg));
        M5.Display.printf("Done: %d networks\n", doneMsg.count);
    }
}

void setup() {
    Serial.begin(115200);
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setTextSize(2);
    M5.Display.println("Master Booting...");

    // Setup WiFi AP on fixed channel for ESP-NOW
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("CarduAP", "pass1234", wifiChannel);
    M5.Display.printf("AP chan %d\n", wifiChannel);

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        M5.Display.println("ESP-NOW init failed");
        return;
    }
    esp_now_register_recv_cb(onReceive);

    // Add broadcast peer (for sending commands to all)
    esp_now_peer_info_t peerInfo = {};
    memset(peerInfo.peer_addr, 0xFF, 6);
    peerInfo.channel = wifiChannel;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
}

void sendCommand(const char* cmd) {
    // Broadcast command to all slaves
    CommandMsg msg;
    msg.msgType = MSG_CMD;
    strncpy(msg.cmd, cmd, sizeof(msg.cmd));
    // Use the broadcast address (all FF) to send to all on channel [oai_citation:5‡randomnerdtutorials.com](https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/#:~:text=esp_err_t%20result%20%3D%20esp_now_send,myData)
    uint8_t broadcastAddr[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    esp_err_t res = esp_now_send(broadcastAddr, (uint8_t*)&msg, sizeof(msg));
    Serial.println(res == ESP_OK ? "Cmd sent" : "Send failed");
}

void loop() {
    static bool started = false;
    if (!started) {
        delay(2000);  // wait for potential slaves
        M5.Display.println("Cmd: SCAN");
        sendCommand(CMD_SCAN);
        started = true;
    }
    // (Could send other commands here, e.g. CHANNEL_HOP or JOIN, on user action)
}
