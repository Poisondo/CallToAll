/**
    * @author Dyakonov Oleg <o.u.dyakonov@gmail.com>
    * 
    * @brief ToneIotClient
*/

#include "ToneIotClient.h"
#include "Arduino.h"

#include "ToneIotSettings.h"

// Constructor
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

ToneIotClient::~ToneIotClient() {
  free(this->buffer);
}

ToneIotClient& ToneIotClient::setServer() {
    #ifdef TOIC_CONNECT_DOMAIN 
        return setServer(TOIC_CONNECT_DOMAIN, TOIC_CONNECT_PORT);
    #elif TOIC_CONNECT_IP
        uint8_t ip[4] = TOIC_CONNECT_IP;
        IPAddress addr(ip[0],ip[1],ip[2],ip[3]);
        return setServer(addr,TOIC_CONNECT_PORT);
    #endif
}

ToneIotClient& ToneIotClient::setServer(uint8_t * ip, uint16_t port) {
    IPAddress addr(ip[0],ip[1],ip[2],ip[3]);
    return setServer(addr,port);
}

ToneIotClient& ToneIotClient::setServer(IPAddress ip, uint16_t port) {
    this->ip = ip;
    this->port = port;
    this->domain = NULL;
    return *this;
}

ToneIotClient& ToneIotClient::setServer(const char * domain, uint16_t port) {
    this->domain = domain;
    this->port = port;
    return *this;
}

ToneIotClient& ToneIotClient::setCallback(TOIC_CALLBACK_SIGNATURE) {
    this->callback = callback;
    return *this;
}

ToneIotClient& ToneIotClient::setClient(Client& client){
    this->_client = &client;
    return *this;
}

ToneIotClient& ToneIotClient::setStream(Stream& stream){
    this->stream = &stream;
    return *this;
}

int ToneIotClient::state() {
    return this->_state;
}

boolean ToneIotClient::setBufferSize(uint16_t size) {
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
    return (this->buffer != NULL);
}

uint16_t ToneIotClient::getBufferSize() {
    return this->bufferSize;
}
ToneIotClient& ToneIotClient::setKeepAlive(uint16_t keepAlive) {
    this->keepAlive = keepAlive;
    return *this;
}
ToneIotClient& ToneIotClient::setSocketTimeout(uint16_t timeout) {
    this->socketTimeout = timeout;
    return *this;
}


boolean ToneIotClient::connect(const char *id) {
    return connect(id, TONE_TOKEN);
}

boolean ToneIotClient::connect(const char *id, const char *token) {
    if (!connected()) {
        int result = 0;


        if(_client->connected()) {
            result = 1;
        } else {
            if (domain != NULL) {
                result = _client->connect(this->domain, this->port);
            } else {
                result = _client->connect(this->ip, this->port);
            }
        }

        if (result == 1) {


            //nextMsgId = 1;
            // Leave room in the buffer for header and variable length field
            uint16_t length = 0;
            unsigned int j = 0;

            while(token[j]){
                this->buffer[length++] = token[j];
                j++;
            }


            this->buffer[length++] = ((this->keepAlive) >> 8);
            this->buffer[length++] = ((this->keepAlive) & 0xFF);

            CHECK_STRING_LENGTH(length,id)
            length = writeString(id,this->buffer,length);


            write(TOICCONNECT,this->buffer,length);

            lastInActivity = lastOutActivity = millis();

            while (!_client->available()) {
                unsigned long t = millis();
                if (t-lastInActivity >= ((int32_t) this->socketTimeout*1000UL)) {
                    _state = TOIC_CONNECTION_TIMEOUT;
                    _client->stop();
                    return false;
                }
            }
            uint8_t llen;
            uint32_t len = readPacket(&llen);

            if (len == 4) {
                if (buffer[3] == 0) {
                    lastInActivity = millis();
                    pingOutstanding = false;
                    _state = TOIC_CONNECTED;
                    return true;
                } else {
                    _state = buffer[3];
                }
            }
            _client->stop();
        } else {
            _state = TOIC_CONNECT_FAILED;
        }
        return false;
    }
    return true;
}

// reads a byte into result
boolean ToneIotClient::readByte(uint8_t * result) {
   uint32_t previousMillis = millis();
   while(!_client->available()) {
     yield();
     uint32_t currentMillis = millis();
     if(currentMillis - previousMillis >= ((int32_t) this->socketTimeout * 1000)){
       return false;
     }
   }
   *result = _client->read();
   return true;
}

// reads a byte into result[*index] and increments index
boolean ToneIotClient::readByte(uint8_t * result, uint16_t * index){
  uint16_t current_index = *index;
  uint8_t * write_address = &(result[current_index]);
  if(readByte(write_address)){
    *index = current_index + 1;
    return true;
  }
  return false;
}


// uint32_t ToneIotClient::readPacket(uint8_t* lengthLength) {
//     uint16_t len = 0;
//     if(!readByte(this->buffer, &len)) return 0;
//     bool isPublish = (this->buffer[0]&0xF0) == MQTTPUBLISH;
//     uint32_t multiplier = 1;
//     uint32_t length = 0;
//     uint8_t digit = 0;
//     uint16_t skip = 0;
//     uint32_t start = 0;

//     do {
//         if (len == 5) {
//             // Invalid remaining length encoding - kill the connection
//             _state = MQTT_DISCONNECTED;
//             _client->stop();
//             return 0;
//         }
//         if(!readByte(&digit)) return 0;
//         this->buffer[len++] = digit;
//         length += (digit & 127) * multiplier;
//         multiplier <<=7; //multiplier *= 128
//     } while ((digit & 128) != 0);
//     *lengthLength = len-1;

//     if (isPublish) {
//         // Read in topic length to calculate bytes to skip over for Stream writing
//         if(!readByte(this->buffer, &len)) return 0;
//         if(!readByte(this->buffer, &len)) return 0;
//         skip = (this->buffer[*lengthLength+1]<<8)+this->buffer[*lengthLength+2];
//         start = 2;
//         if (this->buffer[0]&MQTTQOS1) {
//             // skip message id
//             skip += 2;
//         }
//     }
//     uint32_t idx = len;

//     for (uint32_t i = start;i<length;i++) {
//         if(!readByte(&digit)) return 0;
//         if (this->stream) {
//             if (isPublish && idx-*lengthLength-2>skip) {
//                 this->stream->write(digit);
//             }
//         }

//         if (len < this->bufferSize) {
//             this->buffer[len] = digit;
//             len++;
//         }
//         idx++;
//     }

//     if (!this->stream && idx > this->bufferSize) {
//         len = 0; // This will cause the packet to be ignored.
//     }
//     return len;
// }

boolean ToneIotClient::loop() {
    if (connected()) {
        unsigned long t = millis();
        if ((t - lastInActivity > this->keepAlive*1000UL) || (t - lastOutActivity > this->keepAlive*1000UL)) {
            if (pingOutstanding) {
                this->_state = TOIC_CONNECTION_TIMEOUT;
                _client->stop();
                return false;
            } else {
                this->buffer[0] = TOICPINGREQ;
                this->buffer[1] = 0;
                _client->write(this->buffer,2);
                lastOutActivity = t;
                lastInActivity = t;
                pingOutstanding = true;
            }
        }
        if (_client->available()) {
            uint8_t llen;
            uint16_t len = readPacket(&llen);
            uint16_t msgId = 0;
            uint8_t *payload;
            if (len > 0) {
                lastInActivity = t;
                uint8_t type = this->buffer[0]&0xF0;
                if (type == MQTTPUBLISH) {
                    if (callback) {
                        uint16_t tl = (this->buffer[llen+1]<<8)+this->buffer[llen+2]; /* topic length in bytes */
                        memmove(this->buffer+llen+2,this->buffer+llen+3,tl); /* move topic inside buffer 1 byte to front */
                        this->buffer[llen+2+tl] = 0; /* end the topic as a 'C' string with \x00 */
                        char *topic = (char*) this->buffer+llen+2;
                        // msgId only present for QOS>0
                        if ((this->buffer[0]&0x06) == MQTTQOS1) {
                            msgId = (this->buffer[llen+3+tl]<<8)+this->buffer[llen+3+tl+1];
                            payload = this->buffer+llen+3+tl+2;
                            callback(topic,payload,len-llen-3-tl-2);

                            this->buffer[0] = MQTTPUBACK;
                            this->buffer[1] = 2;
                            this->buffer[2] = (msgId >> 8);
                            this->buffer[3] = (msgId & 0xFF);
                            _client->write(this->buffer,4);
                            lastOutActivity = t;

                        } else {
                            payload = this->buffer+llen+3+tl;
                            callback(topic,payload,len-llen-3-tl);
                        }
                    }
                } else if (type == MQTTPINGREQ) {
                    this->buffer[0] = MQTTPINGRESP;
                    this->buffer[1] = 0;
                    _client->write(this->buffer,2);
                } else if (type == MQTTPINGRESP) {
                    pingOutstanding = false;
                }
            } else if (!connected()) {
                // readPacket has closed the connection
                return false;
            }
        }
        return true;
    }
    return false;
}

boolean PubSubClient::publish(const char* topic, const char* payload) {
    return publish(topic,(const uint8_t*)payload, payload ? strnlen(payload, this->bufferSize) : 0,false);
}

boolean PubSubClient::publish(const char* topic, const char* payload, boolean retained) {
    return publish(topic,(const uint8_t*)payload, payload ? strnlen(payload, this->bufferSize) : 0,retained);
}

boolean PubSubClient::publish(const char* topic, const uint8_t* payload, unsigned int plength) {
    return publish(topic, payload, plength, false);
}

boolean PubSubClient::publish(const char* topic, const uint8_t* payload, unsigned int plength, boolean retained) {
    if (connected()) {
        if (this->bufferSize < MQTT_MAX_HEADER_SIZE + 2+strnlen(topic, this->bufferSize) + plength) {
            // Too long
            return false;
        }
        // Leave room in the buffer for header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        length = writeString(topic,this->buffer,length);

        // Add payload
        uint16_t i;
        for (i=0;i<plength;i++) {
            this->buffer[length++] = payload[i];
        }

        // Write the header
        uint8_t header = MQTTPUBLISH;
        if (retained) {
            header |= 1;
        }
        return write(header,this->buffer,length-MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

boolean PubSubClient::publish_P(const char* topic, const char* payload, boolean retained) {
    return publish_P(topic, (const uint8_t*)payload, payload ? strnlen(payload, this->bufferSize) : 0, retained);
}

boolean PubSubClient::publish_P(const char* topic, const uint8_t* payload, unsigned int plength, boolean retained) {
    uint8_t llen = 0;
    uint8_t digit;
    unsigned int rc = 0;
    uint16_t tlen;
    unsigned int pos = 0;
    unsigned int i;
    uint8_t header;
    unsigned int len;
    int expectedLength;

    if (!connected()) {
        return false;
    }

    tlen = strnlen(topic, this->bufferSize);

    header = MQTTPUBLISH;
    if (retained) {
        header |= 1;
    }
    this->buffer[pos++] = header;
    len = plength + 2 + tlen;
    do {
        digit = len  & 127; //digit = len %128
        len >>= 7; //len = len / 128
        if (len > 0) {
            digit |= 0x80;
        }
        this->buffer[pos++] = digit;
        llen++;
    } while(len>0);

    pos = writeString(topic,this->buffer,pos);

    rc += _client->write(this->buffer,pos);

    for (i=0;i<plength;i++) {
        rc += _client->write((char)pgm_read_byte_near(payload + i));
    }

    lastOutActivity = millis();

    expectedLength = 1 + llen + 2 + tlen + plength;

    return (rc == expectedLength);
}

boolean PubSubClient::beginPublish(const char* topic, unsigned int plength, boolean retained) {
    if (connected()) {
        // Send the header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        length = writeString(topic,this->buffer,length);
        uint8_t header = MQTTPUBLISH;
        if (retained) {
            header |= 1;
        }
        size_t hlen = buildHeader(header, this->buffer, plength+length-MQTT_MAX_HEADER_SIZE);
        uint16_t rc = _client->write(this->buffer+(MQTT_MAX_HEADER_SIZE-hlen),length-(MQTT_MAX_HEADER_SIZE-hlen));
        lastOutActivity = millis();
        return (rc == (length-(MQTT_MAX_HEADER_SIZE-hlen)));
    }
    return false;
}

int PubSubClient::endPublish() {
 return 1;
}

size_t PubSubClient::write(uint8_t data) {
    lastOutActivity = millis();
    return _client->write(data);
}

size_t PubSubClient::write(const uint8_t *buffer, size_t size) {
    lastOutActivity = millis();
    return _client->write(buffer,size);
}

size_t PubSubClient::buildHeader(uint8_t header, uint8_t* buf, uint16_t length) {
    uint8_t lenBuf[4];
    uint8_t llen = 0;
    uint8_t digit;
    uint8_t pos = 0;
    uint16_t len = length;
    do {

        digit = len  & 127; //digit = len %128
        len >>= 7; //len = len / 128
        if (len > 0) {
            digit |= 0x80;
        }
        lenBuf[pos++] = digit;
        llen++;
    } while(len>0);

    buf[4-llen] = header;
    for (int i=0;i<llen;i++) {
        buf[MQTT_MAX_HEADER_SIZE-llen+i] = lenBuf[i];
    }
    return llen+1; // Full header size is variable length bit plus the 1-byte fixed header
}

boolean PubSubClient::write(uint8_t header, uint8_t* buf, uint16_t length) {
    uint16_t rc;
    uint8_t hlen = buildHeader(header, buf, length);

#ifdef MQTT_MAX_TRANSFER_SIZE
    uint8_t* writeBuf = buf+(MQTT_MAX_HEADER_SIZE-hlen);
    uint16_t bytesRemaining = length+hlen;  //Match the length type
    uint8_t bytesToWrite;
    boolean result = true;
    while((bytesRemaining > 0) && result) {
        bytesToWrite = (bytesRemaining > MQTT_MAX_TRANSFER_SIZE)?MQTT_MAX_TRANSFER_SIZE:bytesRemaining;
        rc = _client->write(writeBuf,bytesToWrite);
        result = (rc == bytesToWrite);
        bytesRemaining -= rc;
        writeBuf += rc;
    }
    return result;
#else
    rc = _client->write(buf+(MQTT_MAX_HEADER_SIZE-hlen),length+hlen);
    lastOutActivity = millis();
    return (rc == hlen+length);
#endif
}

boolean PubSubClient::subscribe(const char* topic) {
    return subscribe(topic, 0);
}

boolean PubSubClient::subscribe(const char* topic, uint8_t qos) {
    size_t topicLength = strnlen(topic, this->bufferSize);
    if (topic == 0) {
        return false;
    }
    if (qos > 1) {
        return false;
    }
    if (this->bufferSize < 9 + topicLength) {
        // Too long
        return false;
    }
    if (connected()) {
        // Leave room in the buffer for header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        nextMsgId++;
        if (nextMsgId == 0) {
            nextMsgId = 1;
        }
        this->buffer[length++] = (nextMsgId >> 8);
        this->buffer[length++] = (nextMsgId & 0xFF);
        length = writeString((char*)topic, this->buffer,length);
        this->buffer[length++] = qos;
        return write(MQTTSUBSCRIBE|MQTTQOS1,this->buffer,length-MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

boolean PubSubClient::unsubscribe(const char* topic) {
	size_t topicLength = strnlen(topic, this->bufferSize);
    if (topic == 0) {
        return false;
    }
    if (this->bufferSize < 9 + topicLength) {
        // Too long
        return false;
    }
    if (connected()) {
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        nextMsgId++;
        if (nextMsgId == 0) {
            nextMsgId = 1;
        }
        this->buffer[length++] = (nextMsgId >> 8);
        this->buffer[length++] = (nextMsgId & 0xFF);
        length = writeString(topic, this->buffer,length);
        return write(MQTTUNSUBSCRIBE|MQTTQOS1,this->buffer,length-MQTT_MAX_HEADER_SIZE);
    }
    return false;
}

void PubSubClient::disconnect() {
    this->buffer[0] = MQTTDISCONNECT;
    this->buffer[1] = 0;
    _client->write(this->buffer,2);
    _state = MQTT_DISCONNECTED;
    _client->flush();
    _client->stop();
    lastInActivity = lastOutActivity = millis();
}

uint16_t PubSubClient::writeString(const char* string, uint8_t* buf, uint16_t pos) {
    const char* idp = string;
    uint16_t i = 0;
    pos += 2;
    while (*idp) {
        buf[pos++] = *idp++;
        i++;
    }
    buf[pos-i-2] = (i >> 8);
    buf[pos-i-1] = (i & 0xFF);
    return pos;
}


boolean PubSubClient::connected() {
    boolean rc;
    if (_client == NULL ) {
        rc = false;
    } else {
        rc = (int)_client->connected();
        if (!rc) {
            if (this->_state == MQTT_CONNECTED) {
                this->_state = MQTT_CONNECTION_LOST;
                _client->flush();
                _client->stop();
            }
        } else {
            return this->_state == MQTT_CONNECTED;
        }
    }
    return rc;
}

