/** @file  psTableParse.c
 *
 *  @brief Parses input data tables (finals2000A.data) and outputs files for specified format
 *
 *  @ingroup psTableParse
 *
 *  @author Dave Robbins, MHPCC
 *
 *  @version $Revision: 1.1 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-09-23 03:03:14 $
 *
 *  Copyright 2005 Maui High Performance Computing Center, University of Hawaii
 */

//#include "pslib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static FILE *output = NULL;
static FILE *input = NULL;

static void printHeader(char *filename);

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("\n Insufficient arguments.  Use [-h] for help.\n");
        return 0;
    }
    if (!strncmp(argv[1], "-h", 2) ) {
        printf("\n Usage:  psTableParse inputTable.data outputTable.dat <Sections>.\n");
        printf(" i.e.    psTableParse finals.data finals.dat 1 3 4 8\n");
        printf("\n Sections:\n 1)MJD Date       2)X (Bull A)     3)Y (Bull A)");
        printf("\n 4)X (Bull B)     5)Y (Bull B)     6)dX (Bull A)");
        printf("\n 7)dY (Bull A)    8)dX (Bull B)    9)dY (Bull B)");
        printf("\n 10)UT1-UTC (Bull A)       11)UT1-UTC (Bull B)");
        printf("\n 12)Error in X (Bull A)    13)Error in Y (Bull A)");
        printf("\n 14)Error in dX (Bull A)   15)Error in dY (Bull A)    ");
        printf("\n 16)Error in UT1-UTC (Bull A)\n\n");
        return 0;
    }
    input = fopen(argv[1], "r");
    output = fopen(argv[2], "w");
    if (input == NULL) {
        printf("\nERROR.  Could not open input file.\n");
        return 0;
    }
    if (output == NULL) {
        printf("\nERROR.  Could not open or create output file.\n");
        return 0;
    }
    char data2[200];
    printHeader(argv[1]);
    fprintf(output, "#  ");
    for (int i = 3; i < argc; i++) {
        if(!strncmp(argv[i], "1", 2)) {
            fprintf(output, "MJD       ");
        }
        if(!strncmp(argv[i], "2", 2)) {
            fprintf(output, " X (A)     ");
        }
        if(!strncmp(argv[i], "3", 2)) {
            fprintf(output, " Y (A)     ");
        }
        if(!strncmp(argv[i], "4", 2)) {
            fprintf(output, " X (B)     ");
        }
        if(!strncmp(argv[i], "5", 2)) {
            fprintf(output, "  Y (B)     ");
        }
        if(!strncmp(argv[i], "6", 2)) {
            fprintf(output, " dX (A)     ");
        }
        if(!strncmp(argv[i], "7", 2)) {
            fprintf(output, " dY (A)     ");
        }
        if(!strncmp(argv[i], "8", 2)) {
            fprintf(output, " dX (B)     ");
        }
        if(!strncmp(argv[i], "9", 2)) {
            fprintf(output, " dY (B)     ");
        }
        if(!strncmp(argv[i], "10", 2)) {
            fprintf(output, "UT1-UTC (A)  ");
        }
        if(!strncmp(argv[i], "11", 2)) {
            fprintf(output, "UT1-UTC (B)  ");
        }
        if(!strncmp(argv[i], "12", 2)) {
            fprintf(output, " xErr (A)  ");
        }
        if(!strncmp(argv[i], "13", 2)) {
            fprintf(output, " yErr (A)  ");
        }
        if(!strncmp(argv[i], "14", 2)) {
            fprintf(output, " dXErr (A) ");
        }
        if(!strncmp(argv[i], "15", 2)) {
            fprintf(output, " dYErr (A) ");
        }
        if(!strncmp(argv[i], "16", 2)) {
            fprintf(output, "UT1-UTC Err");
        }
    }
    fprintf(output, "\n#\n#\n");
    int i;
    while ( fscanf(input, "%188c", data2) != EOF) {

        for(int j = 3; j < argc; j++) {
            if(!strncmp(argv[j], "1", 2) ) {
                for (i = 7; i <= 14; i++) {
                    fprintf(output, "%c", data2[i]);
                }
            }
            if(!strncmp(argv[j], "2", 2) ) {
                if (data2[23] == ' ') {
                    fprintf(output, "   0.000 ");
                } else {
                    for (i = 18; i <= 26; i++) {
                        if (i == 18 && data2[i+1] == '-' && data2[i+2] == '.') {
                            fprintf(output, "-0.");
                            i = 21;
                        }
                        if (i == 19) {
                            if (data2[i] == ' ' && data2[i+1] == '.') {
                                fprintf(output, "0");
                                i++;
                            }
                        }
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "3", 2) ) {
                if (data2[42] == ' ') {
                    fprintf(output, "   0.000 ");
                } else {
                    for (i = 37; i <= 45; i++) {
                        if (i == 37 && data2[i+1] == '-' && data2[i+2] == '.') {
                            fprintf(output, "-0.");
                            i = 40;
                        }
                        if (i == 38) {
                            if (data2[i] == ' ' && data2[i+1] == '.') {
                                fprintf(output, "0");
                                i++;
                            }
                        }
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "4", 2) ) {
                if (data2[139] == ' ') {
                    fprintf(output, "    0.000 ");
                    //                    i = 144;
                } else {
                    for (i = 134; i <= 143; i++) {
                        if (i == 135 && data2[i+1] == '-' && data2[i+2] == '.') {
                            fprintf(output, "-0.");
                            i = 138;
                        }
                        if (i == 136) {
                            if (data2[i] == ' ' && data2[i+1] == '.') {
                                fprintf(output, "0");
                                i++;
                            }
                        }
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "5", 2) ) {
                if (data2[149] == ' ') {
                    fprintf(output, "    0.000 ");
                } else {
                    for (i = 144; i <= 153; i++) {
                        if (i == 145 && data2[i+1] == '-' && data2[i+2] == '.') {
                            fprintf(output, "-0.");
                            i = 148;
                        }
                        if (i == 146) {
                            if (data2[i] == ' ' && data2[i+1] == '.') {
                                fprintf(output, "0");
                                i++;
                            }
                        }
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "6", 2) ) {
                if (data2[102] == ' ') {
                    fprintf(output, "   0.000 ");
                } else {
                    for (i = 97; i <= 105; i++) {
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "7", 2) ) {
                if (data2[121] == ' ') {
                    fprintf(output, "   0.000 ");
                } else {
                    for (i = 116; i <= 124; i++) {
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "8", 2) ) {
                if (data2[170] == ' ') {
                    fprintf(output, "    0.000 ");
                } else {
                    for (i = 165; i <= 174; i++) {
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "9", 2) ) {
                if (data2[180] == ' ') {
                    fprintf(output, "    0.000 ");
                } else {
                    for (i = 175; i <= 184; i++) {
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "10", 2) ) {
                if (data2[63] == ' ') {
                    fprintf(output, "    0.000 ");
                } else {
                    for (i = 58; i <= 67; i++) {
                        if (i == 58 && data2[i+1] == '-' && data2[i+2] == '.') {
                            fprintf(output, "-0.");
                            i = 61;
                        }
                        if (i == 59) {
                            if (data2[i] == ' ' && data2[i+1] == '.') {
                                fprintf(output, "0");
                                i++;
                            }
                        }
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "11", 2) ) {
                if (data2[159] == ' ') {
                    fprintf(output, "     0.000 ");
                } else {
                    for (i = 154; i <= 164; i++) {
                        if (i == 155 && data2[i+1] == '-' && data2[i+2] == '.') {
                            fprintf(output, "-0.");
                            i = 158;
                        }
                        if (i == 156) {
                            if (data2[i] == ' ' && data2[i+1] == '.') {
                                fprintf(output, "0");
                                i++;
                            }
                        }
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "12", 2) ) {
                if (data2[32] == ' ') {
                    fprintf(output, "    0.000 ");
                } else {
                    for (i = 27; i <= 35; i++) {
                        if (i == 27 && data2[i+1] == '-' && data2[i+2] == '.') {
                            fprintf(output, "-0.");
                            i = 30;
                        }
                        if (i == 28) {
                            if (data2[i] == ' ' && data2[i+1] == '.') {
                                fprintf(output, "0");
                                i++;
                            }
                        }
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "13", 2) ) {
                if (data2[51] == ' ') {
                    fprintf(output, "     0.000 ");
                } else {
                    for (i = 46; i <= 54; i++) {
                        if (i == 46 && data2[i+1] == '-' && data2[i+2] == '.') {
                            fprintf(output, "-0.");
                            i = 49;
                        }
                        if (i == 47) {
                            if (data2[i] == ' ' && data2[i+1] == '.') {
                                fprintf(output, "0");
                                i++;
                            }
                        }
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "14", 2) ) {
                if (data2[111] == ' ') {
                    fprintf(output, "    0.000 ");
                } else {
                    for (i = 106; i <= 114; i++) {
                        if (i == 106 && data2[i+1] == '-' && data2[i+2] == '.') {
                            fprintf(output, "-0.");
                            i = 109;
                        }
                        if (i == 107) {
                            if (data2[i] == ' ' && data2[i+1] == '.') {
                                fprintf(output, "0");
                                i++;
                            }
                        }
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "15", 2) ) {
                if (data2[131] == ' ') {
                    fprintf(output, "     0.000 ");
                } else {
                    for (i = 125; i <= 133; i++) {
                        if (i == 125 && data2[i+1] == '-' && data2[i+2] == '.') {
                            fprintf(output, "-0.");
                            i = 128;
                        }
                        if (i == 126) {
                            if (data2[i] == ' ' && data2[i+1] == '.') {
                                fprintf(output, "0");
                                i++;
                            }
                        }
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            if(!strncmp(argv[j], "16", 2) ) {
                if (data2[73] == ' ') {
                    fprintf(output, "     0.000 ");
                } else {
                    for (i = 68; i <= 77; i++) {
                        if (i == 155 && data2[i+1] == '-' && data2[i+2] == '.') {
                            fprintf(output, "-0.");
                            i = 71;
                        }
                        if (i == 69) {
                            if (data2[i] == ' ' && data2[i+1] == '.') {
                                fprintf(output, "0");
                                i++;
                            }
                        }
                        fprintf(output, "%c", data2[i]);
                    }
                }
            }
            fprintf(output, "  ");
        }
        fprintf(output, "\n");
    }

    fclose(input);
    fclose(output);
    return 0;
}

void printHeader(char *filename)
{
    if (output == NULL) {
        printf("\n Unable to access output file.  Output is NULL.\n");
    } else {
        fprintf(output, "#\n#  Stripped version of %s file\n#\n#\n", filename);
    }
}

