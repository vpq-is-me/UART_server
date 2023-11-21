#include <iostream>
#include <sys/socket.h>
//#include <sys/un.h>
#include <unistd.h>
//#include <string.h>
//#include <stdlib.h>
//#include <stddef.h>
#include <sys/un.h>
#include <signal.h>
#include "serial.h"
#include "dispatch.h"
#include <sys/time.h>
#include <sys/resource.h>

using namespace std;

/*
//tips for restart operation interrupted by receiving some of signal
restart:
int n = recv(fd, buf, len, 0);
if (n == -1) {
    if (errno == EINTR) {
        goto restart;
    }
    perror("recv");
    return -1;
}
*/


/********************************************//**
 * \brief Callback function, used for kernel signal
 * handling
 * \param signum int - actual signal number
 * \return void
 *
 ***********************************************/
volatile sig_atomic_t stop_flg=0;
void SigStopHandler(int signum){
    if(signum==SIGTSTP)printf("stop program by CTRL+Z\r\r");
    else if(signum==SIGALRM)printf("Timer expiration\r\n");
    else /**< SIGINT */printf("user quit program by CTRL+C\r\n");
    stop_flg=1;
}
/********************************************//**
 * \brief
 *
 * \param void
 * \return void
 *
 ***********************************************/
void SigPreset(void){
    struct sigaction sig;
    sig.sa_handler=SigStopHandler;
    sigemptyset(&sig.sa_mask);
//    sigaddset(&sig.sa_mask,SIGTSTP);
//    sigaddset(&sig.sa_mask,SIGALRM);
//    sigaddset(&sig.sa_mask,SIGINT);
    sig.sa_flags=0;
    sig.sa_flags=SA_RESTART;

    sigaction(SIGTSTP,&sig,NULL);//user press CTRL+Z
    sigaction(SIGALRM,&sig,NULL);//expiration of timer
    sigaction(SIGINT,&sig,NULL);//user press CTRL+C
}



#define UART_SOCKET "/tmp/uart_socket.socket"
#define BACKLOG 10 // how many pending connections queue will hold
int main()
{
    int serv_socket;
    if(int err=setpriority(PRIO_PROCESS,0,-20)<0){
        printf("Error %i to setpriority: %s\n", errno, strerror(errno));
    }
    SigPreset();
    SerialInit();
    unlink(UART_SOCKET);// unbind if previously not properly terminated
    /**< create socket */
    if((serv_socket=socket(AF_LOCAL,SOCK_STREAM,0))<0){
        cerr<<"socket not achieved"<<endl;
        exit(1);
    }
    /**< prepare address structure to bind to socket */
    struct sockaddr_un sock_addr;
    memset(&sock_addr,0,sizeof(struct sockaddr_un));
    sock_addr.sun_family=AF_LOCAL;
    strncpy(sock_addr.sun_path, UART_SOCKET, sizeof(sock_addr.sun_path)-1);
    sock_addr.sun_path[sizeof(sock_addr.sun_path)-1]='\0';
    /**< bind socket to address and create listening queue */
    size_t sock_addr_length=offsetof(struct sockaddr_un, sun_path)+strlen(sock_addr.sun_path);
    if(bind(serv_socket, (struct sockaddr*)&sock_addr, sock_addr_length)<0){
        cerr<<"bind error"<<endl;
        exit(1);
    }
    if(listen(serv_socket,BACKLOG)<0){
        cerr<<"listening"<<endl;
    }
    /**< We will use working in one thread approach with multiple connection rather than creating multiple thread by forking for every new connection */
    /**< so we have to use file descriptor set and select() function */
    fd_set main_fdset, accepting_fdset;
    FD_ZERO(&main_fdset);
    FD_SET(serv_socket,&main_fdset);
    /**< for working with fd_set we will use max iteration boundaries assuming every new file descriptor is as small value as possible in acceding order  */
    /**< otherwise we can use FD_SETSIZE as a upper iteration boundary */
    int fd_max=serv_socket+1;
    pc_transm_msg_t pc2mesh_msg;
    while(!stop_flg){
        accepting_fdset=main_fdset;
        /**< block in waiting some requests from client */
        restart: //if select()interrupted by any signal it is not good idea to finish work
        if(select(fd_max+1,&accepting_fdset,nullptr,nullptr,nullptr)==-1){//for some reason if you live "select(fmax,..." and connected client with even number messages from him don't unblock "select"
            if(errno==EINTR){//some signal
                if(stop_flg)break;
                goto restart;//stop_fg==0
            }
            cerr<<endl<<"select error with errno:"<<strerror(errno)<<endl;
            goto restart;
#warning TODO (vladimir#1#): add counter to restart. May it require restart entire programm
            exit(1);
        }
        for(int i=0;i<=fd_max;i++){
            if(FD_ISSET(i,&accepting_fdset)){// file descriptor with value has activities
                if(i==serv_socket){// somebody new wants to connect!
                    int new_socket;
                    if((new_socket=accept(serv_socket,NULL,NULL))<0){
                        cerr<<"accepting error"<<endl;
                    }
                    FD_SET(new_socket,&main_fdset);
                    LOG("client %d added", new_socket);
                    dispatcher.AddFD(new_socket);
                    if(new_socket>fd_max)fd_max=new_socket+1;
                }else{//new request from client
                    ssize_t length=read(i,pc2mesh_msg.raw_arr,MAX_PC_MSG_LENGTH);
                    if(length<0){
                        cerr<<"reading error"<<endl;
                        exit(1);
                    }else if(length==0){//client disconnected
                        close(i);
                        FD_CLR(i,&main_fdset);
                        dispatcher.RemoveFD(i);
                    }else{//plain message
                        pc2mesh_msg.length=length;
                        cout<<"received :"<<length<<"bytes : ";
                        for(int i=0; i<length;i++){printf(" %02x",(int)pc2mesh_msg.raw_arr[i]);}
                        cout<<endl;
                        if(!SLIP_prepare_packet(&pc2mesh_msg)){
                            SerialWrite((char*)pc2mesh_msg.raw_arr,pc2mesh_msg.length);
                            cout<<"client #" << i<<" send to mesh "<<pc2mesh_msg.length<<"bytes"<<endl;
                        }else{
                            cout<<"client" << i<<"send brocken message (NOT SLIP confirmed)"<<endl;
                        }
                    }
                }

            }
        }
    }
    close(serv_socket);
    unlink(UART_SOCKET);
    SerialClose();
    cout << "Server stopped. Good bay" << endl;
    return 0;
}
