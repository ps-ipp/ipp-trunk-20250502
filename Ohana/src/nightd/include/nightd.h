# include <ohana.h>
# include <signal.h>
# include <errno.h>

# define MY_MAX_PATH 256

FILE *LogFile;
char *Program;

char logfile[MY_MAX_PATH];
char **MainCommand;
char **InitCommand;
char **DoneCommand;

char TestCommand[MY_MAX_PATH];

char DateStr[MY_MAX_PATH];
char TimeStr[MY_MAX_PATH];

char PIDFile[MY_MAX_PATH];

float NightStart;
float NightStop;

int SuspendAction;

int ConfigInit (int *argc, char **argv);
void LoadConfig (int sig);
int dms_to_ddd (double *Value, char *string);
int SetPID (pid_t *Xpid, char *Xuser, char *Xmachine);
int GetDateTime (char *datestr, char *timestr, float *time);
int WaitForMinute (void);
int GetStatus (void);
int ResetConfig (void);
int SendShutdown (void);
int StartUp (void);
int SetSignals (void);
void ToggleSuspend (int sig);
void Shutdown (int sig);
int Expose (char *filename);
int vsystem (char *line);
char *ExpandWords (char *line);
int pcommand (char *line, int timeout);
int WaitForPeriod (void);
int DoCommand (char *command, char *name);
int freeargs (char **arglist);

int PERIOD;
int TIMEOUT;
