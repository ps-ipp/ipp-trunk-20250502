# include "pcontrol.h"

// XXX unify by testing for value of argv[0]
int stdout_pc (int argc, char **argv) {

  int N, JobID, StackID;
  Job *job;
  IOBuffer *buffer;
  char *varName;

  varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: stdout (JobID) [-var name]\n");
    gprint (GP_LOG, "STATUS %d\n", -1);
    return (FALSE);
  }
  
  /* find Job of interest (must be EXIT or CRASH) */
  JobID = atoi (argv[1]);

  StackID = PCONTROL_JOB_EXIT;
  job = PullJobFromStackByID (StackID, JobID);
  if (job != NULL) goto found_stdout;

  StackID = PCONTROL_JOB_CRASH;
  job = PullJobFromStackByID (StackID, JobID);
  if (job != NULL) goto found_stdout;

  gprint (GP_ERR, "job not found in EXIT or CRASH\n");
  if (varName == NULL) {
    gprint (GP_LOG, "STATUS %d\n", -2);
  } else {
    set_str_variable (varName, "NULL");
    free (varName);
  }
  return (FALSE);

found_stdout:
  buffer = &job[0].stdout_buf.buffer;
  if (varName == NULL) {
    fwrite (buffer[0].buffer, 1, buffer[0].Nbuffer, stdout);
    gprint (GP_LOG, "STATUS %d\n", 0);
  } else {
    // XXX this can drop '0' values
    set_str_variable (varName, buffer[0].buffer);
    free (varName);
  }
  PutJob (job, StackID, STACK_BOTTOM);
  return (TRUE);
}

int stderr_pc (int argc, char **argv) {

  int N, JobID, StackID;
  Job *job;
  IOBuffer *buffer;
  char *varName;

  varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: stderr (JobID) [-var name]\n");
    gprint (GP_LOG, "STATUS %d\n", -1);
    return (FALSE);
  }
  
  /* find Job of interest (must be EXIT or CRASH) */
  JobID = atoi (argv[1]);

  StackID = PCONTROL_JOB_EXIT;
  job = PullJobFromStackByID (StackID, JobID);
  if (job != NULL) goto found_stderr;

  StackID = PCONTROL_JOB_CRASH;
  job = PullJobFromStackByID (StackID, JobID);
  if (job != NULL) goto found_stderr;

  gprint (GP_ERR, "job not found in EXIT or CRASH\n");
  if (varName == NULL) {
    gprint (GP_LOG, "STATUS %d\n", -2);
  } else {
    set_str_variable (varName, "NULL");
    free (varName);
  }
  return (FALSE);

found_stderr:
  buffer = &job[0].stderr_buf.buffer;
  if (varName == NULL) {
    fwrite (buffer[0].buffer, 1, buffer[0].Nbuffer, stdout);
    gprint (GP_LOG, "STATUS %d\n", 0);
  } else {
    // XXX this can drop '0' values
    set_str_variable (varName, buffer[0].buffer);
    free (varName);
  }
  PutJob (job, StackID, STACK_BOTTOM);
  return (TRUE);
}
