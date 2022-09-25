#include "userapp.h"

int main(int argc, char* argv[]) {
    uint32_t expire = EXPIRE;
    // Reference: https://stackoverflow.com/questions/7550269/what-is-timenull-in-c
    time_t start_time = time(NULL);

    if (argc > 2) {
        printf("[USAGE]: ./userapp <expire>\n");
        exit(1);
    } else if (argc == 2) {
        if (atoi(argv[1]) < 0) {
            printf("[ERROR]: Time expiration length cannot be negative number\n");
        }
        expire = atoi(argv[1]);
    }

    // Reference: https://www.gnu.org/software/libc/manual/html_node/Process-Identification.html#:~:text=The%20pid_t%20data%20type%20is,ID%20of%20the%20current%20process.
    pid_t pid = getpid();
	FILE *status_file = fopen("/proc/mp1/status", "w");
	if (status_file) {
		fprintf(status_file, "%d", pid);
		fclose(status_file);
	} else {
        printf("[ERROR]: Cannot open file /proc/mp1/status\n");
    }
    
    printf("[INFO]: Executing ./userapp %u with PID: %d\n", expire, pid);
	while ((int)(time(NULL) - start_time) < expire) {
		//random = rand() % 64;
		//n = 1;
		//for (i = 1; i <= random; i++)
		//	n *= i;
		//printf("%lu! = %lu\n", random, n);
	}
    printf("[INFO]: Finish executing ./userapp %u with PID: %d\n", expire, pid);
	return 0;
}
