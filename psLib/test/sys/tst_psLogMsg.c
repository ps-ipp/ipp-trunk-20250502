/*****************************************************************************
    This code will test whether trace levels can be set successfully.
    This code will test whether trace messages can be displayed with printf
    style string.
 *****************************************************************************/
#include <stdio.h>
#include "pslib_strict.h"
#include "psTest.h"
#include <unistd.h>
#include <fcntl.h>

static psS32 testLogMsg00();
static psS32 testLogMsg01();
static psS32 testLogMsg02();
static psS32 testLogMsg03();
static psS32 testLogMsg04();
static psS32 testLogMsg05();
static psS32 testLogMsg06();

testDescription tests[] = {
                              {
                                  testLogMsg00, 0, "default log levels, printf-style strings", 0, false
                              },
                              {
                                  testLogMsg01, 1, "default log levels, psVLogMsg()", 0, false
                              },
                              {
                                  testLogMsg02, 2, "psLogSet/GetLevel()", 0, false
                              },
                              {
                                  testLogMsg03, 3, "psLogSetFormat()", 0, false
                              },
                              {
                                  testLogMsg04, 4, "Output Format", 0, false
                              },
                              {
                                  testLogMsg05, 5, "psLogSet/GetDestination()", 0, false
                              },
                              {
                                  testLogMsg06, 6, "psMessageDestination()", 0, false
                              },
                              {
                                  NULL
                              }
                          };

psS32 main( psS32 argc, char* argv[] )
{
    psLogSetLevel( PS_LOG_INFO );

    return ( ! runTestSuite( stderr, "psLogMsg", tests, argc, argv ) );
}


static void myLogMsg(const char *name,
                     psS32 level,
                     const char *fmt,
                     ...)
{
    va_list ap;

    // Test whether psLogMsgV() accept a va_list for output variables.
    va_start(ap, fmt);
    psLogMsgV(name, level, fmt, ap);
    va_end(ap);
}

static psS32 testLogMsg00()
{
    psS32 i = 0;

    // Send a log messages for levels 0:9.  Only the first four messages
    // should actually be displayed.
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d %f %s\n", i,
                 (float) i, "beep beep");
    }

    return 0;
}

static psS32 testLogMsg01()
{
    psS32 i = 0;

    // Send a log messages for levels 0:9.  Only the first four messages
    // should actually be displayed.
    for (i=0;i<10;i++) {
        myLogMsg(__func__, i, "Hello World!  My level is %d %f %s\n", i,
                 (float) i, "beep beep");
    }

    return 0;
}

static psS32 testLogMsg02()
{

    psLogSetLevel(9);
    // Send a log messages for levels 0:9.
    for (psS32 i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }

    psLogSetLevel(5);
    psLogMsg(__func__, 6, "This should not be displayed (level %d)\n", 6);
    psLogSetLevel(4);
    psLogMsg(__func__, 4, "This should  be displayed (level %d)\n", 4);
    psLogMsg(__func__, 4, "This should display level 4 logging -> level %d\n", psLogGetLevel() );

    return 0;
}

static psS32 testLogMsg03()
{
    psS32 i;

    fprintf(stderr,"------------- psLogSetFormat() -------------\n");
    psLogSetFormat("");
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }

    fprintf(stderr,"------------- psLogSetFormat(NULL) -------------\n");
    psLogSetFormat(NULL);
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }

    fprintf(stderr,"------------- psLogSetFormat(T) -------------\n");
    psLogSetFormat("T");
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }

    fprintf(stderr,"------------- psLogSetFormat(H) -------------\n");
    psLogSetFormat("H");
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }

    fprintf(stderr,"------------- psLogSetFormat(L) -------------\n");
    psLogSetFormat("L");
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }

    fprintf(stderr,"------------- psLogSetFormat(N) -------------\n");
    psLogSetFormat("N");
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }

    fprintf(stderr,"------------- psLogSetFormat(M) -------------\n");
    psLogSetFormat("M");
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }

    fprintf(stderr,"------------- psLogSetFormat(THLNM) -------------\n");
    psLogSetFormat("THLNM");
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }

    return 0;
}


psS32 testLogMsg04()
{
    psLogMsg("Under 15 chars", 0, "Hello World!\n");
    psLogMsg("This string is more than 15 chars", 0, "Hello World!\n");
    psLogMsg(__func__, 0, "Line #1\n");
    psLogMsg(__func__, 0, "Line #2\n");
    psLogMsg(__func__, 0, "Line #3");
    psLogMsg(__func__, 0, "Line #4");

    return 0;
}

psS32 testLogMsg05()
{
    psS32 i = 0;
    //    FILE* file;
    int fd;
    //    char line[256];

    printf("--------------- psLogSetDestination(PS_LOG_NONE) ----------------\n");
    //    psLogSetDestination("none");
    psLogSetDestination(0);
    printf("    File Descriptor = %d \n", psLogGetDestination() );
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }

    printf("------------- psLogSetDestination(PS_LOG_TO_STDERR) -------------\n");
    //    psLogSetDestination("dest:stderr");
    psLogSetDestination(2);
    printf("    File Descriptor = %d \n", psLogGetDestination() );
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }

    printf("------------- psLogSetDestination(PS_LOG_TO_STDOUT) -------------\n");
    //    psLogSetDestination("dest:stdout");
    psLogSetDestination(1);
    printf("    File Descriptor = %d \n", psLogGetDestination() );
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }

    printf("--------------- psLogSetDestination(""file:log.txt"") ---------------\n");
    fd = creat("log.txt", 0666);
    //    psLogSetDestination("file:log.txt");
    psLogSetDestination(fd);
    printf("    File Descriptor = %d \n", psLogGetDestination() );
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }


    //    psLogSetDestination("none");
    FILE *file;
    char line[257];
    //    psLogSetDestination(-1);
    psLogSetDestination(0);
    printf("--------------------- The Contents of log.txt -------------------\n");
    file = fopen("log.txt","r");
    while ( fgets(line,256,file) != NULL ) {
        printf("%s",line);
    }
    fclose(file);
    close(fd);

    int fd2 = creat("eva/log.txt", 0666);
    printf("--------------- psLogSetDestination(""file:eva/log.txt"") ----------\n");
    //    psLogSetDestination("file:/eva/log.txt");
    psLogSetDestination(fd2);
    for ( i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World! My level is %d\n", i);
    }
    close(fd2);

    return 0;
}

psS32 testLogMsg06()
{
    psS32 i = 0;
    //    FILE* file;
    int fd;
    //    char line[256];

    printf("--------------- psMessageDestination(PS_LOG_NONE) ----------------\n");
    psMessageDestination("none");
    //    psLogSetDestination(0);
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }

    printf("------------- psMessageDestination(PS_LOG_TO_STDERR) -------------\n");
    psMessageDestination("stderr");
    //    psLogSetDestination(2);
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d - stderr\n", i);
    }

    printf("------------- psMessageDestination(PS_LOG_TO_STDOUT) -------------\n");
    psMessageDestination("stdout");
    //    psLogSetDestination(1);
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d - stdout\n", i);
    }

    printf("--------------- psMessageDestination(""file:log2.txt"") ---------------\n");
    fd = creat("log2.txt", 0666);
    psMessageDestination("file:log2.txt");
    //    psLogSetDestination(fd);
    for (i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World!  My level is %d\n", i);
    }


    psMessageDestination("none");
    FILE *file;
    char line[257];
    //    psMessageDestination(-1);
    //    psLogSetDestination(0);
    printf("--------------------- The Contents of log2.txt -------------------\n");
    file = fopen("log2.txt","r");
    while ( fgets(line,256,file) != NULL ) {
        printf("%s",line);
    }
    fclose(file);
    close(fd);

    int fd2 = creat("eva/log2.txt", 0666);
    printf("--------------- psMessageDestination(""file:eva/log.txt"") ----------\n");
    psMessageDestination("file:eva/log2.txt");
    //    psLogSetDestination(fd2);
    for ( i=0;i<10;i++) {
        psLogMsg(__func__, i, "Hello World! My level is %d\n", i);
    }
    close(fd2);

    return 0;
}
