/*** shell.h ***/
# include "external.h"

# ifndef SHELL_H
# define SHELL_H

# define ESCAPE(MSG,...) { gprint (GP_ERR, MSG, __VA_ARGS__); return FALSE; }

# define ISVEC(a) (isalnum (a) || (a == ':') || (a == '_') || (a == '.'))
# define ISVAR(a) (isalnum (a) || (a == ':') || (a == '_'))
# define ISREF(a) (isalnum (a) || (a == '_'))
# define ISWORD(a,q) ((q) ? (a != '"') : (isalnum(a) || (a == '/') || (a == '.') || (a == '_') || (a == '-')))
# define ISNUM(c) (isdigit(c) || (c == '-') || (c == '.'))

# define MACRO_STRING(s) #s
# define MACRO_NAME(s) MACRO_STRING(s)

typedef enum {GP_FILE, GP_BUFF} gpMode;
#ifdef NOT_MOVED_TO_LIBOHANA
/* enums used by gprint functions */
typedef enum {GP_LOG, GP_ERR} gpDest;
#endif
typedef enum {OPIHI_VERBOSE_OFF, OPIHI_VERBOSE_ON, OPIHI_VERBOSE_ERROR} OpihiVerboseMode;

typedef int CommandF (int argc, char **argv);

typedef struct sockaddr_in SockAddress;

/*** typedef structs used by shell functions ***/
typedef struct {			/* basic opihi command structure */
  char      real;
  char     *name;
  CommandF *func;    
  char     *help;
} Command;

typedef struct {			/* a macro (collection of commands) */
  char   *name;
  char  **line;
  int     Nlines;
} Macro;

typedef struct {			/* a list (macro/loop currently being executed) */
  char **line;
  int    n;
  int    Nlines;
  int    Nalloc;
} List;

/* structure used to represent the gprint i/o stream */
typedef struct {
  FILE *file;
  char *name;
  IOBuffer *buffer;
  gpMode mode;
  gpDest dest;
  pthread_t thread;
} gpStream;

/*** globals used to track the shell language concepts  ***/
int 	     interrupt;			/* true if C-C has been pressed */
int 	     auto_break;		/* if true, zero exit status forces macros to escape */
int 	     loop_next; 		/* set to true when next (or continue) is called */
int 	     loop_last; 		/* set to true when last (or return) is called */
int 	     loop_break;		/* set to true when break is called */
int          is_script;                 /* being run within a shell script */

/*** basic opihi shell functions ***/
void          general_init            	PROTO((int *argc, char **argv));
void          program_init            	PROTO((int *argc, char **argv));
void          startup               	PROTO((int *argc, char **argv));
int           opihi                     PROTO((int argc, char **argv));
int           multicommand          	PROTO((char *line));
int getServer (void);
void          multicommand_InitServer   PROTO((void));
void multicommand_StopServer (void);

int           command               	PROTO((char *line, char **outline, int VERBOSE));
void          set_verbose_shell         PROTO((int mode));
int           get_verbose_shell         PROTO((void));

char         *expand_vars           	PROTO((char *line));
char         *expand_vectors        	PROTO((char *line));
char         *parse                 	PROTO((int *status, char *line));
char        **parse_commands        	PROTO((char *, int *));
void          welcome                   PROTO((void));

int           add_listentry             PROTO((int ThisList, char *line));
int           is_for_loop           	PROTO((char *line));
int           is_if_block           	PROTO((char *line));
int           is_list               	PROTO((char *line));
int           is_loop               	PROTO((char *line));
int           is_task               	PROTO((char *line));
int           is_task_exit             	PROTO((char *line));
int           is_task_exec             	PROTO((char *line));
int           is_macro_create       	PROTO((char *line));
void          InitLists                 PROTO((void));
int current_list_depth (void);
int increase_list_depth (void);
int decrease_list_depth (void);
char *get_next_listentry (int ThisList);

void          InitCommands              PROTO((void));
void          AddCommand                PROTO((Command *new));
int           DeleteCommand             PROTO((Command *command));
Command      *MatchCommand              PROTO((char *name, int VERBOSE, int EXACT));
void          sort_commands             PROTO((int *seq));

void          SetCurrentMacroData	PROTO((char *name, int depth));
Macro        *NewMacro			PROTO((char *name));
int           DeleteMacro		PROTO((Macro *macro));
Macro        *MatchMacro		PROTO((char *name, int VERBOSE, int EXACT));
void          InitMacros                PROTO((void));
char         *GetMacroName              PROTO((void));
int           GetMacroDepth             PROTO((void));
void          ListMacro                 PROTO((Macro *macro));
void          ListMacros                PROTO((void));
void          FreeMacro                 PROTO((Macro *macro));
CommandF     *find_macro_command        PROTO((char *name));

int           exec_loop                 PROTO((Macro *loop));
/* char         *get_next_listentry    	PROTO((int ThisList)); */
/* char         *remove_listentry      	PROTO((int current)); */

int           ConfigInit            	PROTO((int *argc, char **argv));
void          ConfigFree            	PROTO((void));
char         *VarConfig             	PROTO((char *keyword, char *mode, void *ptr));
char         *VarConfigEntry           	PROTO((char *keyword, char *mode, int entry, void *ptr));

#ifndef NOT_MOVED_TO_DVO
int           init_error                PROTO((void));
int           push_error                PROTO((char *line));
int           print_error               PROTO((void));
#endif

struct sigaction *SetInterrupt          PROTO((void));
int           ClearInterrupt            PROTO((struct sigaction *old_sigaction));
void          handle_interrupt      	PROTO((int));
char        **command_completer     	PROTO((const char *, int, int));
char         *command_generator     	PROTO((const char *text, int state));
char        **completion_matches    	PROTO((char *, rl_compentry_func_t *));
void          print_commands            PROTO((FILE *f));
void          InitCommands              PROTO((void));

/* command line parsing */
char         *thisword              	PROTO((char *));
char         *nextword              	PROTO((char *));
char         *lastword              	PROTO((char *, char *));
char         *thisvar               	PROTO((char *));
char         *thisref               	PROTO((char *));
char         *aftervar              	PROTO((char *));
char         *lastvar               	PROTO((char *, char *));
char         *thiscomm              	PROTO((char *));
char         *nextcomm              	PROTO((char *));
char         *opihi_append              PROTO((char *output, int *Noutput, char *start, char *stop));
void          interpolate_slash         PROTO((char *line));
char         *paste_args                PROTO((int argc, char **argv));

/* macro functions (mapped to commands) */
int 	      macro_create 		PROTO((int, char **)); 
int 	      macro_delete 		PROTO((int, char **)); 
int 	      macro_edit   		PROTO((int, char **));
int 	      macro_exec   		PROTO((int, char **));
int 	      macro_list_f 		PROTO((int, char **));  /* "macro_list" is a readline func */
int 	      macro_read   		PROTO((int, char **));
int 	      macro_write   		PROTO((int, char **));

char 	     *memstr        		PROTO((char *m1, char *m2, int n));
int  	      write_fmt     		PROTO((int fd, char *format, ...)) OHANA_FORMAT(printf, 2, 3);
char 	     *opihi_version 		PROTO((void));
char 	     *strip_version 		PROTO((char *input));

// wrap readline in ohana mem functions:
char         *opihi_readline            PROTO((char *prompt));

int set_list_varname (char *line, char *base, int N, int excelStyle);

/* gprint functions */
void          gprintInit      		PROTO((void));
gpStream     *gprintGetStream 		PROTO((gpDest dest));
void          gprintSetBuffer 		PROTO((gpDest dest));
IOBuffer     *gprintGetBuffer 		PROTO((gpDest dest));
void          gprintSetFileAllThreads   PROTO((gpDest dest, char *filename));
void          gprintSetFileThisThread   PROTO((gpDest dest, char *filename));
void          gprintSetFile   		PROTO((gpStream *stream, gpDest dest, char *filename));
FILE         *gprintGetFile   		PROTO((gpDest dest));
char         *gprintGetName   		PROTO((gpDest dest));
#ifdef NOT_MOVED_TO_LIBOHANA
int           gprint          		PROTO((gpDest dest, char *format, ...)) OHANA_FORMAT(printf, 2, 3);
#endif
int           gwrite          		PROTO((char *buffer, int size, int N, gpDest dest));
int           gprint_syserror           PROTO((gpDest dest, int myError, char *format, ...)) OHANA_FORMAT(printf, 3, 4);
int           gprintv                   PROTO((gpDest dest, char *format, va_list argp));

/* socket functions */
int InitServerSocket (SockAddress *Address, char *hostname, char *portinfo);
int WaitServerSocket (int InitSocket, SockAddress *Address);
int GetClientSocket (char *hostname, char *portinfo);
int InitServerSocket_Named (char *hostname, SockAddress *Address);
int DefineValidIP (void);

void FreeCommands (void);
void FreeMacros (void);
void FreeBuffers (void);
void FreeVectors (void);
void FreeVariables (void);
void FreeLists (void);

# endif
