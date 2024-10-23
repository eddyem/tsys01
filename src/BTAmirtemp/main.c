#include <arpa/inet.h>
#include <errno.h>
#include <libgen.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h> //prctl
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h> // wait
#include <time.h>
#define __USE_BSD
#include <unistd.h>

#include "usefull_macros.h"
#include "bta_shdata.h"

#define HOST        "mirtemp.sao.ru"
#define PORT        "4444"
#define RESOURCE    "Tmean"

static int gotsegm = 0;

void clear_flags(){
    if(!gotsegm) return;
    if(MeteoMode & NET_T3){ // clear "net" & "sensor" flags
        MeteoMode &= ~NET_T3;
        if(MeteoMode & SENSOR_T3) MeteoMode &= ~SENSOR_T3;
    }
}

void signals(int sig){
    char ss[10];
    signal(sig, SIG_IGN);
    switch(sig){
        case SIGHUP : strcpy(ss,"SIGHUP");  break;
        case SIGINT : strcpy(ss,"SIGINT");  break;
        case SIGQUIT: strcpy(ss,"SIGQUIT"); break;
        case SIGFPE : strcpy(ss,"SIGFPE");  break;
        case SIGPIPE: strcpy(ss,"SIGPIPE"); break;
        case SIGSEGV: strcpy(ss,"SIGSEGV"); break;
        case SIGTERM: strcpy(ss,"SIGTERM"); break;
        default:  sprintf(ss,"SIG_%d",sig); break;
    }
    switch(sig){
        default:
        case SIGHUP :
             LOG("%s - Ignore ...", ss);
             fflush(stderr);
             signal(sig, signals);
             return;
        case SIGINT :
        case SIGPIPE:
        case SIGQUIT:
        case SIGFPE :
        case SIGSEGV:
        case SIGTERM:
             signal(SIGALRM, SIG_IGN);
             LOG("%s - Stop!", ss);
             clear_flags();
             exit(sig);
    }
}

/**
 * get mirror temperature over network
 * @return 0 if succeed
 */
int get_mirT(double *T){
    int sockfd = 0;
    char recvBuff[64];
    memset(recvBuff, 0, sizeof(recvBuff));
    struct addrinfo h, *r, *p;
    memset(&h, 0, sizeof(h));
    h.ai_family = AF_INET;
    h.ai_socktype = SOCK_STREAM;
    h.ai_flags = AI_CANONNAME;
    char *host = HOST;
    char *port = PORT;
    if(getaddrinfo(host, port, &h, &r)) WARNX("getaddrinfo()");
    for(p = r; p; p = p->ai_next){
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            WARN("socket()");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            WARN("connect()");
            continue;
        }
        break; // if we get here, we must have connected successfully
    }
    if(p == NULL){
        WARNX("failed to connect");
        return 1;
    }
    freeaddrinfo(r);
    if(send(sockfd, RESOURCE, sizeof(RESOURCE), 0) != sizeof(RESOURCE)){
        WARN("send()");
        return 1;
    }
    ssize_t rd = read(sockfd, recvBuff, sizeof(recvBuff)-1);
    if(rd < 0){
        WARN("read()");
        return 1;
    }else recvBuff[rd] = 0;
    close(sockfd);
    char *eptr;
    *T = strtod(recvBuff, &eptr);
    DBG("Got mirror T=%.1f", *T);
    if(eptr == recvBuff) return 1;
    return 0;
}

int main (int argc, char *argv[]){
    initial_setup();
    if(argc == 2){
        printf("Log file: %s", argv[1]);
        Cl_createlog(argv[1]);
    }
    signal(SIGHUP, signals);
    signal(SIGINT, signals);
    signal(SIGQUIT,signals);
    signal(SIGFPE, signals);
    signal(SIGPIPE,signals);
    signal(SIGSEGV,signals);
    signal(SIGTERM,signals);

    LOG("\nStarted\n");

    #ifndef EBUG
    if(daemon(1, 0)){
        ERR("daemon()");
    }
    while(1){ // guard for dead processes
        pid_t childpid = fork();
        if(childpid){
            LOG("create child with PID %d", childpid);
            wait(NULL);
            LOG("child %d died\n", childpid);
            sleep(1);
        }else{
            prctl(PR_SET_PDEATHSIG, SIGTERM); // send SIGTERM to child when parent dies
            break; // go out to normal functional
        }
    }
    #endif

    time_t tlast = time(NULL);
    while(1){
        if(!gotsegm){
            sdat.mode |= 0200;
            sdat.atflag = 0;
            gotsegm = get_shm_block( &sdat, ClientSide);
            if(!gotsegm){
                LOG("Can't find SHM segment");
            } else get_cmd_queue( &ocmd, ClientSide);
        }
        double T;
        if(time(NULL) - tlast > 900){ // no signal for 15 minutes - clear flags
            LOG("15 minutes - no signal!");
            tlast = time(NULL);
            clear_flags();
        }
        if(get_mirT(&T)){
            sleep(10);
            continue;
        }
        if(gotsegm && 0 == (MeteoMode & INPUT_T3)){ // not manual mode - change Tmir value
            val_T3 = T;
            MeteoMode |= (SENSOR_T3|NET_T3);
            DBG("Change T: %.2f", T);
        }
        sleep(60);
        tlast = time(NULL);
    }
    return 0;
}
