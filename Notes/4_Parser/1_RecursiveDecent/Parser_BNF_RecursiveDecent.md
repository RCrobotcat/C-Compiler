- `main.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

int token; // current token
int token_val; // value of current token (mainly for number)
char *src;
int line; // line number

enum { Num };

// 词法分析器，获取下一个标记，它将自动忽略空白字符
void next()
{
    // skip white space
    while (*src == ' ' || *src == '\t')
    {
        src++;
    }

    token = *src++;

    if (token >= '0' && token <= '9')
    {
        token_val = token - '0';
        token = Num;

        while (*src >= '0' && *src <= '9')
        {
            token_val = token_val * 10 + *src - '0';
            src++;
        }
        return;
    }
}

// 表达式解析 => BNF
int expr();

void match(int tk)
{
    if (token != tk)
    {
        printf("expected token: %d(%c), got: %d(%c)\n", tk, tk, token, token);
        exit(-1);
    }
    next();
}

int factor()
{
    int value = 0;
    if (token == '(')
    {
        match('(');
        value = expr();
        match(')');
    } else
    {
        value = token_val;
        match(Num);
    }
    return value;
}

int term_tail(int lvalue)
{
    if (token == '*')
    {
        match('*');
        int value = lvalue * factor();
        return term_tail(value);
    } else if (token == '/')
    {
        match('/');
        int value = lvalue / factor();
        return term_tail(value);
    } else
    {
        return lvalue;
    }
}

int term()
{
    int lvalue = factor();
    return term_tail(lvalue);
}

int expr_tail(int lvalue)
{
    if (token == '+')
    {
        match('+');
        int value = lvalue + term();
        return expr_tail(value);
    } else if (token == '-')
    {
        match('-');
        int value = lvalue - term();
        return expr_tail(value);
    } else
    {
        return lvalue;
    }
}

int expr()
{
    int lvalue = term();
    return expr_tail(lvalue);
}

int main(int argc, char *argv[])
{
    // size_t linecap = 0;
    // ssize_t linelen;
    // while ((linelen = getline(&line, &linecap, stdin)) > 0)
    // {
    //     src = line;
    //     next();
    //     printf("%d\n", expr());
    // }
    // return 0;

    char buf[4096];

    while (fgets(buf, sizeof(buf), stdin))
    {
        src = buf;
        next();
        printf("%d\n", expr());
    }
    return 0;
}

```