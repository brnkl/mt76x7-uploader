#ifndef XMODEM_H
#define XMODEM_H

#include "legato.h"

int xmodemTransmit(unsigned char* src, int srcsz);
uint8_t _inbyte(unsigned short timeout);
void _outbyte(uint8_t c);

#endif
