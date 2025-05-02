# include "data.h"
# include "basic.h"
# define THREADED

typedef struct timeval Ptime;
typedef unsigned long long IDtype;

/** job status values **/
typedef enum {
  PCONTROL_JOB_NONE, // XXX OK?
  PCONTROL_JOB_ALLJOBS,
  PCONTROL_JOB_PENDING,
  PCONTROL_JOB_BUSY,  
  PCONTROL_JOB_RESP,  
  PCONTROL_JOB_HUNG,  
  PCONTROL_JOB_DONE,  
  PCONTROL_JOB_KILL,  
  PCONTROL_JOB_EXIT,
  PCONTROL_JOB_CRASH,
} JobStat;

/** job status values **/
typedef enum {
  PCONTROL_JOB_ANYHOST,
  PCONTROL_JOB_WANTHOST,
  PCONTROL_JOB_NEEDHOST,
} JobMode;

/** job mode check stages **/
typedef enum {
  PCONTROL_JOB_STAGE_ANYHOST,
  PCONTROL_JOB_STAGE_WANTHOST,
  PCONTROL_JOB_STAGE_NEEDHOST,
  PCONTROL_JOB_STAGE_OLDWANT,
} JobCheckStage;

/** job thread options values **/
typedef enum {
  PCONTROL_JOB_THREADS_NONE,
  PCONTROL_JOB_THREADS_MAX,
} JobThreadMode;

/** host status values **/
typedef enum {
  PCONTROL_HOST_ALLHOSTS,
  PCONTROL_HOST_IDLE,
  PCONTROL_HOST_BUSY,  
  PCONTROL_HOST_RESP,
  PCONTROL_HOST_DOWN,
  PCONTROL_HOST_DONE,
  PCONTROL_HOST_OFF,
} HostStat;

/** host response options **/
typedef enum {
  PCONTROL_RESP_NONE,
  PCONTROL_RESP_START_JOB,
  PCONTROL_RESP_CHECK_BUSY_JOB,  
  PCONTROL_RESP_CHECK_DONE_HOST,  
  PCONTROL_RESP_CHECK_HOST,
  PCONTROL_RESP_KILL_JOB,
  PCONTROL_RESP_STOP_HOST,
  PCONTROL_RESP_DOWN_HOST,
} HostResp;

typedef enum {
  PCONTROL_RUN_UNKNOWN,
  PCONTROL_RUN_NONE,
  PCONTROL_RUN_HOSTS,
  PCONTROL_RUN_REAP,
  PCONTROL_RUN_ALL,
} RunLevels;

/* stack special positions */
typedef enum {
  STACK_TOP = 0,
  STACK_BOTTOM = -1,
} StackWhere;

typedef enum {
  PCLIENT_HUNG = -1,
  PCLIENT_DOWN = 0,
  PCLIENT_GOOD = 1,
} PclientStat;

typedef struct {
  char *buffer;
  int   Nalloc;
  int   Nmaxread;
  int   Nextra;
  int   Nlast;
  int   Nbuffer;
} Fifo;

typedef struct {
  IOBuffer     buffer;
  int          completed;
  int          size;
  int          requested;
} JobOutput;

/* A machine has a unique name and may have multiple Hosts (each of which can run a single job)
   We use this to track aspects of the analysis per machine, to eg, limit the number of jobs 
   desired on a single machine */
typedef struct {
  char *name;
  int Nhosts;		      // how many hosts are selected for this machine (whatever state)
  int NjobsRealhost;
  int NjobsWanthost;
} Machine;

/* data to define a job */
typedef struct {
  int          argc; 
  char       **argv;
  char        *hostname;
  char        *realhost;
  char       **xhosts;
  int          Nxhosts;
  int          exit_status;
  int          Reset;
  int          nicelevel;
  JobMode      mode;
  JobStat      state;
  JobStat      stack;
  JobOutput    stdout_buf;
  JobOutput    stderr_buf;
  Ptime        start;
  Ptime        stop;
  double       dtime;
  int          pid;
  IDtype       JobID;
  struct Host *host;
} Job;

/* data to define a host machine */
typedef struct {
  char       *hostname;
  int         stdin_fd;
  int         stdout_fd;
  int         stderr_fd;
  int         max_threads;
  int         markoff;
  int         pid;
  HostStat    stack;
  IDtype      HostID;
  IOBuffer    comms_buffer;
  char       *response;
  HostResp    response_state;
  Ptime       last_start_try; // last (UNIX) time we attempted to connect to this host (0 on success)
  Ptime       next_start_try; // next (UNIX) time we should attempt to connect to this host (0 on success)
  Ptime       connect_time; // (UNIX) time we connected to this host
  struct Job *job;
} Host;

# if (USE_LLIST)
typedef struct StackItem {
    StackItem *next;
    StackItem *prev;
    void *object;
    char *name;
    int   id;
} StackItem;

/* the Jobs and Hosts are managed in a set of Stacks which define their state */
typedef struct {
    StackItem *head;
    StackItem *tail; // use this?
    int    Nobject;
# ifdef THREADED    
  pthread_mutex_t mutex;
# endif
} Stack;

# else

/* the Jobs and Hosts are managed in a set of Stacks which define their state */
typedef struct {
  void **object;
  char **name;
  int   *id;
  int    Nobject;
  int    NOBJECT;
# ifdef THREADED    
  pthread_mutex_t mutex;
# endif
} Stack;
# endif

/* XXX if this is hard-wired, we can't change shell name in StartHost */
# define PCLIENT_PROMPT "pclient:"

// # define FREE(X) if (X != NULL) { free (X); }
# define CLOSE(FD) { if (FD) close (FD); FD = 0; }
# define DTIME(A,B) ((A.tv_sec - B.tv_sec) + 1e-6*(A.tv_usec - B.tv_usec))
# define ZTIME(A) ((A.tv_sec == 0) && (A.tv_usec == 0))

# define ASSERT(TEST,STRING) { if (!(TEST)) { gprint (GP_ERR, "programming error: %s\n", STRING); abort (); }}
# define ABORT(STRING) { gprint (GP_ERR, "programming error: %s\n", STRING); abort (); }
// # define ASSERT(TEST,STRING) { if (!(TEST)) { gprint (GP_ERR, "programming error: %s\n", STRING); raise (SIGINT); exit (2); }}
// # define ABORT(STRING) { gprint (GP_ERR, "programming error: %s\n", STRING); raise (SIGINT); exit (2); }

void InitPcontrol (void);
void FreePcontrol (void);

/*** StackOps.c ***/
Stack *InitStack (void);
void   FreeStack (Stack *stack);
int    PushStack (Stack *stack, int where, void *object, int id, char *name);
void  *PullStackByLocation (Stack *stack, int where);
void  *PullStackByName (Stack *stack, char *name);
void  *PullStackByID (Stack *stack, int id);
int    RemoveStackEntry (Stack *stack, int where);
void  *RemoveStackByID (Stack *stack, int id);
void   LockStack (Stack *stack);
void   UnlockStack (Stack *stack);

// void  *FindStackByID (Stack *stack, int id);
// void  *FindStackByName (Stack *stack, char *name);

/*** CheckSystem.c ***/
int   CheckSystem (void);
void *CheckSystem_Threaded (void *data);
int   CheckBusyJobs (float delay);
int   CheckDoneJobs (float delay);
int   CheckKillJobs (float delay);
int   CheckDoneHosts (float delay);
int   CheckDownHosts (float delay);
int   CheckIdleHosts (float delay, int Stage);
int   CheckLiveHosts (float delay);
int   SetRunSystem (int state);
RunLevels SetRunLevel (RunLevels level);

/*** own files ***/
int StartJob (Job *job, Host *host);
int StartJobResponse (Host *host);

int CheckHost (Host *host);
int CheckHostResponse (Host *host);

int CheckDoneHost (Host *host);
int CheckDoneHostResponse (Host *host);

int CheckBusyJob (Job *job, Host *host);
int CheckBusyJobResponse (Host *host);

int KillJob (Job *job, Host *host);
int KillJobResponse (Host *host);

int StartHost (Host *host);
int CheckResetHost (Host *host);
int CheckIdleHost (Host *host, int Stage);
int CheckDoneJob (Job *job, Host *host);
int GetJobOutput (char *command, Host *host, JobOutput *output);

int PclientCommand (Host *host, char *command, char *response, HostResp response_state);
int PclientResponse (Host *host, char *response, IOBuffer *buffer);

int CheckRespHosts (float MaxDelay);
int CheckRespHost (Host *host);

/*** misc files ***/
int    VerboseMode (void);  // in verbose.c
void   gotsignal (int signum); // in pcontrol.c

/*** IDops.c ***/
void InitIDs (void);
IDtype NextJobID (void);
IDtype NextHostID (void);
void PrintID (gpDest dest, IDtype ID);
IDtype GetID (char *IDword);

/*** CheckPoint.c ***/
int SetCheckPoint (void);
int ClearCheckPoint (void);
int TestCheckPoint (void);

/*** HostOps.c ***/
void   InitHostStacks (void);
void   FreeHostStacks (void);
Stack *GetHostStack (int StackID);
char  *GetHostStackName (int StackID);
Stack *GetHostStackByName (char *name);
int    PutHost (Host *host, int StackID, int where);
Host  *PullHostByID (IDtype HostID, int *StackID);
Host  *PullHostByName (char *name, int *StackID);
Host  *PullHostFromStackByID (int StackID, IDtype ID);
Host  *PullHostFromStackByName (int StackID, char *name);
IDtype AddHost (char *hostname, int max_threads);
void   DelHost (Host *host);

/*** StopHosts.c ***/
void   DownHost (Host *host);
int    DownHosts (void);
int    DownHostResponse (Host *host);
void   OffHost (Host *host);
int    StopHost (Host *host, int mode);
int    StopHostResponse (Host *host);
int    HarvestHost (int pid);
int    AddZombie(int pid);
int    DelZombies();
int    CheckZombies();

/*** JobOps.c ***/
int InitJobOutput (JobOutput *output);
int ResetJobOutput (JobOutput *output);
void   InitJobStacks (void);
void   FreeJobStacks (void);
Stack *GetJobStack (int StackID);
char  *GetJobStackName (int StackID);
Stack *GetJobStackByName (char *name);
int GetJobStackIDbyName (char *name);
int    PutJob (Job *job, int StackID, int where);
int    PutJobSetState (Job *job, int StackID, int where, int state);
Job   *PullJobByID (IDtype JobID, int *StackID);
Job   *PullJobFromStackByID (int StackID, int ID);
IDtype AddJob (char *hostname, JobMode mode, int timeout, int nicelevel, int argc, char **argv, int Nxhosts, char **xhosts);
void   DelJob (Job *job);
Host  *UnlinkJobAndHost (Job *job);
void   LinkJobAndHost (Job *job, Host *host);

/*** MachineOps.c ***/
void InitMachines ();
void FreeMachines ();
Machine *FindMachineByName (char *name);
Machine *AddMachine (char *name);
int DelMachine (char *name);
int AddMachineHost (Host *host);
int DelMachineHost (Host *host);
int AddMachineJob (Host *host, Job *job);
int DelMachineJob (Host *host, Job *job);
int PrintMachines ();
int CheckMachineJobs (Host *host, Job *job);
int GetMaxUnwantedHostJobs (void);
void SetMaxUnwantedHostJobs (int value);

float GetMaxConnectTime (void);
void SetMaxConnectTime (float value);
float GetMaxWantHostWait (void);
void SetMaxWantHostWait (float value);

void pcontrol_exit (int n);
void QuitCheckSystemThread (void);

// Job   *FindJobByID (IDtype JobID, int *StackID);
// Job   *FindJobInStackByID (int StackID, int ID);
// Host  *FindHostByID (IDtype HostID, int *StackID);
// Host  *FindHostByName (char *name, int *StackID);
// Host  *FindHostInStackByID (int StackID, IDtype ID);
// Host  *FindHostInStackByName (int StackID, char *name);
