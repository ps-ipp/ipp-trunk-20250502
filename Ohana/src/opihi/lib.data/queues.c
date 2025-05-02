# include "data.h"
# define DEBUG 0

Queue **queues;   /* queue to store the list of all queues */
int    Nqueues;   /* number of currently defined queues */
int    NQUEUES;   /* number of currently allocated queues */

void InitQueues () {
  Nqueues = 0;
  NQUEUES = 16;
  ALLOCATE (queues, Queue *, NQUEUES); 
}

void FreeQueues () {

  int i, j;

  for (i = 0; i < Nqueues; i++) {
    for (j = 0; j < queues[i][0].Nlines; j++) {
      free (queues[i][0].lines[j]);
    }
    free (queues[i][0].lines);
    free (queues[i][0].name);
    free (queues[i]);
  }
  free (queues);
}

/* list known queues */
void ListQueues () {

  int i;

  for (i = 0; i < Nqueues; i++) {
    gprint (GP_LOG, "%-15s %3d\n", queues[i][0].name, queues[i][0].Nlines);
  }
  return;
}

/* return the given queue */
Queue *FindQueue (char *name) {

  int i;

  for (i = 0; i < Nqueues; i++) {
    if (!strcmp (queues[i][0].name, name)) {
      return (&queues[i][0]);
    }
  }
  return (NULL);
}

/* make a new named queue */
int InitQueue (Queue *queue) {

  int i;

  for (i = 0; i < queue[0].Nlines; i++) {
    free (queue[0].lines[i]);
  }
  queue[0].Nlines = 0;
  queue[0].NLINES = 16;
  REALLOCATE (queue[0].lines, char *, queue[0].NLINES);

  if (DEBUG) fprintf (stderr, "init: %s (%zx) : %d of %d\n", queue[0].name, (size_t) queue, queue[0].Nlines, queue[0].NLINES);
  
  return (TRUE);
}

/* make a new named queue */
Queue *CreateQueue (char *name) {

  int N;
  Queue *queue;

  queue = FindQueue (name);
  if (queue != NULL) return (queue);

  N = Nqueues;
  Nqueues ++;
  CHECK_REALLOCATE (queues, Queue *, NQUEUES, Nqueues, 16);
  ALLOCATE (queue, Queue, 1);
  queue[0].Nlines = 0;
  queue[0].NLINES = 16;
  queue[0].name = strcreate (name);
  ALLOCATE (queue[0].lines, char *, queue[0].NLINES);
  queues[N] = queue;
  return (queue);
}

/* delete a queue */
int DeleteQueue (Queue *queue) {

  int i, N, NQUEUES_2;

  /* find queue in queue list */
  N = -1;
  for (i = 0; i < Nqueues; i++) {
    if (queues[i] == queue) {
      N = i;
      break;
    }
  }
  if (N == -1) return (FALSE);

  for (i = N; i < Nqueues - 1; i++) {
    queues[i] = queues[i + 1];
  }
  Nqueues --;
  NQUEUES_2 = MAX (16, NQUEUES / 2);
  if (Nqueues < NQUEUES_2) {
    NQUEUES = NQUEUES_2;
    REALLOCATE (queues, Queue *, NQUEUES);
  }

  free (queue[0].name);
  for (i = 0; i < queue[0].Nlines; i++) {
    free (queue[0].lines[i]);
  }
  free (queue[0].lines);
  free (queue);
  return (TRUE);
}

void PushNamedQueue (char *name, char *line) {

  Queue *queue;
  
  queue = FindQueue (name);
  if (queue == NULL) {
    queue = CreateQueue (name);
  }
  PushQueue (queue, line);
  return;
}

/* push line onto queue.  return chars create new lines */
void PushQueue (Queue *queue, char *line) {

  int N;
  char *p, *q;

  p = line;
  q = strchr (line, '\n');
  N = queue[0].Nlines;
  while (q != NULL) {
    queue[0].lines[N] = strncreate (p, q - p);
    N++;
    CHECK_REALLOCATE (queue[0].lines, char *, queue[0].NLINES, N, 16);
    p = q + 1;
    q = strchr (p, '\n');
  }    
  if (*p) {
    queue[0].lines[N] = strcreate (p);
    N++;
    CHECK_REALLOCATE (queue[0].lines, char *, queue[0].NLINES, N, 16);
  }
  queue[0].Nlines = N;
  return;
}

// return a newly allocated string containing the requested key value
char *ChooseSingleKey (char *line, int Key) {

  int i;
  char *key, *p;

  if (Key == -1) {
    key = strcreate (line);
    return (key);
  }

  key = line;
  for (i = 0; (i < Key) && (key != NULL); i++) {
    p = nextword (key);
    key = p;
  }
  key = thisword (key);
  return (key);
}

/* construct merged key given keylist of the form K:N:M */ 
char *ChooseKey (char *line, char *keylist) {

  char *output, *entry, *key, *p, *q;
  int first, keynum;

  if (line == NULL) return (NULL);
  if (keylist == NULL) return (line);

  ALLOCATE (output, char, strlen(line) + 1);
  memset (output, 0, strlen(line) + 1);

  first = TRUE;
  p = q = keylist;
  while (q != NULL) {
    q = strchr (p, ':');
    if (q == NULL) {
      entry = strcreate (p);
    } else {
      entry = strncreate (p, q - p);
    }
    keynum = atoi (entry);
    free (entry);

    key = ChooseSingleKey (line, keynum);

    if (!first) strcat (output, ":");
    if (key) {
	strcat (output, key);
	free (key);
    }

    if (q != NULL) p = q + 1;
    first = FALSE;
  }
  return (output);
}

/* push line onto queue, skipping existing matches (optionally by key) */
void PushQueueUnique (Queue *queue, char *line, char *Key) {

  int i, j, N, found;
  char *p, *q, *key1, *key2;
  Queue tmp;

  /* init tmp queue */
  tmp.Nlines = 0;
  tmp.NLINES = 16;
  ALLOCATE (tmp.lines, char *, tmp.NLINES);

  /* push entries on tmp queue */
  p = line;
  q = strchr (line, '\n');
  N = tmp.Nlines;
  while (q != NULL) {
    tmp.lines[N] = strncreate (p, q - p);
    N++;
    CHECK_REALLOCATE (tmp.lines, char *, tmp.NLINES, N, 16);
    p = q + 1;
    q = strchr (p, '\n');
  }    
  if (*p) {
    tmp.lines[N] = strcreate (p);
    N++;
    CHECK_REALLOCATE (tmp.lines, char *, tmp.NLINES, N, 16);
  }
  tmp.Nlines = N;

  /* add unique entries in tmp to queue */
  for (i = 0; i < tmp.Nlines; i++) {
    key1 = ChooseKey (tmp.lines[i], Key);
    if (key1 == NULL) continue;
    found = FALSE;
    for (j = 0; !found && (j < queue[0].Nlines); j++) {
      key2 = ChooseKey (queue[0].lines[j], Key);
      if (key2 == NULL) continue;
      found = !strcmp (key1, key2);
      free (key2);
    }      
    if (!found) PushQueue (queue, tmp.lines[i]);
    free (key1);
  }
  for (i = 0; i < tmp.Nlines; i++) {
    free (tmp.lines[i]);
  } 
  free (tmp.lines);
  return;
}

/* push line onto queue, replacing matches (optionally by Key) */
void PushQueueReplace (Queue *queue, char *line, char *Key) {

  int i, j, N, found;
  char *p, *q, *key1, *key2;
  Queue tmp;

  /* init tmp queue */
  tmp.Nlines = 0;
  tmp.NLINES = 16;
  ALLOCATE (tmp.lines, char *, tmp.NLINES);

  /* push entries on tmp queue */
  p = line;
  q = strchr (line, '\n');
  N = tmp.Nlines;
  while (q != NULL) {
    tmp.lines[N] = strncreate (p, q - p);
    N++;
    CHECK_REALLOCATE (tmp.lines, char *, tmp.NLINES, N, 16);
    p = q + 1;
    q = strchr (p, '\n');
  }    
  if (*p) {
    tmp.lines[N] = strcreate (p);
    N++;
    CHECK_REALLOCATE (tmp.lines, char *, tmp.NLINES, N, 16);
  }
  tmp.Nlines = N;

  /* add unique entries in tmp to queue */
  for (i = 0; i < tmp.Nlines; i++) {
    key1 = ChooseKey (tmp.lines[i], Key);
    if (key1 == NULL) continue;
    found = FALSE;
    for (j = 0; !found && (j < queue[0].Nlines); j++) {
      key2 = ChooseKey (queue[0].lines[j], Key);
      if (key2 == NULL) continue;
      found = !strcmp (key1, key2);
      if (found) {
	// XXX do I need to free queue[0].lines[j]??
	queue[0].lines[j] = strcreate (tmp.lines[i]);
      }
      free (key2);
    }      
    if (!found) PushQueue (queue, tmp.lines[i]);
    free (key1);
  }
  for (i = 0; i < tmp.Nlines; i++) {
    free (tmp.lines[i]);
  } 
  free (tmp.lines);
  return;
}

char *PopQueue (Queue *queue) {

  int i, NLINES_2;
  char *line;

  if (queue[0].Nlines == 0) return (NULL);
  line = queue[0].lines[0];

  for (i = 0; i < queue[0].Nlines - 1; i++) {
    queue[0].lines[i] = queue[0].lines[i+1];
  }
  queue[0].Nlines --;

  /* shrink queue allocation if small enough */
  NLINES_2 = MAX (16, queue[0].NLINES / 2);
  if (queue[0].Nlines < NLINES_2) {
    queue[0].NLINES = NLINES_2;
    REALLOCATE (queue[0].lines, char *, queue[0].NLINES);
  }    
  return (line);
}

/* pop the first entry which for which the key matches */
char *PopQueueMatch (Queue *queue, char *Key, char *value) {

  int i, choice, NLINES_2;
  char *line, *test;

  if (queue[0].Nlines == 0) return (NULL);

  /* find the matching key */
  choice = -1;
  for (i = 0; (i < queue[0].Nlines) && (choice == -1); i++) {
      test = ChooseKey (queue[0].lines[i], Key);
      if (test == NULL) continue;
      if (strcmp (value, test)) { 
	free (test);
	continue;
      }
      free (test);
      choice = i;
  }
  if (choice == -1) return NULL;

  line = queue[0].lines[choice];

  for (i = choice; i < queue[0].Nlines - 1; i++) {
    queue[0].lines[i] = queue[0].lines[i+1];
  }
  queue[0].Nlines --;

  /* shrink queue allocation if small enough */
  NLINES_2 = MAX (16, queue[0].NLINES / 2);
  if (queue[0].Nlines < NLINES_2) {
    queue[0].NLINES = NLINES_2;
    REALLOCATE (queue[0].lines, char *, queue[0].NLINES);
  }    
  return (line);
}

int PrintQueue (Queue *queue) {

  int i;

  if (queue[0].Nlines == 0) return (TRUE);

  if (DEBUG) fprintf (stderr, "print: %s (%zx) : %d of %d\n", queue[0].name, (size_t) queue, queue[0].Nlines, queue[0].NLINES);

  for (i = 0; i < queue[0].Nlines; i++) {
    gprint (GP_LOG, "%s\n", queue[0].lines[i]);
  }
  return (TRUE);
}

int SaveQueue (Queue *queue, char *filename) {

  if (queue[0].Nlines == 0) return (TRUE);

  FILE *f = fopen (filename, "a");
  if (!f) {
    gprint (GP_ERR, "unable to open output file %s\n", filename);
    return FALSE;
  }

  for (int i = 0; i < queue[0].Nlines; i++) {
    fprintf (f, "%s\n", queue[0].lines[i]);
  }
  fclose (f);
  return (TRUE);
}

