// C library headers
#include <stdio.h>
#include <string.h>
// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <signal.h>
#include "serial.h"
#include <time.h>
#include "mSLIP.h"
#include "dispatch.h"
#include <iostream>


//static int dest_client_fd=-1;


static int serial_port;
pc_transm_msg_t mesh2pc_msg;
int recrec=0;

void read_handler(int signum){
    static bool msg_started=false;
    uint8_t buf[256];
    int len=read(serial_port,buf,255);
    uint8_t c;
    for(int i=0;i<len;i++){
        c=buf[i];
        if(c==SLIP_END){
            if(!msg_started){
                msg_started=true;
                mesh2pc_msg.length=0;
            }else{
                std::cerr<<"\r RESEIVED again "<<recrec++<<"       ";
//!!                LOG("\r\n received something with length %d ",mesh2pc_msg.length+1);
                if(mesh2pc_msg.length==1){//it is 2 END symbols received back to back. Assume we already receive first END, no action required
                    continue;
                }
                mesh2pc_msg.raw_arr[mesh2pc_msg.length++]=c;
                msg_started=false;
                if(OK==SLIP_parse_packet(&mesh2pc_msg)){
                    std::cerr<<"--try dispatch--";
                    dispatcher.BroadcastUp(mesh2pc_msg.raw_arr,mesh2pc_msg.length);
                }
            }
        }//END if(c==SLIP_END)
        if(msg_started) mesh2pc_msg.raw_arr[mesh2pc_msg.length++]=c;
        if(mesh2pc_msg.length>=MAX_PC_MSG_LENGTH){//we try to receive too long message
            mesh2pc_msg.length=0;
            msg_started=false;
        }
    }
}


int SerialInit(void) {
    // Open the serial port. Change device path as needed (currently set to an standard FTDI
//    serial_port = open("/dev/serial0", O_RDWR);
    serial_port = open(SERIAL_PATH, O_RDWR  | O_NOCTTY | O_NONBLOCK);
    // Create new termios struct, we call it 'tty' for convention
    struct termios tty;
    // Read in existing settings, and handle any error
    if(tcgetattr(serial_port, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        return 1;
    }
    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication
    tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size
    tty.c_cflag |= CS8; // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
//    tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX
//    tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESEN
    tty.c_cc[VTIME] = 2; // Wait for up to 0.2s (10 deciseconds), returning as soon as any data received
//    tty.c_cc[VMIN] = 0;
    tty.c_cc[VMIN] = 255;
    // Set in/out baud rate to be 115200
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
//    cfsetispeed(&tty, B9600);
//    cfsetospeed(&tty, B9600);
    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        return 1;
    }
    /**< set serial port for calling back handler upon receive */
    fcntl(serial_port,F_SETOWN,getpid());
    struct sigaction sig;
    sig.sa_handler=read_handler;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags=0;
    sigaction(SIGIO,&sig,NULL);
    /**< asynchronous input mode. If set, then SIGIO signals will be generated when input is available */
    int prev_flags=fcntl(serial_port,F_GETFL,0);
    prev_flags|=O_ASYNC;
    fcntl(serial_port,F_SETFL,prev_flags);

//    timer_create()
    return 0;
}
/********************************************//**
 * \brief
 *
 * \param
 * \param
 * \return
 *
 ***********************************************/
void SerialWrite(char* buf, int length){
    write(serial_port, buf, length);
}
void SerialClose(void){
    close(serial_port);
//    printf("close serial");
}
// See if there are bytes available to read
//int bytes;
//ioctl(fd, FIONREAD, &bytes);

