/*                                                                                                  geany_encoding=koi8-r
 * cmdlnopts.c - the only function that parse cmdln args and returns glob parameters
 *
 * Copyright 2018 Edward V. Emelianoff <eddy@sao.ru>
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
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <usefull_macros.h>

#include "cmdlnopts.h"
#include "term.h"

/*
 * here are global parameters initialisation
 */
int help;
static glob_pars  G;

// default values for Gdefault & help
#define DEFAULT_SOCKPATH    "\0canbus"
#define DEFAULT_PORT        "4444"
#define DEFAULT_PIDFILE     "/tmp/TSYS01daemon.pid"

//            DEFAULTS
// default global parameters
glob_pars const Gdefault = {
    .sockname = DEFAULT_SOCKPATH,
    .port = DEFAULT_PORT,
    .adjfilename = "tempadj.txt",
    .pidfilename = DEFAULT_PIDFILE
};

/*
 * Define command line options by filling structure:
 *  name        has_arg     flag    val     type        argptr              help
*/
static myoption cmdlnopts[] = {
// common options
    {"help",    NO_ARGS,    NULL,   'h',    arg_int,    APTR(&help),        _("show this help")},
    {"name",    NEED_ARG,   NULL,   'n',    arg_string, APTR(&G.sockname),  _("server's UNIX socket name (default: \\0canbus)")},
    {"port",    NEED_ARG,   NULL,   'p',    arg_string, APTR(&G.port),      _("network port to connect (default: " DEFAULT_PORT ")")},
    //{"terminal",NO_ARGS,    NULL,   't',    arg_int,    APTR(&G.terminal),  _("run as terminal")},
    {"savepath",NEED_ARG,   NULL,   's',    arg_string, APTR(&G.savepath),  _("path where files would be saved (if none - don't save)")},
    {"graphplot",NO_ARGS,   NULL,   'g',    arg_int,    APTR(&G.makegraphs),_("make graphics with gnuplot")},
    {"testadjfile",NO_ARGS, NULL,   'T',    arg_int,    APTR(&G.testadjfile),_("test format of file with T adjustements")},
    {"adjname", NEED_ARG,   NULL,   'N',    arg_string, APTR(&G.adjfilename),_("name of adjustements file (default: tempadj.txt)")},
    {"pidfile", NEED_ARG,   NULL,   'P',    arg_string, APTR(&G.pidfilename),_("name of PID file (default: " DEFAULT_PIDFILE ")")},
    //{"dumpoff", NO_ARGS,    NULL,   'd',    arg_string, APTR(&G.dumpoff),   _("dump sensors data & turn all OFF until next request")},
   end_option
};

/**
 * Parse command line options and return dynamically allocated structure
 *      to global parameters
 * @param argc - copy of argc from main
 * @param argv - copy of argv from main
 * @return allocated structure with global parameters
 */
glob_pars *parse_args(int argc, char **argv){
    int i;
    void *ptr;
    ptr = memcpy(&G, &Gdefault, sizeof(G)); assert(ptr);
    // format of help: "Usage: progname [args]\n"
    change_helpstring("Usage: %s [args] [logfile name]\n\n\tWhere args are:\n");
    // parse arguments
    parseargs(&argc, &argv, cmdlnopts);
    if(help) showhelp(-1, cmdlnopts);
    if(argc > 0){
        G.rest_pars_num = argc;
        G.rest_pars = calloc(argc, sizeof(char*));
        for (i = 0; i < argc; i++)
            G.rest_pars[i] = strdup(argv[i]);
    }
    return &G;
}

