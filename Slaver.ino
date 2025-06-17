#include <WiFi.h>
#include <esp_now.h>

// Same message type definitions as master
#define MSG_PAIR        0
#define MSG_CMD         1
#define MSG_SCAN_RESULT 2
#define MSG_SCAN_DONE   3
#define MSG_PAIR_ACK    4

#define CMD_SCAN      "SCAN"
#define CMD_JOIN      "JOIN"
#define CMD_CHANNEL   "CHANNEL_HOP"

// Structures (must match Master definitions)
typedef struct {
    uint8_t msgType;
    uint8_t mac[6];
} PairRequest;

typedef struct {
    uint8_t msgType;
    uint8_t mac[6];
    uint8_t channel;
} PairAck;

typedef struct {
    uint8_t msgType;
    char cmd[12];
} CommandMsg;

typedef struct {
    uint8_t msgType;
    int32_t rssi;
    uint8_t channel;
    char ssid[32];
} ScanResultMsg;

typedef struct {
    uint8_t msgType;
    uint8_t count;
} ScanDoneMsg;

bool paired = false;
uint8_t masterMac[6];

void sendPairing() {
    // Broadcast a pairing request with our MAC
    PairRequest req;
    req.msgType = MSG_PAIR;
    esp_read_mac(req.mac, ESP_MAC_WIFI_STA);
    uint8_t broadcastAddr[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    esp_err_t res = esp_now_send(broadcastAddr, (uint8_t*)&req, sizeof(req));
}

void onReceive(const uint8_t * mac, const uint8_t *incomingData, int len) {
    uint8_t type = incomingData[0];
    if (type == MSG_PAIR_ACK && !paired) {
        // Received master response with its MAC and channel
        PairAck ack;
        memcpy(&ack, incomingData, sizeof(ack));
        memcpy(masterMac, ack.mac, 6);
        // Add master as peer
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, masterMac, 6);
        peerInfo.channel = ack.channel;
        peerInfo.encrypt = false;
        esp_now_add_peer(&peerInfo);
        paired = true;
    }
    else if (type == MSG_CMD && paired) {
        // Received a command
        CommandMsg cmdMsg;
        memcpy(&cmdMsg, incomingData, sizeof(cmdMsg));
        String cmd = String(cmdMsg.cmd);
        if (cmd == CMD_SCAN) {
            // Perform Wi-Fi scan (blocking)
            int n = WiFi.scanNetworks();
            for (int i = 0; i < n; ++i) {
                // Send each AP found
                ScanResultMsg resMsg;
                resMsg.msgType = MSG_SCAN_RESULT;
                resMsg.rssi = WiFi.RSSI(i);
                resMsg.channel = WiFi.channel(i);
                strncpy(resMsg.ssid, WiFi.SSID(i).c_str(), 31);
                resMsg.ssid[31] = '\0';
                esp_now_send(masterMac, (uint8_t*)&resMsg, sizeof(resMsg));
            }
            // Notify done with count
            ScanDoneMsg doneMsg;
            doneMsg.msgType = MSG_SCAN_DONE;
            doneMsg.count = n;
            esp_now_send(masterMac, (uint8_t*)&doneMsg, sizeof(doneMsg));
        }
        // (Other commands like CHANNEL_HOP or JOIN can be handled here.)
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return;
    }
    esp_now_register_recv_cb(onReceive);

    // Keep broadcasting pairing requests until master replies
    sendPairing();
}

void loop() {
    if (!paired) {
        delay(1000);
        sendPairing();
    }
    // All action happens in callbacks
}
