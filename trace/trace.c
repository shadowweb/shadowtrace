// create trace.c to be used with -finstrument-function option

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

static uint64_t fileSizeIncrement = 4096;
static off_t currentFileSize = 0;
static off_t currentFileAvailable = 0;
static pthread_t mainThreadId = 0;
static bool traceEnabled = false;
static int fd = -1;

void __attribute__ ((no_instrument_function)) traceFileUnmap()
{

}

void __attribute__ ((no_instrument_function)) traceFileMap()
{

}

void __attribute__ ((constructor,no_instrument_function)) traceBegin (void)
{
    // create binary file and map it to memory
    // save main thread id
    mainThreadId = pthread_self();
    fileSizeIncrement = getpagesize();
    struct stat traceStat;
    if (stat("./TRACE", &traceStat)  == 0)
    {
        traceEnabled = true;
        fd = open("./TRACE", (O_CREAT | O_WRONLY | O_TRUNC), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
        if (fd >= 0)
        {
            currentFileAvailable += fileSizeIncrement;
            ftruncate(fd, currentFileAvailable);
        }
    }
    /* Tracing requested: a trace file was found */

}
void __attribute__ ((destructor,no_instrument_function)) traceEnd (void)
{
    // do msync and unmap and close file, truncate file to the correct file size
    if (fd > -1)
    {
        ftruncate(fd, currentFileSize);
        close(fd);
    }
}

void __attribute__ ((no_instrument_function)) __cyg_profile_func_enter (void *func, void *caller)
{
    // if thread id is the same as the saved one, write func pointer into the next place in the mapped memory
    // when reached the end, sync, unmap, truncate file to the new size, and map to the new region in the file
}

void __attribute__ ((no_instrument_function)) __cyg_profile_func_exit (void *func, void *caller)
{
    // if thread id is the same as the saved one, put 1 in the first bit of func and write func pointer into the next place in the mapped memory
    // when reached the end, sync, unmap, truncate file to the new size, and map to the new region in the file
}
