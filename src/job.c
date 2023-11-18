#include "job.h"

bool add_job(job_t *jobs, int max_jobs, pid_t pid, job_state_t state, const char *cmd_line) {
    for (int i = 0; i < max_jobs; i++) {
        // If found an empty position in the jobs array, add the job to the array
        // pid = 0 indicates an empty position
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].state = state;
            // Allocate memory for cmd_line and copy the string
            // strdup is basically a combination of malloc and strcpy
            // Must be freed later
            jobs[i].cmd_line = strdup(cmd_line);
            jobs[i].jid = i + 1;
            return true;
        }
    }
    return false;
}

bool delete_job(job_t *jobs, int max_jobs, pid_t pid) {
    for (int i = 0; i < max_jobs; i++) {
        // If found the job with the pid, delete the job
        if (jobs[i].pid == pid) {
            jobs[i].pid = 0;
            jobs[i].state = UNDEFINED;
            // Free the job immediately in the jobs array
            free(jobs[i].cmd_line);
            // Set cmd_line to NULL to prevent freeing of cmd_line 
            jobs[i].cmd_line = NULL;
            jobs[i].jid = 0;
            return true;
        }
    }
    return false;
}

void free_jobs(job_t *jobs, int max_jobs) {
    // Loop through jobs and free cmd_line for each job
    for (int i = 0; i < max_jobs; i++) {
        // If the job is not empty, free the cmd_line
        // Prevents freeing of empty jobs by checking if pid is 0
        if (jobs[i].pid != 0 && jobs[i].cmd_line != NULL) {
            free(jobs[i].cmd_line);
            jobs[i].cmd_line = NULL;
        }
    }
    // Lastly, deallocate jobs array
    free(jobs);
}