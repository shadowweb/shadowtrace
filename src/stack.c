#include <stdlib.h>

#include "stack.h"

swStack *swStackNew(uint32_t size)
{
    swStack *rtn = NULL;
    if (size)
    {
        swStack *candidateRtn = malloc(sizeof(swStack));
        if (candidateRtn)
        {
            if ((candidateRtn->elements = malloc(sizeof(void *) * size)))
            {
                candidateRtn->count = 0;
                candidateRtn->size = size;
                rtn = candidateRtn;
            }
            else
                free(candidateRtn);
        }
    }
    return rtn;
}

void swStackDelete(swStack *stack)
{
    if (stack)
    {
        free(stack->elements);
        free(stack);
    }
}

bool swStackPush(swStack *stack, void *item)
{
    bool rtn = false;
    if (stack && item)
    {
        if (stack->count == stack->size)
        {
            uint32_t sizeNew = stack->size * 2;
            void **elementsNew = realloc(stack->elements, sizeNew * sizeof(void *));
            if (elementsNew)
            {
                stack->elements = elementsNew;
                stack->size = sizeNew;
            }
        }
        if (stack->count < stack->size)
        {
            stack->elements[stack->count] = item;
            stack->count++;
            rtn = true;
        }
    }
    return rtn;
}

void *swStackPop(swStack *stack)
{
    void *rtn = NULL;
    if (stack && stack->count)
    {
        rtn = stack->elements[(stack->count - 1)];
        stack->count--;
    }
    return rtn;
}

void *swStackPeek(swStack *stack)
{
    if (stack)
        return stack->elements[(stack->count - 1)];
    return NULL;
}
