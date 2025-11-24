```c++
else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) // identifiers
{
    // parse identifier
    last_pos = src - 1;
    hash = token;

    while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (
               *src == '_'))
    {
        hash = hash * 147 + *src;
        src++;
    }

    // look for existing identifier, linear search
    // 在符号表中查找已有的标识符，线性搜索
    current_id = symbols;
    while (current_id[Token])
    {
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
}
```

你现在看的其实就是一个**词法分析器（Lexer）中对变量名 / 函数名进行查表的过程**，用一个简陋的符号表 `symbols[]` 存储所有出现过的标识符。

---

# 先整体来看它做什么？

**当扫描到一个标识符（字母或 _ 开头）时：**

1. **扫描完整个名字**（如 `abc123`）
2. **计算 hash**
3. **在符号表中查找是否已存在这个标识符**
4. 若存在 → 返回已有 ID（token type）
5. 若不存在 → 把它加入符号表

这就是一个非常原始的符号表实现。

---

# 第一部分：识别标识符

```cpp
else if ((token >= 'a' && token <= 'z') || 
         (token >= 'A' && token <= 'Z') || 
         (token == '_'))
```

说明：

* 标识符必须以 字母 或 `_` 开头
* 这是 C 语言的合法标识符规则

---

# 第二部分：记录开始位置 + 计算 hash

```cpp
last_pos = src - 1;
hash = token;
```

`last_pos` 用于记录这个标识符在源码中的起始地址。

`hash = token;`
这里 token 就是第一个字符，比如 'a'，用于初始化 hash。

然后扫描后续字符：

```cpp
while (is alpha / digit / '_') {
    hash = hash * 147 + *src;
    src++;
}
```

这是一个简单的**滚动 Hash**：

* 每加入一个字符就更新 hash
* 用 147（任意的质数）增强分布性

⚠️ 注意：这样形成的 hash 不能保证不冲突，但足够快速。

---

# 第三部分：搜索符号表

```cpp
current_id = symbols;

while (current_id[Token])
{
    if (current_id[Hash] == hash &&
        !memcmp((char *)current_id[Name], last_pos, src - last_pos))
    {
        token = current_id[Token];
        return;
    }
    current_id = current_id + IdSize;
}
```

逐行解释：

### 🔸 **current_id = symbols;**

把指针指向整个符号表开始位置。

### 🔸 **while (current_id[Token])**

符号表是按顺序存储的，
当 `Token == 0` 时表示这个位置还没有存储东西。

### 🔸 如果这个条目存在，就检查两个条件：

---

# 条件1：hash 相等？

```cpp
current_id[Hash] == hash
```

意味着：

* 很可能是同一个标识符
* 但 hash 可能碰撞，所以还不能确定完全相等

---

# 条件2：名字是否完全一致？

```cpp
!memcmp(current_id[Name], last_pos, src - last_pos)
```

`memcmp == 0` → 两块内存完全一致。

解释：

* `current_id[Name]`：符号表里存储的原始名字指针
* `last_pos`：当前扫描到的名字起点
* `src - last_pos`：名字长度

因此：

### ✔️ 如果 hash 相等

### ✔️ 且名字真正每个字节都一样

→ 说明这是重复出现的标识符
立即返回它的 Token 值。

---

# 第四部分：未找到 → 加入符号表

如果 while 循环一直**没有 return**，说明这是**新标识符**：

```cpp
current_id[Name] = (int)last_pos;
current_id[Hash] = hash;
token = current_id[Token] = Id;
return;
```

存储内容包括：

* 指向名字在源码中的位置
* 标识符 hash
* Token 类型设置为 `Id`（表示普通标识符）

---

# 总结（超级精炼）

这段代码实现了一个词法分析器中的 **符号表管理**：

1. 扫描标识符
2. 计算 hash
3. 在符号表中线性查找：

    * 如果 hash 相等并且内容 memcmp 完全一致 → 找到了
4. 否则新建一个符号表项

这相当于：

### **“如果变量名之前见过 → 返回它；如果没见过 → 注册它”**

---
```c++
else if (token >= '0' && token <= '9') // numbers
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
}

```
---

# **这段代码在做：解析数字字面量（支持十进制、十六进制、八进制）**

流程如下：

# **1. 先取第一个数字字符**

```cpp
token_val = token - '0';
```

如果首字符不是 `'0'`，那就是 **十进制**。

---

# **2. 十进制解析**

```cpp
if (token_val > 0)
{
    while (*src >= '0' && *src <= '9')
        token_val = token_val * 10 + *src++ - '0';
}
```

例：
`123` → 1 → 12 → 123

---

# **3. 否则首字符 = '0' → 特殊格式（0 开头）**

### （1）十六进制：`0x123`

```cpp
if (*src == 'x' || *src == 'X')
{
    token = *++src;
    while (是 0-9 或 a-f 或 A-F)
        token_val = token_val * 16 + hex值;
}
```

hex 计算方式 `token & 15 + (token >= 'A' ? 9 : 0)`
用于把 `'A'~'F'` 映射到 10~15。

### **表达式用途：把十六进制字符转成数值 0~15**

```c
(token & 15) + (token >= 'A' ? 9 : 0)
```

解释：

* **`token & 15`** 取得字符 ASCII 的低 4 位

    * `'0'`~`'9'` → 正好得到 0~9
    * `'A'`~`'F'` → 得到 1~6

* **`token >= 'A' ? 9 : 0`**

    * 对 `'A'`~`'F'` 加 9，把 1~6 变成 10~15
    * 数字字符不加

### 最终效果：

| 字符      | 结果    |
| ------- | ----- |
| '0'~'9' | 0~9   |
| 'A'~'F' | 10~15 |

就是**十六进制字符转数字**的快速写法。

---

### （2）八进制：`017`

```cpp
while (*src >= '0' && *src <= '7')
    token_val = token_val * 8 + *src++ - '0';
```

---

# **4. 最终将 token 设置为数字：**

```cpp
token = Num;
return;
```

---

# 简短总结

| 格式   | 判断方式        | 基数 |
| ---- | ----------- | -- |
| 十进制  | 不以 0 开头     | 10 |
| 十六进制 | `0x` 或 `0X` | 16 |
| 八进制  | 0 开头且非 0x   | 8  |

---

```c++
else if (token == '"' || token == '\'') // string
{
    // parse string literal, currently, the only supported escape
    // character is '\n', store the string literal into data.
    last_pos = data;
    while (*src != 0 && *src != token)
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

        if (token == '"')
        {
            *data++ = token_val;
        }
    }

    src++;
    // if it is a single character, return Num token
    if (token == '"')
    {
        token_val = (int) last_pos;
    } else
    {
        token = Num;
    }

    return;
}
```

# **这段代码用于解析字符串字面量 `"..."` 或字符字面量 `'...'`**

主要逻辑：

---

## **1. 检查开头是否是 `"` 或 `'`**

```c
else if (token == '"' || token == '\'')
```

---

## **2. 逐字符读取内容**

```c
while (*src != 0 && *src != token)
```

* 读到同样的引号（字符串结束）就停
* 支持简单的转义字符：`'\n'`

---

## **3. 处理转义字符**

```c
if (token_val == '\\') {
    token_val = *src++;
    if (token_val == 'n')
        token_val = '\n';
}
```

目前只支持 `\n`。

---

## **4. 如果是字符串（"..."），就写入 data 区**

```c
if (token == '"')
    *data++ = token_val;
```

字符串内容会存到 data 段。

---

## **5. 根据类型决定 token 值**

* `"..."`

  ```c
  token_val = (int)last_pos;   // 指向字符串起始地址
  ```

* `'x'`

  ```c
  token = Num;                 // 这是单个字符 → 数字 token
  ```

---

```c++
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

```

# *这段代码的作用：初始化符号表（加入关键字和库函数）**

分成三部分：

# **1. 把 C 语言关键字加入符号表**

```c
src = "char else enum if int return sizeof while";
i = Char;
while (i <= While) {
    next();
    current_id[Token] = i++;
}
```

流程：

* `next()` 会从 `src` 中读取一个单词（如 `char`、`else`）
* 把它放进符号表的 `current_id`
* 并把 token 设置为对应的关键字编号（Char, Else, Enum, If...）

---

# **2. 把内置库函数加入符号表**

```c
src = "open read close printf malloc memset memcmp exit void main";
i = OPEN;
while (i <= EXIT) {
    next();
    current_id[Class] = Sys;
    current_id[Type] = INT;
    current_id[Value] = i++;
}
```

例如：

* open
* read
* close
* printf
* …

这些加入符号表并标记为：

* Class = Sys（系统调用）
* Type = INT（返回 int）
* Value = 对应 syscall 编号

---

# **3. 单独处理 void 和 main**

```c
next();
current_id[Token] = Char; // 把 void 当成 char 处理（简化语法）
next();
idmain = current_id;       // 保存 main 函数位置
```

注意：这是简易编译器的技巧，把 `void` 当成一种类型来处理。

---
# 注意：
## `current_id[Value]` = *(current_id + Value)

也就是：

> **current_id 指向一块连续的 int 数组，Value 是一个偏移量（index）。**

整个符号表的每个“标识符条目 (id entry)”是一个连续的 `int` 数组，例如：

```
[ Token ][ Hash ][ Name ][ Type ][ Class ][ Value ][ BType ][ BClass ][ BValue ] ...
```

也就是说：

* `Token` = 0
* `Hash` = 1
* `Name` = 2
* `Type` = 3
* `Class` = 4
* `Value` = 5
* …

因此：

```c
current_id[Value]
```

实际上就等价于：

```c
*(current_id + Value)
```

---

## 为什么可以这样用？

因为符号表是这样定义的：

```c
int *symbols;
int *current_id;
const int IdSize = 8;   // 每个ID占用8个int位置
```

所以每次 next() 读到一个新标识符：

```c
current_id = current_id + IdSize;
```

让它移动到下一个 ID 条目的起始位置。

每个 ID 条目内部字段通过固定的 index（Token, Hash, Name…）访问。

---

## 示例

假设：

```
Token = 0
Hash  = 1
Name  = 2
Type  = 3
Class = 4
Value = 5
```

如果 current_id 指向：

```
base → [Token][Hash][Name][Type][Class][Value][BType]...
```

那么：

```c
current_id[Value] = 123;
```

就等于写入：

```
*(base + 5) = 123;
```
