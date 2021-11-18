/**
    * @author Dyakonov Oleg <o.u.dyakonov@gmail.com>
    * 
    * @brief ToneIotFunction
*/

#ifndef TONEIOTFUNCTION_h
#define TONEIOTFUNCTION_h

#include <functional>

#include "ToneIotClient.h"

#define TOIF_CALLBACK_SIGNATURE std::function<int(uint8_t* buf, size_t len)> callback

class ToneIotFunction {

public:
    
    ToneIotFunction(uint8_t number, TOIF_CALLBACK_SIGNATURE);

    //int server2device(ToneIotClient::packet_t* packet);
    //int device2server(uint8_t* buf, size_t len);

private:
    uint8_t number;
    TOIF_CALLBACK_SIGNATURE;

};


#endif //TONEIOTFUNCTION_h