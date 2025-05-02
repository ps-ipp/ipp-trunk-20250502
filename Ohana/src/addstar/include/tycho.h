
# define NGROUP 6

typedef struct {
  double R, D;
  Average average;
  Measure measure[NGROUP];
  int flag;
} Tycho_Stars;

int USE_PS1_EPOCH;

AddstarClientOptions args_loadtycho (int *argc, char **argv, AddstarClientOptions options);

int loadtycho_rawdata (SkyList *skytable, char *filename, AddstarClientOptions options);

int gettycho_setup ();
int gettycho_sortStars (Tycho_Stars *tstars, int Ntstars);
int gettycho_star (Tycho_Stars *star, char *line);
