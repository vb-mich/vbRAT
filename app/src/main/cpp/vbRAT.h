//
// Created by Asus on 17/04/2020.
//

#ifndef VBRAT_VBRAT_H
#define VBRAT_VBRAT_H
void vbRAT_connected();
void vbRAT_messageReceived(const char* buf, int len);
const char* vbTTY_getError();
#endif //VBRAT_VBRAT_H
