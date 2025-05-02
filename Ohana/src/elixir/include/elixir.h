# include <ohana.h>
# include <gfitsio.h>
# include <signal.h>
# include <errno.h>

# define CR 0x0D
# define LF 0x0A

# define IDLE     0x00
# define BUSY     0x01
# define DOWN     0x02
# define DONE     0x04
# define MESSAGE  0x08
# define SUCCESS  0x10
# define FAILURE  0x20
# define JOBDONE  0x40
# define WAITING  0x80
# define ERROR    0x100
# define CRASH    0x200
# define TIMEOUT  0x400

# define DTIME(A,B) ((A.tv_sec - B.tv_sec) + 1e-6*(A.tv_usec - B.tv_usec))

/* represents a socketed connection to a remote host
   which may or may not be currently running a process */

typedef struct {
  char *buffer;
  int   Nalloc;
  int   Nmaxread;
  int   Nextra;
  int   Nlast;
  int   Nbuffer;
} Fifo;

typedef struct {
  int argc; char **argv; /* a list of words that define this object */
  struct timeval start, accum, timer;
  int   status;
  char *logfile;
  char *lastproc;
} Object;

typedef struct {
  Object **object;
  int    Nobject;
  int    NOBJECT;
} Queue;

typedef struct {
  char   *hostname;
  int     rsock, wsock, pid;
  int     status; /* idle, busy, etc... */
  struct  timeval start, accum, timer, quiet;
  Fifo    fifo;
  int     code;
  Object *object;
} Machine;

typedef struct {
  Machine **machine;
  int     Nmachine;
  int     NMACHINE;
} Cluster;

typedef struct {
  char    *name;
  Queue   *pending;
  Queue   *success;
  Queue   *failure;
  Cluster *cluster;
  int     argc;
  char    **argv;
} Process;

/* we have one ProcessTimer for each existing machine
   each ProcessTimer has one element for each process */
typedef struct {
  struct timeval *timer;
  double       *timesum;
  int          *Njobs;
  int          *active;
  Machine      *machine;
  Process     **process;
} ProcessTimers;

int   VERBOSE;
char  CONNECT[128];

/* prototypes */

Process **DefineProcesses (Process *global, int *nprocess, char *config);
Process **GetProcessInfo (Process **gb, int *np, int *no);
char 	 *BaseFilename (char *file);
char 	 *BuildCode (char *line);
char 	 *BuildName (char *line);
char 	 *ConfigInit (int *argc, char **argv);
Process  *ConfigProcess (char *config, char *procname);
char 	 *ExpandEntry (char *entry, int argc, char **argv);
Machine  *GetMachine (Cluster *cluster);
Object   *GetObject (Queue *queue);
char 	 *GetPhotcode (char *file);
char 	 *GetPhotcodeExt (char *file);
char 	 *GetPhotcodeMef (char *file);
Machine  *GrabMachine (void);
Cluster  *InitCluster (void);
Process  *InitProcess (char *name, Queue *pending, Queue *failure, int (*mkargs)(void));
Queue    *InitQueue (void);
FILE     *LogOpen (char *filename);
char 	 *PathFilename (char *file);
char 	 *RootFilename (char *file);
int 	  CheckCluster (Cluster *cluster, Queue *success, Queue *failure, Queue *pending);
int 	  CheckDepend (Object *object, int argc, char **argv, int *argd);
int 	  CheckEndingState (Process *global, int Nobjects, int Dynamic);
int 	  CheckMachineStatus (Machine *machine);
int 	  CheckMessages (void);
int 	  CheckProcess (Process *process);
void 	  CloseMachine (Machine *machine);
void 	  ConfigPID (char *PIDFile);
int 	  ConnectMachine (Machine *machine);
void 	  DownMachine (Machine *machine);
void 	  DumpFinished (Queue *queue, int Nstart, char *filename);
void 	  DumpMachineStatus (FILE *f);
int 	  DumpProcessTimes (char *filename);
int 	  DumpStatus (char *filename);
void 	  ElixirStop (void);
int 	  FlushFifo (Fifo *fifo);
void 	  FreeArgs (int argc, char **argv, int *argd);
void 	  FreeFifo (Fifo *fifo);
int 	  GetDynamicState (void);
double    GetTimeout (void);
void 	  HaltElixir (char *pidfile);
int 	  HalttoRestart (char *pidfile);
void 	  IdleMachine (Machine *machine);
int 	  InitFifo (Fifo *fifo, int Nalloc, int Nextra);
void 	  InitMachines (char *config);
int 	  InitMsgFile (char *file);
void 	  InitProcessTimers (Process **process, int Nprocess);
void 	  KillElixir (char *pidfile);
int 	  LoadPID (char *file, pid_t *pid, char *username, char *machine);
int 	  LoadPending (Process *global, char *inlist, int *state, int *dynamic);
int 	  MakeArgs (Process *process, Object *object, int *cargc, char ***cargv, int **cargd);
void 	  ParseLine (char *testline, int argc, char **argv, int *depend, char **outline);
void 	  PushMachine (Machine *machine, Cluster *cluster);
void 	  PushObject (Queue *queue, Object *object);
void 	  PutMachine (Machine *machine, Cluster *cluster);
void 	  PutObject (Queue *queue, Object *object);
int 	  ReadMsg (char *fifo, char **message);
int 	  ReadtoFifo (Fifo *fifo, int sock);
void 	  RegisterTimeout (double value);
void 	  RemovePID (void);
void 	  Restart (char **argv);
void 	  RestartMachines (void);
void 	  SIG_DIE (int sig);
void 	  SIG_MESSAGE (int sig);
void 	  SIG_PIPE (int sig);
void 	  SIG_RELOAD (int sig);
void 	  SIG_STOP (int sig);
int 	  SetExitTimer (void);
void 	  SetMessageFile (char *filename);
int 	  ShiftFifo (Fifo *fifo);
void 	  Shutdown (int status);
int 	  SockScan (char *string, Fifo *fifo, int sock);
int 	  StartMachine (Machine *machine, Object *object, int argc, char **argv);
void 	  StartProcessTimer (Process *process, Machine *machine);
void 	  StatusElixir (char *pidfile, char *msgfile);
void 	  StopProcessTimer (Machine *machine);
int 	  TestMachine (Machine *machine);
int 	  WaitMsg (char *fifo, char **message, double maxdelay);
int 	  WriteMsg (char *fifo, char *message);
void      GetConfig (char *config, char *field, char *format, int N, void *ptr);
int       rconnect_elixir (char *hostname, char *command, int *rsock, int *wsock);
