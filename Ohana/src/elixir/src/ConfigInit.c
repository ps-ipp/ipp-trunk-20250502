# include "elixir.h"

void SIG_STOP (int sig) {
  fprintf (stderr, "trapped signal %d, exiting when jobs are done\n", sig);
  ElixirStop ();
}

void SIG_MESSAGE (int sig) {
  fprintf (stderr, "trapped signal %d, reading message\n", sig);
  CheckMessages ();
}

void SIG_DIE (int sig) {
  fprintf (stderr, "trapped signal %d, exiting\n", sig);
  Shutdown (1);
}

void SIG_PIPE (int sig) {
  fprintf (stderr, "pipe signal %d\n", sig);
}

char *ConfigInit (int *argc, char **argv) {

  int N;
  char *config, *file;

  /* use default settings */
  signal (SIGKILL,   SIG_DFL);    
  signal (SIGCONT,   SIG_DFL);    
  signal (SIGSTOP,   SIG_DFL);    
  signal (SIGCHLD,   SIG_DFL);    

  /* exit on these signals */
  signal (SIGILL,    SIG_DIE);     
  signal (SIGABRT,   SIG_DIE);    
  signal (SIGFPE,    SIG_DIE);     
  signal (SIGSEGV,   SIG_DIE);    
  signal (SIGTERM,   SIG_DIE);    
  signal (SIGBUS,    SIG_DIE);     
  signal (SIGTRAP,   SIG_DIE);    
  signal (SIGXCPU,   SIG_DIE);    
  signal (SIGXFSZ,   SIG_DIE);    
  signal (SIGIOT,    SIG_DIE);     
  signal (SIGHUP,    SIG_IGN);
  signal (SIGINT,    SIG_DIE);
  signal (SIGQUIT,   SIG_IGN);
  signal (SIGPIPE,   SIG_PIPE);    
  signal (SIGALRM,   SIG_IGN);    
  signal (SIGUSR1,   SIG_STOP);    
  signal (SIGUSR2,   SIG_MESSAGE);    
  signal (SIGTSTP,   SIG_IGN);    
  signal (SIGTTIN,   SIG_IGN);    
  signal (SIGTTOU,   SIG_IGN);    
  signal (SIGPROF,   SIG_IGN);    
  signal (SIGURG,    SIG_IGN);     
  signal (SIGVTALRM, SIG_IGN);  
  signal (SIGIO,     SIG_IGN);      

  /* signals which are not always defined */
# ifdef SIGPOLL
  signal (SIGPOLL,   SIG_IGN);    
# endif
# ifdef SIGPWR
  signal (SIGPWR, SIG_DIE);     /* power failure (Sys V) */
# endif
# ifdef SIGWINCH
  signal (SIGWINCH, SIG_IGN);   /* window resized (4.3BSD) */
# endif
# ifdef SIGSYS
  signal (SIGSYS,    SIG_DIE);     
# endif
# ifdef SIGEMT
 signal (SIGEMT,     SIG_DIE); 
# endif
# ifdef SIGSTKFLT
  signal (SIGSTKFLT, SIG_DIE);  
# endif
# ifdef SIGINFO
 signal (SIGINFO,    SIG_DIE); 
# endif
# ifdef SIGCLD
  signal (SIGCLD,    SIG_DFL);     
# endif
# ifdef SIGLOST
  signal (SIGLOST,   SIG_IGN); 
# endif
# ifdef SIGUNUSED
  signal (SIGUNUSED, SIG_IGN); 
# endif

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  if (file == (char *) NULL) {
    fprintf (stderr, "ERROR: can't choose configuration file\n");
    Shutdown (1);
  }    
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    // XXX note: the test below fools gcc into thinking file could be NUL
    // even though the test above forces an exit if it is NULL.
    // if (file != (char *) NULL) free (file);
    Shutdown (1);
    exit (2);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);
  free (file);

  if ((N = get_argument (*argc, argv, "-restart"))) {
    char pidfile[128];

    remove_argument (N, argc, argv);
    if (!ScanConfig (config, "global.pid", "%s",  0, pidfile)) {
      fprintf (stderr, "pid file not defined: global.pid\n");
      exit (1);
    }
    HalttoRestart (pidfile);
  }

  if ((*argc != 2) && (*argc != 1)) {
    fprintf (stderr, "USAGE: elixir [list]\n");
    fprintf (stderr, "       elixir [-kill]\n");
    fprintf (stderr, "       elixir [-stop]\n");
    fprintf (stderr, "       elixir [-status]\n");
    Shutdown (1);
  }
  
  /* special command-line options */
  { 
    char pidfile[128], msgfile[128];

    if (!ScanConfig (config, "global.pid", "%s",  0, pidfile)) {
      fprintf (stderr, "pid file not defined: global.pid\n");
      exit (1);
    }
    
    if (!ScanConfig (config, "global.msg", "%s",  0, msgfile)) {
      fprintf (stderr, "msg file not defined: global.msg\n");
      exit (1);
    }
    
    if (!ScanConfig (config, "CONNECT", "%s",  0, CONNECT)) {
      sprintf (CONNECT, "/usr/bin/rsh");
    }
    
    if (get_argument (*argc, argv, "-kill"))   KillElixir (pidfile);
    if (get_argument (*argc, argv, "-stop"))   HaltElixir (pidfile);
    if (get_argument (*argc, argv, "-status")) StatusElixir (pidfile, msgfile);
  }

  return (config);

}

/*

  SIGHUP             1        A      Hangup detected on controlling terminal
                                     or death of controlling process
  SIGINT             2        A      Interrupt from keyboard
  SIGQUIT            3        C      Quit from keyboard
  SIGILL             4        C      Illegal Instruction
  SIGABRT            6        C      Abort signal from abort(3)
  SIGFPE             8        C      Floating point exception
  SIGKILL            9       AEF     Kill signal
  SIGSEGV           11        C      Invalid memory reference
  SIGPIPE           13        A      Broken pipe: write to pipe with no readers
  SIGALRM           14        A      Timer signal from alarm(2)
  SIGTERM           15        A      Termination signal
  SIGUSR1        30,10,16     A      User-defined signal 1
  SIGUSR2        31,12,17     A      User-defined signal 2
  SIGCHLD        20,17,18     B      Child stopped or terminated
  SIGCONT        19,18,25            Continue if stopped
  SIGSTOP        17,19,23    DEF     Stop process
  SIGTSTP        18,20,24     D      Stop typed at tty
  SIGTTIN        21,21,26     D      tty input for background process
  SIGTTOU        22,22,27     D      tty output for background process
  SIGBUS         10,7,10      C      Bus error (bad memory access)
  SIGPOLL                     A      Pollable event (Sys V). Synonym of SIGIO
  SIGPROF        27,27,29     A      Profiling timer expired
  SIGSYS         12,-,12      C      Bad argument to routine (SVID)
  SIGTRAP           5         C      Trace/breakpoint trap
  SIGURG         16,23,21     B      Urgent condition on socket (4.2 BSD)
  SIGVTALRM      26,26,28     A      Virtual alarm clock (4.2 BSD)
  SIGXCPU        24,24,30     C      CPU time limit exceeded (4.2 BSD)
  SIGXFSZ        25,25,31     C      File size limit exceeded (4.2 BSD)
  SIGIOT            6         C      IOT trap. A synonym for SIGABRT
  SIGEMT          7,-,7
  SIGSTKFLT       -,16,-      A      Stack fault on coprocessor
  SIGIO          23,29,22     A      I/O now possible (4.2 BSD)
  SIGCLD          -,-,18             A synonym for SIGCHLD
  SIGPWR         29,30,19     A      Power failure (System V)
  SIGINFO         29,-,-             A synonym for SIGPWR
  SIGLOST         -,-,-       A      File lock lost
  SIGWINCH       28,28,20     B      Window resize signal (4.3 BSD, Sun)
  SIGUNUSED       -,31,-      A      Unused signal (will be SIGSYS)
*/
