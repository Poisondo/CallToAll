/**
    * @author Dyakonov Oleg <o.u.dyakonov@gmail.com>
    * 
    * @brief ToneIotClient
*/

#ifndef TONEIOTCLIENT_h
#define TONEIOTCLIENT_h

#include <Arduino.h>
#include <functional>
//#include <list>
#include "IPAddress.h"
#include "Client.h"
#include "Stream.h"

//#include "ToneIotFunction.h"

// TOIC_MAX_PACKET_SIZE : Maximum packet size. Override with setBufferSize().
#define TOIC_MAX_PACKET_SIZE 256

// TOIC_KEEPALIVE : keepAlive interval in Seconds. Override with setKeepAlive()
#define TOIC_KEEPALIVE 15

// TOIC_SOCKET_TIMEOUT: socket timeout interval in Seconds. Override with setSocketTimeout()
#define TOIC_SOCKET_TIMEOUT 15

/**
 * @brief state
 * 
 */
enum class TOIC_STATE  {
   CONNECTION_TIMEOUT   = -4,
   CONNECTION_LOST      = -3,
   CONNECT_FAILED       = -2,
   DISCONNECTED         = -1,
   CONNECTED            = 0,
   CONNECT_BAD_PROTOCOL = 1,
   CONNECT_UNAUTHORIZED = 2
};

/**
 * @brief system functions
 * 
 */
#define TOIC_FUNCTION_SYS_INIT         0
#define TOIC_FUNCTION_SYS_ACK          1
#define TOIC_FUNCTION_SYS_ERROR        2
#define TOIC_FUNCTION_SYS_KEEPALIVE    3
#define TOIC_FUNCTION_SYS_DISCONNECT   15

/**
 * @brief error codes tone iot server
 * 
 */
#define TOIC_ERROR_FAULT    -1   ///< fault

/**
 * @brief disconnection codes tone iot server
 * 
 */
#define TOIC_DISCONNECT_CODE    0   ///< user

class ToneIotClient {

public:

   typedef void (*cbFunction_t)(uint8_t*, uint16_t);

   ToneIotClient(Client& client);
   ToneIotClient(Client& client, Stream& stream);

   ~ToneIotClient();

   int8_t setToneIotServer(char* tonetoken);
   int8_t setFunction(uint16_t function, cbFunction_t cbFunction);
   void setClient(Client& client);
   void setStream(Stream& stream);
   void setKeepAlive(uint16_t keepAlive);
   void setSocketTimeout(uint16_t timeout);
   int8_t setBufferSize(uint16_t size);
   uint16_t getBufferSize();

   int8_t connect();
   void disconnect();
   boolean connected();
   TOIC_STATE getState();

   int8_t loop();
   int8_t sendFunctio(uint16_t function);
   int8_t sendFunctio(uint16_t function, uint8_t* buf, uint16_t len);

   void sendFunctionAck();
   void sendFunctionError(uint16_t error);

   uint16_t waitServerRespons();
   uint16_t waitServerRespons(uint16_t* function, uint8_t** buf, uint16_t* len);
   uint16_t getErrorCode();

private:

   typedef struct 
   {
      // header
      uint8_t    id[8];    ///< id
      uint16_t   msgId;    ///< packet counter
      uint16_t   function; ///< function number packet
      uint16_t   datalen;  ///< length bytes data
      // data
      uint8_t    pdata[];  ///< pointer buffer data
   } packet_t;

   typedef struct 
   {
      uint8_t     id[8];      ///< device id
      char*       domain;     ///< domain name tone server
      uint8_t*    key;        ///< key aes-256 tone server
      uint16_t    key_len;    ///< length key aes-256 tone server

   } toneiotsettings_t;
   toneiotsettings_t  toneiotsettings;

   typedef void (ToneIotClient::*cbFunctionSys_t)(uint8_t*, uint16_t);
   enum class cbFunctionType_t {sys, user};
   typedef struct 
   {
      // header
      uint16_t    function;   ///< number function
      bool        enable;     ///< enable function
      // callback
      cbFunctionType_t cbFunctionType;
      union{
         cbFunction_t cbFunctionUser;
         cbFunctionSys_t cbFunctionSys;
      };
      // next
      void* nextfunction;
   } itemFunction_t;
   itemFunction_t* listFunction;

   Client*           client;
   Stream*           stream;
   
   uint16_t          bufferSize;
   uint16_t          keepAlive;     ///< keepAlive ms
   uint16_t          socketTimeout; ///< socketTimeout ms
   uint16_t          msgId;
   unsigned long     lastOutActivity;
   unsigned long     lastInActivity;
   bool              pingOutstanding;

   uint8_t*          buffer;
   packet_t*         packet;
   TOIC_STATE        state;

   int8_t readByte(uint8_t* buf);
   int8_t readByte(uint8_t* buf, uint16_t* index);
   int8_t write(uint8_t *buffer, size_t size);


   int8_t readPacket(packet_t** packet);
   int8_t writePacket(packet_t* packet);

   void callFunction(uint16_t function, uint8_t* buf, uint16_t len);
   
   int8_t setFunction(uint16_t function, cbFunction_t cbFunctionUser, cbFunctionSys_t cbFunctionSys);

   void initFunctionSys(void);
   void cbFunctionDisconnect(uint8_t* buf, uint16_t len);

   int8_t sendFunctionInit();
   int8_t sendFunctionKeepAlive();
   void sendFunctionDisconnect(uint16_t code);
};


#endif //TONEIOTCLIENT_h

