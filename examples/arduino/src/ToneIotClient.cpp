/**
    * @author Dyakonov Oleg <o.u.dyakonov@gmail.com>
    * 
    * @brief ToneIotClient
*/

#include "ToneIotClient.h"
#include "Arduino.h"

#include "ToneIotSettings.h"

/**
 *  @brief Constructor default
 */
ToneIotClient::ToneIotClient() {
    this->_state = TOIC_DISCONNECTED;
    setServer();
    this->_client = NULL;
    this->stream = NULL;
    setCallback(NULL);
    this->bufferSize = 0;
    setBufferSize(TOIC_MAX_PACKET_SIZE);
    setKeepAlive(TOIC_KEEPALIVE);
    setSocketTimeout(TOIC_SOCKET_TIMEOUT);
}

/**
 *  @brief Constructor
 *  @param client   {Client&}   - object socket client
 */
ToneIotClient::ToneIotClient(Client& client) {
    this->_state = TOIC_DISCONNECTED;
    setServer();
    setClient(client);
    this->stream = NULL;
    this->bufferSize = 0;
    setBufferSize(TOIC_MAX_PACKET_SIZE);
    setKeepAlive(TOIC_KEEPALIVE);
    setSocketTimeout(TOIC_SOCKET_TIMEOUT);
}

/**
 *  @brief Constructor
 *  @param client   {Client&}   - object socket client
 *  @param stream   {Stream&}   - object stream
 */
ToneIotClient::ToneIotClient(Client& client, Stream& stream) {
    this->_state = TOIC_DISCONNECTED;
    setServer();
    setClient(client);
    setStream(stream);
    this->bufferSize = 0;
    setBufferSize(TOIC_MAX_PACKET_SIZE);
    setKeepAlive(TOIC_KEEPALIVE);
    setSocketTimeout(TOIC_SOCKET_TIMEOUT);
}

/**
 *  @brief Constructor
 *  @param client   {Client&}   - object socket client
 *  @param function {function&} - function callback
 */
ToneIotClient::ToneIotClient(Client& client, TOIC_CALLBACK_SIGNATURE) {
    this->_state = TOIC_DISCONNECTED;
    setServer();
    setCallback(callback);
    setClient(client);
    this->stream = NULL;
    this->bufferSize = 0;
    setBufferSize(TOIC_MAX_PACKET_SIZE);
    setKeepAlive(TOIC_KEEPALIVE);
    setSocketTimeout(TOIC_SOCKET_TIMEOUT);
}

/**
 *  @brief Constructor
 *  @param client   {Client&}   - object socket client
 *  @param stream   {Stream&}   - object stream
 *  @param function {function&} - function callback
 */
ToneIotClient::ToneIotClient(Client& client, Stream& stream, TOIC_CALLBACK_SIGNATURE) {
    this->_state = TOIC_DISCONNECTED;
    setServer();
    setCallback(callback);
    setClient(client);
    setStream(stream);
    this->bufferSize = 0;
    setBufferSize(TOIC_MAX_PACKET_SIZE);
    setKeepAlive(TOIC_KEEPALIVE);
    setSocketTimeout(TOIC_SOCKET_TIMEOUT);
}

/**
 * @brief Destruction
 */
ToneIotClient::~ToneIotClient() {
  free(this->buffer);
}


/**
 * @brief set tone iot server default 
 * 
 * @return ToneIotClient& 
 */
ToneIotClient& ToneIotClient::setServer() {
    #ifdef TOIC_CONNECT_DOMAIN 
        return setServer(TOIC_CONNECT_DOMAIN, TOIC_CONNECT_PORT);
    #elif TOIC_CONNECT_IP
        uint8_t ip[4] = TOIC_CONNECT_IP;
        IPAddress addr(ip[0],ip[1],ip[2],ip[3]);
        return setServer(addr,TOIC_CONNECT_PORT);
    #endif
}

/**
 * @brief set tone iot server 
 * 
 * @param ip    {uint8_t*}  - array ip = {192,168,0,1}
 * @param port  {uint16_t}  - TCP socket
 * @return ToneIotClient& 
 */
ToneIotClient& ToneIotClient::setServer(uint8_t* ip, uint16_t port) {
    IPAddress addr(ip[0],ip[1],ip[2],ip[3]);
    return setServer(addr,port);
}

/**
 * @brief set tone iot server
 * 
 * @param ip    {IPAddress} - ip
 * @param port  {uint16_t}  - TCP socket
 * @return ToneIotClient& 
 */
ToneIotClient& ToneIotClient::setServer(IPAddress ip, uint16_t port) {
    this->ip = ip;
    this->port = port;
    this->domain = NULL;
    return *this;
}

/**
 * @brief set tone iot server
 * 
 * @param domain    {const char*}   - domain name server
 * @param port      {uint16_t}      - TCP socket
 * @return ToneIotClient& 
 */
ToneIotClient& ToneIotClient::setServer(const char* domain, uint16_t port) {
    this->domain = domain;
    this->port = port;
    return *this;
}

/**
 * @brief set callback
 * 
 * @param function  {function&} - function callback
 * @return ToneIotClient& 
 */
ToneIotClient& ToneIotClient::setCallback(TOIC_CALLBACK_SIGNATURE) {
    this->callback = callback;
    return *this;
}

/**
 * @brief set object client
 * 
 * @param client {Client&} - object socket Client
 * @return ToneIotClient& 
 */
ToneIotClient& ToneIotClient::setClient(Client& client){
    this->_client = &client;
    return *this;
}

/**
 * @brief set object stream
 * 
 * @param stream {Stream&} - object stream
 * @return ToneIotClient& 
 */
ToneIotClient& ToneIotClient::setStream(Stream& stream){
    this->stream = &stream;
    return *this;
}

/**
 * @brief get state
 * 
 * @return int 
 */
int ToneIotClient::state() {
    return this->_state;
}

/**
 * @brief set buffer size
 * 
 * @param size {uint16_t} - buffer size
 * @return bool 
 */
bool ToneIotClient::setBufferSize(uint16_t size) {
    if (size == 0) {
        // Cannot set it back to 0
        return false;
    }
    if (this->bufferSize == 0) {
        this->buffer = (uint8_t*)malloc(size);
    } else {
        uint8_t* newBuffer = (uint8_t*)realloc(this->buffer, size);
        if (newBuffer != NULL) {
            this->buffer = newBuffer;
        } else {
            return false;
        }
    }
    this->bufferSize = size;
    packet = (packet_t*) this->buffer;
    return (this->buffer != NULL);
}

/**
 * @brief get buffer size
 * 
 * @return uint16_t 
 */
uint16_t ToneIotClient::getBufferSize() {
    return this->bufferSize;
}

/**
 * @brief set time keep alive
 * 
 * @param keepAlive {uint16_t} - time ms keep alive
 * @return ToneIotClient& 
 */
ToneIotClient& ToneIotClient::setKeepAlive(uint16_t keepAlive) {
    this->keepAlive = keepAlive;
    return *this;
}

/**
 * @brief set socket timeout
 * 
 * @param timeout {uint16_t} - timeout ms
 * @return ToneIotClient& 
 */
ToneIotClient& ToneIotClient::setSocketTimeout(uint16_t timeout) {
    this->socketTimeout = timeout;
    return *this;
}

/**
 * @brief connect tone iot server, token default 
 * 
 * @param id {const char*} - id connecting
 * @return bool - true = ok
 */
bool ToneIotClient::connect(const char* id) {
    return connect(id, TONE_TOKEN);
}

/**
 * @brief connect tone iot server
 * 
 * @param id    {const char*} - id connecting
 * @param token {const char*} - token connecting
 * @return bool - true = ok
 */
bool ToneIotClient::connect(const char* id, const char* token) {
    if (!connected()) {
        int result = 0;


        if(_client->connected()) {
            result = 1;
        } else {
            if (this->domain != NULL) {
                result = _client->connect(this->domain, this->port);
            } else {
                result = _client->connect(this->ip, this->port);
            }
        }

        if (result == 1) {

            this->packet->function = 0;
            this->packet->len_data = strlen(token);
            memcpy(this->packet->pdata, token, this->packet->len_data);

            // this->buffer[length++] = ((this->keepAlive) >> 8);
            // this->buffer[length++] = ((this->keepAlive) & 0xFF);

            if (!writePacket(this->packet)) result = 0;


            // lastInActivity = lastOutActivity = millis();

            // while (!_client->available()) {
            //     unsigned long t = millis();
            //     if (t-lastInActivity >= ((int32_t) this->socketTimeout*1000UL)) {
            //         _state = TOIC_CONNECTION_TIMEOUT;
            //         _client->stop();
            //         return false;
            //     }
            // }
            // uint8_t llen;
            // uint32_t len = readPacket(&llen);

            // if (len == 4) {
            //     if (buffer[3] == 0) {
            //         lastInActivity = millis();
            //         pingOutstanding = false;
                    _state = TOIC_CONNECTED;
                    return true;
            //     } else {
            //         _state = buffer[3];
            //     }
            // }
            // _client->stop();
        } else {
            _state = TOIC_CONNECT_FAILED;
        }
        return false;
    }
    return true;
}

/**
 * @brief reads a byte into buf
 * 
 * @param buf {uint8_t*} - array buffer
 * @return bool - true = ok
 */
bool ToneIotClient::readByte(uint8_t* buf) {
   uint32_t previousMillis = millis();
   while(!_client->available()) {
     yield();
     uint32_t currentMillis = millis();
     if(currentMillis - previousMillis >= ((int32_t) this->socketTimeout * 1000)){
       return false;
     }
   }
   *buf = _client->read();
   return true;
}

/**
 * @brief reads a byte into buf[*index] and increments index
 * 
 * @param buf   {uint8_t*}  - array buffer
 * @param index {uint16_t*} - index read next byte buffer
 * @return bool - true = ok
 */
bool ToneIotClient::readByte(uint8_t* buf, uint16_t* index){
  uint16_t current_index = *index;
  uint8_t* write_address = &(buf[current_index]);
  if(readByte(write_address)){
    *index = current_index + 1;
    return true;
  }
  return false;
}

bool ToneIotClient::write(uint8_t *buf, size_t size) {
    lastOutActivity = millis();
    return (_client->write(buf, size) == size);
}

/**
 * @brief read packet
 * 
 * @param packet {packet_t*} - pointer structure packet
 * @return true - ok
 * @return false - fail
 */
bool ToneIotClient::readPacket(packet_t** _packet) {

    uint16_t _len = 0;

    *_packet = NULL;

    while (_len < 12){
        if(!readByte(this->buffer, &_len)) return false;
    }

    //TODO необходимо проверять id_destination

    // protocol data - 0-65535 byte
    // if (packet->len_data > 0) {
    //     while (_len < packet->len_data + 12) {
    //         if(!readByte(this->buffer, &_len)) return false;
    //     }
    // }
    
    *_packet = packet;
    return true;
    
}

bool ToneIotClient::writePacket(packet_t* _packet) {

    uint8_t* buf = (uint8_t*) _packet;
    uint16_t len = packet->len_data + 12;

    return write(buf, len);
}

bool ToneIotClient::loop() {
    //if (connected()) {
    //     unsigned long t = millis();
    //     if ((t - lastInActivity > this->keepAlive*1000UL) || (t - lastOutActivity > this->keepAlive*1000UL)) {
    //         if (pingOutstanding) {
    //             this->_state = TOIC_CONNECTION_TIMEOUT;
    //             _client->stop();
    //             return false;
    //         } else {
    //             this->buffer[0] = TOICPINGREQ;
    //             this->buffer[1] = 0;
    //             _client->write(this->buffer,2);
    //             lastOutActivity = t;
    //             lastInActivity = t;
    //             pingOutstanding = true;
    //         }
    //     }
    //    if (_client->available()) {
    //         uint8_t llen;
    //         uint16_t len = readPacket(&llen);
    //         uint16_t msgId = 0;
    //         uint8_t *payload;
    //         if (len > 0) {
    //             lastInActivity = t;
    //             uint8_t type = this->buffer[0]&0xF0;
    //             if (type == MQTTPUBLISH) {
    //                 if (callback) {
    //                     uint16_t tl = (this->buffer[llen+1]<<8)+this->buffer[llen+2]; /* topic length in bytes */
    //                     memmove(this->buffer+llen+2,this->buffer+llen+3,tl); /* move topic inside buffer 1 byte to front */
    //                     this->buffer[llen+2+tl] = 0; /* end the topic as a 'C' string with \x00 */
    //                     char *topic = (char*) this->buffer+llen+2;
    //                     // msgId only present for QOS>0
    //                     if ((this->buffer[0]&0x06) == MQTTQOS1) {
    //                         msgId = (this->buffer[llen+3+tl]<<8)+this->buffer[llen+3+tl+1];
    //                         payload = this->buffer+llen+3+tl+2;
    //                         callback(topic,payload,len-llen-3-tl-2);

    //                         this->buffer[0] = MQTTPUBACK;
    //                         this->buffer[1] = 2;
    //                         this->buffer[2] = (msgId >> 8);
    //                         this->buffer[3] = (msgId & 0xFF);
    //                         _client->write(this->buffer,4);
    //                         lastOutActivity = t;

    //                     } else {
    //                         payload = this->buffer+llen+3+tl;
    //                         callback(topic,payload,len-llen-3-tl);
    //                     }
    //                 }
    //             } else if (type == MQTTPINGREQ) {
    //                 this->buffer[0] = MQTTPINGRESP;
    //                 this->buffer[1] = 0;
    //                 _client->write(this->buffer,2);
    //             } else if (type == MQTTPINGRESP) {
    //                 pingOutstanding = false;
    //             }
    //         } else if (!connected()) {
    //             // readPacket has closed the connection
    //             return false;
    //         }
    //     }
    //     return true;
    // }
    return false;
}

void ToneIotClient::disconnect() {
    // this->buffer[0] = MQTTDISCONNECT;
    // this->buffer[1] = 0;
    // _client->write(this->buffer,2);
    _state = TOIC_DISCONNECTED;
    _client->flush();
    _client->stop();
    lastInActivity = lastOutActivity = millis();
}

bool ToneIotClient::connected() {
    bool rc;
    if (_client == NULL ) {
        rc = false;
    } else {
        rc = (int)_client->connected();
        if (!rc) {
            if (this->_state == TOIC_CONNECTED) {
                this->_state = TOIC_CONNECTION_LOST;
                _client->flush();
                _client->stop();
            }
        } else {
            return this->_state == TOIC_CONNECTED;
        }
    }
    return rc;
}

size_t ToneIotClient::write(uint8_t data) {
    lastOutActivity = millis();
    return _client->write(data);
}

size_t ToneIotClient::write(const uint8_t *buffer, size_t size) {
    lastOutActivity = millis();
    return _client->write(buffer,size);
}