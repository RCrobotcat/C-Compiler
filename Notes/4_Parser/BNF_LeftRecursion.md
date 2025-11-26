## 1. BNF 是什么？

BNF（Backus–Naur Form）是一种“写语法的语言”。
我们平时写代码，编译器要知道：

* 什么算是一个“表达式”？
* 什么算是一个“语句”？
* 括号、运算符、标识符按什么顺序出现才合法？

这些规则就可以用 BNF 写下来。BNF 对编译器、解释器、语法分析器来说，就像“语法说明书”。

---

## 2. BNF 里的几种符号

看你图里的规则（我排一下版）：

```bnf
<expr>   ::= <expr> + <term>
          |  <expr> - <term>
          |  <term>

<term>   ::= <term> * <factor>
          |  <term> / <factor>
          |  <factor>

<factor> ::= ( <expr> )
          |  Num
```

逐个解释：

### 2.1 非终结符（Non-terminal）

* **形状：** 用尖括号包起来：`<expr>`、`<term>`、`<factor>`
* **含义：** 抽象的语法成分，是“可以继续展开”的东西。
* 在这段里：

    * `<expr>` 代表“表达式”（expression）
    * `<term>` 代表“项”（term）
    * `<factor>` 代表“因子”（factor）

这些都不是最终出现在程序源代码里的符号，而是**语法结构的名字**。

### 2.2 终结符（Terminal）

* **形状：** 没有尖括号< >，通常是：

    * 具体的符号：`+  -  *  /  (  )`
    * 或记号名（token 名）：`Num`
* 终结符就是**词法分析之后的“单词”**，不能再被语法规则替换。
* 比如源代码里写 `3 + 4`，词法分析器会变成：

    * `Num(3)`   `+`   `Num(4)`
      这几个就是终结符序列。

### 2.3 产生式（Production）和 ::=、|

一条 BNF 规则叫**产生式**，基本形式是：

```bnf
<非终结符> ::=  由它展开成的东西
```

* `::=` 读作“可以被替换为”或“定义为”。
* `|` 表示“多种选择中的一个”。

例如：

```bnf
<expr> ::= <expr> + <term>
        |  <expr> - <term>
        |  <term>
```

读成自然语言就是：

> 一个 `<expr>` 可以是：
>
> * 一个 `<expr>` 后面跟 `+` 再跟一个 `<term>`，
> * 或者一个 `<expr>` 后面跟 `-` 再跟一个 `<term>`，
> * 或者仅仅是一个 `<term>`。

---

## 3. 用例子演示：从语法生成表达式

我们来用这套 BNF，从 `<expr>` 推导出具体的表达式。

### 例子 1：生成 `Num + Num * Num`

目标字符串（终结符序列）：
`Num + Num * Num`

#### 步骤 1：从开始符号 `<expr>` 出发

一般约定**开始符号**是 `<expr>`。

```text
<expr>
```

#### 步骤 2：因为整体形状像 “表达式 + 表达式的一部分”，先用第一条产生式

```bnf
<expr> ::= <expr> + <term>
```

替换后：

```text
<expr> + <term>
```

#### 步骤 3：左边这个 `<expr>` 只想变成一个简单的数，所以用 `<expr> ::= <term>`

```text
<term> + <term>
```

#### 步骤 4：把左边 `<term>` 展开成 `<factor>`

```bnf
<term> ::= <factor>
<factor> ::= Num
```

两步合并来看：

```text
<factor> + <term>
Num + <term>
```

#### 步骤 5：右边 `<term>` 想变成 `Num * Num`，用乘法的规则

```bnf
<term> ::= <term> * <factor>
```

于是：

```text
Num + <term> * <factor>
```

再把这个 `<term>` 简化成 `<factor>`，再到 `Num`：

```text
Num + <factor> * <factor>
Num + Num * <factor>
Num + Num * Num
```

现在整串都是终结符了，**推导完成**。
这说明根据这套 BNF，`Num + Num * Num` 是一个合法的 `<expr>`。

如果把 `Num` 换成具体数字，例如 `3 + 4 * 5`，同样符合这套语法。

---

### 例子 2：带括号 `(Num - Num) / Num`

目标：`(Num - Num) / Num`
我们同样从 `<expr>` 开始推：

1. 观察形式像“一个项除以一个因子”，先用 `<expr> ::= <term>`，再在 `<term>` 上用除法：

   ```text
   <expr>
   ⇒ <term>
   ⇒ <term> / <factor>
   ```

2. 左边 `<term>` 想变成 `(Num - Num)`，但这个有括号包着，看 `<factor>` 的定义：

   ```bnf
   <factor> ::= ( <expr> ) | Num
   ```

   所以要先让 `<term>` 变成 `<factor>`：

   ```text
   <term> / <factor>
   ⇒ <factor> / <factor>
   ⇒ ( <expr> ) / <factor>
   ```

3. 括号里的 `<expr>` 是 `Num - Num`，用减法规则：

   ```text
   ( <expr> ) / <factor>
   ⇒ ( <expr> - <term> ) / <factor>
   ```

4. 把 `<expr>` 和 `<term>` 都收缩到 `Num`：

   ```text
   ( <term> - <term> ) / <factor>
   ⇒ ( <factor> - <factor> ) / <factor>
   ⇒ ( Num - Num ) / <factor>
   ⇒ ( Num - Num ) / Num
   ```

推导完毕。
所以 `(Num - Num) / Num` 也是合法 `<expr>`。

---

## 4. “终结符 vs 非终结符”再对比一下

用这几个例子，区分一下：

* **非终结符**：`<expr>`、`<term>`、`<factor>`

    * 不直接出现在代码里，相当于**语法里的变量**。
    * 只能通过产生式被替换掉。
* **终结符**：`Num`，`+`，`-`，`*`，`/`，`(`，`)`

    * 是“最小单位”，对应编译器词法分析得到的 token。
    * 推导过程结束时，字符串里只能剩终结符。

BNF 的作用，就是用这些产生式**列出所有允许的终结符序列**（所有合法的表达式）。

---

## 5. 再顺带提一句：[]、{} 之类（扩展 BNF）

你截图下面提到的 `[]` “表示选择” 其实已经接近 **EBNF（扩展 BNF）** 的写法了，比如常见用法：

* `[ A ]` 表示 “A 可有可无（0 或 1 次）”
* `{ A }` 表示 “A 可以重复多次（0 次、1 次、2 次…）”

纯 BNF 里是没有 `[]` 和 `{}` 这些符号的，需要用多写几条产生式来模拟。只是教材/文章为了写得简洁，会加一些“扩展符号”，就叫 EBNF。

---

# 关于“左递归”和“消除左递归”的解释

---

# 1. 图里的“原始文法”是左递归文法

你图里最开始的表达式文法（未显示，但在之前的截图中）是这样的：

```
<expr> ::= <expr> + <term>
         | <expr> - <term>
         | <term>
```

右边第一个符号是 `<expr>` 本身，所以：

```
<expr> ::= <expr> + ...
           ↑ 左边又出现自己
```

这就叫 **左递归（Left Recursion）**。

**定义：**

> 如果产生式的右侧最左端是自身非终结符 A，则 A 是左递归的。
> 如：`A ::= A α`

---

# 2. 为什么“左递归语法”不能直接用递归下降解析？

因为递归下降会写成：

```cpp
parseExpr() {
    if (...) parseExpr();  // 又调用自己
}
```

导致：

```
parseExpr()
→ parseExpr()
→ parseExpr()
→ 无限递归（死循环）
```

**所以必须消除左递归。**

这也是你图上写的：

> 左递归的语法是没法直接使用递归下降的方法实现的。

---

# 3. 图中“消除左递归后”的新文法

你图里已经写出消除后的文法（我重排一下排版）：

```
<expr>      ::= <term> <expr_tail>
<expr_tail> ::= + <term> <expr_tail>
              | - <term> <expr_tail>
              | <empty>
```

再看 `<term>` 也做了类似处理：

```
<term>      ::= <factor> <term_tail>
<term_tail> ::= * <factor> <term_tail>
              | / <factor> <term_tail>
              | <empty>
```

**注意：这里已经完全没有“左递归”了。**

---

# 4. 这套新文法为何能消除左递归？（非常关键）

来看原始公式：

```
<expr> ::= <expr> + <term> | <expr> - <term> | <term>
```

标准左递归形式是：

```
A ::= A α | β
```

左递归消除的通用方法是：

```
A  ::= β A'
A' ::= α A' | ε
```

把你的 `<expr>` 套进去：

* β = `<term>`
* α = `+ <term>` 或 `- <term>`

所以：

```
<expr> ::= <term> <expr_tail>
<expr_tail> ::= + <term> <expr_tail>
              |  - <term> <expr_tail>
              |  ε      （empty）
```

这正是你图里的内容。

所以图中的文法，就是把左递归完全按“通用公式”消掉后的结果。

---

# 5. 图中的结构为什么好？因为能递归下降（从左到右解析）

用消除左递归后的文法，递归下降可以这样写（伪代码）：

```cpp
parseExpr() {
    parseTerm();
    parseExprTail();
}

parseExprTail() {
    if (看到 '+') {
        读 '+';
        parseTerm();
        parseExprTail();
    }
    else if (看到 '-') {
        读 '-';
        parseTerm();
        parseExprTail();
    }
    else {
        // empty
    }
}
```

不会再陷入死循环，可以逐步向右“吃掉”更多 token。

---

# 6. 图中的 `<empty>` 是什么？

`<empty>` 等价于 **ε（epsilon）**，表示：

> 这个位置可以什么都不写。

它的作用是：

* `<expr_tail>` 可以结束
* `<term_tail>` 可以结束

即：

```
<expr> = <term> (+ <term> (+ <term> ... ))
```

最终可以产生例如：

```
Num
Num + Num
Num + Num - Num
Num + Num - Num + Num ...
```

符合正常表达式。

---

# 7. 结合图的**重点总结**（最核心理解点）

你图中显示的文法：

```
<expr> ::= <term> <expr_tail>
<expr_tail> ::= + <term> <expr_tail>
              | - <term> <expr_tail>
              | <empty>
```

正是把原来这样的左递归：

```
<expr> ::= <expr> + <term> | <expr> - <term> | <term>
```

转换成：

* **先读一个 `<term>`（保证不会无限左递归）**
* **再读若干个 (+ <term>) / (- <term>)（实现循环累加）**

等价于：

```
term (op term)*
```

这是一个经典的“左递归消除”例子。

---

# 最终一句话帮你完全记住

> **左递归：产生式右侧以自身开头，会使递归下降死循环。
> 图中新的文法把左递归改成了 `<A> ::= β A'` 的形式，实现“向右展开”，因此能直接递归下降解析。**
