#ifndef _NTREE_
#define _NTREE_

#include <stdbool.h>
#include <stdint.h>

typedef struct swNTree
{
    // meta data section
    struct swNTree *children;
    uint32_t count;
    uint32_t size;
    // data section
    uint64_t funcAddress;
    uint32_t repeatCount;
} swNTree;

typedef void swNTreeWriteCB(uint64_t funcAddress, uint32_t repeatCount, uint32_t level, void *data);

swNTree *swNTreeNew(uint64_t funcAddress);
void swNTreeDelete(swNTree *tree);

swNTree *swNTreeAddNext(swNTree *parent, uint64_t funcAddress);
int swNTreeCompare(swNTree *node1, swNTree *node2);
void swNTreePrint(swNTree *root, swNTreeWriteCB *writeCB, void *data);

#endif