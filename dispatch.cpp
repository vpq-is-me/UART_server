#include "dispatch.h"
#include <iostream>
#include <unistd.h>
#include <stdio.h>
tDispatcher_cl dispatcher;

void  tDispatcher_cl::SetFDset(fd_set new_set) {
    consumer_fd_set=new_set;
    for(int i=0; i<FD_SETSIZE; i++) {
        if(FD_ISSET(i,&consumer_fd_set))fd_max_no=i;
    }
}
static int num=0;//!!!
void tDispatcher_cl::BroadcastUp(uint8_t*msg,uint16_t length){
//!!    LOG("re-send message to  max %d clients\r\n", fd_max_no);
    std::cerr<<"<new send "<<num++;
    for(int i=0;i<=fd_max_no;i++){
        if(FD_ISSET(i,&consumer_fd_set)){
//            LOG("re-send message to %d client\r\n", i);
            write(i,msg,length);
        }
    }
    std::cerr<<"<<finish dispatch>>"<<num++;
}
