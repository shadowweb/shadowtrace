#ifndef _STACK_
#define _STACK_

#include <stdint.h>
#include <stdbool.h>

typedef struct swStack
{
    void **elements;
    uint32_t count;
    uint32_t size;
} swStack;

swStack *swStackNew(uint32_t size);
void swStackDelete(swStack *stack);

bool swStackPush(swStack *stack, void *item);
void *swStackPop(swStack *stack);
void *swStackPeek(swStack *stack);

#endif