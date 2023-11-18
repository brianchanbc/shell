#ifndef _JOB_H_
#define _JOB_H_

#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef enum job_state{FOREGROUND, BACKGROUND, SUSPENDED, UNDEFINED} job_state_t;

// Represents a job in a shell.
typedef struct job {
    char *cmd_line;     // The command line for this specific job.
    job_state_t state;  // The current state for this job
    pid_t pid;          // The process id for this job
    int jid;            // The job number for this job
}job_t;

/*
* add_job: add a new job to the jobs array
* 
* jobs: the jobs array
*
* max_jobs: the maximum number of jobs
*
* pid: the process id of the job
*
* state: the state of the job
*
* cmd_line: the command line of the job
*
* returns: true if the job was added successfully, false if no more jobs can be added (i.e. max_jobs is reached)
*/
bool add_job(job_t *jobs, int max_jobs, pid_t pid, job_state_t state, const char *cmd_line);

/*
* delete_job: remove a job from the jobs array based on the pid_t provided
*
* jobs: the jobs array
*
* max_jobs: the maximum number of jobs
*
* pid: the process id of the job to remove
*
* returns: true if the job was removed successfully, false if the job was not found
*/
bool delete_job(job_t *jobs, int max_jobs, pid_t pid);

/*
* free_jobs: free all the memory allocated for the jobs array
* 
* jobs: the jobs array
*
* max_jobs: the maximum number of jobs=
*/
void free_jobs(job_t *jobs, int max_jobs);


#endif