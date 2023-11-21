#ifndef DISPATH_H_INCLUDED
#define DISPATH_H_INCLUDED

#include <sys/select.h>     //everything about fd_set - file descriptor set
#include <stdint.h>         //uint8_t end etc.
#include <unistd.h>         // write(), read(), close()
#include <stdio.h>

#define LOG(format,...)  printf(format, __VA_ARGS__)



//**************************************************************************************************************************
class tDispatcher_cl {
public:

    tDispatcher_cl(void) {
        FD_ZERO(&consumer_fd_set);
    }

    void SetFDset(fd_set new_set);

    void RemoveFD(int fd) {
        FD_CLR(fd,&consumer_fd_set);
        FindFDmaxNo();
    }
    void AddFD(int fd){
        FD_SET(fd,&consumer_fd_set);
        if(fd>fd_max_no)fd_max_no=fd;
    }
    void BroadcastUp(uint8_t*msg,uint16_t length);

private:

    void FindFDmaxNo(void) {
        for(int i=0; i<FD_SETSIZE; i++) {
            if(FD_ISSET(i,&consumer_fd_set))fd_max_no=i;
        }
    }

    fd_set consumer_fd_set;
    int fd_max_no=0;
};
//**************************************************************************************************************************
extern tDispatcher_cl dispatcher;

#endif // DISPATH_H_INCLUDED
