#ifndef _HANDLEPTY_H_
#define _HANDLEPTY_H_
#include <jni.h>
//int execCommand(const char *cmd, char *buf, uint16_t bufsize);
typedef void (*on_ttyout)(char *msg);
int vbTTY_startShell(on_ttyout cb);
int vbTTY_send(const char *cmd);
int vbTTY_stopShell();
bool vbTTY_isOpen();

enum handlePtyErrors
{

};
#endif