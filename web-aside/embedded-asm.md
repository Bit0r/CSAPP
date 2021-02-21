## 1. 汇编代码与C程序的结合

在计算的早期，大多数程序都是用汇编代码编写的。即使是大型操作系统也没有高级语言的帮助。这对于非常复杂的程序来说变得难以管理。由于汇编代码不提供任何形式的类型检查，因此很容易出现基本错误，例如将指针用作整数而不是取消对指针的引用。更糟糕的是，编写汇编代码会将整个程序锁定到特定的机器类中。重写汇编语言程序以在不同的机器上运行可能与从头编写整个程序一样困难。

早期用于高级编程语言的编译器不能生成非常高效的代码，也不能像系统程序员经常要求的那样提供对低级数据表示的访问。需要最高性能或需要对数据结构进行低级访问的程序通常仍然是用汇编代码编写的。然而，如今，优化编译器已经在很大程度上消除了将性能优化作为编写汇编代码的理由。由高质量编译器生成的代码通常与手动生成的代码一样好，甚至更好。C语言在很大程度上消除了低级数据结构访问，这是编写汇编代码的一个原因。通过联合和指针算法访问低级数据表示的能力，以及对位级数据表示进行操作的能力，为大多数程序员提供了足够的机器访问权限。例如，几乎所有现代操作系统的代码，包括Linux、Windows和MacOS，都是用C编写的。

尽管如此，有时编写汇编代码是唯一的选择。在实现操作系统时尤其如此。例如，有许多特殊寄存器存储操作系统必须访问的进程状态信息。有专门的指令或专门的内存位置来执行输入和输出操作。即使对于应用程序编程人员，也有一些机器特性，例如条件代码的值，不能在C中直接访问。

接下来的挑战是将主要由C语言组成的代码与少量用汇编语言编写的代码集成起来。在本文中，我们将描述两种这样的机制。第一种方法是在汇编代码中编写几个关键函数，使用与C编译器相同的参数传递和寄存器使用约定。然后链接器将这两种形式的代码组合成一个程序。这种方法对于简单函数通常是可行的，并且它不需要任何特定于GCC的构造。用C编写整个函数的另一种方法是在C程序中嵌入汇编代码。GCC通过`asm`指令支持*内联汇编*。内联汇编允许用户将汇编代码直接插入到编译器生成的代码序列中。提供的特性向编译器指示如何与插入的代码进行接口。当然，生成的代码只在特定的机器类上运行，但是我们将看到，例如，通常可以在IA32和x86-64机器上使用内联程序集进行正确编译。`asm`指令也是特定于GCC的，这造成了与许多其他编译器的不兼容性。尽管如此，这仍然是将依赖于机器的代码量保持在绝对最小值的有用方法。

我们的陈述来自GCC文档。这是权威的参考，但它相当简洁，包含很少的例子。

## 2. 程序示例

在我们的演示中，我们将使用以下原型开发几个函数的实现。这些例子提供了现实生活中的情况，在这些情况下，访问条件代码将使我们能够监视计算的状态。

```c
/*
Multiply x and y. Store result at dest.
Return 1 if multiplication did not overflow
 */
int tmult_ok(long x, long y, long *dest);
/*
Multiply x and y. Store result at dest.
Return 1 if multiplication did not overflow
 */
int umult_ok(unsigned long x, unsigned long y, unsigned long *dest);
```

每个函数都要计算参数`x`和`y`的乘积，并将结果存储在参数`dest`指定的内存位置。作为返回值，当乘法溢出时，即需要64位以上才能表示真积，它们应该返回0，否则返回1。我们有单独的函数用于有符号和无符号乘法，因为它们的溢出条件不同。我们研究了使用C确定乘法是否溢出的方法，但是所有这些方法都需要执行额外的操作来检查乘法的结果。

检查x86乘法指令`mull`和`imull`的文档，我们发现它们都在溢出时设置进位标志`CF`。因此，通过在执行`x`和`y`的乘法之后插入检查此标志的代码，我们应该能够轻松地测试乘法溢出。

## 3. 手写汇编代码函数

尽管完全用汇编代码编写复杂的程序是一项艰巨的任务，但我们通常可以将需要用汇编代码表示的功能量缩小到一小部分，然后将这些代码作为函数写入单独的文件中。链接器将编译的C代码与汇编的汇编代码组合在一起。例如，如果文件`p1.c`包含c代码，而文件`p2.s`包含汇编代码，那么编译命令

```
linux> gcc -o p p1.c p2.s
```

将导致文件`p1.c`被编译，文件`p2.s`被汇编，产生的目标代码被链接形成一个可执行程序`p`。

为了让汇编器生成链接器所需的关于函数的信息，我们必须将函数声明为*全局*。而在C中，任何函数都是全局函数，除非声明为静态函数，否则汇编程序假定一个文件只能在本地对同一文件中的函数可用，除非声明为全局函数。如果文件中有函数`fun`的汇编代码，那么应该在它前面声明
```asm
.globl fun
```

我们发现，即使在用汇编代码编写函数时，最好让GCC尽可能多地完成工作。为此，用C编写一个类似于所需功能的函数，然后通过使用命令行选项'`-S`'运行GCC来生成汇编代码的初始版本通常会有所帮助。这段代码为获取参数、分配和取消分配堆栈帧等提供了一个良好的起点。编辑现有的汇编代码要比从头开始容易得多。

作为一个例子，考虑以下函数`tmult_ok_asm`：

```c
/* Starter function for tmult_ok */
/* Multiply arguments and indicate whether it did not overflow */
int tmult_ok_asm(long x, long y, long *dest) {
    long p = x*y;
    *dest = p;
    return p > 0;
}
```

这个函数完成了我们对`tmult_ok`的大部分功能——它将参数`x`和`y`相乘，将乘积存储在`dest`，并根据结果返回0或1。它唯一的缺点是检查错误的属性，但这只是整个计算的一部分。

GCC为初始函数生成以下汇编代码我们以文件中显示的确切形式显示这些代码，而不是我们在书中显示汇编代码的更样式化的方式，因为我们将要编辑汇编代码。

```assembly
    .globl tmult_ok_asm
tmult_ok_asm:
    imulq %rdi, %rsi
    movq %rsi, (%rdx)
    testq %rsi, %rsi
    setg %al
    movzbl %al, %eax
    ret
```

请注意此代码中存在`.globl`声明。这个代码只有两行需要更改。第5行基于`x`和`y`的64位乘积设置条件码。我们希望消除此指令，而是依赖`imulq`指令设置的条件码值。第6行根据零和符号标志设置寄存器`%eax`的低位字节。我们想根据进位标志设置字节。

指令`setae`可以用于在进位标志设置时将寄存器的低位字节设置为0，否则设置为1。因此，我们可以对汇编代码进行小的编辑，以获得所需的函数：

```assembly
# Hand-generated code for tmult_ok
.globl tmult_ok_asm
# int tmult_ok_asm(long x, int y, long *dest);
# x in %rdi, y in %rsi, dest in %rdx
    tmult_ok_asm:
    imulq %rdi, %rsi
    movq %rsi, (%rdx)
# Deleted code
    # testq %rsi, %rsi
    # setg %al
# Inserted code
    setae %al # Set low-order byte
# End of inserted code
    movzbl %al, %eax
    ret
```

如本例所示，它有助于将注释作为文档添加到汇编代码中。符号“#”右侧的任何内容都被汇编程序视为注释。

不幸的是，这种原型化和修改的方法不适用于`umult_ok_asm`的实现，在这里需要无符号算法。我们试图用C代码创建一个近似的版本

```c
/* Multiply arguments and indicate whether it did not overflow */
int umult_ok_asm(unsigned long x, unsigned long y, unsigned long *dest) {
    unsigned long p = x*y;
    *dest = p;
    return p > 0;
}
```

GCC为初始函数生成以下汇编代码：

```assembly
    .globl umult_ok_asm
umult_ok_asm:
    imulq %rdi, %rsi
    movq %rsi, (%rdx)
    testq %rsi, %rsi
    setne %al
    movzbl %al, %eax
    ret
```

代码与签名数据的代码几乎相同！特别是，它仍然使用有符号乘法指令`imulq`。正如CS:APP3e第2章中所讨论的，两个64位乘积的低阶64位是相同的，因此该代码计算正确的乘积，但它没有以一种可以让我们检测溢出的方式设置条件代码。

相反，我们必须编写使用无符号乘法指令`mulq`的代码，但此指令只能以单操作数格式提供（CS:APP3e图3.12）。对于一个操作数形式，乘法的另一个参数必须在寄存器`rax`中，乘积存储在寄存器`rdx`（高位8字节）和`rax`（低位8字节）中。使用此指令需要编写与GCC生成的代码非常不同的代码：

```assembly
# Hand-generated code for umult_ok
.globl umult_ok_asm
# int umult_ok_asm(unsigned long x, unsigned long y, unsigned long *dest);
# x in %rdi, y in %rsi, dest in %rdx
umult_ok_asm:
    movq %rdx, %rcx # Save copy of dest
    movq %rsi, %rax # Copy y to %rax
    mulq %rdi # Unsigned multiply by x
    movq %rax, (%rcx) # Store product at dest
    setae %al # Set low-order byte
    movzbl %al, %eax
    ret
```

在这两个例子中，我们可以看到，我们对汇编代码的研究使我们能够编写自己的汇编代码函数。

## 基本内联汇编

使用内联汇编可以让GCC在将手工生成的汇编代码集成到程序中时做更多的工作。

```c
asm( 代码字符串 );
```

术语*代码字符串*表示以一个或多个带引号的字符串（它们之间没有分隔符）形式给出的程序集代码序列。编译器将把这些字符串逐字插入正在生成的程序集代码中，因此编译器提供的程序集和用户提供的程序集将合并在一起。编译器不会检查字符串是否有错误，因此问题的第一个指示可能是来自汇编程序的错误报告。

为了尽可能少地使用汇编代码和详细分析，我们尝试用以下代码实现tmult_ok：

```c
/* First attempt. Does not work */
int tmult_ok1(long x, long y, long *dest){
    long result = 0;
    *dest = x*y;
    asm("setae %al");
    return result;
}
```

这里的策略是利用寄存器`%eax`用于存储返回值这一事实。假设编译器将这个寄存器用于变量`result`，我们的意图是C代码的第一行将寄存器设置为0。内联程序集将插入适当设置该寄存器低位字节的代码，并且该寄存器将用作返回值。

不幸的是，生成的代码没有按预期工作。在运行测试时，每次调用它时都返回0。在检查为该函数生成的汇编代码时，我们发现：

```c
int tmult_ok1(long x, long y, long *dest)
    //x at %rdi, y at %rsi, dest at %rdx
```

```assembly
tmult_ok1:
    imulq %rsi, %rdi    ; Compute x * y
    movq %rdi, (%rdx)   ; Store at destCode generated by asm
    setae %al           ; Set low-order byte of %eax End of asm-generated code */
    movl $0, %eax       ; Set %eax to 0
    ret
```

GCC有自己的代码生成思想。生成的代码不是在函数开头将寄存器`eax`设置为0，而是在最末尾（第5行）设置为0，因此函数始终返回0。基本问题是编译器无法知道程序员的意图是什么，以及汇编代码应该如何与生成的其余代码交互。显然，在C代码中嵌入汇编代码需要更复杂的机制。

## 5. asm的扩展形式

GCC提供了`asm`的扩展版本，允许程序员指定哪些程序值将用作汇编代码序列的操作数，哪些寄存器将被汇编代码覆盖。利用这些信息，编译器可以生成正确设置所需源代码值、执行汇编指令和使用计算结果的代码。它还将具有所需的有关寄存器用法的信息，以便重要的程序值不会被汇编代码指令覆盖。

扩展`asm`指令的一般语法是

```c
asm( code-strings [ : output-list [ : input-list [ : overwrite-list ] ] ] );
```

其中方括号表示可选参数。与基本asm一样，扩展表单包含一个或多个字符串，给出汇编代码行的样式化版本。后面是可选的输出列表（即汇编代码生成的结果）、输入列表（即汇编代码的源值）和被汇编代码覆盖的寄存器。这些列表用冒号（“：”）分隔角色。作为方括号显示，我们只包括最后一个非空列表。

代码字符串的语法让人想起`printf`语句中格式字符串的语法。它由汇编代码指令组成，但操作数是以符号形式编写的，并引用输出和输入列表中的操作数表达式。在汇编代码中，我们为操作数命名。早期版本的GCC要求根据两个列表中操作数的顺序，这些名称的范围为从`%0`、%1等到%9。由于GCC版本3.1，支持更具描述性的命名约定，其中名称用符号`%[name]`书写。寄存器名（如`%ea`x）必须用额外的“%”符号写入，如%`%eax`。
