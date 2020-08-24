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
#include <time.h>
#include <linux/limits.h> // PATH_MAX
#include <signal.h>

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
    }else
        i = vfprintf(stderr, fmt, ar);
    va_end(ar);
    i++;
    fprintf(stderr, OLDCOLOR "\n");
    return i;
}

static const char stars[] = "****************************************";
/*
 * notty variants of coloured printf
 * name: s_WARN, r_pr_notty
 * @param fmt ... - printf-like format
 * @return number of printed symbols
 */
int s_WARN(const char *fmt, ...){
    va_list ar; int i;
    i = fprintf(stderr, "\n%s\n", stars);
    va_start(ar, fmt);
    if(globErr){
        errno = globErr;
        vwarn(fmt, ar);
        errno = 0;
    }else
        i = +vfprintf(stderr, fmt, ar);
    va_end(ar);
    i += fprintf(stderr, "\n%s\n", stars);
    i += fprintf(stderr, "\n");
    return i;
}
int r_pr_notty(const char *fmt, ...){
    va_list ar; int i;
    i = printf("\n%s\n", stars);
    va_start(ar, fmt);
    i += vprintf(fmt, ar);
    va_end(ar);
    i += printf("\n%s\n", stars);
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
    /// "Не задано имя файла!"
    if(!filename){
        WARNX(_("No filename given!"));
        return NULL;
    }
    if((fd = open(filename, O_RDONLY)) < 0){
    /// "Не могу открыть %s для чтения"
        WARN(_("Can't open %s for reading"), filename);
        return NULL;
    }
    if(fstat (fd, &statbuf) < 0){
    /// "Не могу выполнить stat %s"
        WARN(_("Can't stat %s"), filename);
        close(fd);
        return NULL;
    }
    Mlen = statbuf.st_size;
    if((ptr = mmap (0, Mlen, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED){
    /// "Ошибка mmap"
        WARN(_("Mmap error for input"));
        close(fd);
        return NULL;
    }
    /// "Не могу закрыть mmap'нутый файл"
    if(close(fd)) WARN(_("Can't close mmap'ed file"));
    mmapbuf *ret = MALLOC(mmapbuf, 1);
    ret->data = ptr;
    ret->len = Mlen;
    return  ret;
}

void My_munmap(mmapbuf *b){
    if(munmap(b->data, b->len)){
    /// "Не могу munmap"
        WARN(_("Can't munmap"));
    }
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
        /// "Не могу настроить консоль"
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


/******************************************************************************\
 *                              TTY with select()
\******************************************************************************/
static struct termio oldtty, tty; // TTY flags
static int comfd = -1; // TTY fd

// run on exit:
void restore_tty(){
    if(comfd == -1) return;
    ioctl(comfd, TCSANOW, &oldtty ); // return TTY to previous state
    close(comfd);
    comfd = -1;
}

#ifndef BAUD_RATE
#define BAUD_RATE B9600
#endif
// init:
void tty_init(char *comdev){
    DBG("\nOpen port %s ...", comdev);
    do{
        comfd = open(comdev,O_RDWR|O_NOCTTY|O_NONBLOCK);
    }while (comfd == -1 && errno == EINTR);
    if(comfd < 0){
        WARN("Can't use port %s",comdev);
        signals(-1); // quit?
    }
    // make exclusive open
    if(ioctl(comfd, TIOCEXCL)){
        WARN(_("Can't do exclusive open"));
        close(comfd);
        signals(2);
    }
    DBG(" OK\nGet current settings... ");
    if(ioctl(comfd,TCGETA,&oldtty) < 0){  // Get settings
        /// "Не могу получить настройки"
        WARN(_("Can't get settings"));
        signals(-1);
    }
    tty = oldtty;
    tty.c_lflag     = 0; // ~(ICANON | ECHO | ECHOE | ISIG)
    tty.c_oflag     = 0;
    tty.c_cflag     = BAUD_RATE|CS8|CREAD|CLOCAL; // 9.6k, 8N1, RW, ignore line ctrl
    tty.c_cc[VMIN]  = 0;  // non-canonical mode
    tty.c_cc[VTIME] = 5;
    if(ioctl(comfd,TCSETA,&tty) < 0){
        /// "Не могу установить настройки"
        WARN(_("Can't set settings"));
        signals(-1);
    }
    DBG(" OK");
}

/**
 * Read data from TTY
 * @param buff (o) - buffer for data read
 * @param length   - buffer len
 * @return amount of bytes read
 */
size_t read_tty(char *buff, size_t length){
    ssize_t L = 0, l;
    char *ptr = buff;
    fd_set rfds;
    struct timeval tv;
    int retval;
    do{
        l = 0;
        FD_ZERO(&rfds);
        FD_SET(comfd, &rfds);
        // wait for 100ms
        tv.tv_sec = 0; tv.tv_usec = 100000;
        retval = select(comfd + 1, &rfds, NULL, NULL, &tv);
        if (retval < 1) break; // retval == 0 if there's no data, retval == 1 for EINTR
        if(FD_ISSET(comfd, &rfds)){
            if((l = read(comfd, ptr, length)) < 1){
                //return 0;
                ERR("USB disconnected!");
            }
            ptr += l; L += l;
            length -= l;
        }
    }while(l);
    return (size_t)L;
}

int write_tty(char *buff, size_t length){
    ssize_t L = write(comfd, buff, length);
    if((size_t)L != length){
        /// "Ошибка записи!"
        ERR("Write error");
    }
    return 0;
}


/**
 * Safely convert data from string to double
 *
 * @param num (o) - double number read from string
 * @param str (i) - input string
 * @return 1 if success, 0 if fails
 */
int str2double(double *num, const char *str){
    double res;
    char *endptr;
    if(!str) return 0;
    res = strtod(str, &endptr);
    if(endptr == str || *str == '\0' || *endptr != '\0'){
        /// "Неправильный формат числа double!"
        WARNX("Wrong double number format!");
        return FALSE;
    }
    if(num) *num = res; // you may run it like myatod(NULL, str) to test wether str is double number
    return TRUE;
}

//////////// logging

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


/*
 * Copyright (c) 1988, 1993, 1994, 2017
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 *  2017-10-14 Niklas Hamb?chen <mail@nh2.me>
 *  - Extracted signal names mapping from kill.c
 *
 * Copyright (C) 2014 Sami Kerola <kerolasa@iki.fi>
 * Copyright (C) 2014 Karel Zak <kzak@redhat.com>
 * Copyright (C) 2017 Niklas Hamb?chen <mail@nh2.me>
 */

// https://github.com/karelzak/util-linux/blob/master/lib/signames.c
static const struct ul_signal_name {
    const char *name;
    int val;
} ul_signames[] = {
    /* POSIX signals */
    { "HUP",	SIGHUP },	/* 1 */
    { "INT",	SIGINT },	/* 2 */
    { "QUIT",	SIGQUIT },	/* 3 */
    { "ILL",	SIGILL },	/* 4 */
#ifdef SIGTRAP
    { "TRAP",	SIGTRAP },	/* 5 */
#endif
    { "ABRT",	SIGABRT },	/* 6 */
#ifdef SIGIOT
    { "IOT",	SIGIOT },	/* 6, same as SIGABRT */
#endif
#ifdef SIGEMT
    { "EMT",	SIGEMT },	/* 7 (mips,alpha,sparc*) */
#endif
#ifdef SIGBUS
    { "BUS",	SIGBUS },	/* 7 (arm,i386,m68k,ppc), 10 (mips,alpha,sparc*) */
#endif
    { "FPE",	SIGFPE },	/* 8 */
    { "KILL",	SIGKILL },	/* 9 */
    { "USR1",	SIGUSR1 },	/* 10 (arm,i386,m68k,ppc), 30 (alpha,sparc*), 16 (mips) */
    { "SEGV",	SIGSEGV },	/* 11 */
    { "USR2",	SIGUSR2 },	/* 12 (arm,i386,m68k,ppc), 31 (alpha,sparc*), 17 (mips) */
    { "PIPE",	SIGPIPE },	/* 13 */
    { "ALRM",	SIGALRM },	/* 14 */
    { "TERM",	SIGTERM },	/* 15 */
#ifdef SIGSTKFLT
    { "STKFLT",	SIGSTKFLT },	/* 16 (arm,i386,m68k,ppc) */
#endif
    { "CHLD",	SIGCHLD },	/* 17 (arm,i386,m68k,ppc), 20 (alpha,sparc*), 18 (mips) */
#ifdef SIGCLD
    { "CLD",	SIGCLD },	/* same as SIGCHLD (mips) */
#endif
    { "CONT",	SIGCONT },	/* 18 (arm,i386,m68k,ppc), 19 (alpha,sparc*), 25 (mips) */
    { "STOP",	SIGSTOP },	/* 19 (arm,i386,m68k,ppc), 17 (alpha,sparc*), 23 (mips) */
    { "TSTP",	SIGTSTP },	/* 20 (arm,i386,m68k,ppc), 18 (alpha,sparc*), 24 (mips) */
    { "TTIN",	SIGTTIN },	/* 21 (arm,i386,m68k,ppc,alpha,sparc*), 26 (mips) */
    { "TTOU",	SIGTTOU },	/* 22 (arm,i386,m68k,ppc,alpha,sparc*), 27 (mips) */
#ifdef SIGURG
    { "URG",	SIGURG },	/* 23 (arm,i386,m68k,ppc), 16 (alpha,sparc*), 21 (mips) */
#endif
#ifdef SIGXCPU
    { "XCPU",	SIGXCPU },	/* 24 (arm,i386,m68k,ppc,alpha,sparc*), 30 (mips) */
#endif
#ifdef SIGXFSZ
    { "XFSZ",	SIGXFSZ },	/* 25 (arm,i386,m68k,ppc,alpha,sparc*), 31 (mips) */
#endif
#ifdef SIGVTALRM
    { "VTALRM",	SIGVTALRM },	/* 26 (arm,i386,m68k,ppc,alpha,sparc*), 28 (mips) */
#endif
#ifdef SIGPROF
    { "PROF",	SIGPROF },	/* 27 (arm,i386,m68k,ppc,alpha,sparc*), 29 (mips) */
#endif
#ifdef SIGWINCH
    { "WINCH",	SIGWINCH },	/* 28 (arm,i386,m68k,ppc,alpha,sparc*), 20 (mips) */
#endif
#ifdef SIGIO
    { "IO",		SIGIO },	/* 29 (arm,i386,m68k,ppc), 23 (alpha,sparc*), 22 (mips) */
#endif
#ifdef SIGPOLL
    { "POLL",	SIGPOLL },	/* same as SIGIO */
#endif
#ifdef SIGINFO
    { "INFO",	SIGINFO },	/* 29 (alpha) */
#endif
#ifdef SIGLOST
    { "LOST",	SIGLOST },	/* 29 (arm,i386,m68k,ppc,sparc*) */
#endif
#ifdef SIGPWR
    { "PWR",	SIGPWR },	/* 30 (arm,i386,m68k,ppc), 29 (alpha,sparc*), 19 (mips) */
#endif
#ifdef SIGUNUSED
    { "UNUSED",	SIGUNUSED },	/* 31 (arm,i386,m68k,ppc) */
#endif
#ifdef SIGSYS
    { "SYS",	SIGSYS },	/* 31 (mips,alpha,sparc*) */
#endif
};

const char *signum_to_signame(int signum){
    size_t n;
    static char buf[32];
    if(signum < 1) return NULL;
#if defined SIGRTMIN && defined SIGRTMAX
    if(signum >= SIGRTMIN && signum <= SIGRTMAX){
        snprintf(buf, 32, "SIGRT%d", signum - SIGRTMIN);
        return buf;
    }
#endif
    for (n = 0; n < ARRAY_SIZE(ul_signames); n++) {
        if(ul_signames[n].val == signum){
            snprintf(buf, 32, "SIG%s", ul_signames[n].name);
            return buf;
        }
    }
    return NULL;
}
