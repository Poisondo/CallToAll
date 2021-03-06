#include <Arduino.h>

/**************************************************************
 *
 * For this example, you need to install PubSubClient library:
 *   https://github.com/knolleary/pubsubclient
 *   or from http://librarymanager/all#PubSubClient
 *
 * TinyGSM Getting Started guide:
 *   https://tiny.cc/tinygsm-readme
 *
 * For more MQTT examples, see PubSubClient library
 *
 **************************************************************
 * Use Mosquitto client tools to work with MQTT
 *   Ubuntu/Linux: sudo apt-get install mosquitto-clients
 *   Windows:      https://mosquitto.org/download/
 *
 * Subscribe for messages:
 *   mosquitto_sub -h test.mosquitto.org -t GsmClientTest/init -t GsmClientTest/ledStatus -q 1
 * Toggle led:
 *   mosquitto_pub -h test.mosquitto.org -t GsmClientTest/led -q 1 -m "toggle"
 *
 * You can use Node-RED for wiring together MQTT-enabled devices
 *   https://nodered.org/
 * Also, take a look at these additional Node-RED modules:
 *   node-red-contrib-blynk-ws
 *   node-red-dashboard
 *
 **************************************************************/

// Please select the corresponding model

// #define SIM800L_IP5306_VERSION_20190610
// #define SIM800L_AXP192_VERSION_20200327
// #define SIM800C_AXP192_VERSION_20200609
#define SIM800L_IP5306_VERSION_20200811

#include "utilities.h"

// Select your modem:
#define TINY_GSM_MODEM_SIM800

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

// See all AT commands, if wanted
//#define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

// Add a reception delay - may be needed for a fast processor at a slow baud rate
// #define TINY_GSM_YIELD() { delay(2); }

// Define how you're planning to connect to the internet
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[] = "m.tinkoff.";
const char gprsUser[] = "";
const char gprsPass[] = "";

#include <TinyGsmClient.h>
#include <ToneIotClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif
TinyGsmClient client(modem);
ToneIotClient toneiotclient(client);

int ledStatus = LOW;

uint32_t lastReconnectAttempt = 0;

// void mqttCallback(char *topic, byte *payload, unsigned int len)
// {
//     SerialMon.print("Message arrived [");
//     SerialMon.print(topic);
//     SerialMon.print("]: ");
//     SerialMon.write(payload, len);
//     SerialMon.println();

//     // Only proceed if incoming message's topic matches
//     if (String(topic) == topicLed) {
//         ledStatus = !ledStatus;
//         digitalWrite(LED_GPIO, ledStatus);
//         mqtt.publish(topicLedStatus, ledStatus ? "1" : "0");
//     }
// }

boolean toneiotConnect()
{
    SerialMon.print("Connecting to ");

    // Connect to tone iot server
    int status = toneiotclient.connect();

    if (status == -1) {
        SerialMon.println(" fail");
        return false;
    }
    SerialMon.println(" success");
    // mqtt.publish(topicInit, "CallToAll started");
    // mqtt.subscribe(topicLed);
    return toneiotclient.connected();
}

int8_t modemConnect()
{
    setupModem();

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    SerialMon.println("Initializing modem...");
    modem.restart();
    // modem.init();

    String modemInfo = modem.getModemInfo();
    SerialMon.print("Modem Info: ");
    SerialMon.println(modemInfo);

#if TINY_GSM_USE_GPRS
    // Unlock your SIM card with a PIN if needed
    if ( GSM_PIN && modem.getSimStatus() != 3 ) {
        modem.simUnlock(GSM_PIN);
    }
#endif

    SerialMon.print("Waiting for network...");
    if (!modem.waitForNetwork()) {
        SerialMon.println(" fail");
        delay(10000);
        return -1;
    }
    SerialMon.println(" success");

    if (modem.isNetworkConnected()) {
        SerialMon.println("Network connected");
    }

    // GPRS connection parameters are usually set after network registration
    SerialMon.print(F("Connecting to "));
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        SerialMon.println(" fail");
        delay(10000);
        return -1;
    }
    SerialMon.println(" success");

    if (modem.isGprsConnected()) {
        SerialMon.println("GPRS connected");
    }

    return 0;

}

void setup()
{
    // Set console baud rate
    SerialMon.begin(115200);

    delay(10);

    SerialMon.println("Started device!!!");
    SerialMon.println("Wait...");

    // Set GSM module baud rate and UART pins
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

    delay(6000);

    if (modemConnect() != 0){
      return;
    }


    
}

void loop()
{

    if (!toneiotclient.connected()) {
        SerialMon.println("=== MQTT NOT CONNECTED ===");
        // Reconnect every 10 seconds
        uint32_t t = millis();
        if (t - lastReconnectAttempt > 10000L) {
            lastReconnectAttempt = t;
            if (toneiotConnect()) {
                lastReconnectAttempt = 0;
            }
        }
        delay(100);
        return;
    }

    //ToneIotClient::packet_t *packet = NULL;

    // if (toneiotclient.readPacket(&packet) == 0){
    //     SerialMon.print("-->>readPacket:");
    //     // for (int i = 0; i < packet->length + 10; i++) {
    //     //     SerialMon.print(toneiotclient.buffer[i]);
    //     //     SerialMon.print(" ");
    //     // }
    //     SerialMon.println();
    //     SerialMon.print("----->>id:");
    //     for (int i = 0; i < 8; i++) {
    //         SerialMon.print(packet->id[i]);
    //         SerialMon.print(" ");
    //     }
    //     SerialMon.print(" len:");
    //     SerialMon.print(packet->length);
    //     SerialMon.print(" func:");
    //     SerialMon.print(packet->function);
    //     SerialMon.print(" data:");
    //     for (int i = 0; i < packet->length - 2; i++) {
    //         SerialMon.print(packet->pdata[i]);
    //         SerialMon.print(" ");
    //     }
    //     SerialMon.println();
    // }

    yield();
}
