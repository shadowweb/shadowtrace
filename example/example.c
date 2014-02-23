#include <stdio.h>

int bar()
{
    return 5;
}

int foo()
{
    return bar();
}

void test1()
{
    printf ("test\n");
}

void test()
{
    test1();
    printf ("test\n");
}

int main(int argc, char *argv[])
{
    test();
    test();
    printf("foo returns = %d\n", foo());
    return 0;
}