//
// Created by 25190 on 2025/11/23.
//

#include <stdio.h>

int fibonacci(int i) {
    if (i <= 1) {
        return 1;
    }
    return fibonacci(i - 1) + fibonacci(i - 2);
}

int main() {
    // must firstly declare variables at the beginning of a block
    int x;
    int a;
    int i;
    char* str;

    // then statements
    str = "Hello, World!\n";
    printf("%s", str);

    x = 1;
    a = x == 0 ? 10 : 20;
    printf("a = %d\n", a);

    i = 0;

    while (i <= 10) {
        printf("fibonacci(%2d) = %d\n", i, fibonacci(i));
        i = i + 1;
    }
    return 0;
}
