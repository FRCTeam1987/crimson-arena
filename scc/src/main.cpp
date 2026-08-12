#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoHttpClient.h>

#include "config.h"
#include "status_led.h"

// W5500 Ethernet chip pins
#define W5500_CS 14   // Chip Select pin
#define W5500_RST 9   // Reset pin
#define W5500_INT 10  // Interrupt pin
#define W5500_MISO 12 // MISO pin
#define W5500_MOSI 11 // MOSI pin
#define W5500_SCK 13  // Clock pin

Config config;

byte macScoring[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
byte macRed[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE};
byte macBlue[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF};

IPAddress dns(0, 0, 0, 0);
IPAddress gateway(0, 0, 0, 0);
IPAddress subnet(255, 255, 255, 0);

const char *kFmsHost = "10.0.100.5";
constexpr uint16_t kFmsPort = 8080;
const char *kFmsPath = "/scc/websocket";

constexpr unsigned long kReconnectIntervalMs = 500;
constexpr unsigned long kPingIntervalMs = 1000;
constexpr unsigned long kLivenessTimeoutMs = 3000;

EthernetClient client;
WebSocketClient webSocket(client, kFmsHost, kFmsPort);

bool wsConnected = false;
unsigned long lastReconnectAttemptMs = 0;
unsigned long lastPingSentMs = 0;
unsigned long lastActivityMs = 0;
EthernetLinkStatus lastLinkStatus = Unknown;

constexpr uint8_t kEStopPin1 = 48;
constexpr uint8_t kEStopPin2 = 47;
constexpr uint8_t kEStopPin3 = 46;

volatile bool eStopChanged = false;

void IRAM_ATTR OnEStopChanged()
{
    eStopChanged = true;
}

void ReadEStopStatus(bool *eStop1, bool *eStop2, bool *eStop3)
{
    *eStop1 = digitalRead(kEStopPin1) != 0;
    if (config.location == SccLocation::SCORING)
    {
        *eStop2 = false;
        *eStop3 = false;
    }
    else
    {
        *eStop2 = digitalRead(kEStopPin2) != 0;
        *eStop3 = digitalRead(kEStopPin3) != 0;
    }
}

void SendButtonStatus(bool eStop1, bool eStop2, bool eStop3)
{
    if (!wsConnected)
    {
        return;
    }
    char payload[160];
    snprintf(payload, sizeof(payload),
             "{\"type\":\"sccupdate\",\"data\":{\"alliance\":\"%s\",\"eStop1\":%s,\"eStop2\":%s,\"eStop3\":%s}}",
             GetSCCLocationString(config.location), eStop1 ? "true" : "false", eStop2 ? "true" : "false",
             eStop3 ? "true" : "false");
    webSocket.beginMessage(TYPE_TEXT);
    webSocket.print(payload);
    webSocket.endMessage();
}

void ConnectWebSocket()
{
    if (Ethernet.linkStatus() == LinkOFF)
    {
        // If the Ethernet link gets disconnected, update the WebSocket state.
        wsConnected = false;
        return;
    }

    Serial.println("Connecting to FMS...");
    webSocket.stop();
    if (webSocket.begin(kFmsPath) == 0)
    {
        Serial.println("WebSocket connected.");
        wsConnected = true;
        lastActivityMs = millis();
        lastPingSentMs = millis();
        bool eStop1, eStop2, eStop3;
        ReadEStopStatus(&eStop1, &eStop2, &eStop3);
        SendButtonStatus(eStop1, eStop2, eStop3);
        eStopChanged = false;
    }
    else
    {
        Serial.println("WebSocket connection failed. Retrying...");
        wsConnected = false;
    }
}

byte *GetMACAddress(SccLocation location)
{
    switch (location)
    {
    case SccLocation::RED:
        return macRed;
    case SccLocation::BLUE:
        return macBlue;
    case SccLocation::SCORING:
        return macScoring;
    }
    return macScoring;
}

IPAddress GetIPAddress(SccLocation location)
{
    switch (location)
    {
    case SccLocation::RED:
        return IPAddress(10, 0, 100, 41);
    case SccLocation::BLUE:
        return IPAddress(10, 0, 100, 42);
    case SccLocation::SCORING:
        return IPAddress(10, 0, 100, 40);
    }
    return IPAddress(10, 0, 100, 40);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ; // Wait until serial port is ready
    }

    StatusLedInit();

    config = LoadConfig();
    Serial.print("Location: ");
    Serial.println(GetSCCLocationString(config.location));

    byte *mac = GetMACAddress(config.location);
    IPAddress ip = GetIPAddress(config.location);

    // Use internal pull-up resistor. Attach hardware interrupts to each e-stop pin.
    pinMode(kEStopPin1, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(kEStopPin1), OnEStopChanged, CHANGE);
    if (config.location != SccLocation::SCORING)
    {
        pinMode(kEStopPin2, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(kEStopPin2), OnEStopChanged, CHANGE);
        pinMode(kEStopPin3, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(kEStopPin3), OnEStopChanged, CHANGE);
    }

    // Initialize SPI
    SPI.begin(W5500_SCK, W5500_MISO, W5500_MOSI, W5500_CS);

    // Initialize Ethernet
    Ethernet.init(W5500_CS);
    Ethernet.begin(mac, ip, dns, gateway, subnet);

    // Verify IP address
    if (Ethernet.localIP() == IPAddress(0, 0, 0, 0))
    {
        HaltWithError("Failed to configure Ethernet with static IP");
    }

    Serial.print("IP Address: ");
    Serial.println(Ethernet.localIP());

    // Connect to the FMS
    lastReconnectAttemptMs = millis();
    ConnectWebSocket();
}

void loop()
{
    EthernetLinkStatus linkStatus = Ethernet.linkStatus();
    if (linkStatus != lastLinkStatus)
    {
        if (linkStatus == LinkOFF)
        {
            Serial.println("Ethernet link down.");
        }
        else if (linkStatus == LinkON)
        {
            Serial.println("Ethernet link up.");
            lastReconnectAttemptMs = 0;
        }
        lastLinkStatus = linkStatus;
    }

    if (wsConnected && !webSocket.connected())
    {
        Serial.println("Lost connection to FMS. Reconnecting...");
        wsConnected = false;
    }

    if (wsConnected && (millis() - lastActivityMs >= kLivenessTimeoutMs))
    {
        Serial.println("No response from FMS. Reconnecting...");
        webSocket.stop();
        wsConnected = false;
    }

    if (!wsConnected)
    {
        if (millis() - lastReconnectAttemptMs >= kReconnectIntervalMs)
        {
            lastReconnectAttemptMs = millis();
            ConnectWebSocket();
        }
        StatusLedUpdate(wsConnected);
        return;
    }

    StatusLedUpdate(wsConnected);

    if (client.available() > 0)
    {
        lastActivityMs = millis();
        webSocket.parseMessage();
    }

    if (eStopChanged)
    {
        eStopChanged = false;
        bool eStop1, eStop2, eStop3;
        ReadEStopStatus(&eStop1, &eStop2, &eStop3);
        SendButtonStatus(eStop1, eStop2, eStop3);
    }

    if (millis() - lastPingSentMs >= kPingIntervalMs)
    {
        lastPingSentMs = millis();
        webSocket.ping();
    }
}
