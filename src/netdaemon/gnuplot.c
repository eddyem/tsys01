/*
 *                                                                                                  geany_encoding=koi8-r
 * gnuplot.c
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
#include <stdio.h>   // file operations
#include <unistd.h>  // access() to check file exists
#include <linux/limits.h> // PATH_MAX
#include "usefull_macros.h" // putlog
#include "cmdlnopts.h"   // glob_pars

extern const int SensCoords[8][8][2];
extern glob_pars *G;

static char fullpath[PATH_MAX];

// create full name from path and file
char *mkname(char *path, char *fname){
    if(path[strlen(path)-1] == '/') snprintf(fullpath, PATH_MAX, "%s%s", path, fname);
    else snprintf(fullpath, PATH_MAX, "%s/%s", path, fname);
    DBG("fullpath: %s", fullpath);
    return fullpath;
}

/**
 * form files for gnuplot
 * @param fname (i) - filename with full path
 * @param data  (i) - thermal data array
 * @return 1 if all OK
 */
static int formfile(char *fname, double data[8][8]){
    FILE *F = fopen(fname, "w");
    if(!F) return 0;
    int i, N;
    for(i = 1; i < 8; ++i){
        for(N = 0; N < 8; ++ N){
            double T = data[N][i];
            if(T > -100. && T < 100.){
                fprintf(F, "%d\t%d\t%.2f\n", SensCoords[N][i][0],
                                    SensCoords[N][i][1], T);
            }
        }
    }
    fclose(F);
    DBG("File %s ready", fname);
    return 1;
}

/**
 * plot drawings with gnuplot
 * @param path  (i) - path to directory with data & scripts
 * @param fname (i) - name of file with data
 */
static void gnuplot(char *path, char *fname){
    char *ctmp = mkname(path, "plot");
    char buf[PATH_MAX*2];
    size_t L = PATH_MAX*2;
    if(access(ctmp, F_OK)){
        WARNX(_("Don't find %s to plot graphics"), ctmp);
        putlog("Don't find %s to plot graphics", ctmp);
        return;
    }
    ssize_t l = snprintf(buf, L, "%s ", ctmp);
    if(l < 1) return;
    ctmp = mkname(path, fname);
    snprintf(buf+l, L, "%s", ctmp);
    DBG("Run %s", buf);
    if(system(buf)){
        WARNX(_("Can't run `%s`"), buf);
        putlog("Can't run `%s`", buf);
    }
}

void plot(double data[2][8][8], char *savepath){
    double dY[8][8]; // vertical gradients (top - bottom)
    int i, N;
    // calculate gradients
    for(i = 1; i < 8; ++i){
        for(N = 0; N < 8; ++ N){
            double Ttop = data[0][N][i], Tbot = data[1][N][i];
            if(Ttop > -100. && Ttop < 100. && Tbot > -100. && Tbot < 100.){
                double dT = Ttop - Tbot;
                if(dT > -2. && dT < 2.) dY[N][i] = dT;
                else dY[N][i] = -300.;
            }else dY[N][i] = -300.;
        }
    }
    char *ctmp = mkname(savepath, "T0");
    if(formfile(ctmp, data[0])) if(G->makegraphs) gnuplot(savepath, "T0");
    ctmp = mkname(savepath, "T1");
    if(formfile(ctmp, data[1])) if(G->makegraphs) gnuplot(savepath, "T1");
    ctmp = mkname(savepath, "Tgrad");
    if(formfile(ctmp, dY)) if(G->makegraphs) gnuplot(savepath, "Tgrad");
}
