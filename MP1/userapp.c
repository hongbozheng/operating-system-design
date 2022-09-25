#include "userapp.h"

void register_proc(int pid) {
    // Reference: https://stackoverflow.com/questions/3068397/finding-the-length-of-an-integer-in-c
    unsigned int pid_len = floor(log10(abs(pid))) + 1;
    int a;
    printf("pid: %d, num_digits: %d\n", pid, pid_len);
    char cmd[5+pid_len+1];
    a = sprintf(cmd, "echo %d > /proc/mp1/status",pid);
    printf("a: %d\n", a);
    //cmd[5+pid_len] = '\0';
    system(cmd);
}

int main(int argc, char* argv[]) {
    unsigned long expire = EXPIRE;
    // Reference: https://stackoverflow.com/questions/7550269/what-is-timenull-in-c
    time_t start_time = time(NULL);

    if (argc > 2) {
        printf("[USAGE] ./userapp <expire>\n");
        exit(1);
    } else if (argc == 2) {
        if (atoi(argv[1]) < 0) {
            printf("[ERROR]: Time expiration length cannot be negative number\n");
        }
        expire = atoi(argv[1]);
    }
    
    // Reference: https://www.gnu.org/software/libc/manual/html_node/Process-Identification.html#:~:text=The%20pid_t%20data%20type%20is,ID%20of%20the%20current%20process.
    pid_t pid = getpid();

    register_proc(pid);

    while(1) {
        if ((int)(time(NULL) - start_time) > expire) {
            break;
        }
    }

	return 0;
}
