#ifndef LOGGER_H
#define LOGGER_H

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <stdarg.h>

#define FALSE 0
#define TRUE 1
#define THREAD_END_VALUE -1
#define FILE_NAME_ARRAY_LENGTH 25

#define CHANGE_IMPORTANCE_SIGNAL SIGRTMIN       
#define SWITCH_LOGGING_SIGNAL    (SIGRTMIN + 1) 
#define DUMP_SIGNAL              (SIGRTMIN + 2) 

typedef enum 
{
    MIN,
    STD,
    MAX
} ImportanceType;

short init(void (*dumpFuncPtr)(void));
void destroy(void);

void myLog(const int level, const char* format, ...);

#endif