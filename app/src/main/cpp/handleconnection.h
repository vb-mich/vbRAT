#ifndef VBRAT_HANDLECONNECTION_H
#define VBRAT_HANDLECONNECTION_H
#include "tiny_websockets/client.hpp"

namespace vbRAT
{
    class vbConnection : websockets::WebsocketsClient
    {


    };
}

int vbRAT_start();

enum connectionState {
    S_ERROR = 0,
    S_INIT = 1,
    S_CONNECTING = 2,
    S_CONNECTED = 3,
    S_DISCONNECTED = 4,
    S_RECONNECTION = 5
};
const char* getStates();

#endif //VBRAT_HANDLECONNECTION_H
