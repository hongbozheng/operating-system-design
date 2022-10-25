/**
 * FILENAME: userapp.c
 *
 * DESCRIPTION: mp2 userapp
 *
 * AUTHOR: Hongbo Zheng
 *
 * DATE: Sunday Oct 23th, 2022
 */

#include "userapp.h"

/**
 * function to register a process to /proc/mp2/status
 * with syntax "R, <pid>, <period>, <computation_time>"
 *
 * @param pid           process id
 * @param period        task period
 * @param computation   task computation time
 */
void register_process(pid_t pid, unsigned long period, unsigned long computation) {
    FILE *f;
    f = fopen(FILENAME,"w");
    fprintf(f,"R, %d, %lu, %lu", pid, period, computation);
    fclose(f);
}

/**
 * function to read /proc/mp2/status and check if
 * pid is written successfully in /proc/mp2/status
 *
 * @param pid
 * @return int  1-pid exists    0-pid not exist
 */
int process_exist(pid_t pid) {
    FILE *f;
    char buf[20];
    int pid_regd;
    int cnt = 0;

    f = fopen("/proc/mp2/status","r");      // open file /proc/mp2/status

    while(fscanf(f, "%s" , buf)) {
        sscanf(buf, "%d", &pid_regd);

        if ((cnt % SKIP_NUM_VAL) == 0) {    // check every three num
            if (pid == pid_regd) {          // pid exists in /proc/mp2/status
                fclose(f);                  // close f
                return 1;                   // return 1
            }
        }
        ++cnt;                              // increment cnt
    }
    fclose(f);                              // close f
    return 0;                               // pid DNE, return 0
}

/**
 * function that yield a process
 * with syntax "Y, <pid>"
 *
 * @param pid   process id
 * @return void
 */
void yield_process(pid_t pid) {
    FILE *f;
    f = fopen(FILENAME, "w");
    fprintf(f, "Y, %d", pid);
    fclose(f);
}

/**
 * function that de-register a process
 * with syntax "D, <pid>"
 *
 * @param pid   process id
 * @return void
 */
void deregister_process(pid_t pid) {
    FILE *f;
    f = fopen(FILENAME, "w");
    fprintf(f, "D, %d", pid);
    fclose(f);
}

/**
 * do some random things when process is woke up
 *
 * @param
 * @return void
 */
void do_job(int n) {
    unsigned int factorial = 0;
    for (int i = 1; i < n; ++i) {
        factorial *= i;
    }
}

int main(int argc, char* argv[]) {
    pid_t pid = getpid();                           // get task pid
    unsigned long period, computation;
    unsigned int freq = atoi(argv[3]);              // get task frequency
    // Reference: https://pubs.opengroup.org/onlinepubs/7908799/xsh/systime.h.html
    struct timeval t0, start, end;                  // struct timeval record time

    if (argc != 4) {
        printf("[USRAPP]: ./userapp <pid> <PERIOD> <COMPUTATION_TIME> <FREQUENCY>\n");
        return 1;
    }

    sscanf(argv[1],"%lu",&period);                  // get task period
    sscanf(argv[2],"%lu",&computation);             // get task computation time

    printf("[USRAPP]: REGISTER TASK PID: %d C: %lu P: %lu F: %u\n", pid, computation, period, freq);
    // register process through /proc/mp2/status
    register_process(pid, period, computation);

    // read /proc/mp2/status and check if the process registered successfully
    if (!process_exist(pid)) {
        printf("[USRAPP]: PROCESS ID NOT EXISTS\n");
        exit(1);
    }

    // record the time when the task starts
    // Reference: https://man7.org/linux/man-pages/man2/gettimeofday.2.html
    gettimeofday(&t0, NULL);
    printf("[USRAPP]: TASK START TIME %ld\n", t0.tv_sec);

    // write yield to /proc/mp2/status
    printf("[USRAPP]: YIELD PROCESS WITH PID %d\n", pid);
    yield_process(pid);

    printf("[USRAPP]: START EXECUTION\n");
    // real-time loop, simulate periodic application
    for (int i = 0; i < freq; ++i) {
        printf("--------------------------------------------------\n");
        gettimeofday(&start, NULL);
        printf("[USRAPP]: WAKEUP TIME %ld\n", (start.tv_sec - t0.tv_sec));
        do_job(10);
        gettimeofday(&end, NULL);
        printf("[USRAPP]: YIELD PROCESS WITH PID %d\n", pid);
        yield_process(pid);
    }
    printf("--------------------------------------------------\n");
    printf("[USRAPP]: DEREGISTER PROCESS WITH PID %d\n", pid);
    deregister_process(pid);

	return 0;
}