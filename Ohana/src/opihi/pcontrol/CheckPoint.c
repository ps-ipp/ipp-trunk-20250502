# include "pcontrol.h"

// one thread (user) interacts with the user and blocks for long periods on input.
// one thread (client) spins continuously and monitors the hosts and jobs
// in some cases, the user thread needs to set a check point on the client thread 
// to ensure the all HOSTs and JOBs are on one of the stacks (nothing 'in flight').
// these are not symmetric: the client thread should not call Set/Clear, the user 
// thread should not call Test

# ifdef THREADED
static pthread_mutex_t client = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t user = PTHREAD_MUTEX_INITIALIZER;
# endif

// The user thread calls this to stop the client thread from shuffling the Host/Job stacks
int SetCheckPoint () {

# ifdef THREADED
  int status;
  int Nwait;

  // set my lock
  pthread_mutex_lock (&user);

  // wait until client thread sets its lock
  Nwait = 0;
  while (1) {
    status = pthread_mutex_trylock (&client);
    if (status == EBUSY) {
      // client has reached the check-point
      return (TRUE);
    }
    pthread_mutex_unlock (&client);
    usleep (10000); // wait for client thread to set lock
    Nwait ++;
  }
  // put in a timeout?  (client thread not spinning...)
  return (FALSE);
# else
  return (TRUE);
# endif
}

// The user thread calls this to allow the client thread to continue
int ClearCheckPoint () {
  // clear my lock
# ifdef THREADED
  pthread_mutex_unlock (&user);
# endif
  return (TRUE);
}

// The client thread calls in the thread loop somewhere the stacks are stable
// (ie, no jobs or hosts currently in flight)
int TestCheckPoint () {

# ifdef THREADED
  // set my lock
  pthread_mutex_lock (&client);
  
  // try the user-thread lock
  pthread_mutex_lock (&user);
  pthread_mutex_unlock (&user);

  // clear my lock
  pthread_mutex_unlock (&client);
# endif
  return (TRUE);
}
