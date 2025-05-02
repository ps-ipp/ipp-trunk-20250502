# include "pcontrol.h"

int check (int argc, char **argv) {

  int N, Save;
  int JobID, HostID;

  Stack *stack = NULL;
  Job *job = NULL;
  Host *host = NULL;

  Save = FALSE;
  if ((N = get_argument (argc, argv, "-save"))) {
    remove_argument (N, &argc, argv);
    Save = TRUE;
  }

  if (argc != 3) {
    gprint (GP_LOG, "USAGE: check job (JobID)\n");
    gprint (GP_LOG, "USAGE: check host (HostID)\n");
    return (FALSE);
  }

  if (!strcasecmp (argv[1], "JOB")) {
    JobID = GetID (argv[2]);
    if (!JobID) {
      gprint (GP_ERR, "invalid job id %s\n", argv[2]);
      return (FALSE);
    }

    stack = GetJobStack (PCONTROL_JOB_ALLJOBS);
    job = PullStackByID (stack, JobID);
    if (job == NULL) {
      gprint (GP_LOG, "job not found\n");
      return (FALSE);
    }

    gprint (GP_LOG, "STATUS %s\n", GetJobStackName(job[0].stack));
    gprint (GP_LOG, "EXITST %d\n", job[0].exit_status);
    gprint (GP_LOG, "STDOUT %d\n", job[0].stdout_buf.size);
    gprint (GP_LOG, "STDERR %d\n", job[0].stderr_buf.size);
    gprint (GP_LOG, "DTIME %lf\n", job[0].dtime);
    if (job[0].realhost) {
	gprint (GP_LOG, "HOSTNAME %s\n", job[0].realhost);
    } else {
	gprint (GP_LOG, "HOSTNAME NONE\n");
    }

    if (Save) {
	set_str_variable ("JOB_STATUS", GetJobStackName(job[0].stack));
	set_int_variable ("JOB_EXITST", job[0].exit_status);
	set_int_variable ("JOB_STDOUT_SIZE", job[0].stdout_buf.size);
	set_int_variable ("JOB_STDERR_SIZE", job[0].stderr_buf.size);
	set_variable ("JOB_DTIME", job[0].dtime);
	set_str_variable ("JOB_HOSTNAME", job[0].hostname);
	if (job[0].realhost) {
	    set_str_variable ("JOB_REALHOST", job[0].realhost);
	} else {
	    set_str_variable ("JOB_REALHOST", "NONE");
	}
    }

    PushStack (stack, STACK_BOTTOM, job, job[0].JobID, job[0].argv[0]);
    return (TRUE);
  }

  if (!strcasecmp (argv[1], "HOST")) {
    HostID = atoi (argv[2]);

    stack = GetHostStack (PCONTROL_HOST_ALLHOSTS);
    host = PullStackByID (stack, HostID);
    if (host == NULL) {
      gprint (GP_LOG, "host not found\n");
      return (FALSE);
    }
    gprint (GP_LOG, "host %s\n", GetHostStackName(host[0].stack));

    if (Save) {
	set_str_variable ("HOST_STATE", GetHostStackName(host[0].stack));
    }

    PushStack (stack, STACK_BOTTOM, host, host[0].HostID, host[0].hostname);
    return (TRUE);
  }

  gprint (GP_LOG, "unknown item to check\n");
  return (FALSE);
}
