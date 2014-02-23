#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        else
            memset(tree, 0, sizeof(swNTree));
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
            printf ("newSize = %u, parent->children = %p\n", newSize, parent->children);
            // TODO: figure out why realloc with memset does not work
            // swNTree *newChildren = realloc(parent->children, (size_t)newSize * sizeof(swNTree));
            swNTree *newChildren = calloc((size_t)newSize, sizeof(swNTree));
            if (newChildren)
            {
                if (parent->size)
                {
                    memcpy(newChildren, parent->children, parent->count * sizeof(swNTree));
                    free(parent->children);
                }
                // memset (&newChildren[parent->count], 0, (parent->size - parent->count) * sizeof(swNTree));
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

static void swNTreePrintNode(swNTree *node, swNTreeWriteCB *writeCB, void *data, uint32_t level)
{
    writeCB(node->funcAddress, node->repeatCount, level, data);
    for (uint32_t i = 0; i < node->count; i++)
        swNTreePrintNode(&(node->children[i]), writeCB, data, level + 1);
}

void swNTreePrint(swNTree *root, swNTreeWriteCB *writeCB, void *data)
{
    if (root && writeCB)
    {
        uint32_t level = 0;
        if (!(root->funcAddress & NTREE_ROOT_FLAG))
        {
            writeCB(root->funcAddress, root->repeatCount, level, data);
            level++;
        }
        for (uint32_t i = 0; i < root->count; i++)
            swNTreePrintNode(&(root->children[i]), writeCB, data, level);
    }
}
