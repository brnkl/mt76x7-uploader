#include "legato.h"
#include "interfaces.h"
#include "xmodem.h"
#include "util.h"

#define MTK7697_BAUD 115200
#define MTK7697_BAUDX 115200 * 8
static const char* SERIAL_PORT_PATH = "/dev/ttyHS0";
static const char* LDR_BIN_PATH = "/home/root/mtfiles/mt7697_bootloader.bin";
static const char* N9_BIN_PATH =
    "/home/root/mtfiles/WIFI_RAM_CODE_MT76X7_in_flash.bin";
static const char* CM4_BIN_PATH = "/home/root/mtfiles/ble_smart_connect.bin";
static const char* DA_PATH = "/home/root/mtfiles/da97.bin";

typedef enum { LDR = 1, N9 = 3, CM4 = 2 } MtkMemorySegment;

typedef struct {
  int serialPort;
  int errorCount;
  int cCount;
  int retry;
  int startTime;
} FlashState;

FlashState state = {0, 0, 0, 0};

void configureSerialPort(FlashState* s, int baud) {
  close(s->serialPort);
  s->serialPort = fd_openSerial(SERIAL_PORT_PATH, baud);
}

bool flashDa(FlashState* s) {
  configureSerialPort(&state, MTK7697_BAUD);
  LE_INFO("Sending DA over xmodem");
  bool r = XSend(s->serialPort, DA_PATH) == 0;
  LE_INFO("DA success: %d", r);
  return r;
}

void putIntoBootloader() {
  // make sure the bootstrap pin is on
  mtBootstrap_Activate();
  // reset state && wait
  mtRst_Deactivate();
  sleep(1);
  // pull out of reset
  mtRst_Activate();
}

void retryInitSequence(FlashState* s) {
  s->retry++;
  s->cCount = 0;
  s->errorCount = 0;
  s->startTime = util_getUnixDatetime();
  close(s->serialPort);
  putIntoBootloader();
  configureSerialPort(s, MTK7697_BAUD);
}

bool verifyInitSequence(FlashState* s) {
  uint8_t data;
  bool initDone = false;
  s->startTime = util_getUnixDatetime();
  while (!initDone) {
    data = fd_getChar(s->serialPort);
    fd_flush(s->serialPort);
    if (data == 'C') {
      s->cCount++;
      LE_INFO("Got a C");
    } else if (data != 0) {
      s->errorCount++;
      LE_INFO("Got an error");
    }
    if (s->cCount > 1) {
      initDone = true;
      LE_INFO("Init done");
      break;
    }
    if (s->errorCount > 3 || (util_getUnixDatetime() - s->startTime > 3)) {
      LE_INFO("Retrying...");
      retryInitSequence(s);
    }
    if (s->retry > 3) {
      LE_INFO("Aborting");
      break;
    }
  }
  return initDone;
}

bool flashBinary(FlashState* s, MtkMemorySegment segment, const char* binPath) {
  putIntoBootloader();
  configureSerialPort(s, MTK7697_BAUD);
  bool init = verifyInitSequence(s);
  LE_INFO("Init verified: %d", init);
  flashDa(s);
  configureSerialPort(s, MTK7697_BAUDX);
  sleep(1);
  bool initDone = false;
  int data = '0';
  switch (segment) {
    case LDR:
      fd_puts(s->serialPort, "1\r");
      break;
    case N9:
      fd_puts(s->serialPort, "3\r");
      break;
    case CM4:
      fd_puts(s->serialPort, "2\r");
      break;
  }
  while (!initDone) {
    LE_INFO("here %c", data);
    data = fd_getChar(s->serialPort);
    if (data == 'C') {
      initDone = true;
      break;
    }
  }
  fd_flush(s->serialPort);
  LE_INFO("Sending %s over xmodem", binPath);
  bool r = XSend(s->serialPort, binPath) == 0;
  LE_INFO("%s success: %d", binPath, r);
  sleep(1);
  fd_puts(s->serialPort, "C\r");
  fd_flush(s->serialPort);
  return true;
}

bool flashLdr(FlashState* s) {
  LE_INFO("Flashing LDR segment");
  return flashBinary(s, LDR, LDR_BIN_PATH);
}

bool flashN9(FlashState* s) {
  LE_INFO("Flashing N9 segment");
  return flashBinary(s, N9, N9_BIN_PATH);
}

bool flashCm4(FlashState* s) {
  LE_INFO("Flashing CM4");
  return flashBinary(s, CM4, CM4_BIN_PATH);
}

void resetPins() {
  mtBootstrap_Deactivate();
}

void configureGpio() {
  mtRst_SetPushPullOutput(MTRST_ACTIVE_HIGH, true);
  mtBootstrap_SetPushPullOutput(MTBOOTSTRAP_ACTIVE_HIGH, true);
}

COMPONENT_INIT {
  configureGpio();
  flashLdr(&state);
  flashN9(&state);
  flashCm4(&state);
  resetPins();
  close(state.serialPort);
}
