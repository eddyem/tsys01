/*                                                                                                  geany_encoding=koi8-r
 * cmdlnopts.c - the only function that parse cmdln args and returns glob parameters
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
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include "cmdlnopts.h"
#include "usefull_macros.h"

/*
 * here are global parameters initialisation
 */
int help;
glob_pars  G;

#define DEFAULT_COMDEV  "/dev/ttyUSB0"
//            DEFAULTS
// default global parameters
glob_pars const Gdefault = {
    .device = DEFAULT_COMDEV,
    .speed = 115200,
    .rewrite_ifexists = 0,
    .outpname = NULL,
    .polling_interval = 5.
};

/*
 * Define command line options by filling structure:
 *  name        has_arg     flag    val     type        argptr              help
*/
myoption cmdlnopts[] = {
// common options
    {"help",    NO_ARGS,    NULL,   'h',    arg_int,    APTR(&help),        _("show this help")},
    {"device",  NEED_ARG,   NULL,   'i',    arg_string, APTR(&G.device),    _("serial device name (default: " DEFAULT_COMDEV ")")},
    {"baudrate",NEED_ARG,   NULL,   'b',    arg_int,    APTR(&G.speed),     _("connect at given baudrate (115200 by default)")},
    {"poll-time",NEED_ARG,  NULL,   't',    arg_double, APTR(&G.polling_interval),_("pause between subsequent readings (default: 5sec)")},
    {"output",  NEED_ARG,   NULL,   'o',    arg_string, APTR(&G.outpname),  _("output file name")},
    {"rewrite", NO_ARGS,    NULL,   'r',    arg_int,    APTR(&G.rewrite_ifexists), _("rewrite existing log file")},
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
    change_helpstring("Usage: %s [args]\n\n\tWhere args are:\n");
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

