#ifndef VBRAT_VBCONNECTION_H
#define VBRAT_VBCONNECTION_H
#include "tiny_websockets/client.hpp"

#define SERVER_ADDRESS_MAX_LEN 512

enum vbConnection_error {
    NO_ERRORS = 0,
    ERR_SERVER_ADDR_NOT_SET = 1,
    ERR_SOCKET_NOT_INITIALIZED = 2,
    ERR_MUTEX_ERROR = 3,
    ERR_PTHREAD_ERROR = 4,
    ERR_GENERIC_ERROR = 5,
    ERR_CANNOT_CONNECT = 6,
    ERR_SERVER_ADDR_INVALID = 7
};

enum vbConnection_state {
    S_TOINIZIALIZE = 0,
    S_INIT = 1,
    S_CONNECTING = 2,
    S_CONNECTED = 3,
    S_DISCONNECTED = 4,
    S_RECONNCTING = 5,
    S_ERROR = 6,
};
typedef void (*vbConnection_onEvent)();
typedef void (*vbConnection_onReceived)(const char *, int);
void vbConnection_onConnected(vbConnection_onEvent cb);
void vbConnection_onDisconnected(vbConnection_onEvent cb);
void vbConnection_onMessage(vbConnection_onReceived cb);

int32_t vbConnection_Connect(const char *address);
vbConnection_error vbConnection_Send(const char *msg);
vbConnection_state vbConnection_getState();
vbConnection_error vbConnection_getError();

#endif //VBRAT_VBCONNECTION_H
