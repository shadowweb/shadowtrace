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
    printf ("'%s': entered\n", __func__);
    bool rtn = false;
    if (functionAddress)
    {
        if (end)
        {
            printf ("'%s': remove case\n", __func__);
            swNTree *lastChild = swStackPop(callStack);
            swNTree *currentParent = swStackPeek(callStack);
            if (lastChild && currentParent)
            {
                if (currentParent->count > 1)
                {
                    printf ("'%s': count = %u, prev child = %p, last child = %p, real last child = %p\n",
                            __func__, currentParent->count, &(currentParent->children[currentParent->count - 2]), &(currentParent->children[currentParent->count - 1]), lastChild);
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
            printf ("'%s': insert case\n", __func__);
            swNTree *currentParent = swStackPeek(callStack);
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
    return rtn;
}

void printFuncAddress(uint64_t funcAddress, uint32_t repeatCount, uint32_t level, void *data)
{
    FILE *out = (FILE *)data;
    // for (uint32_t l = 0; l < level; l++)
    //     fprintf(out, "  ");
    fprintf(out, "%*s%#016lx (%u)\n", level * 2, ((level)? "\\-" : ""), funcAddress, repeatCount);
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
                        // printf ("funcAddressesStart = %p, funcAddressesEnd = %p, funcAddressesCurrent = %p, addr = %lx, end = %u\n",
                        //         (void *)funcAddressesStart, (void *)funcAddressesEnd, (void *)funcAddressesCurrent, addr, end);
                        if (functionTreeAppend(addr, end))
                            funcAddressesCurrent++;
                        else
                            break;
                    }
                    if (funcAddressesCurrent < (uint64_t *)funcAddressesEnd)
                        break;
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
                // TODO: print the tree to output file here
                FILE *outStream = fopen(outFile, "w");
                if (outStream)
                {
                    swNTreePrint(functionTree, printFuncAddress, outStream);
                    fclose(outStream);
                }
                exitCode = EXIT_SUCCESS;
            }
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