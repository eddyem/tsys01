/*                                                                                                  geany_encoding=koi8-r
 * main.c
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
 */
#include <signal.h>
#include <sys/wait.h> // wait
#include <sys/prctl.h> //prctl
#include "checkfile.h"
#include "cmdlnopts.h"
#include "socket.h"
#include "usefull_macros.h"

glob_pars *G; // non-static: some another files should know about it!
static pid_t childpid = 0; // PID of child - to kill it if need

// default signals handler
void signals(int signo){
    restore_console();
    restore_tty();
    putlog("exit with status %d", signo);
    exit(signo);
}

// SIGUSR1 handler - re-read Tadj file
void refreshAdj(_U_ int signo){
    DBG("refresh adj");
    if(childpid){ // I am a master
        putlog("Force child %d to re-read adj-file", childpid);
        kill(childpid, SIGUSR1);
    }else{ // I am a child
        putlog("Re-read adj-file");
        read_adj_file(G->adjfilename);
    }
}

int main(int argc, char **argv){
    initial_setup();
    G = parse_args(argc, argv);
    if(G->rest_pars_num)
        openlogfile(G->rest_pars[0]);
    if(G->makegraphs && !G->savepath){
        ERRX(_("Point the path to graphical files"));
    }
    DBG("t=%d", G->testadjfile);
    int raf = read_adj_file(G->adjfilename);
    pid_t runningproc = check4running(G->pidfilename);
    if(G->testadjfile){
        if(raf == 0){
            green("Format of file %s is right\n", G->adjfilename);
            if(runningproc){ // fore running process to re-read it
                kill(runningproc, SIGUSR1);
            }
            return 0;
        }
        return 1;
    }
    if(runningproc) ERRX("Found running process, pid=%d.", runningproc);
    signal(SIGTERM, signals); // kill (-15) - quit
    signal(SIGHUP, SIG_IGN);  // hup - ignore
    signal(SIGINT, signals);  // ctrl+C - quit
    signal(SIGQUIT, signals); // ctrl+\ - quit
    signal(SIGTSTP, SIG_IGN); // ignore ctrl+Z
    signal(SIGUSR1, refreshAdj); // refresh adjustements
    #ifndef EBUG
    if(!G->terminal){
        if(daemon(1, 0)){
            ERR("daemon()");
        }
        while(1){ // guard for dead processes
            childpid = fork();
            if(childpid){
                putlog("create child with PID %d\n", childpid);
                DBG("Created child with PID %d\n", childpid);
                wait(NULL);
                putlog("child %d died\n", childpid);
                WARNX("Child %d died\n", childpid);
                sleep(1);
            }else{
                prctl(PR_SET_PDEATHSIG, SIGTERM); // send SIGTERM to child when parent dies
                break; // go out to normal functional
            }
        }
    }
    #endif
    DBG("dev: %s", G->device);
    try_connect(G->device);
    if(check_sensors()){
        putlog("No CAN-controllers detected");
        if(!poll_sensors(0)){ // there's not main controller connected to given terminal
            putlog("Opened device is not main controller");
            if(!G->terminal) signals(15);
        }
    }
    if(G->terminal) run_terminal();
    else daemonize(G->port);
    return 0;
}
