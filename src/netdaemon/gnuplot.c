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
#include <time.h>
#include <unistd.h>  // access() to check file exists
#include <linux/limits.h> // PATH_MAX
#include "usefull_macros.h"
#include "cmdlnopts.h"   // glob_pars

extern glob_pars *G;

static char fullpath[PATH_MAX];

// create full name from path and file
char *mkname(char *path, char *fname){
    if(!path) return fname;
    if(path[strlen(path)-1] == '/') snprintf(fullpath, PATH_MAX, "%s%s", path, fname);
    else snprintf(fullpath, PATH_MAX, "%s/%s", path, fname);
    DBG("fullpath: %s", fullpath);
    return fullpath;
}

/**
 * form files for gnuplot
 * @param fname (i) - filename with full path
 * @param data  (i) - thermal data array
 * @param Z     (i) - 0 for upper and 1 for lower sensors (Z-coord)
 * @return 1 if all OK
 */
static int formfile(char *fname, double data[2][NCHANNEL_MAX+1][NCTRLR_MAX+1], int Z){
    FILE *F = fopen(fname, "w");
    if(!F) return 0;
    int nonzero_values = 0;
    for(int i = 1; i <= NCTRLR_MAX; ++i){
        for(int N = 0; N <= NCHANNEL_MAX; ++ N) for(int p = 0; p < 2; ++p){
            double T = data[p][N][i];
            if(T > -100. && T < 100.){
                const sensor_data *sdata = get_sensor_location(i, N, p);
                if(!sdata) continue;
                if(Z != sdata->Z) continue;
                fprintf(F, "%d\t%d\t%.2f\n", sdata->X, sdata->Y, T - sdata->dt - sdata->Tadj);
                ++nonzero_values;
            }
        }
    }
    fclose(F);
    if(!nonzero_values) return 0;
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
    char buf[PATH_MAX*2], *ptr = buf;
    size_t L = PATH_MAX*2;
    if(access(ctmp, F_OK)){
        WARNX(_("Don't find %s to plot graphics"), ctmp);
        return;
    }
    ssize_t l = snprintf(ptr, L, "%s ", ctmp);
    if(l < 1) return;
    L -= l; ptr += l;
    ctmp = mkname(path, fname);
    time_t t = time(NULL);
    struct tm *timeptr = localtime(&t);
    l = snprintf(ptr, L, "%s ", ctmp);
    L -= l; ptr += l;
    strftime(ptr, L, "\"%d/%m/%y %H:%M:%S\"", timeptr);
    DBG("Run %s", buf);
    if(system(buf)){
        WARNX(_("Can't run `%s`"), buf);
    }else LOG("created chart %s", fname);
}

void plot(double data[2][NCHANNEL_MAX+1][NCTRLR_MAX+1], char *savepath){
    char *ctmp = mkname(savepath, "T0");
    if(formfile(ctmp, data, 0)) if(G->makegraphs) gnuplot(savepath, "T0");
    ctmp = mkname(savepath, "T1");
    if(formfile(ctmp, data, 1)) if(G->makegraphs) gnuplot(savepath, "T1");
}
