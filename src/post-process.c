#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "ntree.h"
#include "stack.h"

size_t getFileSize(const char* filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}

#define FUNCTION_END    0x8000000000000000UL
static swNTree *functionTree = NULL;
static swStack *callStack = NULL;

bool initDataStructures()
{
    bool rtn = false;
    if (!functionTree && !callStack)
    {
        if ((functionTree = swNTreeNew(0)))
        {
            if ((callStack = swStackNew(32)))
            {
                if (swStackPush(callStack, functionTree))
                    rtn = true;
                else
                {
                    swStackDelete(callStack);
                    callStack = NULL;
                }
            }
            if (!rtn)
            {
                swNTreeDelete(functionTree);
                functionTree = NULL;
            }
        }
    }
    return rtn;
}

void clearDataStructures()
{
    if (functionTree)
    {
        swNTreeDelete(functionTree);
        functionTree = NULL;
    }
    if (callStack)
    {
        swStackDelete(callStack);
        callStack = NULL;
    }
}

bool functionTreeAppend(uint64_t functionAddress, bool end)
{
    // printf ("'%s': entered\n", __func__);
    bool rtn = false;
    if (functionAddress)
    {
        if (end)
        {
            swNTree *lastChild = swStackPop(callStack);
            swNTree *currentParent = swStackPeek(callStack);
            // printf ("'%s': remove case: parent = %p, child = %p\n", __func__, currentParent, lastChild);
            if (lastChild && currentParent)
            {
                if (currentParent->count > 1)
                {
                    // printf ("'%s': count = %u, prev child = %p, last child = %p, real last child = %p\n",
                    //         __func__, currentParent->count, &(currentParent->children[currentParent->count - 2]), &(currentParent->children[currentParent->count - 1]), lastChild);
                    if (swNTreeCompare(&(currentParent->children[currentParent->count - 2]), lastChild) == 0)
                    {
                        currentParent->count--;
                        currentParent->children[currentParent->count - 1].repeatCount++;
                        swNTreeDelete(lastChild);
                    }
                    rtn = true;
                }
                else
                    rtn = true;
            }
            else
                fprintf(stderr, "lastChild = %p, currentParent = %p\n", (void *)lastChild, (void *)currentParent);
        }
        else
        {
            swNTree *currentParent = swStackPeek(callStack);
            // printf ("'%s': insert case: parent = %p\n", __func__, currentParent);
            if (currentParent)
            {
                swNTree *nextChild = swNTreeAddNext(currentParent, functionAddress);
                if (nextChild)
                {
                    rtn = swStackPush(callStack, nextChild);
                    if (!rtn)
                        fprintf (stderr, "Failed to push next child to stack\n");
                }
                else
                    fprintf (stderr, "Failed to add next child to currentParent\n");
            }
            else
                fprintf (stderr, "No parent on the stack\n");
        }
    }
    // swNTreeDump(functionTree);
    return rtn;
}

/*
typedef struct swPrintTree
{
    char *mmapStartPtr;
    char *mmapCurrentPtr;
    char *mmapEndPtr;
    off_t currentFileOffset;
    size_t fileSizeReserved;
    size_t fileSizeIncrement;
    off_t segmentUsed;
    int fd;
    unsigned valid : 1;
} swPrintTree;

swPrintTree outStreamData = { .fd = -1 };

void outputFileUnmap()
{
    if (outStreamData.valid && (outStreamData.fd > -1) && outStreamData.mmapStartPtr)
    {
        msync (outStreamData.mmapStartPtr, outStreamData.fileSizeIncrement, MS_SYNC);
        munmap ((void *)outStreamData.mmapStartPtr, outStreamData.fileSizeIncrement);
        outStreamData.mmapStartPtr = outStreamData.mmapCurrentPtr = outStreamData.mmapEndPtr = NULL;
        outStreamData.valid = false;
    }
}

void outputFileMap()
{
    if (!outStreamData.valid && (outStreamData.fd > -1) && !outStreamData.mmapStartPtr)
    {
        if (ftruncate(outStreamData.fd, (outStreamData.currentFileOffset + outStreamData.fileSizeIncrement)) == 0 )
        {
            if ((outStreamData.mmapStartPtr = (char *)mmap(NULL, outStreamData.fileSizeIncrement, (PROT_WRITE), MAP_SHARED, outStreamData.fd, outStreamData.currentFileOffset)) != MAP_FAILED)
            {
                outStreamData.mmapCurrentPtr = outStreamData.mmapStartPtr;
                outStreamData.mmapEndPtr = outStreamData.mmapCurrentPtr + outStreamData.fileSizeIncrement;
                outStreamData.currentFileOffset += outStreamData.fileSizeIncrement;
                outStreamData.valid = true;
            }
            else
            {
                fprintf(stderr, "mmap(NULL, %lu, (PROT_WRITE), MAP_SHARED, %d, %ld)\n", outStreamData.fileSizeIncrement, outStreamData.fd, outStreamData.currentFileOffset);
                fprintf(stderr, "Failed mmap with error %s\n", strerror(errno));
            }
        }
        else
        {
            close(outStreamData.fd);
            outStreamData.fd = -1;
        }
    }
}


bool outputInit(const char *fileName)
{
    bool rtn = false;
    if (fileName)
    {
        if ((outStreamData.fd = open(fileName, (O_CREAT | O_RDWR | O_TRUNC), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))) > -1)
        {
            outStreamData.fileSizeIncrement = (size_t)getpagesize() * 1024;
            outputFileMap();
            if (outStreamData.valid)
                rtn = true;
        }
    }
    return rtn;
}

void outputClose()
{
    if (outStreamData.fd > -1)
    {
        off_t currentFileSize = outStreamData.currentFileOffset - (off_t)(outStreamData.mmapEndPtr - outStreamData.mmapCurrentPtr);
        outputFileUnmap();
        if (ftruncate(outStreamData.fd, currentFileSize) < 0)
            fprintf (stderr, "failed to trancate trace file to the right size\n");
        close(outStreamData.fd);
        outStreamData.fd = -1;
        outStreamData.valid = false;
    }
}

void outputFuncAddress(uint64_t funcAddress, uint32_t repeatCount, uint32_t level, void *data)
{
    if (outStreamData.valid)
    {
        int bytesNeeded = snprintf(NULL, 0, "%u %#lx %u\n", level, funcAddress, repeatCount);
        if (bytesNeeded > 0)
        {
            size_t bytesAvailable = outStreamData.mmapEndPtr - outStreamData.mmapCurrentPtr;
            if ((size_t)bytesNeeded < bytesAvailable)
            {
                if (snprintf(outStreamData.mmapCurrentPtr, bytesAvailable, "%u %#lx %u\n", level, funcAddress, repeatCount) == bytesNeeded)
                {
                    outStreamData.mmapCurrentPtr += bytesNeeded;
                }
                else
                {
                    fprintf(stderr, "Unexpected return from snprintf\n");
                    outputFileUnmap();
                }
            }
            else
            {
                char tmp[bytesNeeded + 1];
                if (snprintf(tmp, bytesNeeded, "%u %#lx %u\n", level, funcAddress, repeatCount) == bytesNeeded)
                {
                    memcpy(outStreamData.mmapCurrentPtr, tmp, bytesAvailable);
                    outStreamData.mmapCurrentPtr += bytesAvailable;
                    outputFileUnmap();
                    outputFileMap();
                    if (outStreamData.valid)
                    {
                        size_t bytesLeft = bytesNeeded - bytesAvailable;
                        if (bytesLeft)
                        {
                            memcpy(outStreamData.mmapCurrentPtr, &tmp[bytesAvailable], bytesLeft);
                            outStreamData.mmapCurrentPtr += bytesLeft;
                        }
                    }
                }
                else
                {
                    fprintf(stderr, "Unexpected return from snprintf\n");
                    outputFileUnmap();
                }

            }
        }
    }
}
*/

void printFuncAddress(uint64_t funcAddress, uint32_t repeatCount, uint32_t level, void *data)
{
    FILE *out = (FILE *)data;
    fprintf(out, "%u %#lx %u\n", level, funcAddress, repeatCount);
}

int main(int argc, char *argv[])
{
    int exitCode = EXIT_FAILURE;
    if (argc < 3)
    {
        fprintf(stderr, "Insuficient number of arguments\n");
        exit(exitCode);
    }

    char *inFile = argv[1];
    char *outFile = argv[2];

    size_t fileSize = getFileSize(inFile);
    off_t mapOffset = 0;
    size_t mapSize = (size_t)getpagesize() * 1024;

    int fd = open(inFile, O_RDONLY, 0);
    if (fd > -1)
    {
        if (initDataStructures())
        {
            while (mapOffset < (off_t)fileSize)
            {
                mapSize = ((fileSize >= (mapOffset + mapSize))? mapSize : (fileSize - mapOffset));
                char *funcAddressesStart = (char *)mmap(NULL, mapSize, (PROT_READ), (MAP_PRIVATE | MAP_POPULATE), fd, mapOffset);
                if (funcAddressesStart)
                {
                    char *funcAddressesEnd = funcAddressesStart + mapSize;
                    uint64_t *funcAddressesCurrent = (uint64_t *)funcAddressesStart;
                    while (funcAddressesCurrent < (uint64_t *)funcAddressesEnd)
                    {
                        uint64_t addr = (*funcAddressesCurrent) & (~FUNCTION_END);
                        bool end = ((*funcAddressesCurrent & FUNCTION_END) != 0 );
                        if (functionTreeAppend(addr, end))
                            funcAddressesCurrent++;
                        else
                        {
                            fprintf(stderr, "failed to append %s address 0x%lX to the tree; offset = 0x%lX\n", ((end)? "END" : "START"), addr,
                                mapOffset + (uint64_t)funcAddressesCurrent - (uint64_t)funcAddressesStart);
                            break;
                        }
                    }
                    if (funcAddressesCurrent < (uint64_t *)funcAddressesEnd)
                    {
                        fprintf(stderr, "funcAddressesCurrent(%p) < funcAddressesEnd(%p)\n", funcAddressesCurrent, funcAddressesEnd);
                        break;
                    }
                    munmap(funcAddressesStart, mapSize);
                    mapOffset += mapSize;
                }
                else
                {
                    fprintf(stderr, "mmap failed to create memory map of size %zu with offset %ld, \'%s\'", mapSize, mapOffset, strerror(errno));
                    break;
                }
            }
            if (mapOffset == (off_t)fileSize)
            {
                FILE *outStream = fopen(outFile, "w");
                if (outStream)
                {
                    swNTreePrint(functionTree, printFuncAddress, outStream);
                    fclose(outStream);
                }
                exitCode = EXIT_SUCCESS;
            }
            else
              fprintf(stderr, "mapOffset(%zd) != fileSize(%zd)\n", mapSize, fileSize);
            clearDataStructures();
        }
        else
            fprintf(stderr, "Failed to initialize data structures\n");
        close (fd);
    }
    else
        fprintf(stderr, "Failed to open input file\n");
    exit(exitCode);
}
