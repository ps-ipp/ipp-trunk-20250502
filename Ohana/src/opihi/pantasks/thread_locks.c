# include "pantasks.h"

/* mutex to lock Client table operations */
static pthread_mutex_t ClientMutex = PTHREAD_MUTEX_INITIALIZER;

void ClientLock (void) {
  pthread_mutex_lock (&ClientMutex);
}

void ClientUnlock (void) {
  pthread_mutex_unlock (&ClientMutex);
}

/* mutex to lock Command / Multicommand operations */
static pthread_mutex_t CommandMutex = PTHREAD_MUTEX_INITIALIZER;

void CommandLock (void) {
  //  fprintf (stderr, "command lock\n");
  pthread_mutex_lock (&CommandMutex);
}

void CommandUnlock (void) {
  //  fprintf (stderr, "command unlock\n");
  pthread_mutex_unlock (&CommandMutex);
}

/* mutex to lock Control operations */
static pthread_mutex_t ControlMutex = PTHREAD_MUTEX_INITIALIZER;

void ControlLock (const char *func) {
  OHANA_UNUSED_PARAM(func);
  // fprintf (stderr, "control lock %s\n", func);
  pthread_mutex_lock (&ControlMutex);
}

void ControlUnlock (const char *func) {
  OHANA_UNUSED_PARAM(func);
  // fprintf (stderr, "control unlock %s\n", func);
  pthread_mutex_unlock (&ControlMutex);
}

/* mutex to lock Job / Task operations */
static pthread_mutex_t JobTaskMutex = PTHREAD_MUTEX_INITIALIZER;

void JobTaskLock (void) {
  //  fprintf (stderr, "jobtask lock\n");
  pthread_mutex_lock (&JobTaskMutex);
}

void JobTaskUnlock (void) {
  //  fprintf (stderr, "jobtask unlock\n");
  pthread_mutex_unlock (&JobTaskMutex);
}

