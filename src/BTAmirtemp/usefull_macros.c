/*
 * usefull_macros.h - a set of usefull functions: memory, color etc
 *
 * Copyright 2013 Edward V. Emelianoff <eddy@sao.ru>
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
 */

#include "usefull_macros.h"

#include <pthread.h>

/**
 * function for different purposes that need to know time intervals
 * @return double value: time in seconds
 */
double dtime(){
    double t;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    t = tv.tv_sec + ((double)tv.tv_usec)/1e6;
    return t;
}

/******************************************************************************\
 *                          Coloured terminal
\******************************************************************************/
int globErr = 0; // errno for WARN/ERR

// pointers to coloured output printf
int (*red)(const char *fmt, ...);
int (*green)(const char *fmt, ...);
int (*_WARN)(const char *fmt, ...);

/*
 * format red / green messages
 * name: r_pr_, g_pr_
 * @param fmt ... - printf-like format
 * @return number of printed symbols
 */
int r_pr_(const char *fmt, ...){
    va_list ar; int i;
    printf(RED);
    va_start(ar, fmt);
    i = vprintf(fmt, ar);
    va_end(ar);
    printf(OLDCOLOR);
    return i;
}
int g_pr_(const char *fmt, ...){
    va_list ar; int i;
    printf(GREEN);
    va_start(ar, fmt);
    i = vprintf(fmt, ar);
    va_end(ar);
    printf(OLDCOLOR);
    return i;
}
/*
 * print red error/warning messages (if output is a tty)
 * @param fmt ... - printf-like format
 * @return number of printed symbols
 */
int r_WARN(const char *fmt, ...){
    va_list ar; int i = 1;
    fprintf(stderr, RED);
    va_start(ar, fmt);
    if(globErr){
        errno = globErr;
        vwarn(fmt, ar);
        errno = 0;
        globErr = 0;
    }else
        i = vfprintf(stderr, fmt, ar);
    va_end(ar);
    i++;
    fprintf(stderr, OLDCOLOR "\n");
    return i;
}

/*
 * notty variants of coloured printf
 * name: s_WARN, r_pr_notty
 * @param fmt ... - printf-like format
 * @return number of printed symbols
 */
int s_WARN(const char *fmt, ...){
    va_list ar; int i = 0;
    va_start(ar, fmt);
    if(globErr){
        errno = globErr;
        vwarn(fmt, ar);
        errno = 0;
        globErr = 0;
    }else
        i = +vfprintf(stderr, fmt, ar);
    va_end(ar);
    i += fprintf(stderr, "\n");
    return i;
}
int r_pr_notty(const char *fmt, ...){
    va_list ar; int i = 0;
    va_start(ar, fmt);
    i += vprintf(fmt, ar);
    va_end(ar);
    i += printf("\n");
    return i;
}

/**
 * Run this function in the beginning of main() to setup locale & coloured output
 */
void initial_setup(){
    // setup coloured output
    if(isatty(STDOUT_FILENO)){ // make color output in tty
        red = r_pr_; green = g_pr_;
    }else{ // no colors in case of pipe
        red = r_pr_notty; green = printf;
    }
    if(isatty(STDERR_FILENO)) _WARN = r_WARN;
    else _WARN = s_WARN;
    // Setup locale
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");
#if defined GETTEXT_PACKAGE && defined LOCALEDIR
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    textdomain(GETTEXT_PACKAGE);
#endif
}

/******************************************************************************\
 *                                  Memory
\******************************************************************************/
/*
 * safe memory allocation for macro ALLOC
 * @param N - number of elements to allocate
 * @param S - size of single element (typically sizeof)
 * @return pointer to allocated memory area
 */
void *my_alloc(size_t N, size_t S){
    void *p = calloc(N, S);
    if(!p) ERR("malloc");
    //assert(p);
    return p;
}

/**
 * Mmap file to a memory area
 *
 * @param filename (i) - name of file to mmap
 * @return stuct with mmap'ed file or die
 */
mmapbuf *My_mmap(char *filename){
    int fd;
    char *ptr;
    size_t Mlen;
    struct stat statbuf;
    if(!filename) ERRX(_("No filename given!"));
    if((fd = open(filename, O_RDONLY)) < 0)
        ERR(_("Can't open %s for reading"), filename);
    if(fstat (fd, &statbuf) < 0)
        ERR(_("Can't stat %s"), filename);
    Mlen = statbuf.st_size;
    if((ptr = mmap (0, Mlen, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED)
        ERR(_("Mmap error for input"));
    if(close(fd)) ERR(_("Can't close mmap'ed file"));
    mmapbuf *ret = MALLOC(mmapbuf, 1);
    ret->data = ptr;
    ret->len = Mlen;
    return  ret;
}

void My_munmap(mmapbuf *b){
    if(munmap(b->data, b->len))
        ERR(_("Can't munmap"));
    FREE(b);
}


/******************************************************************************\
 *                          Terminal in no-echo mode
\******************************************************************************/
static struct termios oldt, newt; // terminal flags
static int console_changed = 0;
// run on exit:
void restore_console(){
    if(console_changed)
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // return terminal to previous state
    console_changed = 0;
}

// initial setup:
void setup_con(){
    if(console_changed) return;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    if(tcsetattr(STDIN_FILENO, TCSANOW, &newt) < 0){
        WARN(_("Can't setup console"));
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        signals(0); //quit?
    }
    console_changed = 1;
}

/**
 * Read character from console without echo
 * @return char readed
 */
int read_console(){
    int rb;
    struct timeval tv;
    int retval;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    tv.tv_sec = 0; tv.tv_usec = 10000;
    retval = select(1, &rfds, NULL, NULL, &tv);
    if(!retval) rb = 0;
    else {
        if(FD_ISSET(STDIN_FILENO, &rfds)) rb = getchar();
        else rb = 0;
    }
    return rb;
}

/**
 * getchar() without echo
 * wait until at least one character pressed
 * @return character readed
 */
int mygetchar(){ // getchar() without need of pressing ENTER
    int ret;
    do ret = read_console();
    while(ret == 0);
    return ret;
}

// logging


static Cl_log log = {0};
/**
 * @brief Cl_createlog - create log file: init mutex, test file open ability
 * @param log - log structure
 * @return 0 if all OK
 */
int Cl_createlog(char *logname){
    if(log.logpath){
        FREE(log.logpath);
        pthread_mutex_destroy(&log.mutex);
    }
    FILE *logfd = fopen(logname, "a");
    if(!logfd){
        WARN("Can't open log file");
        return 2;
    }
    log.logpath = strdup(logname);
    fclose(logfd);
    if(pthread_mutex_init(&log.mutex, NULL)){
        WARN("Can't init log mutes");
        return 3;
    }
    return 0;
}

/**
 * @brief Cl_putlog - put message to log file with/without timestamp
 * @param timest - ==1 to put timestamp
 * @param log - pointer to log structure
 * @param lvl - message loglevel (if lvl > loglevel, message won't be printed)
 * @param fmt - format and the rest part of message
 * @return amount of symbols saved in file
 */
int Cl_putlogt(const char *fmt, ...){
    if(pthread_mutex_lock(&log.mutex)){
        WARN("Can't lock log mutex");
        return 0;
    }
    int i = 0;
    FILE *logfd = fopen(log.logpath, "a");
    if(!logfd) goto rtn;
    char strtm[128];
    time_t t = time(NULL);
    struct tm *curtm = localtime(&t);
    strftime(strtm, 128, "%Y/%m/%d-%H:%M:%S", curtm);
    i = fprintf(logfd, "%s\t", strtm);
    va_list ar;
    va_start(ar, fmt);
    i += vfprintf(logfd, fmt, ar);
    va_end(ar);
    i += fprintf(logfd, "\n");
    fclose(logfd);
rtn:
    pthread_mutex_unlock(&log.mutex);
    return i;
}

