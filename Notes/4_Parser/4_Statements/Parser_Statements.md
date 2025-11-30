```c
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

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

int token; // current token
int token_val; // value of current token (mainly for number)
char *src, *old_src; // pointer to source code string;
int poolsize; // default size of text/data/stack
int line; // line number

int *text; // text segment
int *old_text; // for dump text segment
int *stack; // stack
char *data; // data segment

int *pc, *sp, *bp, ax, cycle; // virtual machine registers
// ax stores the return value of expressions

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

// types of variable/function
enum { CHAR, INT, PTR }; // PTR means pointer to ...

int *idmain; // the `main` function

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

int base_type; // the type of a declaration, make it global for convenience
int expr_type; // the type of an expression

void match(int tk)
{
    if (token == tk)
    {
        next();
    } else
    {
        printf("%d: expected token: %d\n", line, tk);
        exit(-1);
    }
}

// 解析表达式
void expression(int level)
{
    // TODO: next chapters
    return;
}

// 解析语句
void statement()
{
    // there are 6 kinds of statements here:
    // 1. if (...) <statement> [else <statement>]
    // 2. while (...) <statement>
    // 3. { <statement> }
    // 4. return xxx;
    // 5. <empty statement>;
    // 6. expression; (expression end with semicolon)

    int *a, *b; // bess for branch control

    if (token == If)
    {
        // if (...) <statement> [else <statement>]
        //
        //   if (...)           <cond>
        //                      JZ a
        //     <statement>      <statement>
        //   else:              JMP b
        // a:                 a:
        //     <statement>      <statement>
        // b:                 b:
        //
        //
        match(If);
        match('(');
        expression(Assign); // parse condition
        match(')');

        // emit code for if
        *++text = JZ;
        b = ++text; // 等待后续回填 JZ 的跳转地址

        statement(); // parse statement
        if (token == Else)
        {
            // parse else
            match(Else);

            // emit code for JMP B
            *b = (int) (text + 3); // 之前 JZ 的跳转地址 *b 被回填为 text + 3
            *++text = JMP;
            b = ++text;

            statement();
        }

        *b = (int) (text + 1);
    } else if (token == While)
    {
        //
        // a:                     a:
        //    while (<cond>)        <cond>
        //                          JZ b
        //     <statement>          <statement>
        //                          JMP a
        // b:                     b:
        match(While);

        a = text + 1;

        match('(');
        expression(Assign);
        match(')');

        *++text = JZ;
        b = ++text; // 跳出循环的地址 等待回填

        statement();

        // 循环体后生成跳回条件的 JMP
        *++text = JMP;
        *++text = (int) a;
        *b = (int) (text + 1); // 回填跳出循环的地址
    } else if (token == '{')
    {
        // { <statement> ... }
        match('{');

        while (token != '}')
        {
            statement();
        }

        match('}');
    } else if (token == Return)
    {
        // return [expression];
        match(Return);

        if (token != ';')
        {
            expression(Assign);
        }

        match(';');

        // emit code for return
        *++text = LEV; // leave the function
    } else if (token == ';')
    {
        // empty statement
        match(';');
    } else
    {
        // a = b; or function_call();
        expression(Assign);
        match(';');
    }
}

#pragma region Function Declaration Parsing
// function frame
//
// 0: arg 1
// 1: arg 2
// 2: arg 3
// 3: return address
// 4: old bp pointer  <- index_of_bp
// 5: local var 1
// 6: local var 2
int index_of_bp; // index of bp pointer on stack
void function_parameter()
{
    int type;
    int params;
    params = 0;
    while (token != ')')
    {
        // int name, ...
        type = INT;
        if (token == Int)
        {
            match(Int);
        } else if (token == Char)
        {
            type = CHAR;
            match(Char);
        }

        // pointer type
        while (token == Mul)
        {
            match(Mul);
            type = type + PTR;
        }

        // parameter name
        if (token != Id)
        {
            printf("%d: bad parameter declaration\n", line);
            exit(-1);
        }
        if (current_id[Class] == Loc)
        {
            printf("%d: duplicate parameter declaration\n", line);
            exit(-1);
        }

        match(Id);

        // 先将全局变量的信息保存 => 以防局部变量和全局变量同名 后续函数结束后再恢复全局变量
        // store the local variable
        current_id[BClass] = current_id[Class];
        current_id[Class] = Loc;
        current_id[BType] = current_id[Type];
        current_id[Type] = type;
        current_id[BValue] = current_id[Value];
        current_id[Value] = params++; // index of current parameter

        if (token == ',')
        {
            match(',');
        }
    }

    index_of_bp = params + 1;
}

void function_body()
{
    // type func_name (...) {...}
    //                   -->|   |<--

    // ... {
    // 1. local declarations
    // 2. statements
    // }

    int pos_local; // position of local variables on the stack.
    int type;
    pos_local = index_of_bp;

    // local declarations
    // 解析函数体内的局部变量的定义，代码与全局的变量定义几乎一样
    while (token == Int || token == Char)
    {
        // local variable declaration, just like global ones.
        base_type = (token == Int) ? INT : CHAR;
        match(token);

        while (token != ';')
        {
            type = base_type;
            while (token == Mul)
            {
                match(Mul);
                type = type + PTR;
            }

            if (token != Id)
            {
                // invalid declaration
                printf("%d: bad local declaration\n", line);
                exit(-1);
            }
            if (current_id[Class] == Loc)
            {
                // identifier exists
                printf("%d: duplicate local declaration\n", line);
                exit(-1);
            }
            match(Id);

            // store the local variable
            current_id[BClass] = current_id[Class];
            current_id[Class] = Loc;
            current_id[BType] = current_id[Type];
            current_id[Type] = type;
            current_id[BValue] = current_id[Value];
            current_id[Value] = ++pos_local; // index of current parameter

            if (token == ',')
            {
                match(',');
            }
        }
        match(';');
    }

    // 在栈上为局部变量预留空间
    // save the stack size for local variables
    *++text = ENT;
    *++text = pos_local - index_of_bp;

    // statements
    while (token != '}')
    {
        statement();
    }

    // emit code for leaving the sub function
    *++text = LEV;
}

void function_declaration()
{
    // type func_name (...) {...}
    //               | this part

    match('(');
    function_parameter();
    match(')');
    match('{');
    function_body();
    //match('}'); // moved to global_declaration's while loop

    // 将符号表中的信息恢复成全局的信息 => 以防局部变量和全局变量同名 因此函数结束后再恢复全局变量
    // unwind local variable declarations for all local variables.
    current_id = symbols;
    while (current_id[Token])
    {
        if (current_id[Class] == Loc)
        {
            current_id[Class] = current_id[BClass];
            current_id[Type] = current_id[BType];
            current_id[Value] = current_id[BValue];
        }
        current_id = current_id + IdSize;
    }
}
#pragma endregion

#pragma region Variables Declaration Parsing
void enum_declaration()
{
    // parse enum [id] { a = 1, b = 3, ...}
    int i;
    i = 0;
    while (token != '}')
    {
        if (token != Id)
        {
            printf("%d: bad enum identifier %d\n", line, token);
            exit(-1);
        }
        next();

        if (token == Assign) // =
        {
            // like {a=10}
            next();
            if (token != Num)
            {
                printf("%d: bad enum initializer\n", line);
                exit(-1);
            }
            i = token_val;
            next();
        }

        current_id[Class] = Num;
        current_id[Type] = INT;
        current_id[Value] = i++;

        if (token == ',')
        {
            next();
        }
    }
}

void global_declaration()
{
    // global_declaration ::= enum_decl | variable_decl | function_decl
    //
    // id => identifier
    //
    // enum_decl ::= 'enum' [id] '{' id ['=' 'num'] {',' id ['=' 'num'} '}'
    //
    // variable_decl ::= type {'*'} id { ',' {'*'} id } ';'
    // {} means 0 or more repetitions; {}表示大括号内的字符0次或多次重复
    //
    // function_decl ::= type {'*'} id '(' parameter_decl ')' '{' body_decl '}'


    int type; // tmp, actual type for variable
    int i; // tmp

    base_type = INT;

    // parse enum, this should be treated alone.
    if (token == Enum)
    {
        // enum [id] { a = 10, b = 20, ... }
        match(Enum);
        if (token != '{')
        {
            match(Id); // skip the [id] part
        }
        if (token == '{')
        {
            // parse the assign part
            match('{');
            enum_declaration();
            match('}');
        }

        match(';');
        return;
    }

    // parse type information
    if (token == Int)
    {
        match(Int);
    } else if (token == Char)
    {
        match(Char);
        base_type = CHAR;
    }

    // parse the comma seperated variable declaration.
    while (token != ';' && token != '}')
    {
        type = base_type;
        // parse pointer type, note that there may exist `int ****x;`
        // CHAR = 0, INT = 1, PTR = 2
        // 1 + 2 = 3  -> int*
        // 1 + 2 + 2 = 5 -> int**
        while (token == Mul)
        {
            match(Mul);
            type = type + PTR;
        }

        if (token != Id)
        {
            // invalid declaration
            printf("%d: bad global declaration\n", line);
            exit(-1);
        }
        if (current_id[Class])
        {
            // identifier exists
            printf("%d: duplicate global declaration\n", line);
            exit(-1);
        }
        match(Id);
        current_id[Type] = type;

        if (token == '(')
        {
            current_id[Class] = Fun;
            current_id[Value] = (int) (text + 1); // the memory address of function
            function_declaration();
        } else
        {
            // variable declaration
            current_id[Class] = Glo; // global variable
            current_id[Value] = (int) data; // assign memory address
            data = data + sizeof(int);
        }

        if (token == ',')
        {
            match(',');
        }
    }
    next();
}
#pragma endregion

// 语法分析的入口，分析整个 C 语言程序
void program()
{
    next(); // get next token
    while (token > 0)
    {
        global_declaration();
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
        else if (op == ENT) // 创建新的栈帧（stack frame） make new stack frame
        {
            *--sp = (int) bp; // 保存旧的 bp => 调用者 bp
            bp = sp; // 设置新的 bp
            sp = sp - *pc++; // 为局部变量分配空间
        }
        // Adjust stack 弹出参数（函数返回前恢复栈）
        else if (op == ADJ) { sp = sp + *pc++; } // add esp, <size> 
        else if (op == LEV) // leave subroutine
        {
            sp = bp; // 把 sp 直接恢复到当前函数的基址（bp），也就是局部变量开始的位置
            bp = (int *) *sp++; // 恢复调用者的 bp
            pc = (int *) *sp++; // 恢复返回地址 pc
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

int main(int argc, char **argv)
{
#define int intptr_t

    int i, fd;
    int *tmp;

    argc--;
    argv++;

    poolsize = 256 * 1024; // arbitrary size
    line = 1;

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
    if (!(symbols = malloc(poolsize)))
    {
        printf("could not malloc(%d) for symbol table\n", poolsize);
        return -1;
    }

    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);
    memset(symbols, 0, poolsize);
    bp = sp = (int *) ((int) stack + poolsize);
    ax = 0;

    src = "char else enum if int return sizeof while "
            "open read close printf malloc memset memcmp exit void main";

    // add keywords to symbol table
    i = Char;
    while (i <= While)
    {
        next();
        current_id[Token] = i++;
    }

    // add library to symbol table
    i = OPEN;
    while (i <= EXIT)
    {
        next();
        current_id[Class] = Sys;
        current_id[Type] = INT;
        current_id[Value] = i++;
    }

    next();
    current_id[Token] = Char; // handle void type
    next();
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

    if (!(pc = (int *) idmain[Value]))
    {
        printf("main() not defined\n");
        return -1;
    }

    // setup stack
    sp = (int *) ((int) stack + poolsize);
    *--sp = EXIT; // call exit if main returns
    *--sp = PUSH;
    tmp = sp;
    *--sp = argc;
    *--sp = (int) argv;
    *--sp = (int) tmp;

    return eval();
}

```