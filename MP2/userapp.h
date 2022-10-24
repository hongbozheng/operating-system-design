#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define DIRECTORY     "mp2"
#define FILENAME      "status"
#define COMMANDLEN    1000
#define FILENAMELEN   500
#define BUFFERLEN     5000
#define PIDLISTLEN    100
#define MAPPINGFILE   "mapping.csv"
#define LOGFILENAME   "log"

int is_exist_in_array(unsigned int *array, int array_len, int number)
{
    int i;
    for (i = 0; i < array_len; i ++) {
        if (array[i] == number) {
            return 1;
        }
    }
    return 0;
}

// check if the process exist in /proc/mp2/status
int is_process_exist(unsigned int pid)
{
    FILE* f;
    f = fopen("/proc/mp2/status","r");
    char buf[20];
    int num;
    int count = 0;
    while(fscanf(f,"%s",buf)) {
        //printf("buf %s\n",buf);
        sscanf(buf, "%d", &num);

        if ((count%3)==0) {
            if (pid == num) {
                fclose(f);
                return 1;
            }
        }
        ++count;
    }
    fclose(f);
    return 0;
//    char filename[FILENAMELEN], buf[BUFFERLEN];
//    FILE *fd;
//    unsigned int pid_list[PIDLISTLEN], tmp_pid;
//    int i, len, pid_list_num = 0;
//    char *tmp;
//
//    len = sprintf(filename, "/proc/%s/%s", DIRECTORY, FILENAME);
//    filename[len] = '\0';
//
//    fd = fopen(filename, "r");
//    len = fread(buf, sizeof(char), BUFFERLEN - 1, fd);
//    buf[len] = '\0';
//    fclose(fd);
//
//    if ((tmp = strtok(buf, "\n")) == NULL) {
//        return 0;
//    }
//    sscanf(tmp, "%u", &tmp_pid);
//    while (!is_exist_in_array(pid_list, pid_list_num, tmp_pid)) {
//        pid_list[pid_list_num] = tmp_pid;
//        pid_list_num ++;
//        tmp = strtok(NULL, "\n");
//        sscanf(tmp, "%u", &tmp_pid);
//    }
//
//    for (i = 0; i < pid_list_num; i ++) {
//        if (pid_list[i] == pid) {
//            return 1;
//        }
//    }
//    return 0;
}

void register_process(unsigned int pid, int period, int computation)
{
    FILE *f;
    f= fopen("/proc/mp2/status","w");
    //char command[COMMANDLEN];
    //memset(command, '\0', COMMANDLEN);
    //sprintf(command, "echo \"R, %u, %s, %s\" > /proc/%s/%s", pid, period, computation, DIRECTORY, FILENAME);
//    int p = atoi(&period);
//    int c = atoi(&computation);
    fprintf(f,"R, %d, %d, %d", pid, period, computation);
    fclose(f);
    //printf("register process success\n");
}

void yield_process(unsigned int pid)
{
    FILE *f;
    f = fopen("/proc/mp2/status","w");
//    char command[COMMANDLEN];
//    memset(command, '\0', COMMANDLEN);
//    sprintf(command, "echo \"Y, %u\" > /proc/%s/%s", pid, DIRECTORY, FILENAME);
//    system(command);
    fprintf(f,"Y, %d", pid);
    fclose(f);
}

void unregister_process(unsigned int pid)
{
    FILE *f;
    f = fopen("/proc/mp2/status","w");
//    char command[COMMANDLEN];
//    memset(command, '\0', COMMANDLEN);
//    sprintf(command, "echo \"D, %u\" > /proc/%s/%s", pid, DIRECTORY, FILENAME);
//    system(command);
    fprintf(f,"D, %d", pid);
    fclose(f);
}

void do_job(int n)
{
    int i, j, k ,tmp;
    for (i = 0; i < n; i ++) {
        for (j = 0; j < n; j ++) {
            for (k = 0; k < n; k ++) {
                tmp = i * j * k - i - j - k;
            }
        }
    }
}

double time_diff(struct timeval before, struct timeval after)
{
    return (after.tv_sec * 1000.0 + after.tv_usec / 1000.0) - (before.tv_sec * 1000.0 + before.tv_usec / 1000.0);
}

// return iteration according to the mapping in the <filename>
int get_iteration(char *filename, int computation)
{
    int i, tmp_i, tmp_c;
    FILE *fp;

    fp = fopen(filename, "r");
    while (fscanf(fp, "%d,%d", &tmp_i, &tmp_c) != EOF) {
        if (tmp_c >= computation) {
            fclose(fp);
            return tmp_i;
        }
    }
    fclose(fp);
    return tmp_i;
}

// wirte message to log
void write_log(const char *buf)
{
    FILE *log = fopen(LOGFILENAME, "a");
    fputs(buf, log);
    fclose(log);
}
