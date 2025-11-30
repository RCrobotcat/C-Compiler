```c
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
        b = ++text;

        statement(); // parse statement
        if (token == Else)
        {
            // parse else
            match(Else);

            // emit code for JMP B
            *b = (int) (text + 3);
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
        b = ++text;

        statement();

        *++text = JMP;
        *++text = (int) a;
        *b = (int) (text + 1);
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
        *++text = LEV;
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

```
这个 `statement()` 函数是在做「语句级」的递归下降语法分析 + 代码生成，可以理解为一个小型 C 解释器 / 编译器里“解析一条语句并生成虚拟机指令”的核心函数。

我按结构一点点讲：

---

## 整体作用

```c
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
    ...
}
```

* 这个函数不返回值，它的任务是：**根据当前 `token`，识别出接下来的一条语句，并在 `text` 代码段中生成相应的虚拟机指令**。
* 它支持 6 种语句形式：

    1. `if (...) statement [else statement]`
    2. `while (...) statement`
    3. 复合语句：`{ statement ... }`
    4. `return expr;`
    5. 空语句：`;`
    6. 一般表达式语句：`expression;`（如 `a = b + 1;` 或 `foo();`）
* `int *a, *b` 用来做 **分支回填（backpatching）**：即先生成跳转指令占位，等后续知道具体跳转位置后，再把指令中的地址补上。

代码里用到的几个“环境变量”：

* `token`：当前词法记号（来自词法分析器），比如 `If`, `While`, `{`, `Return` 等。
* `match(x)`：检查当前 `token` 是否是 `x`，然后读下一个 token。
* `expression(Assign)`：解析一个表达式，优先级为 `Assign`，并生成相应指令。
* `text`：指向“指令数组”的指针，`*++text = ...` 就是在生成一条虚拟机指令。
* 指令码：

    * `JZ`：如果栈顶值为 0，则跳转（jump if zero）。
    * `JMP`：无条件跳转。
    * `LEV`：函数返回（leave）。

---

## 1. 处理 if / else 语句

```c
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
    b = ++text;

    statement(); // parse statement
    if (token == Else)
    {
        // parse else
        match(Else);

        // emit code for JMP B
        *b = (int) (text + 3);
        *++text = JMP;
        b = ++text;

        statement();
    }

    *b = (int) (text + 1);
}
```

流程拆开：

1. 识别 `if`：

    * `match(If);` 吃掉 `if`
    * `match('(');` 吃掉 `(`
    * `expression(Assign);` 解析条件表达式，并把结果压栈
    * `match(')');` 吃掉 `)`

2. 生成条件跳转：

   ```c
   *++text = JZ;
   b = ++text;
   ```

    * 先写入一条 `JZ` 指令：栈顶为 0 时跳转
    * 紧接着的那个 `int` 位置用来存“跳转目标地址”，但此刻还不知道跳到哪里，所以：

        * 把当前位置指针保存到 `b`，后面再回填

   此时伪指令大致是：

   ```asm
   ...       ; 条件表达式已经把结果压栈
   JZ ?      ; ? 还不知道，等回填
   ```

3. 解析 if 的主体语句：

   ```c
   statement();
   ```

    * 这里会生成 if 分支的代码（true 分支）

4. 检查是否有 `else`：

   ```c
   if (token == Else)
   {
       match(Else);

       // emit code for JMP B
       *b = (int) (text + 3);
       *++text = JMP;
       b = ++text;

       statement();
   }
   ```

   若有 `else`：

    * 之前 `JZ` 的跳转地址 `*b` 被回填为 `text + 3`：

        * 也就是：条件为假时，直接跳过 if 分支代码和后面即将插入的 `JMP` 指令，跳到 else 分支的起始位置。
      
          | 位置     | 内容               | 说明                |
          | ------ |------------------| ----------------- |
          | text   | JMP              | 无条件跳转             |
          | text+1 | 目标地址占位 L_end          | 下次回填              |
          | text+2 | 下一条指令（这一条指令无特殊含义）L_else: |                   |
          | text+3 | else 分支代码开始处     | statement() 会从这里写 |
    ```asm
    if (cond)
        A;
    else
        B;
    
    /* 编译后的指令布局 */
    
    cond
    JZ L_else
    A:
        ...
        JMP L_end
    L_else:
        B:
            ...
    L_end:
          
    ```
  * 然后现在在 if 分支代码后面生成：

    ```c
    *++text = JMP;
    b = ++text;
    ```

    这是一条无条件跳转，目的是 if 分支执行完后，跳过 else 分支。

      * 再次把这个 `JMP` 的“目标地址字段”位置保存到 `b`，记录为待回填。
  * 接着 `statement();` 解析 `else` 分支的语句，生成 else 分支代码。

5. 最后的回填：

   ```c
   *b = (int) (text + 1);
   ```

    * 不论有没有 else，这里的 `b` 最终指向“应该在结束时跳到哪里”的地址字段。
    * 如果没有 else：`b` 还是原来 `JZ` 后的那个位置，因此条件假就跳到 `text + 1`（即 if 分支后面的第一条指令）。
    * 如果有 else：`b` 最后是 if 分支后生成的 `JMP` 的目标位置，所以 if 执行完会跳到整个 if-else 的结尾。

---

## 2. 处理 while 语句

```c
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
    b = ++text;

    statement();

    *++text = JMP;
    *++text = (int) a;
    *b = (int) (text + 1);
}
```

步骤：

1. `match(While);` 吃掉 `while`。

2. `a = text + 1;`

    * 这里记录循环开始位置 `a`，注意是在生成条件前就记录：相当于 L_start 标签：

        * 之后会在 `JMP a` 时使用，使循环体结束后跳回到条件判断位置。

3. 括号里的条件：

    * `match('('); expression(Assign); match(')');`
    * 条件表达式执行完后，结果在栈顶。

4. 生成 `JZ`：

   ```c
   *++text = JZ;
   b = ++text;
   ```

    * 与 if 中一样，`JZ` 后面要跟一个“跳出循环”的地址，先占位置，用 `b` 保存。

5. 解析循环体：

   ```c
   statement();
   ```

6. 循环体后生成跳回条件的 `JMP`：

   ```c
   *++text = JMP;
   *++text = (int) a;
   ```

    * 这就是 `JMP a`，无条件跳回循环开头（即条件检查的位置）。

7. 最后回填 `JZ` 的跳转目标：

   ```c
   *b = (int) (text + 1);
   ```

    * 这时 `text` 已经指向 `JMP a` 的地址字段后面，所以 `text + 1` 就是循环之后的第一条指令。
    * 效果是：当条件为假时，`JZ` 会跳到 `b` 指向的位置，也就是整个循环之后。

整体结构等价于：

```asm
L_start:
  cond
  JZ L_end
  [body]
  JMP L_start
L_end:
```

---

## 3. 处理复合语句 { ... }

```c
} else if (token == '{')
{
    // { <statement> ... }
    match('{');

    while (token != '}')
    {
        statement();
    }

    match('}');
}
```

* 看到 `{`，说明是一个**代码块 / 作用域**。
* 逻辑：

    1. `match('{');` 吃掉 `{`
    2. 当下一个 token 不是 `}` 时，一直递归调用 `statement();` 解析一条条语句。
    3. 遇到 `}` 后结束循环，`match('}');` 吃掉 `}`。

注意：这个版本里没有实现变量作用域的 symbol table 入栈/出栈，只是单纯把 `{` ... `}` 当作“语句序列”。

---

## 4. 处理 return 语句

```c
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
    *++text = LEV;
}
```

* 如果当前 token 是 `Return`：

    1. `match(Return);` 吃掉 `return`
    2. 检查下一个是不是 `;`

        * 不是 `;` 就说明有返回表达式，比如 `return a + b;`
        * 调用 `expression(Assign);` 解析表达式并生成“求值并压栈”的代码
    3. `match(';');` 吃掉分号
    4. 生成 `LEV` 指令：

        * `LEV` 通常表示“函数返回”：弹出栈帧、恢复寄存器、跳回调用点。
        * 返回值通常已经在栈顶，由调用者处理。

---

## 5. 处理空语句 ;

```c
} else if (token == ';')
{
    // empty statement
    match(';');
}
```

* 当前是一个孤立的分号 `;`
* 即 C 中合法但没任何操作的空语句，例如：

  ```c
  ;
  while (x++) ;
  ```
* 这里仅仅吃掉 `;` 就好，不生成任何指令。

---

## 6. 处理一般表达式语句

```c
} else
{
    // a = b; or function_call();
    expression(Assign);
    match(';');
}
```

* 剩下的情况只可能是“以表达式开头的语句”：

    * 比如赋值：`a = b + 1;`
    * 或函数调用：`foo(x, y);`
    * 或其他表达式：`x++;`，`--y;` 等
* 处理方式：

    1. `expression(Assign);`
       解析表达式并生成相应的虚拟机指令。
       多数表达式执行完会在栈顶留下一个值；如果只是函数调用，可能仅有副作用。
    2. `match(';');`
       确认语句以 `;` 结束。

很多 C 编译器内部就是这么区分「以关键字开头的语句」和「以表达式开头的语句」。

---

## 总结

* `statement()` 是一个典型的递归下降语法分析函数，它：

    * 根据当前 `token` 类型，决定是哪种语句；
    * 调用 `match` / `expression` / 自己本身等函数递归解析；
    * 同时向 `text` 这个指令数组中写入虚拟机指令（`JZ`, `JMP`, `LEV` 等）。
* `int *a, *b` 配合 `text` 指针，用指针算术和回填技巧实现 `if` / `while` 的跳转逻辑。
* 处理了：

    * if/else：条件跳转 + 分支回填；
    * while：循环入口和出口的跳转；
    * 复合语句：多个 `statement()` 拼接；
    * return：生成 `LEV` 返回；
    * 空语句：只吃掉分号；
    * 普通表达式语句：表达式 + 分号。
