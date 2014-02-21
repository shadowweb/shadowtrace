// create trace.c to be used with -finstrument-function option

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

static pthread_t mainThreadId = 0;
static size_t fileSizeIncrement = 4096;
static off_t currentFileOffset = 0;
static char *memoryPtr = NULL;
static char *memoryCurrentPtr = NULL;
static char *memoryEndPtr = NULL;
static int fd = -1;
static bool traceEnabled = false;

void __attribute__ ((no_instrument_function)) traceFileUnmap()
{
    // fprintf(stderr, "%s: enter, addressCount = %u\n", __func__, addressCount);
    if ((fd > -1) && memoryPtr)
    {
        msync (memoryPtr, fileSizeIncrement, MS_SYNC);
        munmap ((void *)memoryPtr, fileSizeIncrement);
        memoryPtr = memoryCurrentPtr = memoryEndPtr = NULL;
        traceEnabled = false;
    }
    // fprintf(stderr, "%s: exit\n", __func__);
}

void __attribute__ ((no_instrument_function)) traceFileMap()
{
    // fprintf(stderr, "%s: enter\n", __func__);
    if ((fd > -1) && !memoryPtr)
    {
        if (ftruncate(fd, (currentFileOffset + fileSizeIncrement)) == 0 )
        {
            if ((memoryPtr = (char *)mmap(NULL, fileSizeIncrement, (PROT_WRITE), MAP_SHARED, fd, currentFileOffset)) != MAP_FAILED)
            {
                memoryCurrentPtr = memoryPtr;
                memoryEndPtr = memoryCurrentPtr + fileSizeIncrement;
                currentFileOffset += fileSizeIncrement;
                traceEnabled = true;
            }
            else
                fprintf(stderr, "Failed mmape with error %s\n", strerror(errno));
        }
        else
        {
            close(fd);
            fd = -1;
        }
    }
    // fprintf(stderr, "%s: exit\n", __func__);
}

void __attribute__ ((constructor,no_instrument_function)) traceBegin (void)
{
    // fprintf(stderr, "%s: enter\n", __func__);
    // create binary file and map it to memory
    // save main thread id
    mainThreadId = pthread_self();
    fileSizeIncrement = (size_t)getpagesize() * 1024;
    struct stat traceStat = { 0 };
    // TODO: use file name that consists of program_invocation_short_name and ".TRACE" in the current running directory
    if (stat("./TRACE", &traceStat) == 0)
    {
        fd = open("./TRACE", (O_CREAT | O_RDWR | O_TRUNC), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
        if (fd >= 0)
            traceFileMap();
    }
    // fprintf(stderr, "%s: exit\n", __func__);
}

void __attribute__ ((destructor,no_instrument_function)) traceEnd (void)
{
    // fprintf(stderr, "%s: enter\n", __func__);
    // do msync and unmap and close file, truncate file to the correct file size
    if (fd > -1)
    {
        off_t currentFileSize = currentFileOffset - (off_t)(memoryEndPtr - memoryCurrentPtr);
        traceFileUnmap();
        if (ftruncate(fd, currentFileSize) < 0)
            fprintf (stderr, "failed to trancate trace file to the right size\n");
        close(fd);
    }
    // fprintf(stderr, "%s: exit\n", __func__);
}

void __attribute__ ((no_instrument_function)) __cyg_profile_func_enter (void *func, void *caller)
{
    // if thread id is the same as the saved one, write func pointer into the next place in the mapped memory
    // when reached the end, sync, unmap, truncate file to the new size, and map to the new region in the file
    if (traceEnabled && (pthread_self() == mainThreadId) && memoryPtr)
    {
        if (!func)
            fprintf (stderr, "Enter with NULL function pointer\n");
        unsigned long pointer = (unsigned long)(func);
        *(unsigned long *)memoryCurrentPtr = pointer;
        memoryCurrentPtr += sizeof(pointer);
        if (memoryCurrentPtr == memoryEndPtr)
        {
            traceFileUnmap();
            traceFileMap();
        }
    }
}

void __attribute__ ((no_instrument_function)) __cyg_profile_func_exit (void *func, void *caller)
{
    // if thread id is the same as the saved one, put 1 in the first bit of func and write func pointer into the next place in the mapped memory
    // when reached the end, sync, unmap, truncate file to the new size, and map to the new region in the file
    if (traceEnabled && (pthread_self() == mainThreadId) && memoryPtr)
    {
        if (!func)
            fprintf (stderr, "Exit with NULL function pointer\n");
        unsigned long pointer = (unsigned long)(func);
        pointer |= 0x8000000000000000UL;
        *(unsigned long *)memoryCurrentPtr = pointer;
        memoryCurrentPtr += sizeof(pointer);
        if (memoryCurrentPtr == memoryEndPtr)
        {
            traceFileUnmap();
            traceFileMap();
        }
    }
}
