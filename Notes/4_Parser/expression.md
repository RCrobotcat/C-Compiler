```c
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
```
---
# 1. 这段代码对应的 BNF 文法（非常重要）

你的解析器所实现的文法正是：

```
Expr       ::= Term Expr'
Expr'      ::= + Term Expr'
             | - Term Expr'
             | ε

Term       ::= Factor Term'
Term'      ::= * Factor Term'
             | / Factor Term'
             | ε

Factor     ::= ( Expr )
             | Num
```

也就是我们上面一起推导出的 **去左递归表达式文法**。

这个文法是 **右递归** → 可以安全用于递归下降。

代码里的函数与文法是一一对应的：

| 文法       | C 函数                    |
| -------- | ----------------------- |
| `Expr`   | `expr()`                |
| `Expr'`  | `expr_tail(int lvalue)` |
| `Term`   | `term()`                |
| `Term'`  | `term_tail(int lvalue)` |
| `Factor` | `factor()`              |

这是非常标准的设计。

---

# 2. 开始逐段解释代码

---

# `match(int tk)`

用于“匹配并消费一个 token”。

```c
void match(int tk)
{
    if (token != tk)
    {
        printf("expected token: %d(%c), got: %d(%c)\n", tk, tk, token, token);
        exit(-1);
    }
    next();
}
```

* 如果当前 token 不符合预期，报错并退出
* 否则调用 `next()` 取下一个 token

---

# `factor()` —— 解析 Factor

文法：

```
Factor ::= ( Expr ) | Num
```

代码：

```c
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
```

逻辑：

* 如果是 `'('` → 说明是括号表达式 `( expr )`
* 否则必须是一个数字 Num → 返回数字值

这是基础构造块，优先级最低。

---

# `term()` 与 `term_tail()` —— 解析乘除

文法：

```
Term     ::= Factor Term'
Term'    ::= * Factor Term'
           | / Factor Term'
           | ε
```

对应代码两部分：

### `term()`

```c
int term()
{
    int lvalue = factor();
    return term_tail(lvalue);
}
```

理解：

* 先解析 **一个 factor()**
* 再进入 tail 处理连续的 `* /`

### `term_tail(int lvalue)`

```c
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
```

解释：

* 如果看到 `*`

    * 消费 `*`
    * 再解析右侧的一个 factor()
    * 把结果与左值 lvalue 相乘
    * 继续处理后续的可选 `* factor`（右递归 tail）
* 同理处理 `/`
* 否则返回当前结果（ε）

**为什么传 lvalue？**
因为这个解析器是 **即时计算**（evaluation while parsing）
不是构造 AST，直接算出结果。

例如输入：

```
2 * 3 * 4
```

解析过程：

```
factor → 2
term_tail(2)
   sees '*'
   factor → 3
   new = 2 * 3 = 6
term_tail(6)
   sees '*'
   factor → 4
   new = 6 * 4 = 24
term_tail(24)
   sees nothing → ε → return 24
```

---

# `expr()` 与 `expr_tail()` —— 解析加减

文法：

```
Expr     ::= Term Expr'
Expr'    ::= + Term Expr'
           | - Term Expr'
           | ε
```

代码：

### `expr()`

```c
int expr()
{
    int lvalue = term();
    return expr_tail(lvalue);
}
```

### `expr_tail(int lvalue)`

```c
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
```

解释：

* `expr()` 先读一个 term（因为乘除优先级更高）
* 然后在 tail 中处理 `+ -` 操作
* 每次读一个 term 进行加减
* 用右递归继续处理更多 `+ term` 或 `- term`

例如：

```
1 + 2 + 3
```

解析：

```
expr():
  term() → 1
  expr_tail(1)
     sees '+'
     term() → 2
     expr_tail(1+2 = 3)
        sees '+'
        term() → 3
        expr_tail(3+3 = 6)
           sees nothing → return 6
```

结果是 **6**。

---

# 3. 这段代码能正确处理优先级吗？

✔ 乘除在 term & term_tail
✔ 加减在 expr & expr_tail
✔ 因此乘除优先于加减
✔ 括号调用 expr()
✔ 递归结构保证优先级正确

例如：

```
1 + 2 * 3
```

解析过程：

```
expr():
   term() → factor(1) → 1
   expr_tail(1)
      sees '+'
      term():
          factor(2)
          term_tail(2)
              sees '*'
              factor(3)
              term_tail(2*3 = 6)
      expr_tail(1+6 = 7)
```

→ 正确输出 **7**

---

# 4. 结合文法 + 代码的最终图示

```
Expr  → Term Expr'
Expr' → (+ Term Expr') | (- Term Expr') | ε

Term  → Factor Term'
Term' → (* Factor Term') | (/ Factor Term') | ε

Factor → ( Expr ) | Num
```

对应函数：

```
Expr      → expr()
Expr'     → expr_tail(lvalue)
Term      → term()
Term'     → term_tail(lvalue)
Factor    → factor()
```

每个函数严格匹配文法结构，非常专业。
