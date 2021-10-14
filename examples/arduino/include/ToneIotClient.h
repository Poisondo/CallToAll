/**
    * @author Dyakonov Oleg <o.u.dyakonov@gmail.com>
    * 
    * @brief ToneIotClient
*/

#ifndef TONEIOTCLIENT_h
#define TONEIOTCLIENT_h

#include <Arduino.h>
#include "IPAddress.h"
#include "Client.h"
#include "Stream.h"

// TOIC_DOMAIN : domain to tone iot server
#ifndef TOIC_CONNECT_DOMAIN
#define TOIC_CONNECT_DOMAIN "v304929.hosted-by-vdsina.ru"
#endif

// TOIC_DOMAIN : domain to tone iot server
//#define TOIC_CONNECT_IP {94,103,84,128}

// TOIC_DOMAIN : tcp port to tone iot server
#ifndef TOIC_CONNECT_PORT
#define TOIC_CONNECT_PORT 40000
#endif

// TOIC_MAX_PACKET_SIZE : Maximum packet size. Override with setBufferSize().
#ifndef TOIC_MAX_PACKET_SIZE
#define TOIC_MAX_PACKET_SIZE 256
#endif

// TOIC_KEEPALIVE : keepAlive interval in Seconds. Override with setKeepAlive()
#ifndef TOIC_KEEPALIVE
#define TOIC_KEEPALIVE 15
#endif

// TOIC_SOCKET_TIMEOUT: socket timeout interval in Seconds. Override with setSocketTimeout()
#ifndef TOIC_SOCKET_TIMEOUT
#define TOIC_SOCKET_TIMEOUT 15
#endif

// Possible values for client.state()
#define TOIC_CONNECTION_TIMEOUT     -4
#define TOIC_CONNECTION_LOST        -3
#define TOIC_CONNECT_FAILED         -2
#define TOIC_DISCONNECTED           -1
#define TOIC_CONNECTED               0
#define TOIC_CONNECT_BAD_PROTOCOL    1
#define TOIC_CONNECT_BAD_CLIENT_ID   2
#define TOIC_CONNECT_UNAVAILABLE     3
#define TOIC_CONNECT_BAD_CREDENTIALS 4
#define TOIC_CONNECT_UNAUTHORIZED    5

// #define TOICCONNECT     1 << 4  // Client request to connect to Server
// #define TOICCONNACK     2 << 4  // Connect Acknowledgment
// #define TOICPINGREQ     12 << 4 // PING Request
// #define TOICPINGRESP    13 << 4 // PING Response
// #define TOICDISCONNECT  14 << 4 // Client is Disconnecting
// #define TOICReserved    15 << 4 // Reserved

#if defined(ESP8266) || defined(ESP32)
#include <functional>
#define TOIC_CALLBACK_SIGNATURE std::function<void(char*, uint8_t*, unsigned int)> callback
#else
#define TOIC_CALLBACK_SIGNATURE void (*callback)(char*, uint8_t*, unsigned int)
#endif

#define CHECK_STRING_LENGTH(l,s) if (l+2+strnlen(s, this->bufferSize) > this->bufferSize) {_client->stop();return false;}

class ToneIotClient : public Print {
private:
   Client* _client;
   uint8_t* buffer;
   uint16_t bufferSize;
   uint16_t keepAlive;
   uint16_t socketTimeout;
   //uint16_t nextMsgId;
   unsigned long lastOutActivity;
   unsigned long lastInActivity;
   bool pingOutstanding;
   TOIC_CALLBACK_SIGNATURE;
   // uint32_t readPacket(uint8_t*);
   boolean readByte(uint8_t * result);
   boolean readByte(uint8_t * result, uint16_t * index);
   boolean write(uint8_t header, uint8_t* buf, uint16_t length);
   uint16_t writeString(const char* string, uint8_t* buf, uint16_t pos);
   // Build up the header ready to send
   // Returns the size of the header
   // Note: the header is built at the end of the first TOIC_MAX_HEADER_SIZE bytes, so will start
   //       (TOIC_MAX_HEADER_SIZE - <returned size>) bytes into the buffer
   size_t buildHeader(uint8_t header, uint8_t* buf, uint16_t length);
   IPAddress ip;
   const char* domain;
   uint16_t port;
   Stream* stream;
   int _state;
public:
   ToneIotClient();
   ToneIotClient(Client& client);
   ToneIotClient(Client& client, Stream&);
   ToneIotClient(Client& client, TOIC_CALLBACK_SIGNATURE);
   ToneIotClient(Client& client, Stream&, TOIC_CALLBACK_SIGNATURE);

   ~ToneIotClient();

   ToneIotClient& setServer();
   ToneIotClient& setServer(uint8_t * ip, uint16_t port);
   ToneIotClient& setServer(IPAddress ip, uint16_t port);
   ToneIotClient& setServer(const char * domain, uint16_t port);
   ToneIotClient& setCallback(TOIC_CALLBACK_SIGNATURE);
   ToneIotClient& setClient(Client& client);
   ToneIotClient& setStream(Stream& stream);
   ToneIotClient& setKeepAlive(uint16_t keepAlive);
   ToneIotClient& setSocketTimeout(uint16_t timeout);

   boolean setBufferSize(uint16_t size);
   uint16_t getBufferSize();

   boolean connect(const char* id);
   boolean connect(const char* id, const char* token);
   void disconnect();


   // boolean publish(const char* topic, const char* payload);
   // boolean publish(const char* topic, const char* payload, boolean retained);
   // boolean publish(const char* topic, const uint8_t * payload, unsigned int plength);
   // boolean publish(const char* topic, const uint8_t * payload, unsigned int plength, boolean retained);
   // boolean publish_P(const char* topic, const char* payload, boolean retained);
   // boolean publish_P(const char* topic, const uint8_t * payload, unsigned int plength, boolean retained);
   // // Start to publish a message.
   // // This API:
   // //   beginPublish(...)
   // //   one or more calls to write(...)
   // //   endPublish()
   // // Allows for arbitrarily large payloads to be sent without them having to be copied into
   // // a new buffer and held in memory at one time
   // // Returns 1 if the message was started successfully, 0 if there was an error
   // boolean beginPublish(const char* topic, unsigned int plength, boolean retained);
   // // Finish off this publish message (started with beginPublish)
   // // Returns 1 if the packet was sent successfully, 0 if there was an error
   // int endPublish();
   // // Write a single byte of payload (only to be used with beginPublish/endPublish)
   // virtual size_t write(uint8_t);
   // // Write size bytes from buffer into the payload (only to be used with beginPublish/endPublish)
   // // Returns the number of bytes written
   // virtual size_t write(const uint8_t *buffer, size_t size);
   // boolean subscribe(const char* topic);
   // boolean subscribe(const char* topic, uint8_t qos);
   // boolean unsubscribe(const char* topic);
   boolean loop();
   boolean connected();
   int state();

};


#endif //TONEIOTCLIENT_h