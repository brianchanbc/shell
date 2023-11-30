#include "signal_handlers.h"
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "csapp.h"

volatile sig_atomic_t fg_pid;
extern msh_t *shell;

/*
* sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
*     a child job terminates (becomes a zombie), or stops because it
*     received a SIGSTOP or SIGTSTP signal. The handler reaps all
*     available zombie children, but doesn't wait for any other
*     currently running children to terminate.
* Citation: Bryant and O’Hallaron, Computer Systems: A Programmer’s Perspective, Third Edition
*/
void sigchld_handler(int sig)
{   
    // Initialize signal masks and variables
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    pid_t pid;
    int status;
    // Block all signals
    Sigfillset(&mask_all);
    // Reap all available zombie children
    while ((pid = waitpid(-1, &status, WNOHANG|WUNTRACED|WCONTINUED)) > 0) { 
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // Case 1: Child process terminated normally or by a signal
            if (pid == fg_pid) {
                // If the child process is the foreground process, set fg_pid to 0 so the parent will know
                fg_pid = 0;
                Sio_puts("pid "); Sio_putl(pid); Sio_puts(" Done\n");
                Sio_puts("msh> ");
            } else {
                Sio_puts("pid "); Sio_putl(pid); Sio_puts(" Done\n");
                Sio_puts("msh> ");
            }
            // Block and delete the job from the job list
            Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
            delete_job(shell->jobs, shell->max_jobs, pid);
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);
        } 
        
        if (WIFSTOPPED(status)) {
            // Case 2: Child process stopped by a signal
            if (pid == fg_pid) {
                fg_pid = 0;
            }
            // Block and change the job state to suspended
            Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
            change_job_state(shell->jobs, shell->max_jobs, pid, SUSPENDED);
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);
            Sio_puts("pid "); Sio_putl(pid); Sio_puts(" Stopped\n");
            Sio_puts("msh> ");
        }

        if (WIFCONTINUED(status)) {
            // Case 3: Child process continued by a signal
            fg_pid = pid;
            // Block and change the job state to continue running again
            Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
            change_job_state(shell->jobs, shell->max_jobs, pid, FOREGROUND);
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);
            Sio_puts("pid "); Sio_putl(pid); Sio_puts(" Continue\n");
            Sio_puts("msh> ");
        }
    }
    // Restore errno, prevent unintended side effects
    errno = olderrno;

}

/*
* sigint_handler - The kernel sends a SIGINT to the shell whenever the
*    user types ctrl-c at the keyboard.  Catch it and send it along
*    to the foreground job.
* Citation: Bryant and O’Hallaron, Computer Systems: A Programmer’s Perspective, Third Edition
*/
void sigint_handler(int sig)
{   
    // Terminate the process
    pid_t pid = getpid();
    if (fg_pid == 0) {
        // Restore the default signal handler for stopping the shell
        Signal(SIGINT, SIG_DFL);
        Kill(-pid, SIGINT);
    } else {
        Kill(-fg_pid, SIGINT);
    }
}

/*
* sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
*     the user types ctrl-z at the keyboard. Catch it and suspend the
*     foreground job by sending it a SIGTSTP.
* Citation: Bryant and O’Hallaron, Computer Systems: A Programmer’s Perspective, Third Edition
*/
void sigtstp_handler(int sig)
{
    // Stop the process
    pid_t pid = getpid();
    if (fg_pid == 0) {
        Signal(SIGINT, SIG_DFL);
        Kill(-fg_pid, SIGTSTP);
    } else {
        Kill(-fg_pid, SIGTSTP);
    }
}

/*
* setup_handler - wrapper for the sigaction function
*
* Citation: Bryant and O’Hallaron, Computer Systems: A Programmer’s Perspective, Third Edition
*/
typedef void handler_t(int);
handler_t *setup_handler(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0) {
        perror("Signal error");
        exit(1);
    }
    return (old_action.sa_handler);
}


void initialize_signal_handlers() {

    // sigint handler: Catches SIGINT (ctrl-c) signals.
    setup_handler(SIGINT,  sigint_handler);   /* ctrl-c */
    // sigtstp handler: Catches SIGTSTP (ctrl-z) signals.
    setup_handler(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    // sigchld handler: Catches SIGCHILD signals.
    setup_handler(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */
}