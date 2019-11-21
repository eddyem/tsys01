/*
 * This file is part of the TSYS01_netdaemon project.
 * Copyright 2019 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sens_place.h"
#include "stdbool.h"
#include "usefull_macros.h"
#include "math.h" // fabs

/**
   Sensor   place           Dt           X          Y          Z
      100    36           -0.07         19          7          0
      101    19            0.03         20          0          0
      110    20            0.03         19         -7          0
      111   20-21         -0.05         17        -10          1
      120    40            0.02         17        -22          0
      121    21           -0.03         15        -13          0
      130    39            0.02         22        -17          0
      131   38-39         -0.09         24        -14          1
      140    38            0.03         25        -10          0
      141    37            0.02         27         -4          0
      150    60           -0.01         27          4          0
      151    59           -0.09         25         10          0
      160    58            0.07         22         17          0
      161   58-59             0         24         14          1
      170    56            0.02         10         25          0
      171    57            0.08         17         22          0
      200    55           -0.01          4         27          0
      201   54-55         -0.03          0         27          1
      210    54            0.04         -4         27          0
      211    53           -0.01        -10         25          0
      220    52           -0.02        -17         22          0
      221    51           -0.04        -22         17          0
      230    50            0.03        -25         10          0
      231   50-51         -0.03        -24         14          1
      240    48           -0.03        -27         -4          0
      241    49            0.03        -27          4          0
      250    47           -0.05        -25        -10          0
      251   46-47             0        -24        -14          1
      260    46            0.03        -22        -17          0
      261    45           -0.05        -17        -22          0
      270    44           -0.02        -10        -25          0
      271    43            0.04         -4        -27          0
      300    24            0.12         -3        -20          0
      301    25               0        -10        -17          0
      310    26           -0.08        -15        -13          0
      311   26-27         -0.08        -17        -10          1
      320    27            -0.1        -19         -7          0
      321    28           -0.04        -20          0          0
      330    29           -0.08        -19          7          0
      331   29-30          0.03        -17         10          1
      340    30            0.11        -15         13          0
      341    31            0.07        -10         17          0
      350    32            0.09         -3         20          0
      351   32-33         -0.09          0         20          1
      360    33            -0.1          3         20          0
      361    34            0.04         10         17          0
      370    35            0.05         15         13          0
      371   35-36         -0.04         17         10          1
      400    17            0.01          9          9          0
      401   17-18         -0.04         11          7          1
      410    16            0.01          3         13          0
      411   15-16         -0.04          0         13          1
      420    14            0.14         -9          9          0
      421    15               0         -3         13          0
      430    13            0.07        -13          3          0
      431   13-14         -0.02        -11          7          1
      440    12            0.04        -13         -3          0
      441   11-12         -0.06        -11         -7          1
      450    11            0.01         -9         -9          0
      451    10            0.08         -3        -13          0
      460     9           -0.09          3        -13          0
      461   9-10          -0.04          0        -13          1
      470    23            0.06          3        -20          0
      471   23-24         -0.05          0        -20          1
      500    42            0.05          4        -27          0
      501   42-43         -0.08          0        -27          1
      510    22             0.1         10        -17          0
      511    41           -0.05         10        -25          0
      520     8               0          9         -9          0
      521    7-8           0.01         11         -7          1
      530     2               0          3         -5          0
      531    2-3          -0.02          0         -6          1
      540     3            0.05         -3         -5          0
      541     4           -0.03         -6          0          0
      550     5            0.03         -3          5          0
      551    5-6           0.06          0          6          1
      560     6               0          3          5          0
      561     1           -0.06          6          0          0
      570     7           -0.07         13         -3          0
      571    18            0.02         13          3          0
 */

static sensor_data sensors[NSENSORS] = {
//  {Dt,X,Y,Z},
    {-0.07, 19, 7, 0, 0}, // 0
    {0.03, 20, 0, 0, 0}, // 1
    {0.03, 19, -7, 0, 0}, // 2
    {-0.05, 17, -10, 1, 0}, // 3
    {0.02, 17, -22, 0, 0}, // 4
    {-0.03, 15, -13, 0, 0}, // 5
    {0.02, 22, -17, 0, 0}, // 6
    {-0.09, 24, -14, 1, 0}, // 7
    {0.03, 25, -10, 0, 0}, // 8
    {0.02, 27, -4, 0, 0}, // 9
    {-0.01, 27, 4, 0, 0}, // 10
    {-0.09, 25, 10, 0, 0}, // 11
    {0.07, 22, 17, 0, 0}, // 12
    {0, 24, 14, 1, 0}, // 13
    {0.02, 10, 25, 0, 0}, // 14
    {0.08, 17, 22, 0, 0}, // 15
    {-0.01, 4, 27, 0, 0}, // 16
    {-0.03, 0, 27, 1, 0}, // 17
    {0.04, -4, 27, 0, 0}, // 18
    {-0.01, -10, 25, 0, 0}, // 19
    {-0.02, -17, 22, 0, 0}, // 20
    {-0.04, -22, 17, 0, 0}, // 21
    {0.03, -25, 10, 0, 0}, // 22
    {-0.03, -24, 14, 1, 0}, // 23
    {-0.03, -27, -4, 0, 0}, // 24
    {0.03, -27, 4, 0, 0}, // 25
    {-0.05, -25, -10, 0, 0}, // 26
    {0, -24, -14, 1, 0}, // 27
    {0.03, -22, -17, 0, 0}, // 28
    {-0.05, -17, -22, 0, 0}, // 29
    {-0.02, -10, -25, 0, 0}, // 30
    {0.04, -4, -27, 0, 0}, // 31
    {0.12, -3, -20, 0, 0}, // 32
    {0, -10, -17, 0, 0}, // 33
    {-0.08, -15, -13, 0, 0}, // 34
    {-0.08, -17, -10, 1, 0}, // 35
    {-0.1, -19, -7, 0, 0}, // 36
    {-0.04, -20, 0, 0, 0}, // 37
    {-0.08, -19, 7, 0, 0}, // 38
    {0.03, -17, 10, 1, 0}, // 39
    {0.11, -15, 13, 0, 0}, // 40
    {0.07, -10, 17, 0, 0}, // 41
    {0.09, -3, 20, 0, 0}, // 42
    {-0.09, 0, 20, 1, 0}, // 43
    {-0.1, 3, 20, 0, 0}, // 44
    {0.04, 10, 17, 0, 0}, // 45
    {0.05, 15, 13, 0, 0}, // 46
    {-0.04, 17, 10, 1, 0}, // 47
    {0.01, 9, 9, 0, 0}, // 48
    {-0.04, 11, 7, 1, 0}, // 49
    {0.01, 3, 13, 0, 0}, // 50
    {-0.04, 0, 13, 1, 0}, // 51
    {0.14, -9, 9, 0, 0}, // 52
    {0, -3, 13, 0, 0}, // 53
    {0.07, -13, 3, 0, 0}, // 54
    {-0.02, -11, 7, 1, 0}, // 55
    {0.04, -13, -3, 0, 0}, // 56
    {-0.06, -11, -7, 1, 0}, // 57
    {0.01, -9, -9, 0, 0}, // 58
    {0.08, -3, -13, 0, 0}, // 59
    {-0.09, 3, -13, 0, 0}, // 60
    {-0.04, 0, -13, 1, 0}, // 61
    {0.06, 3, -20, 0, 0}, // 62
    {-0.05, 0, -20, 1, 0}, // 63
    {0.05, 4, -27, 0, 0}, // 64
    {-0.08, 0, -27, 1, 0}, // 65
    {0.1, 10, -17, 0, 0}, // 66
    {-0.05, 10, -25, 0, 0}, // 67
    {0, 9, -9, 0, 0}, // 68
    {0.01, 11, -7, 1, 0}, // 69
    {0, 3, -5, 0, 0}, // 70
    {-0.02, 0, -6, 1, 0}, // 71
    {0.05, -3, -5, 0, 0}, // 72
    {-0.03, -6, 0, 0, 0}, // 73
    {0.03, -3, 5, 0, 0}, // 74
    {0.06, 0, 6, 1, 0}, // 75
    {0, 3, 5, 0, 0}, // 76
    {-0.06, 6, 0, 0, 0}, // 77
    {-0.07, 13, -3, 0, 0}, // 78
    {0.02, 13, 3, 0, 0}, // 79
};

/**
 * @brief get_sensor_location - search given sensor in `sensors` table
 * @param Nct - controller number
 * @param Nch - channel number
 * @param Ns  - sensor number
 * @return pointer to sensor_data for given sensor
 */
const sensor_data *get_sensor_location(int Nct, int Nch, int Ns){
    if(Nct < 1 || Nct > NCTRLR_MAX || Nch > NCHANNEL_MAX || Ns > 1){
        WARNX("Wrong sencor code: %s (%d%d%d)\n", Nct, Nct, Nch, Ns);
        return NULL;
    }
    int idx = 2*(NCHANNEL_MAX+1)*(Nct - 1) + 2*Nch + Ns;
    //DBG("Sensor code %d%d%d (idx=%d):\n", Nct, Nch, Ns, idx);
    const sensor_data *s = sensors + idx;
    //DBG("\tdT=%g (adj:%g); coords=(%d, %d, %d)\n", s->dt, s->Tadj, s->X, s->Y, s->Z);
    return s;
}

// return next non-space character in line until first  '\n' or NULL if met '#' or '\n'
static char *omitspaces(char *str){
    if(!str) return NULL;
    char ch;
    do{
        ch = *str;
        if(!ch) return NULL; // EOL
        if(ch == ' ' || ch == '\t'){
            ++str;
            continue;
        }
        if(ch == '#' || ch == '\n'){
            return NULL; // comment
        }
        return str;
    }while(1);
    return NULL;
}

/**
 * @brief read_adj_file - try to read file with thermal adjustments
 * @param fname - filename
 * @return 0 if all OK
 * thermal adjustments file should have simple structure:
 * No of sensor (like in output format, e.g. 561) and Tcorr value (Treal = T - Tcorr)
 * all data after # ignored
 */
int read_adj_file(char *fname){
    double Tadj[NSENSORS] = {0,};
    if(!fname){
        WARNX("read_adj_file(): filename missing");
        return 1;
    }
    mmapbuf *buf = My_mmap(fname);
    if(!buf) return 1;
    char *adjf = buf->data, *eof = adjf + buf->len;
    int strnum = 1; // start string number from 1
    while(adjf < eof){
        char *eol = strchr(adjf, '\n');
        char *nextchar = omitspaces(adjf);
        if(!nextchar){
            goto cont;
        }
        char *endptr = NULL;
        long num = strtol(nextchar, &endptr, 10);
        if(endptr == nextchar || !endptr){
            WARNX("Wrong integer number!");
            goto reperr;
        }
        int Nctrl = num / 100;
        int Nch = (num - Nctrl*100) / 10;
        int Nsen = num%10;
        if(num < 0 || (Nsen != 0 && Nsen != 1) || (Nch < 0 || Nch > NCHANNEL_MAX) || (Nctrl < 1 || Nctrl > NCTRLR_MAX)){
            WARNX("Wrong sensor number: %ld", num);
            goto reperr;
        }
        int idx = 2*(NCHANNEL_MAX+1)*(Nctrl - 1) + 2*Nch + Nsen;
        if(idx < 0 || idx > NSENSORS-1){
            WARNX("Sensor index (%d) over range 0..%d", idx, NSENSORS-1);
        }
        nextchar = omitspaces(endptr);
        if(!nextchar){
            WARNX("There's no temperature data after sensor's number");
            goto reperr;
        }
        double t = strtod(nextchar, &endptr);
        if(endptr == nextchar || !endptr){
            WARNX("Wrong double number!");
            goto reperr;
        }
        Tadj[idx] = t;
        if(omitspaces(endptr)){
            WARNX("Wrong file format: each string should include two numbers (and maybe comment after #)");
            goto reperr;
        }
cont:
        if(!eol) break;
        adjf = eol + 1;
        ++strnum;
    }
    My_munmap(buf);
    printf("Non-zero components:\n");
    for(int i = 0; i < NSENSORS; ++i){
        if(fabs(Tadj[i]) > DBL_EPSILON){
            printf("\tTadj[%02d] = %g\n", i, Tadj[i]);
        }
        if(fabs(sensors[i].Tadj - Tadj[i]) > 0.001) putlog("Tadj[%d] = %g", i, Tadj[i]);
        sensors[i].Tadj = Tadj[i];
    }
    return 0;
reperr:
    red("Error in string %d:\n", strnum);
    printf("%s\n", adjf);
    putlog("Erroneous log file %s in line %d", fname, strnum);
    return 1;
}
