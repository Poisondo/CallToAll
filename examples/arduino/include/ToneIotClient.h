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
//#define TOIC_CONNECT_BAD_CLIENT_ID   2
//#define TOIC_CONNECT_UNAVAILABLE     3
//#define TOIC_CONNECT_BAD_CREDENTIALS 4
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

class ToneIotClient : public Print {
private:
   Client* _client;
   
   uint16_t bufferSize;
   uint16_t keepAlive;
   uint16_t socketTimeout;
   //uint16_t nextMsgId;
   unsigned long lastOutActivity;
   unsigned long lastInActivity;
   bool pingOutstanding;
   TOIC_CALLBACK_SIGNATURE;
   
   bool readByte(uint8_t* buf);
   bool readByte(uint8_t* buf, uint16_t* index);
   bool write(uint8_t* buf, size_t size);

   IPAddress ip;
   const char* domain;
   uint16_t port;
   Stream* stream;
   int _state;
public:

   uint8_t* buffer;
   /**
    * @brief struct packet
    * 
    */
   typedef struct 
   {
      // header
      uint8_t    id_destination[8];      ///< id destination
      uint16_t   len_data;            ///< length buffer data
      uint16_t   function;            ///< number packet
      // data
      uint8_t    pdata[];               ///< pointer buffer data
   } packet_t;
   
   packet_t *packet;


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

   bool readPacket(packet_t** packet);
   bool writePacket(packet_t* packet);

   virtual size_t write(uint8_t);
   // Write size bytes from buffer into the payload (only to be used with beginPublish/endPublish)
   // Returns the number of bytes written
   virtual size_t write(const uint8_t *buffer, size_t size);

   boolean loop();
   boolean connected();
   int state();

};


#endif //TONEIOTCLIENT_h