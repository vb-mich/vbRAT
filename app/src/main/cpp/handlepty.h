#ifndef _HANDLEPTY_H_
#define _HANDLEPTY_H_
#include <jni.h>
//int execCommand(const char *cmd, char *buf, uint16_t bufsize);
typedef void (*on_ttyout)(char *msg);
int startTTY(on_ttyout cb);
int ttySend(const char *cmd);

enum handlePtyErrors
{

};
#endif