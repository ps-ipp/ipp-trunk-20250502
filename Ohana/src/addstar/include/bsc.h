
# define NMEAS_MAX 3

typedef struct {
  double R, D;
  Average average;
  Measure measure[NMEAS_MAX];
  int Nmeasure;
  int flag;
} BSC_Stars;

int USE_PS1_EPOCH;

AddstarClientOptions args_loadbsc (int *argc, char **argv, AddstarClientOptions options);

int loadbsc_rawdata (SkyList *skytable, char *filename, AddstarClientOptions options);

int getbsc_setup ();
int getbsc_sortStars (BSC_Stars *tstars, int Ntstars);
int getbsc_star (BSC_Stars *star, char *line);
