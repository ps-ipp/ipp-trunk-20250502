#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <pslib.h>
#include <psPipe.h>

void closePipes (int *stdin_fd, int *stdout_fd, int *stderr_fd)
{

    if (stdin_fd[0]  != 0)
        close (stdin_fd[0]);
    if (stdin_fd[1]  != 0)
        close (stdin_fd[0]);
    if (stdout_fd[0] != 0)
        close (stdout_fd[0]);
    if (stdout_fd[1] != 0)
        close (stdout_fd[1]);
    if (stderr_fd[0] != 0)
        close (stderr_fd[0]);
    if (stderr_fd[1] != 0)
        close (stderr_fd[1]);
}

static void psPipeFree (psPipe *pipe)
{
    return;
}

psPipe *psPipeAlloc (void)
{
    psPipe *pipe = (psPipe *)psAlloc(sizeof(psPipe));
    psMemSetDeallocator(pipe, (psFreeFunc) psPipeFree);

    pipe->fd_stdin  = 0;
    pipe->fd_stdout = 0;
    pipe->fd_stderr = 0;
    return (pipe);
}

psPipe *psPipeOpen (char *command)
{

    int stdin_fd[2], stdout_fd[2], stderr_fd[2], status;
    pid_t pid;

    memset(stdin_fd,  '\0', 2*sizeof(int));
    memset(stdout_fd, '\0', 2*sizeof(int));
    memset(stderr_fd, '\0', 2*sizeof(int));

    if (pipe (stdin_fd)  < 0) {
        psError (PS_ERR_UNKNOWN, true, "cannot create pipe file descriptor");
        closePipes (stdin_fd, stdout_fd, stderr_fd);
        return NULL;
    }
    if (pipe (stdout_fd) < 0) {
        psError (PS_ERR_UNKNOWN, true, "cannot create pipe file descriptor");
        closePipes (stdin_fd, stdout_fd, stderr_fd);
        return NULL;
    }
    if (pipe (stderr_fd) < 0) {
        psError (PS_ERR_UNKNOWN, true, "cannot create pipe file descriptor");
        closePipes (stdin_fd, stdout_fd, stderr_fd);
        return NULL;
    }

    psArray *cmd = psStringSplitArray (command, " ", false);
    if (cmd->n <= 0) {
        psError (PS_ERR_UNKNOWN, true, "empty command for pipe");
        psFree (cmd);
        closePipes (stdin_fd, stdout_fd, stderr_fd);
        return NULL;
    }

    // create the command line array needed by execvp
    char **argv = (char **)psAlloc((cmd->n+1)*sizeof(char *));
    for (int i = 0; i < cmd->n; i++) {
        argv[i] = cmd->data[i];
    }
    argv[cmd->n] = NULL;

    pid = fork ();
    if (!pid) { /* must be child process */
        /* close the other ends of the pipes */
        close (stdin_fd[1]);
        close (stdout_fd[0]);
        close (stderr_fd[0]);

        /* tie our ends of the pipes to stdin, stdout, stderr */
        dup2 (stdin_fd[0],  STDIN_FILENO);
        dup2 (stdout_fd[1], STDOUT_FILENO);
        dup2 (stderr_fd[1], STDERR_FILENO);

        /* set all three unblocking */
        setvbuf (stdin,  (char *) NULL, _IONBF, BUFSIZ);
        setvbuf (stdout, (char *) NULL, _IONBF, BUFSIZ);
        setvbuf (stderr, (char *) NULL, _IONBF, BUFSIZ);

        status = execvp (argv[0], argv);
        if (status < 0) { psWarning ("error running exec for child process"); }
        exit (1); // this statement exits the child, not the parent, process
    }
    psFree (cmd);
    psFree (argv);

    if (pid == -1) {
        psError (PS_ERR_UNKNOWN, true, "unable to create child process");
        closePipes (stdin_fd, stdout_fd, stderr_fd);
        return NULL;
    }

    /* close the other ends of the pipes */
    close (stdin_fd[0]);
    stdin_fd[0]  = 0;
    close (stdout_fd[1]);
    stdout_fd[1] = 0;
    close (stderr_fd[1]);
    stderr_fd[1] = 0;

    /* make the pipes non-blocking */
    fcntl (stdin_fd[1],  F_SETFL, O_NONBLOCK);
    fcntl (stdout_fd[0], F_SETFL, O_NONBLOCK);
    fcntl (stderr_fd[0], F_SETFL, O_NONBLOCK);

    psPipe *pipe = psPipeAlloc();

    pipe->pid    = pid;
    pipe->fd_stdin  = stdin_fd[1];
    pipe->fd_stdout = stdout_fd[0];
    pipe->fd_stderr = stderr_fd[0];

    return (pipe);
}

// this function returns the exit status of the called function
// or a value > 255 on an error
int psPipeClose (psPipe *pipe)
{
    int close_status;
    int exit_status;
    int wait_status;
    int result;

    PS_ASSERT_PTR_NON_NULL(pipe, false);

    close_status = true;
    if (close (pipe->fd_stdin) != 0) {
        psError(PS_ERR_IO, true, "error closing the pipe stdin (pid %d, error %s)\n", pipe->pid, strerror(errno));
        close_status = false;
    }
    if (close (pipe->fd_stdout) != 0) {
        psError(PS_ERR_IO, true, "error closing the pipe stdout (pid %d, error %s)\n", pipe->pid, strerror(errno));
        close_status = false;
    }
    if (close (pipe->fd_stderr) != 0) {
        psError(PS_ERR_IO, true, "error closing the pipe sterr (pid %d, error %s)\n", pipe->pid, strerror(errno));
        close_status = false;
    }

    // we expect the child process to have exited.
    // wait for the exit condition, but no longer than 100ms
    for (int i = 0; i < 10; i++) {
        result = waitpid (pipe->pid, &wait_status, WNOHANG);
        switch (result) {
        case -1:   // error on waitpid
            switch (errno) {
            case ECHILD:
                psError(PS_ERR_IO, true, "unknown PID, not a child process: %d\n", pipe->pid);
                return 0x100;
            default:
                psAbort("unexpected response to waitpid: %d\n", result);
            }
            break;

        case 0:   // child not yet exited
            usleep (10000);
            continue;

        default:
            if (result != pipe->pid) {
                psAbort("waitpid error: mis-matched PID (%d vs %d).  programming error\n", result, pipe->pid);
            }
            if (WIFEXITED(wait_status)) {
                exit_status = WEXITSTATUS(wait_status);
                if (close_status) {
                    return exit_status;
                } else {
                    return (0x100);
                }
            }
            if (WIFSIGNALED(wait_status)) {
                psError(PS_ERR_IO, true, "job %d exited on signal %d\n", pipe->pid, WTERMSIG(wait_status));
                return (0x100 + WTERMSIG(wait_status));
            }
            if (WIFSTOPPED(wait_status)) {
                psAbort("waitpid returns 'stopped' programming error\n");
            }
        }
    }
    psError(PS_ERR_IO, true, "child process pid %d did not exit\n", pipe->pid);
    return 0x100;
}
