#include "logger.h"

short isInitialized_bo = FALSE;
FILE *file;
char fileNameArray[FILE_NAME_ARRAY_LENGTH] = {0};
pthread_t importanceThread, switchLoggingThread, dumpThread;

atomic_int importanceState;
atomic_int flag_change_importance;
atomic_int loggingEnabledState;
atomic_int flag_switch_logging;
atomic_int flag_dump;

pthread_mutex_t saveToFileLock;

void (*dumpFuncPtr)(void) = NULL;

void handler_change_importance()
{
    atomic_store(&flag_change_importance, TRUE);
}

void handler_switch_logging()
{
    atomic_store(&flag_switch_logging, TRUE);
}

void handler_dump()
{
    atomic_store(&flag_dump, TRUE);
}

void *dumpHandler(void *args)
{
    sigset_t set;
    sigfillset(&set);

    sigdelset(&set, DUMP_SIGNAL);
    pthread_sigmask(SIG_SETMASK, &set, NULL);

    while (TRUE)
    {
        if (TRUE == atomic_load(&flag_dump))
        {
            if (dumpFuncPtr != NULL)
            {
                (*dumpFuncPtr)();
            }

            printf("Dump function used has been used (if defined)\n");
            atomic_store(&flag_dump, FALSE);
        }
        else if (THREAD_END_VALUE == atomic_load(&flag_dump))
        {
            break;
        }

        usleep(200);
    }
}

void *switchLoggingHandler(void *args)
{
	sigset_t set;
    sigfillset(&set);

    sigdelset(&set, SWITCH_LOGGING_SIGNAL);
    pthread_sigmask(SWITCH_LOGGING_SIGNAL, &set, NULL);

	while (TRUE)
	{

        if (TRUE == atomic_load(&flag_switch_logging))
        {
            if (FALSE == atomic_load(&loggingEnabledState))
            {
                atomic_store(&loggingEnabledState, TRUE);
            }
            else
            {
                atomic_store(&loggingEnabledState, FALSE);
            }

            printf("Current logging flag: %d\n", atomic_load(&loggingEnabledState));
            atomic_store(&flag_switch_logging, FALSE);
        }
        else if (THREAD_END_VALUE == atomic_load(&flag_switch_logging))
        {
            break;
        }

        usleep(200);
	}
    
	return NULL;
}

void *importanceHandler(void *args)
{
	sigset_t set;
    sigfillset(&set);

    sigdelset(&set, CHANGE_IMPORTANCE_SIGNAL);
    pthread_sigmask(SIG_SETMASK, &set, NULL);

	while (TRUE)
	{

        if (TRUE == atomic_load(&flag_change_importance))
        {
            if (MIN == atomic_load(&importanceState))
            {
                atomic_store(&importanceState, STD);
                printf("Current logging importance is STD\n");
            }
            else if (STD == atomic_load(&importanceState))
            {
                atomic_store(&importanceState, MAX);
                printf("Current logging importance is MAX\n");
            }
            else if (MAX == atomic_load(&importanceState))
            {
                atomic_store(&importanceState, MIN);
                printf("Current logging importance is MIN\n");
            }

            atomic_store(&flag_change_importance, FALSE);
        }
        else if (THREAD_END_VALUE == atomic_load(&flag_change_importance))
        {
            break;
        }

        usleep(200);
	}
    
	return NULL;
}

void myLog(const int level, const char* format, ...)
{
    char buffer[256];
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 256, format, args);
    va_end (args);

    if (NULL == file)
    {
        puts("file is null\n");
    }

    if (0 == pthread_mutex_lock(&saveToFileLock))
    {
        if (TRUE == atomic_load(&loggingEnabledState) && level >= atomic_load(&importanceState))
        {
            file = fopen(fileNameArray, "a");

            if (file)
            {
                switch (level)
                {
                    case MIN:
                        fprintf(file, "MIN: ");
                        break;
                    case STD:
                        fprintf(file, "STD: ");
                        break;
                    case MAX:
                        fprintf(file, "MAX: ");
                        break;
                }

                fprintf(file, "%s\n", buffer);
            }

        }

        pthread_mutex_unlock(&saveToFileLock);
    }
}

void setFileName(char *fileNameArray)
{
    time_t currentTime = time(NULL);
    struct tm timeStruct = *localtime(&currentTime);
    snprintf(fileNameArray, FILE_NAME_ARRAY_LENGTH - 1, "%d-%02d-%02d %02d:%02d:%02d.log", timeStruct.tm_year + 1900, 
            timeStruct.tm_mon + 1, timeStruct.tm_mday, timeStruct.tm_hour, timeStruct.tm_min, timeStruct.tm_sec);
}

short init(void (*dumpFuncPtrDefinition)(void))
{   
    atomic_store(&importanceState, MIN);
    atomic_store(&loggingEnabledState, TRUE);
    atomic_store(&flag_change_importance, FALSE);
    atomic_store(&flag_switch_logging, FALSE);
    atomic_store(&flag_dump, FALSE);
    dumpFuncPtr = dumpFuncPtrDefinition;

    if (TRUE == isInitialized_bo)
    {
        return EXIT_FAILURE;
    }
    else
    {
        isInitialized_bo = TRUE;
    }

    setFileName(fileNameArray);

    file = fopen(fileNameArray, "w");

    if (NULL == file)
    {
        return EXIT_FAILURE;
    }
    else
    {
        fclose(file);
    }

    if (0 != pthread_mutex_init(&saveToFileLock, NULL))
    {
        fclose(file);
    }

    pthread_create(&importanceThread, NULL, importanceHandler, NULL);
    pthread_create(&switchLoggingThread, NULL, switchLoggingHandler, NULL);
    pthread_create(&dumpThread, NULL, dumpHandler, NULL);

    struct sigaction act;

    sigset_t set;
    sigfillset(&set);

    act.sa_sigaction = handler_change_importance;
    act.sa_mask = set;
    act.sa_flags = 0;

    sigaction(CHANGE_IMPORTANCE_SIGNAL, &act, NULL);

    act.sa_sigaction = handler_switch_logging;
    act.sa_mask = set;
    act.sa_flags = 0;

    sigaction(SWITCH_LOGGING_SIGNAL, &act, NULL);

    act.sa_sigaction = handler_dump;
    act.sa_mask = set;
    act.sa_flags = 0;

    sigaction(DUMP_SIGNAL, &act, NULL);

    act.sa_handler = SIG_IGN;

    for (int i = DUMP_SIGNAL + 1; i <= SIGRTMAX; i++)
    {
        sigaction(i, &act, NULL);
    }

    return EXIT_SUCCESS;
}

void destroy(void)
{
    atomic_store(&flag_change_importance, THREAD_END_VALUE);
    atomic_store(&flag_switch_logging, THREAD_END_VALUE);
    atomic_store(&flag_dump, THREAD_END_VALUE);

    pthread_join(importanceThread, NULL);
    pthread_join(switchLoggingThread, NULL);
    pthread_join(dumpThread, NULL);

    pthread_mutex_destroy(&saveToFileLock);
}