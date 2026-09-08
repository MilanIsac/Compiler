#include <stdio.h>

extern int add(int a, int b);

int main(void)
{
    printf("10 + 5 = %d\n", add(10, 5));
    printf("20 + 30 = %d\n", add(20, 30));
    printf("-5 + 8 = %d\n", add(-5, 8));
    printf("100 + 200 = %d\n", add(100, 200));

    return 0;
}
