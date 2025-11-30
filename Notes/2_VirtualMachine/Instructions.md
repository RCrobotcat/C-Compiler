```c
// 虚拟机的入口，用于解释目标代码
int eval()
{
    int op, *tmp;
    while (1)
    {
        if (op == IMM) { ax = *pc++; } // load immediate value to ax
        else if (op == LC) { ax = *(char *) ax; } // load character to ax, address in ax
        else if (op == LI) { ax = *(int *) ax; } // load integer to ax, address in ax
        else if (op == SC) { ax = *(char *) *sp++ = ax; } // save character to address, value in ax, address on stack
        else if (op == SI) { *(int *) *sp++ = ax; } // save integer to address, value in ax, address on stack
    }

    return 0;
}

```
---

# 总体流程

你的 `while(1)` 是一个指令解释循环（instruction dispatch loop）。

`op` 是当前操作码
`pc` 是指令指针（program counter）
`sp` 是栈指针（stack pointer）
`ax` 是累加器寄存器（accumulator），类似于 CPU 的 AX 寄存器

---

## 1. `if (op == IMM) { ax = *pc++; }`

**含义：Load Immediate（加载立即数）**

* 从 `pc` 指向的下一段字节（通常是 4 字节 int）中取出一个值
* 放到 `ax` 里
* `pc++` => 指令指针前移到下一条指令

**就像汇编中的：**

```
mov ax, #123
```

**示例**
假设指令流里是：`IMM 5`
执行后：`ax = 5`

---

## 2. `else if (op == LC) { ax = *(char *) ax; }`

**含义：Load Char（从 ax 中的地址取一个 char 出来）**

* 现在 `ax` 不是值，而是一个地址
* 取这个地址里的 `char`
* 存入 `ax`

**汇编类比：**

```
mov al, [ax]     ; 把 ax 指向的内存里的 1 字节读到 ax(低8位)
```

**示例**
如果 ax = 1000（地址），内存 1000 里是 `'A'`
执行后：`ax = 'A'`

---

## 3. `else if (op == LI) { ax = *(int *) ax; }`

**含义：Load Integer（从 ax 中的地址取一个 int 出来）**

* ax 里放的是地址
* 从这个地址读 4 字节整数到 ax

**就像：**

```
mov eax, [eax]   ; 取内存里的 4 字节
```

如果 ax=2000，内存 2000~2003 存着 12345
=> 执行后：`ax = 12345`

---

## 4. `else if (op == SC) { ax = *(char *) *sp++ = ax; }`

**含义：Store Char（存储 char）**

拆开理解：

`*sp++`

* 取出栈顶的地址（sp 指向的值）
* sp++ 出栈（sp 指向下一个位置）

`*(char*)address = ax`

* 把 ax 的低 8 位写入该地址

**最终 ax = 写入的那个字节**

等价于：

```
address = *sp;
sp++;
*(char*)address = (char)ax;
ax = (char)ax;
```

**用途：写一个 byte 到内存里。**

---

## 5. `else if (op == SI) { *(int *) *sp++ = ax; }`

**含义：Store Int（存储 4 字节整数）**

同 SC，流程是：

* 栈里弹出一个地址
* 将 ax（一个 int）写入那段内存

相当于：

```
address = *sp;
sp++;
*(int*)address = ax;
```

**用途：把 ax 里的 int 写入内存。**

---

# 总结表（最便于记忆）

| 指令  | 作用                   | ax 是什么 | sp 的变化 | 操作内容            |
| --- | -------------------- | ------ | ------ | --------------- |
| IMM | ax = *pc++           | 值      | pc++   | 从指令流取立即数        |
| LC  | ax = *(char*)ax      | 地址     | 不变     | 从内存加载 1 字节      |
| LI  | ax = *(int*)ax       | 地址     | 不变     | 从内存加载 4 字节      |
| SC  | *(char*)(*sp++) = ax | 值      | sp++   | 把 ax 写入地址（char） |
| SI  | *(int*)(*sp++) = ax  | 值      | sp++   | 把 ax 写入地址（int）  |

---
```c
else if (op == PUSH) { *--sp = ax; } // push the value of ax onto the stack
else if (op == JMP) { pc = (int *) *pc; } // jump to the address
else if (op == JZ) { pc = ax ? pc + 1 : (int *) *pc; } // jump if ax is zero
else if (op == JNZ) { pc = ax ? (int *) *pc : pc + 1; } // jump if ax is not zero
```
---

# 1. `else if (op == PUSH) { *--sp = ax; }`

## 含义：Push（向栈中压入 ax 的值）

`*--sp = ax` 分为两步：

1. `--sp` → 栈指针向低地址移动一格
   （此 VM 的栈是从高地址向低地址增长的）

2. `*sp = ax` → 将 ax 写入新的栈顶

等价于：

```c
sp = sp - 1;
*sp = ax;
```

### 用途

把寄存器 ax 的值压到栈里，用于：

* 保存临时值
* 传参
* 保存旧的返回地址/局部变量

---

# 2. `else if (op == JMP) { pc = (int *) *pc; }`

## 含义：JMP（无条件跳转）

* `*pc` 取得指令流中的下一项（这是 jump 目标地址）
* 将 `pc` 设置为该地址，实现跳转

等价于：

```
target = *pc;
pc = target;
```

### 示例

假设指令流中是：

```
JMP 1000
```

执行后：`pc = 1000` → 直接跳过去。

---

# 3. `else if (op == JZ) { pc = ax ? pc + 1 : (int *) *pc; }`

## 含义：JZ（Jump if Zero）

JZ = “jump if ax == 0”

翻译成自然语言：

* 如果 ax ≠ 0：不跳，跳过跳转目标（pc + 1）
* 如果 ax == 0：跳转到 `*pc`

### 等价逻辑

```c
if (ax == 0) {
    pc = (int*) *pc;   // 跳转
} else {
    pc = pc + 1;       // 不跳，跳过目标字段
}
```

### 背后原因

因为指令格式是：

```
JZ <目标地址>
```

所以：

* 如果不跳，需要跳过这个地址数据 → `pc + 1`

---

# 4. `else if (op == JNZ) { pc = ax ? (int *) *pc : pc + 1; }`

## 含义：JNZ（Jump if Not Zero）

JNZ = “jump if ax != 0”

自然语言：

* ax ≠ 0 → 跳转到 `*pc`
* ax == 0 → 不跳，pc+1 跳过目标字段

### 等价 C 代码

```c
if (ax != 0) {
    pc = (int*)*pc;
} else {
    pc = pc + 1;
}
```

---

# 这四条指令的总结（记住以下心法即可）

| 指令   | 作用           | 条件      | pc 行为                |
| ---- | ------------ | ------- | -------------------- |
| PUSH | 把 ax 压栈      | 无       | `*--sp = ax`         |
| JMP  | 无条件跳转        | 无       | `pc = *pc`           |
| JZ   | ax == 0 → 跳转 | ax == 0 | `pc = *pc` （否则 pc+1） |
| JNZ  | ax != 0 → 跳转 | ax != 0 | `pc = *pc` （否则 pc+1） |

---
```c
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
else if (op == LEA) { ax = (int) (bp + *pc++); } // load address for arguments.
```
---

# 1. `CALL`

```c
else if (op == CALL) {
    *--sp = (int)(pc + 1);
    pc = (int*)*pc;
}
```

## 含义：函数调用（CALL subroutine）

这段做了两件事：

### (1) 保存返回地址到栈

```
*--sp = pc + 1
```

* `pc + 1` 是下一条指令（返回后继续执行的位置）
* 栈是向下长的，所以 `--sp` 先移动栈，再写入
* 等价于：

  ```c
  sp = sp - 1;
  *sp = return_address;
  ```

### (2) pc 跳转到函数入口

```
pc = (int*)*pc
```

* 当前的 pc 指向 CALL 后面的那个“目标地址”字段
* 将它取出来，作为新的 pc

### 总结

CALL =

```
push return_address
jump to target_function_address
```

和汇编一致：

```
push return_address
jmp func
```

---

# 2. `ENT`

```c
else if (op == ENT) {
    *--sp = (int)bp;
    bp = sp;
    sp = sp - *pc++;
}
```

## 含义：进入函数（Enter）= 创建新的栈帧（stack frame）

执行顺序如下：

### (1) 保存旧的 bp

```
*--sp = bp
```

作用：为当前函数创建新的栈帧结构。

### (2) 设置新的 bp

```
bp = sp
```

新的 bp 指向当前栈顶，也就是当前函数的“栈帧基址”。

### (3) 为局部变量分配空间

```
sp = sp - *pc++;
```

* 指令格式：`ENT local_var_size`
* pc++ 移过参数，取地址后得到 `local_var_size`
* sp 向低地址移动，开辟局部变量空间

### 栈帧结构变化（视觉化）

```
调用者的栈
 ────────────────────────────
 push return_address  ← CALL
 push old_bp           ← ENT
 new bp → [old_bp]
          [local var 1]
          [local var 2]
          ...
 sp →    [local var N]
```

---

# 3. `LEV`

```c
else if (op == LEV) {
    sp = bp;
    bp = (int *) *sp++;
    pc = (int *) *sp++;
}
```

## 含义：离开函数（Leave）= 销毁当前栈帧并回到调用者

这条指令与 ENT 完全相反，是函数结束时恢复调用者现场的步骤。

执行顺序如下：

### (1) 丢弃当前函数的局部变量

```
sp = bp
```

作用：
把 sp 直接恢复到当前函数的基址（bp），也就是局部变量开始的位置。

* ENT 时 sp 向下移动创建局部变量空间
* LEV 时 sp 一次性跳回 bp，直接丢掉整个局部变量区域

### (2) 恢复调用者的 bp

```
bp = (int *) *sp++;
```

作用：

* 从当前栈帧顶部（bp 位置）取出调用者的 bp
* sp++ 跳过该槽位
* 使 bp 恢复到进入当前函数之前的值

这一步正好和 ENT 中的：

```
*--sp = old_bp
```

相反。

### (3) 恢复返回地址（pc）

```
pc = (int *) *sp++;
```

作用：

* 从栈中取回 CALL 时压入的 return address
* 写入 pc，让程序回到调用者继续执行
* sp++ 跳过返回地址槽位

## 栈帧结构恢复（视觉化）

以下是调用者的栈在函数返回（LEV）时的恢复过程：

```
当前函数的栈帧（执行 LEV 前）
──────────────────────────────────
[ local var N     ]
[ local var ...    ]
[ local var 1      ]
bp → [ old_bp      ]
      [ return_addr ]  ← CALL 时压入
sp →  (其他栈内容)

执行 LEV：
1) sp = bp           → 丢弃所有局部变量
2) bp = *sp++        → 恢复 old_bp
3) pc = *sp++        → 恢复 return_addr

恢复后的栈：
──────────────────────────────────
调用者的栈（回到 CALL 后继续执行）
```

---

# 4. `ADJ`

```c
else if (op == ADJ) { 
    sp = sp + *pc++; 
}
```

## 含义：Adjust stack = 弹出参数（函数返回前恢复栈）

ADJ 通常在函数返回后使用：

```
ADJ <n>
```

表示：

```
sp += n
```

### 用途

* 清理函数参数
* C4 VM 用的是 caller 清理模式（和 C 一样）

比如：

```
foo(a, b, c)
```

返回后：

```
ADJ 3   // 弹掉这三个参数
```

---

# 5. `LEA`

```c
else if (op == LEA) {
    ax = (int)(bp + *pc++);
}
```

## 含义：Load Effective Address（取地址）

LEA 得到一个相对 bp 的偏移量地址。

例如：

```
LEA 1
```

= `ax = bp + 1`

### 用途

* 获取函数参数地址
* 获取局部变量地址
* 相当于 C 中的取地址运算符 `&`

### 栈帧结构回忆：

假设是这样的布局：

```
[bp + 0] = old_bp
[bp + 1] = return_address
[bp + 2] = arg1
[bp + 3] = arg2
...
[bp - 1] = local1
[bp - 2] = local2
```

那么：

```
LEA 2  => 获得 arg1 地址
LEA -1 => 获得 local1 地址
```

---

# 整体串联（CALL → ENT → LEA → ADJ）

一个典型函数调用过程：

### 调用者（caller）执行：

```
PUSH arg2
PUSH arg1
CALL func
ADJ 2    // 弹掉参数
```

### 函数入口处执行：

```
ENT 3    // 为 3 个局部变量开栈
```

函数内部例如访问第一个参数：

```
LEA 2   // ax = 地址
LI      // ax = *(int*)(地址)
```

函数结束：

```
RET
```

而 RET（你是注释掉的版本）

```
pc = *sp++;
```

重新跳回 caller 中保存的 return_addr

---

# 总结表（完整一套函数栈帧指令）

| 指令   | 功能    | 栈变化                    | pc 变化  | 主要用途        |
| ---- | ----- | ---------------------- | ------ | ----------- |
| CALL | 调用函数  | push return_addr       | 跳到函数入口 | 函数调用        |
| ENT  | 建立栈帧  | push old_bp，sp-=locals | pc++   | 创建局部变量区     |
| ADJ  | 清理栈参数 | sp+=size               | pc++   | 函数返回后弹掉参数   |
| LEA  | 计算地址  | 不动                     | pc++   | 获取局部变量/参数地址 |

---
啊，我明白你真正想问的点了。

你问的是：

> **为什么压入参数（PUSH）和弹出参数（ADJ）时，使用的“参数大小 / 偏移量”等数据都是 `*pc++`，而不是 `*sp` 或其他地方？**

也就是说：

* `ADJ` 使用 `sp = sp + *pc++;`
* `LEA` 使用 `ax = bp + *pc++;`
* `ENT` 使用 `sp = sp - *pc++;`
* `JMP/JZ/JNZ/CALL` 都用 `*pc++` 读跳转地址

而不是读栈中的任何值。

你想知道：**为什么这些“立即数参数”都是跟着指令存在指令流里，由 `pc` 来读取？**

下面我给你彻底讲清楚。

---

# 核心原因一句话说明

**因为这些指令后的数字是“指令参数（immediate operand）”，属于指令流的一部分，只能由 PC 读取，而不是数据栈 SP 读取。**

它不是存储在运行时的栈里，而是存储在“代码区”（指令区）中。  
并且前置自增 (++i) 和后置自增 (i++) 的优先级都比 *（解引用）高。

---

# （1）`*pc++` 的本质：读取指令的“立即参数”

例如指令：

```
ADJ 3
```

在 VM 的代码区中存储为两个单元：

| 地址   | 内容  |
| ---- | --- |
| pc   | ADJ |
| pc+1 | 3   |

所以执行 ADJ：

```
sp = sp + *pc++;
```

这里 `pc++` 的含义是：

* 首先读取 `3` 这个立即数
* 然后 PC 自动移动到下一条指令

这是正常的“取指操作（instruction operand fetch）”。

---

# （2）为什么不是从栈 sp 读？

因为参数不是运行时数据，而是“编译时决定的指令参数”。

比如：

```
ENT 5
```

这里“5”表示函数的局部变量空间大小，这个信息**写在指令序列里**，不是在栈里。

栈 sp 本来用来放：

* 返回地址
* 临时值
* 参数的实际值
* 局部变量

而不是“指令操作数”。

举例：

```
ADJ 2
```

“2” 只是告诉 VM：
**函数返回时要弹掉 2 个参数。**

这个数量不会放在运行时的数据栈里，当然不能用 `sp` 读取。

---

# （3）为什么立即数跟在指令后面？（经典 VM 设计）

例如指令流：

```
PUSH 5
PUSH 10
CALL func
ADJ 2
```

在内存里是这样：

```
[PC]   PUSH
[PC+1] 5
[PC+2] PUSH
[PC+3] 10
[PC+4] CALL
[PC+5] <func address>
[PC+6] ADJ
[PC+7] 2
```

VM 执行 PUSH：

```
*--sp = *pc++;     // 读取“5”
```

执行 CALL：

```
*--sp = (pc + 1);  // push return
pc = (int*) *pc;   // 跳转到函数入口
```

执行 ADJ：

```
sp = sp + *pc++;   // 用 ADJ 后面的那个参数“2”
```

这些全都是从指令流读取的。

---

# （4）为什么 CALL/JMP/JZ/JNZ 也都是 `*pc++`？

理由相同：

跳转地址是一个“指令操作数（operand）”，不是运行时数据。

例如：

```
JZ else_block
```

机器码：

```
[PC]   JZ
[PC+1] &else_block
```

VM 执行时：

```
pc = ax ? pc + 1 : (int*)*pc;
```

* `*pc` 是跳转目标地址
* `pc+1` 是跳过该地址字段

这是经典 RISC/VM 的设计。

---

# （5）总结（最关键）

| 指令              | 操作对象     | 为什么用 *pc    |
| --------------- | -------- | ----------- |
| ADJ             | 弹掉多少参数   | 参数是写在指令里的数字 |
| ENT             | 分配多少局部变量 | 写在指令里的数字    |
| LEA             | 加多少偏移    | 写在指令里的数字    |
| JMP/JZ/JNZ/CALL | 跳转目标地址   | 也是写在指令里的立即数 |
| PUSH imm        | 立即数      | 也是从 pc 读    |

一句话：

> **所有立即数（操作数）都跟在指令后面，是指令流的一部分，由 PC 来读取，这叫 IMMEDIATE OPERAND FETCH。**

而不是运行时栈的数据，所以不能用 SP。
