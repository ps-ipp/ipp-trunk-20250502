# include "basic.h"

int basename_opihi  PROTO((int, char **));
int config          PROTO((int, char **));
int exec_sleep      PROTO((int, char **));
int exec_usleep     PROTO((int, char **));
int cd              PROTO((int, char **));
int date            PROTO((int, char **));
int dirname_opihi   PROTO((int, char **));
int echo            PROTO((int, char **));
int exec_last       PROTO((int, char **));
int exec_next       PROTO((int, char **));
int exec_break      PROTO((int, char **));
int file            PROTO((int, char **));
int getchr_func     PROTO((int, char **));
int getargs         PROTO((int, char **));
int help            PROTO((int, char **));
int input           PROTO((int, char **));
int inthash         PROTO((int, char **));
int list            PROTO((int, char **));
int list_help       PROTO((int, char **));
int list_vars       PROTO((int, char **));
int local           PROTO((int, char **)); /* data? */
int macro           PROTO((int, char **));
int memory          PROTO((int, char **));
int mkdir_opihi     PROTO((int, char **));
int module          PROTO((int, char **));
int nop             PROTO((int, char **));
int opihi_setmode   PROTO((int, char **));
int output          PROTO((int, char **));
int pwd             PROTO((int, char **));
int quit            PROTO((int, char **));
int run_for         PROTO((int, char **));
int run_foreach     PROTO((int, char **));
int run_if          PROTO((int, char **));
int run_while       PROTO((int, char **));
int scan            PROTO((int, char **));
int scannext        PROTO((int, char **));
int shell           PROTO((int, char **));
int sprintf_opihi   PROTO((int, char **));
int fprintf_opihi   PROTO((int, char **));
int strlen_func     PROTO((int, char **));
int strmatch        PROTO((int, char **));
int substr_func     PROTO((int, char **));
int strstr_func     PROTO((int, char **));
int strpop          PROTO((int, char **));
int strhash         PROTO((int, char **));
int strsub          PROTO((int, char **));
int wait_func       PROTO((int, char **));
int which           PROTO((int, char **));

/** mapping of the command names to command functions **/
static Command cmds[] = {  
  {1, "config",        config,             "(re)load config file?"},
  {1, "sleep",         exec_sleep,         "sleep for N seconds"},
  {1, "usleep",        exec_usleep,        "sleep for N microseconds"},
  {1, "cd",            cd,                 "change directory"},
  {1, "date",          date,               "get current date"},
  {1, "basename",      basename_opihi,     "built-in basename function"},
  {1, "dirname",       dirname_opihi,      "built-in dirname function"},
  {1, "echo",          echo,               "type this line *"},
  {1, "break",         exec_break,         "escape from function *"},
  {1, "continue",      exec_next,          "next loop iteration"},
  {1, "next",          exec_next,          "next loop iteration"},
  {1, "last",          exec_last,          "last loop iteration"},
  {1, "return",        exec_last,          "exit from macro"},
  {1, "file",          file,               "test file existence"},
  {1, "getchr",        getchr_func,        "find character in string"},
  {1, "getargs",       getargs,            "find and remove arguments from the macro argument list"},
  {1, "help",          help,               "get help on a function *"},
  {1, "input",         input,              "read command lines from a file *"},
  {1, "inthash",       inthash,            "generate a hash for a word treated as an integer"},
  {1, "list",          list,               "get variable list"},
  {1, "?",             list_help,          "list commands *"},
  {1, "??",            list_vars,          "list variables *"},
  {1, "#",             nop,                "a NOP function"},
  {1, "##",            nop,                "a NOP function"},
  {1, "###",           nop,                "a NOP function"},
  {1, "local",         local,              "define local variables"},
  {1, "macro",         macro,              "deal with the macros *"}, 
  {1, "memory",        memory,             "long listing of the allocated memory"},
  {1, "mkdir",         mkdir_opihi,        "built-in mkdir command"},
  {1, "module",        module,             "load script file from the modules directories"},
  {1, "nop",           nop,                "a NOP function"},
  {1, "opihi",         opihi_setmode,      "get / set opihi behavior options"},
  {1, "output",        output,             "redirect output to file"},
  {1, "pwd",           pwd,                "print current working directory"},
  {1, "exit",          quit,               "exit program *"}, 
  {1, "quit",          quit,               "exit program *"},
  {1, "for",           run_for,            "for loop"}, 
  {1, "foreach",       run_foreach,        "foreach loop"}, 
  {1, "if",            run_if,             "logical cases *"}, 
  {1, "while",         run_while,          "while loop"}, 
  {1, "scan",          scan,               "scan line from keyboard or file to variable *"},
  {1, "scannext",      scannext,           "scan next line from file to variable (file stays open)"},
  {1, "!",             shell,              "system call"},
  {1, "exec",          shell,              "system call"},
  {1, "sprintf",       sprintf_opihi,      "formatted print to a variable"},
  {1, "fprintf",       fprintf_opihi,      "formatted print to standard output"},
  {1, "strlen",        strlen_func,        "string length"},
  {1, "substr",        substr_func,        "extract a substring"},
  {1, "strstr",        strstr_func,        "find a substring"},
  {1, "strhash",       strhash,            "generate a hash for a string"},
  {1, "strpop",        strpop,             "pop a string"},
  {1, "strsub",        strsub,             "replace instances of a key in a string"},
  {1, "strmatch",      strmatch,           "string length"},
  {1, "wait",          wait_func,          "wait until return is typed"},
  {1, "which",         which,              "show command *"}
};

void InitBasic () {
  
  int i;

  InitCommands ();
  InitMacros ();
  InitBuffers ();
  InitVectors ();
  InitVariables ();
  InitLists ();

  for (i = 0; i < sizeof (cmds) / sizeof (Command); i++) {
    AddCommand (&cmds[i]);
  }
  
}

void InitBasic_PantasksClient () {
  
  int i;

  InitCommands ();
  InitMacros ();
  InitBuffers ();
  InitVectors ();
  InitVariables ();
  InitLists ();

  for (i = 0; i < sizeof (cmds) / sizeof (Command); i++) {
    if (!strcmp (cmds[i].name, "quit")) goto valid;
    if (!strcmp (cmds[i].name, "exit")) goto valid;
    if (!strcmp (cmds[i].name, "exec")) goto valid;
    if (!strcmp (cmds[i].name, "!")) goto valid;
    continue;

  valid:
    AddCommand (&cmds[i]);
  }
}

void FreeBasic () {

  FreeCommands ();
  FreeMacros ();
  FreeBuffers ();
  FreeVectors ();
  FreeVariables ();
  FreeLists ();
}
