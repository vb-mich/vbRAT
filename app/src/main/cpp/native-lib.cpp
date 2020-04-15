#include <jni.h>
#include <string>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "handleconnection.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_vbrat_MainActivity_vbRATstart(JNIEnv *env, jobject MainActivity)
{
    vbRAT_start();
    return env->NewStringUTF("INIT...");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_vbrat_MainActivity_checkStatus(JNIEnv *env, jobject MainActivity)
{
    return env->NewStringUTF(getStates());
}
