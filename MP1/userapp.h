#ifndef USERAPP_H
#define USERAPP_H

#include <stddef.h>     // lib for NULL
#include <stdio.h>      // lib for printf
#include <stdlib.h>     // lib for exit
#include <sys/types.h>  // lib for time_t
#include <time.h>       // lib for time
#include <unistd.h>     // lib for getpid
// Reference: https://stackoverflow.com/questions/1033898/why-do-you-have-to-link-the-math-library-in-c
#include <math.h>       // lib for floor & log10

#define EXPIRE  10  // default timer expire time
#define CMD_LEN 24  // cmd length exclude pid
#endif
