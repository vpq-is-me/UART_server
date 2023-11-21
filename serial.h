#ifndef SERIAL_H_INCLUDED
#define SERIAL_H_INCLUDED

#include "mSLIP.h"
//#ifdef __cplusplus
// extern "C" {
//#endif

//NOTE before using serial port don't forget add current user to special group: "sudo adduser USERNAME dialout" and reboot PC
//and may be it will require disable console at boot in raspi-config
//#define SERIAL_PATH "/dev/ttyUSB0"
#define SERIAL_PATH "/dev/serial0"
//#define SERIAL_PATH "/dev/tty0"


int SerialInit(void);
void SerialWrite(char* buf, int length);
void SerialClose(void);

void SerialSetClient(int fd);

//#ifdef __cplusplus
//}
//#endif

#endif // SERIAL_H_INCLUDED
