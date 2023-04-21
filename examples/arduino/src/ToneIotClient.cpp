/**
    * @author Dyakonov Oleg <o.u.dyakonov@gmail.com>
    * 
    * @brief ToneIotClient
*/

#include "ToneIotClient.h"
#include "Arduino.h"

#include "ToneIotSettings.h"
#include "Base64.h"


// ======================================== public ======================================
/**
 *  @brief Constructor
 *  @param client - object socket client
 */
ToneIotClient::ToneIotClient(Client& client) {

    this->state = TOIC_STATE::DISCONNECTED;
    setToneIotServer(TONE_TOKEN);
    setClient(client);
    this->stream = NULL;
    this->listFunction = NULL;
    this->bufferSize = 0;
    setBufferSize(TOIC_MAX_PACKET_SIZE);
    setKeepAlive(TOIC_KEEPALIVE);
    setSocketTimeout(TOIC_SOCKET_TIMEOUT);
    initFunctionSys();
}

/**
 *  @brief Constructor
 *  @param client - object socket client
 *  @param stream - object stream
 */
ToneIotClient::ToneIotClient(Client& client, Stream& stream) {

    this->state = TOIC_STATE::DISCONNECTED;
    setToneIotServer(TONE_TOKEN);
    setClient(client);
    setStream(stream);
    this->listFunction = NULL;
    this->bufferSize = 0;
    setBufferSize(TOIC_MAX_PACKET_SIZE);
    setKeepAlive(TOIC_KEEPALIVE);
    setSocketTimeout(TOIC_SOCKET_TIMEOUT);
    initFunctionSys();
}

/**
 * @brief Destruction
 */
ToneIotClient::~ToneIotClient() {

  if (this->buffer) free(this->buffer);
}

/**
 * @brief set tone iot server tocken 
 * 
 * @param tonetoken - device tone token
 * @return int8_t = 0 - ok; -1 - error
 */
int8_t ToneIotClient::setToneIotServer(char* tonetoken) {

    uint8_t p_tonetoken_max = 3;
    char* p_tonetoken[p_tonetoken_max] = {0};
    int  len_tonetoken[p_tonetoken_max] = {0};
    int tonetokenLength = 0;
    int len = 0;
    int index = 0;
    int decodedLength = 0;
    char* decodedbuf = NULL;

    //init structure toneiotsettigs
    for (int i = 0; i < 8; i++) this->toneiotsettings.id[i] = 0;

    tonetokenLength = strlen(tonetoken);
    p_tonetoken[index] = tonetoken;
    for (int i = 0;  i < tonetokenLength; i++) {

        if (tonetoken[i] == '.'){
            index++;
            i++;
            if (index >= p_tonetoken_max) return -1;
            p_tonetoken[index] = &tonetoken[i];
            len_tonetoken[index - 1] = len;
            len = 0;
        }

        len++;
    }
    len_tonetoken[index] = len;

    // failed structure token
    if (index != p_tonetoken_max - 1) return -1;
    // cycle by token structure
    for (int i = 0; i < p_tonetoken_max; i++){

        // element token length equal to 0 failed
        if (len_tonetoken[i] == 0) return -1;
        decodedLength = Base64.decodedLength(p_tonetoken[i], len_tonetoken[i]);
        // element token decode base64 failed
        if (decodedLength <= 0) return -1;
        decodedbuf = (char*)malloc(decodedLength + 1);
        // not enough memory
        if (decodedbuf == NULL) return -1;
        // decode element token
        Base64.decode(decodedbuf, p_tonetoken[i], len_tonetoken[i]);
        // set settings
        if (i == 0){ //id
            for (int ii = 0; ii < 8; ii++){
                if (ii < decodedLength) this->toneiotsettings.id[7 - ii] = (uint8_t)decodedbuf[decodedLength - ii - 1];
                else this->toneiotsettings.id[7 - ii] = 0;
            }
            free(decodedbuf); 
        }else if (i == 1){ //domain
            this->toneiotsettings.domain = decodedbuf;
        }else if (i == 2){ //key
            this->toneiotsettings.key = (uint8_t*)decodedbuf;
            this->toneiotsettings.key_len = decodedLength;
        }
        decodedbuf = NULL;
    }

    return 0;
}

/**
 * @brief set function callback
 * 
 * @param function number packet function
 * @param cbFunction callback user function
 * @return int8_t = 0 - ok; -1 - error
 */
int8_t ToneIotClient::setFunction(uint16_t function, cbFunction_t cbFunction){
    return setFunction(function, cbFunction, NULL);
}

/**
 * @brief set object client
 * 
 * @param client - object socket Client
 */
void ToneIotClient::setClient(Client& client){
    this->client = &client;
}

/**
 * @brief set object stream
 * 
 * @param stream - object stream
 */
void ToneIotClient::setStream(Stream& stream){
    this->stream = &stream;
}

/**
 * @brief set time keep alive
 * 
 * @param keepAlive - time ms keep alive
 */
void ToneIotClient::setKeepAlive(uint16_t keepAlive) {
    this->keepAlive = keepAlive;
}

/**
 * @brief set socket timeout
 * 
 * @param timeout - timeout ms
 */
void ToneIotClient::setSocketTimeout(uint16_t timeout) {
    this->socketTimeout = timeout;
}

/**
 * @brief set buffer size
 * 
 * @param size - buffer size
 * @return int8_t = 0 - ok; -1 - error
 */
int8_t ToneIotClient::setBufferSize(uint16_t size) {

    uint8_t* newBuffer = NULL;

    if (size == 0) {
        // Cannot set it back to 0
        return -1;
    }
    if (this->bufferSize == 0) {
        this->buffer = (uint8_t*)malloc(size);
    } else {
        newBuffer = (uint8_t*)realloc(this->buffer, size);
        if (newBuffer != NULL) {
            this->buffer = newBuffer;
        } else {
            return -1;
        }
    }
    this->bufferSize = size;
    this->packet = (packet_t*) this->buffer;
    if (this->buffer == NULL) return -1;
    return 0;
}

/**
 * @brief get buffer size
 * 
 * @return uint16_t size
 */
uint16_t ToneIotClient::getBufferSize() {
    return this->bufferSize;
}

/**
 * @brief connect to tone iot server
 * 
 * @return int8_t = 0 - ok; -1 - error
 */
int8_t ToneIotClient::connect() {

    if (this->toneiotsettings.domain == NULL || this->toneiotsettings.key == NULL || this->toneiotsettings.key_len == 0){
        this->state = TOIC_STATE::CONNECT_BAD_PROTOCOL;
        return -1;
    }

    if (connected()) return 0;
          
    //tcp connect to tone iot server
    if(!this->client->connected()) {
        if(this->client->connect(this->toneiotsettings.domain, TONE_CONNECT_PORT) != 1){ // error connect tone iot server
            this->state = TOIC_STATE::CONNECT_FAILED;
            return -1;
        }
    }

    //function init verify key connected tone iot server
    if (sendFunctionInit()) {
        this->state = TOIC_STATE::CONNECT_BAD_PROTOCOL;
        goto ERROR;
    }

    this->state = TOIC_STATE::CONNECTED;
    return 0;
ERROR:
    this->client->flush();
    this->client->stop();
    return -1;
}

/**
 * @brief disconnect to tone iot server
 * 
 */
void ToneIotClient::disconnect() {

    //not tcp connect to tone iot server
    if(!this->client->connected()) return;

    sendFunctionDisconnect(0);

    this->state = TOIC_STATE::DISCONNECTED;
    this->client->flush();
    this->client->stop();
    lastInActivity = lastOutActivity = millis();
}

/**
 * @brief connected to tone iot server
 * 
 * @return true - connected
 * @return false - disconnected
 */
bool ToneIotClient::connected() {

    bool rc;
    if (this->client == NULL ) {
        rc = false;
    } else {
        rc = (int)this->client->connected();
        if (!rc) {
            if (this->state == TOIC_STATE::CONNECTED) {
                this->state = TOIC_STATE::CONNECTION_LOST;
                this->client->flush();
                this->client->stop();
            }
        } else {
            return this->state == TOIC_STATE::CONNECTED;
        }
    }
    return rc;
}

/**
 * @brief get state
 * 
 * @return uint8_t return state
 */
TOIC_STATE ToneIotClient::getState() {
    return this->state;
}

/**
 * @brief protocol processing cycle
 * 
 * @return int8_t = 0 - ok; -1 - error
 */
int8_t ToneIotClient::loop() {

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

int8_t ToneIotClient::sendFunctio(uint16_t function){
    return sendFunctio(function, NULL, 0);
}

int8_t ToneIotClient::sendFunctio(uint16_t function, uint8_t* buf, uint16_t len){
    memcpy(this->packet->id, this->toneiotsettings.id, 8); 
    this->packet->msgId = ++this->msgId;
    this->packet->function = function;
    if (buf != NULL) {
        memcpy(this->packet->pdata, buf, len); 
        this->packet->datalen = len;
    }
    return writePacket(this->packet);
}

void ToneIotClient::sendFunctionAck(){
    memcpy(this->packet->id, this->toneiotsettings.id, 8); 
    this->packet->function = TOIC_FUNCTION_SYS_ACK;
    this->packet->datalen = 0;
    writePacket(this->packet);
}

void ToneIotClient::sendFunctionError(uint16_t error){
    memcpy(this->packet->id, this->toneiotsettings.id, 8);
    this->packet->datalen = 2; // header + id + keepAlive
    this->packet->function = TOIC_FUNCTION_SYS_ERROR;
    memcpy(&this->packet->pdata, &error, 2);           // error 2 byte
    writePacket(this->packet);
}

uint16_t ToneIotClient::waitServerRespons(){
    if (readPacket(&packet) == -1){ 
        this->packet->function = TOIC_FUNCTION_SYS_ERROR;
        this->packet->pdata[0] = 0;
        this->packet->pdata[1] = 0;
    }
    return this->packet->function;
}

uint16_t ToneIotClient::waitServerRespons(uint16_t* function, uint8_t** buf, uint16_t* len){
    waitServerRespons();
    *function = this->packet->function;
    *buf = this->packet->pdata;
    *len = this->packet->datalen;
    return this->packet->function;
}

// last error code
uint16_t ToneIotClient::getErrorCode(){
    return (this->packet->pdata[0] << 8) | this->packet->pdata[1];
}

// =============================================== private =================================

/**
 * @brief reads a byte into buf
 * 
 * @param buf - array buffer
 * @return int8_t = 0 - ok; -1 - error
 */
int8_t ToneIotClient::readByte(uint8_t* buf) {

   uint32_t previousMillis = 0;
   if (!this->client->connected()) return -1;
   previousMillis = millis();
   while(!this->client->available()) {
     yield();
     uint32_t currentMillis = millis();
     if(currentMillis - previousMillis >= ((int32_t) this->socketTimeout * 1000)){
       return -1;
     }
   }
   *buf = this->client->read();
   return 0;
}

/**
 * @brief reads a byte into buf[*index] and increments index
 * 
 * @param buf - array buffer
 * @param index - index read next byte buffer
 * @return int8_t = 0 - ok; -1 - error
 */
int8_t ToneIotClient::readByte(uint8_t* buf, uint16_t* index){
    int8_t ret = 0;
    uint16_t current_index = *index;
    uint8_t* write_address = &(buf[current_index]);
    ret = readByte(write_address);
    if (ret == 0) *index = current_index + 1;
    return ret;
}

/**
 * @brief read packet
 * 
 * @param packet - pointer structure packet
 * @return int8_t = 0 - ok; -1 - error; 1 - not equally id 
 */
int8_t ToneIotClient::readPacket(packet_t** packet) {

    uint16_t len = 0;

    *packet = NULL;

    while (len < 14){
        if(readByte(this->buffer, &len) == -1) return -1;
    }

    // protocol data - 0-65535 byte
    while (len < this->packet->datalen + 14) {
        if(readByte(this->buffer, &len) == -1) return -1;
    }

    
    // check id device
    if (memcmp(this->toneiotsettings.id, this->packet->id, 8)) return 1;

    // check msgId
    if (this->msgId <= this->packet->msgId) {
        if (this->msgId == this->packet->msgId 
            && this->packet->function == TOIC_FUNCTION_SYS_ACK
            && this->packet->function == TOIC_FUNCTION_SYS_ERROR
            ) return 0;
        return 2;
    }
    

    *packet = this->packet;
    return 0;
    
}

/**
 * @brief send server buffer
 * @param buf - array buffer
 * @param size - size
 * @return int8_t = 0 - ok; -1 - error
 */
int8_t ToneIotClient::write(uint8_t *buf, size_t size) {
    
    if (!this->client->connected()) return -1;
    lastOutActivity = millis();
    if (this->client->write(buf, size) != size) return -1;
    return 0;
}

/**
 * @brief send server pocket
 * 
 * @param packet - pointer structure pocket
 * @return int8_t = 0 - ok; -1 - error
 */
int8_t ToneIotClient::writePacket(packet_t* packet) {

    uint8_t* buf = (uint8_t*) packet;
    uint16_t len = this->packet->datalen + 14;
    return write(buf, len);
}

/**
 * @brief call the function callback
 * 
 * @param function function
 * @param buf - array buffer
 * @param len - length buffer
 */
void ToneIotClient::callFunction(uint16_t function, uint8_t* buf, uint16_t len){
    itemFunction_t* itemFunction = this->listFunction;
    while(itemFunction != NULL) {
        if (itemFunction->function == function){
            if (itemFunction->enable) {
                if (itemFunction->cbFunctionType == cbFunctionType_t::sys && itemFunction->cbFunctionSys != NULL){
                    ((this)->*(itemFunction->cbFunctionSys))(buf, len);
                } else if (itemFunction->cbFunctionType == cbFunctionType_t::user && itemFunction->cbFunctionUser != NULL){
                    itemFunction->cbFunctionUser(buf, len);
                }
            } 
            break;
        }
        itemFunction = (itemFunction_t*)itemFunction->nextfunction;
    }
}

int8_t ToneIotClient::setFunction(uint16_t function, cbFunction_t cbFunctionUser, cbFunctionSys_t cbFunctionSys) {

    itemFunction_t* newfcb = NULL;
    itemFunction_t* itemfcb = this->listFunction;

    if (cbFunctionUser == NULL && cbFunctionSys == NULL) return -1;

    // create item function
    newfcb = new itemFunction_t;
    // not enough memory
    if (newfcb == NULL) return -1;
    newfcb->function = function;
    newfcb->enable = false;
    if (cbFunctionUser != NULL){
        newfcb->cbFunctionType = cbFunctionType_t::user;
        newfcb->cbFunctionUser = cbFunctionUser;
    } else if (cbFunctionSys != NULL){
        newfcb->cbFunctionType = cbFunctionType_t::sys;
        newfcb->cbFunctionSys = cbFunctionSys;
    }
    newfcb->nextfunction = NULL;

    if (itemfcb == NULL) { // first item
        this->listFunction = newfcb;
    } else { // next item
        while(itemfcb->nextfunction != NULL) itemfcb = (itemFunction_t*)itemfcb->nextfunction;
        itemfcb->nextfunction = newfcb;
    }

    return 0;
}



//============================================ private standart function ==================================================

void ToneIotClient::initFunctionSys(void){

    setFunction(TOIC_FUNCTION_SYS_DISCONNECT, NULL, &ToneIotClient::cbFunctionDisconnect);
}

void ToneIotClient::cbFunctionDisconnect(uint8_t* buf, uint16_t len){

    sendFunctionAck();

    this->state = TOIC_STATE::DISCONNECTED;
    this->client->flush();
    this->client->stop();
    lastInActivity = lastOutActivity = millis();
}

int8_t ToneIotClient::sendFunctionInit(){
    
    itemFunction_t* itemFunction = this->listFunction;

    memcpy(this->packet->id, this->toneiotsettings.id, 8);
    this->packet->msgId = 0;   
    this->packet->function = TOIC_FUNCTION_SYS_INIT;

    struct
    {
        uint8_t  header;         // header 1 byte
        uint32_t salt;           // salt 1 byte
        uint8_t  id[8];          // id 8 bytes
        uint8_t  deviceType;     // device type 1 byte
        uint8_t  versionMajor;   // major version 1 byte
        uint8_t  versionMinor;   // minor version 1 byte
        uint8_t  date[12];       // compilation date 11 byte
        uint16_t keepAlive;     // keepAlive 2 byte
    } headerdata = {
        .header = 0,
        .deviceType = TONE_DEVICE_TYPE,
        .versionMajor = TONE_VERSION_MAJOR,
        .versionMinor = TONE_VERSION_MINOR,
        .keepAlive = this->keepAlive
    };
    

    //this->packet->pdata[0] = 0;   // header 1 byte
    memcpy(&this->packet->pdata[1], this->toneiotsettings.id, 8);   // id 8 bytes
    //this->packet->pdata[9] = TONE_DEVICE_TYPE;   // device type 1 byte
    //this->packet->pdata[10] = TONE_VERSION_MAJOR;   // major version 1 byte
    //this->packet->pdata[11] = TONE_VERSION_MINOR;   // minor version 1 byte
    memcpy(&this->packet->pdata[12], __DATE__, 11);    // DATE 11 byte
    //this->packet->pdata[23] = '\0';
    //memcpy(&this->packet->pdata[24], &this->keepAlive, 2);           // keepAlive 2 byte

    this->packet->datalen = sizeof(headerdata); // header + id + TONE_DEVICE_TYPE + TONE_VERSION_MAJOR + TONE_VERSION_MINOR + DATE + keepAlive
    memcpy(this->packet->pdata, &headerdata, this->packet->datalen);

    // 26 bytes is the beginning of the supported functions
    while(itemFunction != NULL) {
        if (this->packet->datalen > TOIC_MAX_PACKET_SIZE - 20) break;
        memcpy(&this->packet->pdata[this->packet->datalen], &itemFunction->function, 2);           // add number function 2 byte
        itemFunction = (itemFunction_t*)itemFunction->nextfunction;
        this->packet->datalen += 2;
    }

    //TODO Encrypt the data packet
    //TODO this->packet->length will change after encryption

    if (writePacket(this->packet)) return -1;

    //TODO wait init package, parse the init function, enable function

    if (waitServerRespons() != TOIC_FUNCTION_SYS_INIT) return -1;

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
    //        this->state = TOIC_STATE::CONNECTED;
    //        return true;
    //     } else {
    //         _state = buffer[3];
    //     }
    // }
    // _client->stop();

    return 0;
}

int8_t ToneIotClient::sendFunctionKeepAlive(){
    sendFunctio(TOIC_FUNCTION_SYS_KEEPALIVE);
}

void ToneIotClient::sendFunctionDisconnect(uint16_t code){
    sendFunctio(TOIC_FUNCTION_SYS_DISCONNECT, (uint8_t*)&code, 2);
    waitServerRespons();
}