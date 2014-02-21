#include <stdlib.h>

#include "ntree.h"

#define NTREE_ROOT_FLAG 0x8000000000000000UL

swNTree *swNTreeNew(uint64_t funcAddress)
{
    swNTree *tree = calloc(1, sizeof(swNTree));
    if (tree)
    {
        tree->funcAddress = funcAddress;
        tree->repeatCount = 1;
        tree->funcAddress |= NTREE_ROOT_FLAG;
    }
    return tree;
}

void swNTreeDelete(swNTree *tree)
{
    if (tree)
    {
        if (tree->children)
        {
            for (uint32_t i = 0; i < tree->count; i++)
                swNTreeDelete(&tree->children[i]);
            free(tree->children);
        }
        if (tree->funcAddress & NTREE_ROOT_FLAG)
            free(tree);
    }
}

swNTree *swNTreeAddNext(swNTree *parent, uint64_t funcAddress)
{
    swNTree *child = NULL;
    if (parent)
    {
        bool canAdd = false;
        if (parent->count < parent->size)
            canAdd = true;
        else
        {
            uint32_t newSize = (parent->size)? (parent->size * 2) : 1;
            swNTree *newChildren = realloc(parent->children, newSize);
            if (newChildren)
            {
                parent->children = newChildren;
                parent->size = newSize;
                canAdd = true;
            }
        }
        if (canAdd)
        {
            child = &parent->children[parent->count];
            parent->count++;
            child->funcAddress = funcAddress;
            child->repeatCount = 1;
        }
    }
    return child;
}

int swNTreeCompare(swNTree *node1, swNTree *node2)
{
    int rtn = 0;
    if (node1 && node2)
    {
        rtn = (node1->funcAddress > node2->funcAddress) - (node1->funcAddress < node2->funcAddress);
        if (!rtn)
        {
            rtn = (node1->repeatCount > node2->repeatCount) - (node1->repeatCount < node2->repeatCount);
            if (!rtn)
            {
                rtn = (node1->count > node2->count) - (node1->count < node2->count);
                for (uint32_t i = 0; !rtn && (i < node1->count); i++)
                    rtn = swNTreeCompare(&node1->children[i], &node2->children[i]);
            }
        }
    }
    else
        rtn = (node1 > node2) - (node1 < node2);
    return rtn;
}
