#ifndef MTK_UPLOAD_H
#define MTK_UPLOAD_H

#define MTK7697_BAUD 115200

static const char* MTK_SERIAL_PORT_PATH = "/dev/ttyHS0";

typedef struct {
  int serialPort;
  int errorCount;
  int cCount;
  int retry;
  int startTime;
} FlashState;

bool mtk_verifyInitSequence(FlashState* s);

void mtk_configureGpio();

#endif
