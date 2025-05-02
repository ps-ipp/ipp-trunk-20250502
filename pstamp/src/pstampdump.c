// pstampdump  - read a fits file  containing a postage stamp request or response table and
//               print the contents to stdout
//
//              Actually this should work fine for dumping the rows of any of any kind of fits table
//              The -header option is pstamp table specific

#include <pslib.h>
#include <psmodules.h>
#include <string.h>

static bool readFitsFile(psString fileName, psMetadata **pHeader, psArray **pTable)
{
    psFits *fits = psFitsOpen(fileName, "r");
    if (fits == NULL) {
        psError(PS_ERR_IO, false, "failed to open %s for output", fileName);
        return false;
    }
    if (!psFitsMoveExtNum(fits, 1, true)) {
        psError(PS_ERR_IO, false, "failed to move to first extension from %s", fileName);
        return false;
    }

    if (pHeader) {
        *pHeader = psFitsReadHeader(NULL, fits);
        if (!*pHeader) {
            psError(PS_ERR_IO, false, "failed to header from %s", fileName);
            return false;
        }
    }

    if (pTable) {
        *pTable = psFitsReadTable(fits);
        if (*pTable == NULL) {
            psError(PS_ERR_IO, false, "psFitsReadTable failed for %s", fileName);
            return false;
        }
    }

    psFitsClose(fits);

    return true;
}

static void
usage(char *program_name)
{
    fprintf(stderr, "usage: %s [-header] [-simple] filename\n", program_name);
    exit(1);
}

int main(int argc, char *argv[])
{
    bool dumpHeader = false;
    bool simple = false;
    bool dumpTable = true;
    int argnum;
    if ((argnum = psArgumentGet(argc, argv, "-header"))) {
        dumpHeader = true;
        psArgumentRemove(argnum, &argc, argv);
    }
    if ((argnum = psArgumentGet(argc, argv, "-simple"))) {
        simple = true;
        psArgumentRemove(argnum, &argc, argv);
    }
    if ((argnum = psArgumentGet(argc, argv, "-headeronly"))) {
        dumpTable = false;
        dumpHeader = true;
        psArgumentRemove(argnum, &argc, argv);
    }

    if (argc != 2) {
        usage(argv[0]);
    }

    psString fileName = argv[1];

    psMetadata *header;
    psArray *array;
    if (!readFitsFile(fileName, dumpHeader ? &header : NULL, dumpTable ? &array : NULL)) {
        psErrorStackPrint(stderr, "failed to process fits table from: %s\n", fileName);
        return PS_EXIT_DATA_ERROR;
    }
    if (dumpHeader) {
        psString extname = psMetadataLookupStr(NULL, header, "EXTNAME");
        if (!extname) {
            psErrorStackPrint(stderr, "failed to find EXTNAME in fits header of: %s\n", fileName);
            return PS_EXIT_DATA_ERROR;
        }
        psString req_name = psMetadataLookupStr(NULL, header, "REQ_NAME");
        if (!req_name) {
            psErrorStackPrint(stderr, "failed to find REQ_NAME in fits header of: %s\n", fileName);
            return PS_EXIT_DATA_ERROR;
        }
        if (!strcmp(extname, "PS1_PS_REQUEST")) {
            psString extver = psMetadataLookupStr(NULL, header, "EXTVER");
            psString action = psMetadataLookupStr(NULL, header, "ACTION");
            psString email = psMetadataLookupStr(NULL, header, "EMAIL");
            if (!extver) {
                // work around bug in MOPS request files
                // Accept an integer for the version number
                psS32 extver_num = psMetadataLookupS32(NULL, header, "EXTVER");
                if (extver_num) {
                    psStringAppend(&extver, "%d", extver_num);
                } else {
                    psErrorStackPrint(stderr, "failed to find EXTVER in fits header of: %s\n", fileName);
                    return PS_EXIT_DATA_ERROR;
                }
            }
            if (!strcmp(extver, "1")) {
                printf("%s %s %s\n", extname, extver, req_name);
            } else {
                if (!action) {
                    psErrorStackPrint(stderr, "failed to find action in fits header of: %s\n", fileName);
                    return PS_EXIT_DATA_ERROR;
                }
                if (!email) {
                    psErrorStackPrint(stderr, "failed to find action in fits header of: %s\n", "NULL");
                    return PS_EXIT_DATA_ERROR;
                }
                printf("%s %s %s %s %s\n", extname, extver, req_name, action, email);
            }
        } else if (!strcmp(extname, "PS1_PS_RESULTS")) {
            psS64 req_id = psMetadataLookupS64(NULL, header, "REQ_ID");
            printf("%s %s %" PRId64 "\n", extname, req_name, req_id);
        } else {
            psErrorStackPrint(stderr, "do not recognize extname: %s in %s\n", extname, fileName);
            return PS_EXIT_DATA_ERROR;
        }
    }
    if (!dumpTable) {
        // done
        return 0;
    }

    if (!psArrayLength(array)) {
        fprintf(stderr, "%s contains an empty table\n", fileName);
        return 0;
    }

    for (int i=0; i<psArrayLength(array); i++) {
        psString str = psMetadataConfigFormat(array->data[i]);
        if (!str) {
            psErrorStackPrint(stderr, "failed to format metadata item\n");
            return (PS_EXIT_SYS_ERROR);
        }
        if (!simple) {
            printf("ROW_%d METADATA\n", i+1);
            printf("%s", str);
            printf("END\n");
        } else {
            // simple output format space separated values one line per row
            char *p = str;
            char *pnl;
            while ((pnl = strchr(p, '\n'))) {
                // terminate the string for this line
                *pnl = 0;
                bool blank = (p == pnl);
                if (blank) {
                    p = pnl + 1;
                    continue;
                }
                // split line into space separated tokens
                char *name = strtok(p, " ");
                char *type = strtok(NULL, " ");
                char *val = strtok(NULL, " ");

                // avoid unused variables warning/error
                (void) name; (void) type;

                if (val) {
                    printf("%s ", val);
                }
                // next line
                p = pnl + 1;
            }
            printf("\n");
        }
    }

    return 0;
}
