#ifndef XMODEM_H
#define XMODEM_H

#include "legato.h"

enum {
  PROTOCOL_XMODEM,
  PROTOCOL_YMODEM,
};

int outputFd;

int xymodem_send(int serial_fd, const char* filename, int protocol, int wait);
int open_serial(const char* path, int baud);
int writeAndIntercept(int fd, const void* buf, size_t count);

#endif
