# include "data.h"
# include "basic.h"

/** pclient global data **/

/** child status values **/
enum {
  PCLIENT_NONE,
  PCLIENT_BUSY,  
  PCLIENT_EXIT,
  PCLIENT_CRASH,
};

int ChildPID;				/** current child PID **/
int ChildStatus;			/** current status of child **/
int ChildExitStatus;			/** recent exit status of child **/

/** child may be in the following states:
    
PCLIENT_NONE
PCLIENT_BUSY
PCLIENT_EXIT
PCLIENT_CRASH

allowed transitions:

PCLIENT_NONE  ->    PCLIENT_BUSY  (start a job)

PCLIENT_BUSY  ->    PCLIENT_EXIT  (job complete)
PCLIENT_BUSY  ->    PCLIENT_CRASH (job crashed)

PCLIENT_EXIT  ->    PCLIENT_NONE  (cleanup)
PCLIENT_CRASH ->    PCLIENT_NONE  (cleanup)

**/

/** file descriptors for child I/O **/
int child_stdin_fd[2];
int child_stdout_fd[2];
int child_stderr_fd[2];

/** buffers for I/O storage **/
IOBuffer child_stdout;
IOBuffer child_stderr;

int InitChild (void);
int FreeChild (void);
int CheckChild (void);
void CheckChildStatus (void);

void InitPclient (void);
void FreePclient (void);
void gotsignal (int signum);
void pipe_signal (int signum);
void pipe_signal_clear(void);

# define DTIME(A,B) ((A.tv_sec - B.tv_sec) + 1e-6*(A.tv_usec - B.tv_usec))
