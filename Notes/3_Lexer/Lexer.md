- `main.c`

```c
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

int token; // current token
char *src, *old_src; // pointer to source code string;
int poolsize; // default size of text/data/stack
int line; // line number

int *text; // text segment
int *old_text; // for dump text segment
int *stack; // stack
char *data; // data segment

int *pc, *sp, *bp, ax, cycle; // virtual machine registers

// instructions
enum
{
    LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH,
    OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
    OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT
};

// tokens and classes (operators last and in precedence order)
enum
{
    Num = 128, Fun, Sys, Glo, Loc, Id,
    Char, Else, Enum, If, Int, Return, Sizeof, While,
    Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
/* Map POSIX names to MSVC names */
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#endif
#define open  _open
#define read  _read
#define close _close
#else
#include <unistd.h>
#include <fcntl.h>
#endif

int token_val; // value of current token (mainly for number)

int *current_id; // current parsed ID
int *symbols; // symbol table 符号表

// fields of identifier
enum { Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize };

// 词法分析器，获取下一个标记，它将自动忽略空白字符
void next()
{
    char *last_pos;
    int hash;

    while (token = *src) // token == 0 means end of source code
    {
        ++src;
        if (token == '\n') // newline
        {
            ++line;
        } else if (token == '#') // macro 宏定义
        {
            // skip macro, because we will not support it
            while (*src != 0 && *src != '\n')
            {
                src++;
            }
        } else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) // identifiers
        {
            // parse identifier
            last_pos = src - 1;
            hash = token;

            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9')
                   || (*src == '_'))
            {
                hash = hash * 147 + *src;
                src++;
            }

            // look for existing identifier, linear search
            // 在符号表中查找已有的标识符，线性搜索
            current_id = symbols;
            while (current_id[Token]) // current_id[TOKEN] == 0 means empty entry
            {
                // compare the hash first then the name
                if (current_id[Hash] == hash && !memcmp((char *) current_id[Name], last_pos, src - last_pos))
                {
                    // found one, return
                    token = current_id[Token];
                    return;
                }
                current_id = current_id + IdSize;
            }

            // store new ID
            current_id[Name] = (int) last_pos;
            current_id[Hash] = hash;
            token = current_id[Token] = Id;
            return;
        } else if (token >= '0' && token <= '9') // numbers
        {
            // parse number, three kinds: dec(123) hex(0x123) oct(017)
            token_val = token - '0';

            if (token_val > 0)
            {
                // dec, starts with [1-9]
                while (*src >= '0' && *src <= '9')
                {
                    token_val = token_val * 10 + *src++ - '0';
                }
            } else
            {
                // starts with number 0
                if (*src == 'x' || *src == 'X')
                {
                    //hex
                    token = *++src;
                    while ((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || (
                               token >= 'A' && token <= 'F'))
                    {
                        token_val = token_val * 16 + (token & 15) + (token >= 'A' ? 9 : 0);
                        token = *++src;
                    }
                } else
                {
                    // oct
                    while (*src >= '0' && *src <= '7')
                    {
                        token_val = token_val * 8 + *src++ - '0';
                    }
                }
            }

            token = Num;
            return;
        } else if (token == '"' || token == '\'') // string
        {
            // parse string literal, currently, the only supported escape
            // character is '\n', store the string literal into data.
            last_pos = data;
            while (*src != 0 && *src != token) // 读到同样的引号（字符串结束）就停
            {
                token_val = *src++;
                if (token_val == '\\')
                {
                    // escape character
                    token_val = *src++;
                    if (token_val == 'n') // 遇上了换行符 \n
                    {
                        token_val = '\n';
                    }
                }

                if (token == '"') // 如果为字符串"..." 则存入data
                {
                    *data++ = token_val;
                }
            }

            src++;
            // if it is a single character, return Num token
            if (token == '"')
            {
                token_val = (int) last_pos; // 指向字符串起始地址
            } else
            {
                token = Num; // 这是单个字符 => 数字 token
            }

            return;
        } else if (token == '/') // comments
        {
            if (*src == '/')
            {
                // skip comments
                while (*src != 0 && *src != '\n')
                {
                    ++src;
                }
            } else
            {
                // divide operator
                token = Div;
                return;
            }
        }

        // parse operators and punctuators(标点符号)
        else if (token == '=')
        {
            // parse '==' and '='
            if (*src == '=')
            {
                src++;
                token = Eq;
            } else
            {
                token = Assign;
            }
            return;
        } else if (token == '+')
        {
            // parse '+' and '++'
            if (*src == '+')
            {
                src++;
                token = Inc;
            } else
            {
                token = Add;
            }
            return;
        } else if (token == '-')
        {
            // parse '-' and '--'
            if (*src == '-')
            {
                src++;
                token = Dec;
            } else
            {
                token = Sub;
            }
            return;
        } else if (token == '!')
        {
            // parse '!='
            if (*src == '=')
            {
                src++;
                token = Ne;
            }
            return;
        } else if (token == '<')
        {
            // parse '<=', '<<' or '<'
            if (*src == '=')
            {
                src++;
                token = Le;
            } else if (*src == '<')
            {
                src++;
                token = Shl;
            } else
            {
                token = Lt;
            }
            return;
        } else if (token == '>')
        {
            // parse '>=', '>>' or '>'
            if (*src == '=')
            {
                src++;
                token = Ge;
            } else if (*src == '>')
            {
                src++;
                token = Shr;
            } else
            {
                token = Gt;
            }
            return;
        } else if (token == '|')
        {
            // parse '|' or '||'
            if (*src == '|')
            {
                src++;
                token = Lor;
            } else
            {
                token = Or;
            }
            return;
        } else if (token == '&')
        {
            // parse '&' and '&&'
            if (*src == '&')
            {
                src++;
                token = Lan;
            } else
            {
                token = And;
            }
            return;
        } else if (token == '^')
        {
            token = Xor;
            return;
        } else if (token == '%')
        {
            token = Mod;
            return;
        } else if (token == '*')
        {
            token = Mul;
            return;
        } else if (token == '[')
        {
            token = Brak;
            return;
        } else if (token == '?')
        {
            token = Cond;
            return;
        } else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token
                   == ']' || token == ',' || token == ':')
        {
            // directly return the character as token;
            return;
        }
    }

    return;
}

// 解析一个表达式
void expression(int level)
{
    // do nothing
}

// 语法分析的入口，分析整个 C 语言程序
void program()
{
    next(); // get next token
    while (token > 0)
    {
        printf("token is: %c\n", token);
        next();
    }
}

// 虚拟机的入口，用于解释目标代码
int eval()
{
    int op, *tmp;
    while (1)
    {
        op = *pc++; // get next operation code

        if (op == IMM) { ax = *pc++; } // load immediate value to ax => mov ax, #123
        else if (op == LC) { ax = *(char *) ax; } // load character to ax, address in ax => mov al, [ax]
        else if (op == LI) { ax = *(int *) ax; } // load integer to ax, address in ax
        else if (op == SC) { ax = *(char *) *sp++ = ax; } // save character to address, value in ax, address on stack
        else if (op == SI) { *(int *) *sp++ = ax; } // save integer to address, value in ax, address on stack
        else if (op == PUSH) { *--sp = ax; } // push the value of ax onto the stack
        else if (op == JMP) { pc = (int *) *pc; } // jump to the address
        else if (op == JZ) { pc = ax ? pc + 1 : (int *) *pc; } // jump if ax is zero
        else if (op == JNZ) { pc = ax ? (int *) *pc : pc + 1; } // jump if ax is not zero
        else if (op == CALL)
        {
            *--sp = (int) (pc + 1);
            pc = (int *) *pc;
        } // call subroutine
        //else if (op == RET)  { pc = (int *) *sp++; } // return from subroutine;
        else if (op == ENT)
        {
            *--sp = (int) bp;
            bp = sp;
            sp = sp - *pc++;
        } // make new stack frame
        else if (op == ADJ) { sp = sp + *pc++; } // add esp, <size>
        else if (op == LEV) // leave subroutine
        {
            sp = bp;
            bp = (int *) *sp++;
            pc = (int *) *sp++;
        } // restore call frame and PC
        else if (op == LEA) { ax = (int) (bp + *pc++); } // load address for arguments.

        // binary operations
        else if (op == OR) ax = *sp++ | ax;
        else if (op == XOR) ax = *sp++ ^ ax;
        else if (op == AND) ax = *sp++ & ax;
        else if (op == EQ) ax = *sp++ == ax;
        else if (op == NE) ax = *sp++ != ax;
        else if (op == LT) ax = *sp++ < ax;
        else if (op == LE) ax = *sp++ <= ax;
        else if (op == GT) ax = *sp++ > ax;
        else if (op == GE) ax = *sp++ >= ax;
        else if (op == SHL) ax = *sp++ << ax;
        else if (op == SHR) ax = *sp++ >> ax;
        else if (op == ADD) ax = *sp++ + ax;
        else if (op == SUB) ax = *sp++ - ax;
        else if (op == MUL) ax = *sp++ * ax;
        else if (op == DIV) ax = *sp++ / ax;
        else if (op == MOD) ax = *sp++ % ax;

            // functions
        else if (op == EXIT)
        {
            printf("exit(%d)", *sp);
            return *sp;
        } else if (op == OPEN)
        {
            ax = open((char *) sp[1], sp[0]);
        } else if (op == CLOS)
        {
            ax = close(*sp);
        } else if (op == READ)
        {
            ax = read(sp[2], (char *) sp[1], *sp);
        } else if (op == PRTF)
        {
            tmp = sp + pc[1];
            ax = printf((char *) tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]);
        } else if (op == MALC)
        {
            ax = (int) malloc(*sp);
        } else if (op == MSET)
        {
            ax = (int) memset((char *) sp[2], sp[1], *sp);
        } else if (op == MCMP)
        {
            ax = memcmp((char *) sp[2], (char *) sp[1], *sp);
        } else
        {
            printf("unknown instruction:%d\n", op);
            return -1;
        }
    }

    return 0;
}

// types of variable/function
enum { CHAR, INT, PTR };

int *idmain; // the `main` function

int main(int argc, char **argv)
{
#define int long long // work with 64bit target

    int i, fd;

    argc--;
    argv++;

    poolsize = 256 * 1024; // arbitrary size
    line = 1;

    if ((fd = open(*argv, 0)) < 0)
    {
        printf("could not open(%s)\n", *argv);
        return -1;
    }

    // allocate memory for virtual machine
    if (!(text = old_text = malloc(poolsize)))
    {
        printf("could not malloc(%d) for text area\n", poolsize);
        return -1;
    }
    if (!(data = malloc(poolsize)))
    {
        printf("could not malloc(%d) for data area\n", poolsize);
        return -1;
    }
    if (!(stack = malloc(poolsize)))
    {
        printf("could not malloc(%d) for stack area\n", poolsize);
        return -1;
    }

    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);
    bp = sp = (int *) ((int) stack + poolsize);
    ax = 0;

    
    src = "char else enum if int return sizeof while "
            "open read close printf malloc memset memcmp exit void main";

    // add keywords to symbol table
    // 把 C 语言关键字加入符号表
    i = Char;
    while (i <= While)
    {
        next();
        current_id[Token] = i++;
    }

    // add library to symbol table
    // 把内置库函数加入符号表
    i = OPEN;
    while (i <= EXIT)
    {
        next();
        current_id[Class] = Sys;
        current_id[Type] = INT;
        current_id[Value] = i++;
    }

    next();
    // 把 void 当成 char 处理（简化语法）
    current_id[Token] = Char; // handle void type
    
    next();
    // 保存 main 函数位置
    idmain = current_id; // keep track of main


    // read the source file
    if ((fd = open(*argv, 0)) < 0)
    {
        printf("could not open(%s)\n", *argv);
        return -1;
    }

    if (!(src = old_src = malloc(poolsize)))
    {
        printf("could not malloc(%d) for source area\n", poolsize);
        return -1;
    }
    // read the source file
    if ((i = read(fd, src, poolsize - 1)) <= 0)
    {
        printf("read() returned %d\n", i);
        return -1;
    }
    src[i] = 0; // add EOF character
    close(fd);

    program();

    return eval();
}

```