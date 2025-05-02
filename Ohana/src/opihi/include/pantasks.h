# include "data.h"
# include "basic.h"
# include "astro.h"

# include <sys/time.h>
# include <time.h>
# include <zlib.h>

# define DEBUG 0

typedef int IDtype;

typedef enum {
  JOB_NONE,
  JOB_BUSY, 
  JOB_EXIT, 
  JOB_HUNG,
  JOB_CRASH,
  JOB_PENDING,
} JobStat;

typedef enum {
  JOB_LOCAL, 
  JOB_CONTROLLER, 
} JobMode;

typedef enum {
  CONTROLLER_HUNG = -1,
  CONTROLLER_DOWN = 0,
  CONTROLLER_GOOD = 1,
} ControllerStat;

enum {RANGE_ABS, RANGE_DAY, RANGE_WEEK};
enum {TIMER_ALLJOBS, TIMER_SUCCESS, TIMER_FAILURE};

enum {TASK_NONE, 
      TASK_EMPTY, 
      TASK_COMMENT, 
      TASK_NMAX, 
      TASK_ACTIVE, 
      TASK_TRANGE, 
      TASK_END, 
      TASK_HOST, 
      TASK_NICE, 
      TASK_STDOUT, 
      TASK_STDERR, 
      TASK_COMMAND, 
      TASK_OPTIONS, 
      TASK_PERIODS, 
      TASK_NPENDING, 
      TASK_EXIT, 
      TASK_EXEC
} TaskHashResults;

typedef struct {
  time_t start;
  time_t stop;
  char type;
  char include;
  int Nmax;
  int Nrun;
} TimeRange;

/* data to define a host machine */
typedef struct {
  char       *hostname;
  int         max_threads;
} Host;

/* a task is a description of the wrapping of a job */
typedef struct {
  Macro  *exec;				/* name is 'exec' */
  Macro  *crash;			/* name is 'crash' */
  Macro  *timeout;
  Macro  *defexit;

  int     NEXIT;
  int     Nexit;
  Macro **exit;				/* name is exit status */

  int     argc;
  char  **argv;

  int     optc;
  char  **optv;

  char   *host;
  int     host_required;

  char   *name;

  char   *stdout_dump;
  char   *stderr_dump;

  int       Nranges;
  TimeRange *ranges;

  int     Nmax;  // only construct Ntotal jobs for this task
  int     Njobs;

  int     Npending;  // number of currently pending jobs
  int     NpendingMax;  // max number of pending jobs allowed

  float   poll_period;
  float   exec_period;
  float   timeout_period;

  struct timeval last;

  int Nsuccess;
  int Nfailure;
  int Ntimeout;
  int Nskipexec;

  double dtimeAve_alljobs, dtimeMin_alljobs, dtimeMax_alljobs;
  double dtimeAve_success, dtimeMin_success, dtimeMax_success;
  double dtimeAve_failure, dtimeMin_failure, dtimeMax_failure;

  int active;
  int nicelevel;

} Task;

// time period include/exclude periods: 
// date ranges (e_time - e_time) 
// time ranges (e_time - e_time) (use e_time % 86400)
// time-of-week ranges  (e_time - e_time
// -trange Mon Fri (inclusive on days -- end defaults to Fri@23:59:59)
// -trange 08:00 - 17:00
// -trange 2005/12/24 2005/12/31
// be careful of HST!
// type: day, week, date
// keep: TRUE: perform action within this time period
// keep: FALSE: do not perform action within this time period
  
typedef struct {
  IDtype JobID;				/* internal ID for job */
  int pid;				/* external ID for job */

  struct timeval last;
  struct timeval start;
  int state;
  int exit_status;

  int     argc;
  char  **argv;

  int     optc;
  char  **optv;

  Task   *task;

  /* this cries out for another structure... */
  IOBuffer    stdout_buff;    /* stdout storage buffer */
  char       *stdout_dump;    // output target file for stdout
  int         stdout_size;    /* size of pending stdout buffer (controller) */
  int         stdout_fd;      /* stdout pipe (local only) */

  IOBuffer    stderr_buff;    /* stderr storage buffer */
  char       *stderr_dump;    // output target file for stderr
  int         stderr_size;    /* size of pending stderr buffer (controller) */
  int         stderr_fd;      /* stderr pipe (local only) */

  JobMode     mode;			/* local or controller? */
  int     nicelevel;
  char   *realhost;

  double dtime;
} Job;

# define CONTROLLER_PROMPT "pcontrol:"

/* scheduler prototypes */

void InitPantasks (void);
void FreePantasks (void);

void InitPantasksServer (void);
void FreePantasksServer (void);

void InitPantasksClient (void);
void FreePantasksClient (void);

void InitTasks (void);
void FreeTasks (void);

Task *NextTask (void);
Task *FindTask (char *name);
void ListTasks (int verbose);
int ShowTask (char *name);
int FreeTask (Task *task);
Task *CreateTask (char *name);
int ValidateTask (Task *task, int RequireStatic);
int RegisterNewTask (void);
int DeleteNewTask (void);
Task *GetNewTask (void);
Task *GetActiveTask (void);
void SetTaskTimer (struct timeval *timer);
double GetTaskTimer (struct timeval start, int verbose);
void InitTaskTimers (void);
int TaskHash (char *input);
int RemoveTask (Task *task);
Task *SetNewTask (Task *task);
void ListTaskStats (char *regex);
void ResetTaskStats (char *regex);
void UpdateTaskTimerStats (Task *task, int mode, double dtime);

IDtype NextJobID (void);
void InitJobIDs (void);
void FreeJobIDs (void);

void InitJobs (void);
void FreeJobs (void);

Job *NextJob (void);
Job *FindJob (IDtype JobID);
void ListJobs (void);
Job *CreateJob (Task *task);
int SubmitJob (Job *job);
int CheckJob (Job *job);
int DeleteJob (Job *job);
void FreeJob (Job *job);
char *JobStateToString (JobStat state);

float CheckJobs (void);
float CheckTasks (void);
int CheckSystem (void);
int CheckController (void);
int CheckTimeRanges (TimeRange *ranges, int Nranges);
int BumpTimeRanges (TimeRange *ranges, int Nranges);

int GetJobOutput (char *channel, int pid, IOBuffer *buffer, int Nbytes);
int CheckControllerJob (Job *job);
int CheckControllerJobStatus (Job *job);
int SubmitControllerJob (Job *job);
int DeleteControllerJob (Job *job);
Job *FindControllerJob (IDtype JobID);
int StartController (void);
int ControllerCommand (char *command, char *response, IOBuffer *buffer);
int SubmitLocalJob (Job *job);
int CheckLocalJob (Job *job);
int CheckLocalJobStatus (Job *job);
void InitTaskTimers (void);
CommandF *FindControllerCommand (char *cmd);
int QuitController (void);
int StopController (void);
int RestartController (void);
int VerboseMode (void);
int KillLocalJob (Job *job);
int CheckControllerOutput (void);
int PrintControllerOutput (void);
void PrintControllerBusyJobs ();

int AddHost (char *hostname, int max_threads);
int DeleteHost (char *hostname);

int FlushControllerOutput (void);
int KillControllerJob (Job *job);
int CheckControllerStatus (void);
void gotsignal (int signum);
int client_shell (int argc, char **argv);

void InitClients (void);
void AddNewClient (int client);
int  DeleteClient (int client);
void *ListenClients (void *data);
void QuitClientThread (void);

// functions related to the server threads
void *CheckJobsAndTasksThread (void *data);
void QuitJobsAndTasksThread (void);
void QuitControllerThread (void);

void CheckTasksSetState (int state);
int CheckTasksGetState (void);

void CheckJobsSetState (int state);
int CheckJobsGetState (void);

void CheckControllerSetState (int state);
int CheckControllerGetState (void);
void *CheckControllerThread (void *data);

void CheckInputsSetState (int state);
int CheckInputsGetState (void);
void *CheckInputsThread (void *data);

// functions related to the queue of input files
void InitInputs (void);
void FreeInputs (void);
void AddNewInput (char *input);
int DeleteInput (char *input);
void CheckInputs (void);

void ClientLock (void);
void ClientUnlock (void);
void CommandLock (void);
void CommandUnlock (void);
void ControlLock (const char *func);
void ControlUnlock (const char *func);
void JobTaskLock (void);
void JobTaskUnlock (void);

int InitPassword (void);
int CheckPassword (int BindSocket);

int FlushJobs (void);
