/*
 *                                                                                                  geany_encoding=koi8-r
 * socket.c - socket IO
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "usefull_macros.h"
#include "socket.h"
#include "term.h"
#include <netdb.h>      // addrinfo
#include <arpa/inet.h>  // inet_ntop
#include <pthread.h>
#include <limits.h>     // INT_xxx
#include <signal.h> // pthread_kill
#include <unistd.h> // daemon
#include <sys/syscall.h> // syscall

#include "sens_place.h"  // sensors coordinates
#include "gnuplot.h"     // plot graphs
#include "cmdlnopts.h"   // glob_pars

#define BUFLEN    (10240)
// Max amount of connections
#define BACKLOG   (30)

// temperatures: T0 (mirror surface), T1 (mirror bottom), T2 (inside NES)
static char strT[3][BUFLEN];
// mean temperature
static double meanT;

extern glob_pars *G;

// turn off sensors after measurements request or right now
static bool TurnOff = FALSE;
void TurnOFF(){TurnOff = TRUE;}

// return pointer to strT[N]
const char *gotstr(int N){
    if(N < 0 || N > 2) return NULL;
    return strT[N];
}

/**************** COMMON FUNCTIONS ****************/
/**
 * wait for answer from socket
 * @param sock - socket fd
 * @return 0 in case of error or timeout, 1 in case of socket ready
 */
static int waittoread(int sock){
    fd_set fds;
    struct timeval timeout;
    int rc;
    timeout.tv_sec = 1; // wait not more than 1 second
    timeout.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    do{
        rc = select(sock+1, &fds, NULL, NULL, &timeout);
        if(rc < 0){
            if(errno != EINTR){
                WARN("select()");
                return 0;
            }
            continue;
        }
        break;
    }while(1);
    if(FD_ISSET(sock, &fds)) return 1;
    return 0;
}

/**************** SERVER FUNCTIONS ****************/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
/**
 * Send data over socket
 * @param sock      - socket fd
 * @param webquery  - ==1 if this is web query
 * @param Nsens     - 0 for upper sensors, 1 for lower and 2 for inner (N2)
 * @return 1 if all OK
 */
static int send_data(int sock, int webquery, int Nsens){
    if(Nsens < 0 || Nsens > 2) return 0;
    DBG("webq=%d", webquery);
    ssize_t L, Len;
    char tbuf[BUFLEN];
    char *buf;
    // fill transfer buffer
    buf = strT[Nsens];
    Len = strlen(buf);
    // OK buffer ready, prepare to send it
    if(webquery){
        L = snprintf((char*)tbuf, BUFLEN,
            "HTTP/2.0 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST\r\n"
            "Access-Control-Allow-Credentials: true\r\n"
            "Content-type: text/plain\r\nContent-Length: %zd\r\n\r\n", Len);
        if(L < 0){
            WARN("sprintf()");
            return 0;
        }
        if(L != write(sock, tbuf, L)){
            WARN("write");
            return 0;
        }
    }
    // send data
    //DBG("send %zd bytes\nBUF: %s", Len, buf);
    if(Len != write(sock, buf, Len)){
        WARN("write()");
        return 0;
    }
    return 1;
}

// search a first word after needle without spaces
static char* stringscan(char *str, char *needle){
    if(!str || !needle) return NULL;
    char *a, *e;
    char *end = str + strlen(str);
    a = strstr(str, needle);
    if(!a) return NULL;
    a += strlen(needle);
    while (a < end && (*a == ' ' || *a == '\r' || *a == '\t' || *a == '\r')) a++;
    if(a >= end) return NULL;
    e = strchr(a, ' ');
    if(e) *e = 0;
    return a;
}
typedef struct{
    int sockn;
    char *host;
} conn;

static void *handle_socket(void *asock){
    FNAME();
    if(!asock) return NULL;
    conn C = *((conn*)asock);
    int sock = C.sockn; //*((int*)asock);
    int webquery = 0; // whether query is web or regular
    char buff[BUFLEN];
    char tbuf[128];
    int Nsens = -1;
    ssize_t rd, L;
    double t0 = dtime();
    while(dtime() - t0 < SOCKET_TIMEOUT){
        if(!waittoread(sock)){ // no data incoming
            continue;
        }
        if(!(rd = read(sock, buff, BUFLEN-1))){
            break;
        }
        DBG("Got %zd bytes", rd);
        if(rd < 0){ // error
            DBG("Nothing to read from fd %d (ret: %zd)", sock, rd);
            break;
        }
        // add trailing zero to be on the safe side
        buff[rd] = 0;
        // now we should check what do user want
        char *got, *found = buff;
        if((got = stringscan(buff, "GET")) || (got = stringscan(buff, "POST"))){ // web query
            webquery = 1;
            char *slash = strchr(got, '/');
            if(slash) found = slash + 1;
            // web query have format GET /some.resource
        }
        // here we can process user data
        //printf("user send: %s\nfound=%s", buff,found);
        if(strlen(found) < 5 && found[0] == 'T'){
            Nsens = found[1] - '0';
            if(Nsens < 0 || Nsens > 2){
                L = write(sock, "Only T0, T1 and T2 available\n", 29);
                break; // wrong query
            }
            pthread_mutex_lock(&mutex);
            if(!send_data(sock, webquery, Nsens)){
                LOG("can't send data, some error occured");
            }
            pthread_mutex_unlock(&mutex);
        }else if(strncmp("Tmean", found, 5) == 0){ // send user meanT
            L = snprintf(tbuf, 128, "%.2f\n", meanT);
            if(L != write(sock, tbuf, L)) WARN("write()");
        }else if(strncmp("ReBoOt", found, 6) == 0){
            LOG("Reboot command from %s", C.host);
            L = write(sock, "Reboot system\n", 14);
            if(0 != system("sudo reboot")) WARN("Can't reboot");
        // here can be more parsers
        }else L = write(sock, "Unrecognized command\n", 21);
        break;
    }
    close(sock);
    FREE(C.host);
    pthread_exit(NULL);
    return NULL;
}

// main socket server
static void *server(void *asock){
    if(!asock) return NULL;
    //LOG("server(): getpid: %d, pthread_self: %lu, tid: %lu",getpid(), pthread_self(), syscall(SYS_gettid));
    int sock = *((int*)asock);
    if(listen(sock, BACKLOG) == -1){
        WARN("listen()");
        return NULL;
    }
    while(1){
        socklen_t size = sizeof(struct sockaddr_in);
        struct sockaddr_in their_addr;
        int newsock;
        if(!waittoread(sock)) continue;
        newsock = accept(sock, (struct sockaddr*)&their_addr, &size);
        if(newsock <= 0){
            WARN("accept()");
            continue;
        }
        struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&their_addr;
        struct in_addr ipAddr = pV4Addr->sin_addr;
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ipAddr, str, INET_ADDRSTRLEN);
        //LOG("get connection from %s", str);
        red("Got connection from %s\n", str);
        pthread_t handler_thread;
        conn C = {.sockn = newsock};
        C.host = strdup(str);
        if(pthread_create(&handler_thread, NULL, handle_socket, (void*)&C)){
            WARN("pthread_create()");
        }else{
            DBG("Thread created, detouch");
            pthread_detach(handler_thread); // don't care about thread state
        }
    }
    LOG("server(): UNREACHABLE CODE REACHED!");
}

typedef double Item;
#define ELEM_SWAP(a, b) {register Item t = a; a = b; b = t;}
#define PIX_SORT(a, b)  {if (a > b) ELEM_SWAP(a, b);}
/**
 * quick select - algo for approximate median calculation for array idata of size n
 */
Item quick_select(Item *idata, int n){
    int low, high;
    int median;
    int middle, ll, hh;
    Item *arr = MALLOC(Item, n);
    memcpy(arr, idata, n*sizeof(Item));
    low = 0 ; high = n-1 ; median = (low + high) / 2;
    for(;;){
        if(high <= low) // One element only
            break;
        if(high == low + 1){ // Two elements only
            PIX_SORT(arr[low], arr[high]) ;
            break;
        }
        // Find median of low, middle and high items; swap into position low
        middle = (low + high) / 2;
        PIX_SORT(arr[middle], arr[high]) ;
        PIX_SORT(arr[low], arr[high]) ;
        PIX_SORT(arr[middle], arr[low]) ;
        // Swap low item (now in position middle) into position (low+1)
        ELEM_SWAP(arr[middle], arr[low+1]) ;
        // Nibble from each end towards middle, swapping items when stuck
        ll = low + 1;
        hh = high;
        for(;;){
            do ll++; while (arr[low] > arr[ll]);
            do hh--; while (arr[hh] > arr[low]);
            if(hh < ll) break;
            ELEM_SWAP(arr[ll], arr[hh]) ;
        }
        // Swap middle item (in position low) back into correct position
        ELEM_SWAP(arr[low], arr[hh]) ;
        // Re-set active partition
        if (hh <= median) low = ll;
        if (hh >= median) high = hh - 1;
    }
    Item ret = arr[median];
    FREE(arr);
    return ret;
}
#undef PIX_SORT
#undef ELEM_SWAP

static void process_T(){
    int i, N, p, Num = 0;
    time_t tmeasmax = 0;
    double arr[NSENSORS];
    // mean temperatures over 15 scans
    static double Tmean[2][NCHANNEL_MAX+1][NCTRLR_MAX+1];
    static int Nmean[2][NCHANNEL_MAX+1][NCTRLR_MAX+1], Nmeanmax; // amount of measurements for Tmean
    // get statistics
    poll_sensors(0); // poll N2
    // scan over controllers on mirror & calculate median
    for(i = 1; i <= NCTRLR_MAX; ++i){
        if(poll_sensors(i)){
            int N, p;
            for(p = 0; p < 2; ++p) for(N = 0; N <= NCHANNEL_MAX; ++ N){
                double T = t_last[p][N][i];
                time_t t = tmeasured[p][N][i];
                if(T > -100. && T < 100.){
                    arr[Num++] = T;
                    if(t > tmeasmax) tmeasmax = t;
                }
            }
        }
    }
    // calculate mean
    double Tmed = quick_select(arr, Num);
    double Tbot = Tmed - 10., Ttop = Tmed + 10.;
    DBG("Got %d values, Tmed=%g", Num, Tmed);
    // throw out all more than +-10degrC and calculate meanT
    Num = 0;
    double Tsum = 0.;
    for(i = 1; i <= NCTRLR_MAX; ++i){
        for(p = 0; p < 2; ++p) for(N = 0; N <= NCHANNEL_MAX; ++N){
            double T = t_last[p][N][i];
            if(T > Ttop || T < Tbot || tmeasmax - tmeasured[p][N][i] > 1800){ // not longer than 3 minutes ago!
                t_last[p][N][i] = -300.;
            }else{
                ++Num; Tsum += T;
                Tmean[p][N][i] += T;
                ++Nmean[p][N][i];
            }
        }
    }
    // make graphics
    if(G->savepath){
        DBG("GOT %dTH MEAN DATA", Nmeanmax+1);
        if(++Nmeanmax == GRAPHS_AMOUNT){
            for(i = 1; i <= NCTRLR_MAX; ++i)for(N = 0; N <= NCHANNEL_MAX; ++ N)for(p = 0; p < 2; ++p){
                if(Nmean[p][N][i]){
                    Tmean[p][N][i] /= Nmean[p][N][i];
                    Nmean[p][N][i] = 0;
                }else Tmean[p][N][i] = -300.; // no data
            }
            plot(Tmean, G->savepath);
            DBG("memset start");
            memset(Tmean, 0, sizeof(double)*2*(NCTRLR_MAX+1)*(NCHANNEL_MAX+1));
            DBG("memset end");
            Nmeanmax = 0;
        }
    }
    meanT = Tsum / Num;
    DBG("got %d, mean: %g\n\n", Num, meanT);
}

// data gathering & socket management
static void daemon_(int sock){
    if(sock < 0) return;
    pthread_t sock_thread;
    if(pthread_create(&sock_thread, NULL, server, (void*) &sock)){
        ERR("pthread_create()");
    }
    double tgot = 0., tlastoff = dtime();
    do{
        if(pthread_kill(sock_thread, 0) == ESRCH){ // died
            WARNX("Sockets thread died");
            pthread_join(sock_thread, NULL);
            if(pthread_create(&sock_thread, NULL, server, (void*) &sock)){
                ERR("pthread_create()");
            }
        }
        usleep(1000); // sleep a little or thread's won't be able to lock mutex
        if(TurnOff){
            TurnOff = FALSE;
            turn_all_off();
            tlastoff = dtime();
        }
        if(dtime() - tlastoff > T_OFF_INTERVAL){
            turn_all_off();
            tlastoff = dtime();
        }
        if(dtime() - tgot < T_INTERVAL) continue;
        // get data
        int i;
        char bufs[3][BUFLEN]; // temporary buffers: T0, T1, T2
        char *ptrs[3] = {bufs[0], bufs[1], bufs[2]};
        size_t lens[3] = {BUFLEN, BUFLEN, BUFLEN}; // free space
        tgot = dtime();
        process_T(); // get new temperatures & throw out bad results
        for(i = 0; i <= NCTRLR_MAX; ++i){ // scan over controllers
            int N, p;
            for(N = 0; N <= NCHANNEL_MAX; ++N) for(p = 0; p < 2; ++p){
                double T = t_last[p][N][i];
                char **buf;
                size_t *len;
                //DBG("T%d [%d][%d] = %g",i, p,N,T);
                if(T > -100. && T < 100.){ // fill buffer
                    size_t l;
                    if(i == 0){
                        buf = &ptrs[2]; len = &lens[2];
                        // Nsens Npair T time
                        l = snprintf(*buf, *len, "%d\t%d\t%.2f\t%ld\n", N, p, T,
                                        tmeasured[p][N][i]);
                    }else{
                        const sensor_data *sdata = get_sensor_location(i, N, p);
                        if(!sdata) continue; // wrong sensor number???
                        buf = &ptrs[sdata->Z]; len = &lens[sdata->Z];
                        // iNp x y T(corrected) time
                        l = snprintf(*buf, *len, "%d%d%d\t%d\t%d\t%.2f\t%ld\n", i, N, p,
                                     sdata->X, sdata->Y, T - sdata->dt - sdata->Tadj, tmeasured[p][N][i]);
                    }
                    *len -= l;
                    *buf += l;
                }
            }
        }
        //DBG("BUF0:\n%s\nBUF1:\n%s\nBUF2:\n%s", bufs[0],bufs[1],bufs[2]);
        // copy temporary buffers to main
        pthread_mutex_lock(&mutex);
        memcpy(strT, bufs, sizeof(strT));
        pthread_mutex_unlock(&mutex);
    }while(1);
    LOG("daemon_(): UNREACHABLE CODE REACHED!");
}

/**
 * Run daemon service
 */
void daemonize(char *port){
    if(!port) return;
    FNAME();
    int sock = -1;
    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if(getaddrinfo(NULL, port, &hints, &res) != 0){
        ERR("getaddrinfo");
    }
    struct sockaddr_in *ia = (struct sockaddr_in*)res->ai_addr;
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ia->sin_addr), str, INET_ADDRSTRLEN);
    DBG("canonname: %s, port: %u, addr: %s\n", res->ai_canonname, ntohs(ia->sin_port), str);
    // loop through all the results and bind to the first we can
    for(p = res; p != NULL; p = p->ai_next){
        if((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            WARN("socket");
            continue;
        }
        int reuseaddr = 1;
        if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1){
            ERR("setsockopt");
        }
        if(bind(sock, p->ai_addr, p->ai_addrlen) == -1){
            close(sock);
            WARN("bind");
            continue;
        }
        break; // if we get here, we have a successfull connection
    }
    if(p == NULL){
        // looped off the end of the list with no successful bind
        ERRX("failed to bind socket");
    }
    freeaddrinfo(res);
    daemon_(sock);
    close(sock);
    LOG("daemonize(): UNREACHABLE CODE REACHED!");
    signals(0);
}

