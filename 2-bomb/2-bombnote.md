
# Boom！！！
## 要干啥：
顾名思义，拆炸弹，一共有6关，你要在每一关输入指定的字符串来拆除炸弹。
## 怎么干：

### gdb基础：
具体使用看官网或者技术博客，我只在这里介绍本实验用到的gdb命令。

`quit`是退出gdb调试，可简写为`q`
`run`让程序跑起来，一般是设置断点后输入该命令进行调试，可简写为`r`
`break`设置断点，`break`后面跟着的可以是行，可以是函数名称，可以是地址，可简写为`b`
`step`若为函数则进入函数，但是在本例中我使用`s`（`step`的简写情况）则直接跳过了进入函数步骤，到达主函数的下一条语句中。所以也导致了我第一颗雷爆炸了。T.T
`disas`查看反汇编代码，后面可以跟着地址，可以跟着函数名称，也是本实验的主要命令。
`print`查看各种变量的命令，后面可以跟着寄存器名称如`$rip`，可以跟着地址，可以跟着变量名等等你也可以查看变量的16进制形式输出，10进制输出，2进制输出。
`list`查看代码。
### 查看汇编代码：
1. `objdump -d ./bomb >> bomb_ass.txt`来反编译可执行性文件，查看所有的汇编代码。
2. `gdb`根据`list`我们可以知道作者自定义的函数有`initialize_bomb()`，`read_line()`，`phase_i`其中i为1到6，`phase_defused()`,我是使用`disas`命令查看其汇编代码。
本实验使用第二种方法来查看汇编代码：
#### 本实验使用注视与文字相结合的方式来解析问题
#### `main`
没用。
#### `initialize_bomb`
也没用但程序较少贴出来看看把。
```Assembly
(gdb) disas initialize_bomb
Dump of assembler code for function initialize_bomb:
   0x00000000004013a2 <+0>:     sub    $0x8,%rsp
   0x00000000004013a6 <+4>:     mov    $0x4012a0,%esi
   0x00000000004013ab <+9>:     mov    $0x2,%edi
   0x00000000004013b0 <+14>:    call   0x400b90 <signal@plt>
   0x00000000004013b5 <+19>:    add    $0x8,%rsp
   0x00000000004013b9 <+23>:    ret    
End of assembler dump.
```
#### `read_line`
没用，不用看。
#### `phase_1`
```Assembly
(gdb) disas phase_1
Dump of assembler code for function phase_1:
   0x0000000000400ee0 <+0>:     sub    $0x8,%rsp
   0x0000000000400ee4 <+4>:     mov    $0x402400,%esi# %rdi为调用phase_1的第一个参数也作为调用strings_not_equal的第一个参数。
   0x0000000000400ee9 <+9>:     call   0x401338 <strings_not_equal>
   0x0000000000400eee <+14>:    test   %eax,%eax #%eax为0才跳过炸弹。
   0x0000000000400ef0 <+16>:    je     0x400ef7 <phase_1+23>
   0x0000000000400ef2 <+18>:    call   0x40143a <explode_bomb>
   0x0000000000400ef7 <+23>:    add    $0x8,%rsp
   0x0000000000400efb <+27>:    ret    
End of assembler dump.
```
##### `phase_1`解决步骤：
1. 从`phase_1`code中我们可以知道只有`strings_not_equal`返回0才会跳过炸弹启动代码。所以我们进入`strings_not_equal`
#### `strings_not_equal`
```Assembly
0000000000401338 <strings_not_equal>: 
# 在 phase_1 中 ：
# 第一个实参是%rdi也是输入的字符串的值，第二个形参是%rsi是一个固定值：$0x402400
# return 必须为0才能不爆炸，即%rax必须为0
  401338:	41 54                	push   %r12
  40133a:	55                   	push   %rbp
  40133b:	53                   	push   %rbx
  40133c:	48 89 fb             	mov    %rdi,%rbx # %rbx = %rdi（这个字符串的地址值，第一个实参）
  40133f:	48 89 f5             	mov    %rsi,%rbp # %rbp = %rsi（$0x402400，第二个实参）
  401342:	e8 d4 ff ff ff       	call   40131b <string_length>
  401347:	41 89 c4             	mov    %eax,%r12d # %r12d = string_length(%rdi)
  40134a:	48 89 ef             	mov    %rbp,%rdi # %rdi = %rbp（$0x402400）
  40134d:	e8 c9 ff ff ff       	call   40131b <string_length> # %rax = string_length(%rdi),这里的%rdi就是$0x402400
  # 通过 (gdb) x /3sb 0x402400 可以得到其值："Border relations with Canada have never been better."如果是中文则改变一个编码的长度也就是将b变为h
  401352:	ba 01 00 00 00       	mov    $0x1,%edx # %rdx = 1
  401357:	41 39 c4             	cmp    %eax,%r12d # %r12d = %r12d - %rax ,其中原来的%r12d是401347的值
  40135a:	75 3f                	jne    40139b <strings_not_equal+0x63> # 如果两次调用string_length的返回值不相同就 to 40139B
  40135c:	0f b6 03             	movzbl (%rbx),%eax # %rax = 输入的字符串的值
  40135f:	84 c0                	test   %al,%al
  401361:	74 25                	je     401388 <strings_not_equal+0x50> # to 401388
  401363:	3a 45 00             	cmp    0x0(%rbp),%al
  401366:	74 0a                	je     401372 <strings_not_equal+0x3a> # to 401372
  401368:	eb 25                	jmp    40138f <strings_not_equal+0x57> # to 40138F
  40136a:	3a 45 00             	cmp    0x0(%rbp),%al
  40136d:	0f 1f 00             	nopl   (%rax) # 虽然带有参数，但该指令什么都不做
  401370:	75 24                	jne    401396 <strings_not_equal+0x5e> # to 401396
  401372:	48 83 c3 01          	add    $0x1,%rbx
  401376:	48 83 c5 01          	add    $0x1,%rbp
  40137a:	0f b6 03             	movzbl (%rbx),%eax # %rax = M[R[%rbx]], 因为是0扩展，所以近似于赋值给%rax
  40137d:	84 c0                	test   %al,%al #
  40137f:	75 e9                	jne    40136a <strings_not_equal+0x32> # to 40136A # %rax非0则跳转，若为0到下一条
  401381:	ba 00 00 00 00       	mov    $0x0,%edx # 走这也行。
  401386:	eb 13                	jmp    40139b <strings_not_equal+0x63> # to 40139B
  401388:	ba 00 00 00 00       	mov    $0x0,%edx # 走这也行
  40138d:	eb 0c                	jmp    40139b <strings_not_equal+0x63> # to 40139B
  40138f:	ba 01 00 00 00       	mov    $0x1,%edx
  401394:	eb 05                	jmp    40139b <strings_not_equal+0x63> # to 40139B
  401396:	ba 01 00 00 00       	mov    $0x1,%edx
  40139b:	89 d0                	mov    %edx,%eax # %rax = %rdx， 所以要想返回值为0,%rdx必须为0
  40139d:	5b                   	pop    %rbx
  40139e:	5d                   	pop    %rbp
  40139f:	41 5c                	pop    %r12
  4013a1:	c3                   	ret    
```
2. `strings_not_equal`有很多条线可以走，你要耐心的仔细的找，我是从后往前推导知道了走哪条路可以走到正确的结果，避免了无效的尝试。
3. 通过分析代码我们可以知道问题的关键是`string_length`程序处，在`strings_not_equal`中在`401342`与 `40134d`分别调用了`string_length`，并且在401357进行比较，这是关键点。如果二者相同程序返回0退出，进而拆弹成功。（如果你害怕作者用`strings_not_equal`的名字骗你的话，去看一下`strings_not_equal`的源程序，我是这样做的...以防万一嘛^_^）
4. 看两次调用`string_length`我们就可以知道，进行比较的分别是`%rdi,%rsi`所以从`phase_1`我们找到了，与输入值进行比较的字符串所在的地址`$0x402400`，通过`x /3sb 0x402400` 可以得到其值：`"Border relations with Canada have never been better."`即为答案。
##### 第一问的疑问与思考：（一些废话）
1. `strings_not_equal`仅仅是比较长度这么简单？那我把`"Border relations with Canada have never been better."`换成同样长度的字符可不可以，答案是肯定不可以啊。好吧这也是我没认真看汇编+爱玩的代价，又是一颗雷。为什么呢？
2. 因为`40135a`的位置判断的是`jne`，如果长度相同肯定进行下一步的每个字符进行比较的操作了，那块就是我所说的比较嘈杂的部分了，如果你比较懒那你惟一的选择就是相信作者： `Dr. Evil`的代码和函数名实现的是一个功能喽。
#### `phase_defused`
可能的挖掘彩蛋的点
#### `phase_2`
```Assembly
(gdb) 
0000000000400efc <phase_2>:
# 就一个参数，存在%rdi中
# 进入函数后%rsp要-0x8
  400efc:	55                   	push   %rbp # 被调用者保存 %rsp -= 8
  400efd:	53                   	push   %rbx # 被调用者保存 %rsp -= 8
  400efe:	48 83 ec 28          	sub    $0x28,%rsp # 申请一段栈空间
  400f02:	48 89 e6             	mov    %rsp,%rsi # %rsp作为调用read_six_numbers的第二个参数为：7fffffffd5c0
  400f05:	e8 52 05 00 00       	call   40145c <read_six_numbers> # read_six_numbers(%rdi, %rsi),其中%rsi = %rsp
  # read_six_numbers(%rdi, %rsi)返回 1
  400f0a:	83 3c 24 01          	cmpl   $0x1,(%rsp) # M[R[%rsp]] - 1
  400f0e:	74 20                	je     400f30 <phase_2+0x34> # M[R[%rsp]]为1则跳转到400f30
  400f10:	e8 25 05 00 00       	call   40143a <explode_bomb> # M[R[%rsp]]不为1则触发炸弹
  400f15:	eb 19                	jmp    400f30 <phase_2+0x34>
  400f17:	8b 43 fc             	mov    -0x4(%rbx),%eax # %eax = M[R[%rbx] - 0x4] # %eax = 1
  400f1a:	01 c0                	add    %eax,%eax # %rax = 2 * %rax  # 从400f2c频繁的跳到这来，进行*2操作，并且每次循环R[%rbx]的位置+0x4
  400f1c:	39 03                	cmp    %eax,(%rbx) # M[R[%rbx]] - %rax #由此判断7fffffffd5c4存的数为2
  400f1e:	74 05                	je     400f25 <phase_2+0x29> # 若二者相等则跳转到400f25
  400f20:	e8 15 05 00 00       	call   40143a <explode_bomb> # 二者不相等则触发炸弹
  400f25:	48 83 c3 04          	add    $0x4,%rbx # %rbx = %rbx + 0x4 # %rbx = 7fffffffd5c8
  400f29:	48 39 eb             	cmp    %rbp,%rbx # %rbx - %rbp 
  400f2c:	75 e9                	jne    400f17 <phase_2+0x1b> # 二者不相等则跳转到400f17，否则
  400f2e:	eb 0c                	jmp    400f3c <phase_2+0x40> # 二者相等跳转到400f3c，炸弹2解除
  400f30:	48 8d 5c 24 04       	lea    0x4(%rsp),%rbx # %rbx = %rsp + 0x4 # %rbx = 7fffffffd5c4
  400f35:	48 8d 6c 24 18       	lea    0x18(%rsp),%rbp # %rbp = %rsp + 0x18 # %rbp = 7fffffffd5d8,若以4为步长则%rbx与%rbp差距4步，与进入循环前的已知的两个数相加正好6个数字
  400f3a:	eb db                	jmp    400f17 <phase_2+0x1b>
  400f3c:	48 83 c4 28          	add    $0x28,%rsp # 终点
  400f40:	5b                   	pop    %rbx
  400f41:	5d                   	pop    %rbp
  400f42:	c3                   	ret    
```

#### 解决：
在执行`phase_2`前运行gdb查看rsp的值(每次运行值可能结果不同但思路相通)
```
(gdb) p $rsp
$1 = (void *) 0x7fffffffd600
```
然后进入函数，以此为基础可以计算出其他的寄存器值，具体的看我的注释。注意：你的寄存器的数字和我的不一样是正常的，注释只是将汇编语言进行翻译和分析。我们到达`400f05`并且可以求出所有的参数值，进入`read_six_numbers`
#### `read_six_numbers`
```Assembly
000000000040145c <read_six_numbers>:
# 最终改动：不知道咋回事，如果第二个参数输入的是%rsp作为%rsi的值，那么，他存的的6个数字是连续的从调用read_six_numbers时的M[R[%rsp]+xx]可以访问到他们
# phase_2所调用的函数
# 第一个实参是输入的字符串地址，存在%rdi里，第二个实参是进入phase_2时%rsp值-0x28存在%rsi里
# %rsp = 7fffffffd5c0 - 8 = 7fffffffd5b8
  40145c:	48 83 ec 18          	sub    $0x18,%rsp # 申请了3个0x8的空间，用来存数据,执行完之后%rsp = 7fffffffd5c0
  401460:	48 89 f2             	mov    %rsi,%rdx # %rdx = %rsi（第二个实参）7fffffffd5c0
  401463:	48 8d 4e 04          	lea    0x4(%rsi),%rcx # %rcx = %rsi + 4 所以 %rcx = 7fffffffd5c4
  401467:	48 8d 46 14          	lea    0x14(%rsi),%rax # %rax = %rsi + 0x14 所以%rax = 7fffffffd5d8
  40146b:	48 89 44 24 08       	mov    %rax,0x8(%rsp) # M[R[%rsp]+0x8] = %rax # 存的第二个，可能这一个用了16字节的空间
  401474:	48 89 04 24          	mov    %rax,(%rsp) # M[R[%rsp]] = %rax # 存的第一个，大小为0x8,这里更改了以%rsp为地址的值：
  401478:	4c 8d 4e 0c          	lea    0xc(%rsi),%r9 # %r9 = %rsi + 0xc # %r9 = 7fffffffd5cc
  40147c:	4c 8d 46 08          	lea    0x8(%rsi),%r8 # %r8 = %rsi + 0x8 # %r8 = 7fffffffd5c8
  401480:	be c3 25 40 00       	mov    $0x4025c3,%esi # %rsi = 0x4025c3
  401485:	b8 00 00 00 00       	mov    $0x0,%eax # %rax = 0
  40148a:	e8 61 f7 ff ff       	call   400bf0 <__isoc99_sscanf@plt> # 调用sscanf(%rdi, %rsi, %rdx, %rcx, %r8, %r9， %rsp + 0x8, %rsp+0xc) #第一个参数是输入的字符串地址
# (gdb) x /1sb 0x4025c3
# 0x4025c3:       "%d %d %d %d %d %d"
# 完毕之后输入值依次被存放到：（以%d的形式）
# 0x7fffffffd5c0
# 7fffffffd5c4
# 7fffffffd5c8
# 7fffffffd5cc
# 7fffffffd5c8
# 7fffffffd5d8
# 再次之前：7fffffffd5c0里存放着：7fffffffd5d8， 7fffffffd5c8里存放着也是：7fffffffd5d8
  40148f:	83 f8 05             	cmp    $0x5,%eax # %rax - 0x5 #所以该函数return 1
  401492:	7f 05                	jg     401499 <read_six_numbers+0x3d> # rax大于5则跳过炸弹，说明sscanf成功读入了5个变量
  401494:	e8 a1 ff ff ff       	call   40143a <explode_bomb>
  401499:	48 83 c4 18          	add    $0x18,%rsp
  40149d:	c3                   	ret    
```
根据`401480`可以得到`%rsi`的值，这正好是`401485`调用`sscanf`的第二个参数，使用`(gdb) x /1sb 0x4025c3`进行查看可以得到：`0x4025c3:       "%d %d %d %d %d %d"`说明一共有6个数字从输入读入变量中，所以`sscanf`一共有8个变量，说明有两个变量是存在栈上的，即`sscanf(%rdi, %rsi, %rdx, %rcx, %r8, %r9， %rsp + 8, %rsp+16)`
其返回值是成功读入的数量为6，`40148f`与`401492`可以最终确定，读入的数字是6个（6个以上的后面再说），然后从`read_six_numbers`跳转出来之后，`400f0e`与`400f10`我们可以知道第一个数字是1，从`400f1a`与`400f1c`可以知道第二个数字为2，然后会进入一个小循环，循环次数由`%rbx`与`%rbp`之间的差值决定。而他们存的值正好对应着`sscanf`中存数的地址。而每一次程序都会比较这个值是否是上个值的二倍，如果是则进行下一次循环，如果不是则起爆炸弹。所以答案就是`1 2 4 8 16 32`
##### 该问题的思考：（一些废话）
比较有意思的是，虽然`0x4025c3`存了6个`%d`但是如果我输入7个行不行呢？
```
Phase 1 defused. How about the next one?
1 2 4 8 16 32 hhh
That's number 2.  Keep going!
```

答案是可以的，因为`sscanf`返回的是成功写入的个数，而返回后`phase_2`并没有对`%rax`有任何限制。
##### 学到的东西：
`display /i $pc`+`si`进行汇编级别的调试。
`display /[] $[]` 进行对寄存器值进行查看，以任意的方式
`x /1sb <addr>`进行地址数据查看，可以调整查看的元数据的长度单位，格式，查看数量等。

#### `phase_3`
```Assembly
(gdb) disas phase_3
0000000000400f43 <phase_3>:
  400f43:	48 83 ec 18          	sub    $0x18,%rsp # 执行完之后%rsp = 0x7fffffffd880
  400f47:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx # %rcx = %rsp + 0xc（4字节）
  400f4c:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx # %rdx = %rsp + 0x8（4字节）
  400f51:	be cf 25 40 00       	mov    $0x4025cf,%esi # %rsi = 0x4025cf
  400f56:	b8 00 00 00 00       	mov    $0x0,%eax # %rax = 0
  400f5b:	e8 90 fc ff ff       	call   400bf0 <__isoc99_sscanf@plt>
  400f60:	83 f8 01             	cmp    $0x1,%eax # %rax - 1
  400f63:	7f 05                	jg     400f6a <phase_3+0x27> # 说明只有2个变量被读入，我们将第一个被读入的变量为x1,第二个为x2
# (gdb) x /1sb 0x4025cf
# 0x4025cf:       "%d %d"
  400f65:	e8 d0 04 00 00       	call   40143a <explode_bomb>
  400f6a:	83 7c 24 08 07       	cmpl   $0x7,0x8(%rsp) # M[R[%rsp] + 0x8 ] - 0x7,x1-0x7
  400f6f:	77 3c                	ja     400fad <phase_3+0x6a> # 如果x1 > 7 则引爆炸弹
  400f71:	8b 44 24 08          	mov    0x8(%rsp),%eax # %rax = M[R[%rsp] + 0x8], 将x1赋值给rax
  400f75:	ff 24 c5 70 24 40 00 	jmp    *0x402470(,%rax,8) # 跳转到8*%rax + (*0x402470),其中(*0x402470) = 0x 400f7c
  400f7c:	b8 cf 00 00 00       	mov    $0xcf,%eax # %rax = $0xcf x1 = 0跳转到这里， rax = (207)_10
  400f81:	eb 3b                	jmp    400fbe <phase_3+0x7b> 
  400f83:	b8 c3 02 00 00       	mov    $0x2c3,%eax # %rax = $0x2c3
  400f88:	eb 34                	jmp    400fbe <phase_3+0x7b>
  400f8a:	b8 00 01 00 00       	mov    $0x100,%eax # %rax = $0x100
  400f8f:	eb 2d                	jmp    400fbe <phase_3+0x7b>
  400f91:	b8 85 01 00 00       	mov    $0x185,%eax # %rax = $0x185
  400f96:	eb 26                	jmp    400fbe <phase_3+0x7b>
  400f98:	b8 ce 00 00 00       	mov    $0xce,%eax # %rax = $0xce
  400f9d:	eb 1f                	jmp    400fbe <phase_3+0x7b>
  400f9f:	b8 aa 02 00 00       	mov    $0x2aa,%eax # %rax = $0x2aa
  400fa4:	eb 18                	jmp    400fbe <phase_3+0x7b> # 跳转到这里可以正好对齐指令，这里对应的%rax = 5，即第一个输入为5
  400fa6:	b8 47 01 00 00       	mov    $0x147,%eax # %rax = $0x147
  400fab:	eb 11                	jmp    400fbe <phase_3+0x7b>
  400fad:	e8 88 04 00 00       	call   40143a <explode_bomb>
  400fb2:	b8 00 00 00 00       	mov    $0x0,%eax # %rax = 0
  400fb7:	eb 05                	jmp    400fbe <phase_3+0x7b>
  400fb9:	b8 37 01 00 00       	mov    $0x137,%eax # %rax = $0x137
  400fbe:	3b 44 24 0c          	cmp    0xc(%rsp),%eax # rax - M[R[%rsp] + 0xc],说明第二个数字为%rax不同地方跳转过来的答案也就不同,从x1 = 0过来则x2 = 207
  400fc2:	74 05                	je     400fc9 <phase_3+0x86>
  400fc4:	e8 71 04 00 00       	call   40143a <explode_bomb>
  400fc9:	48 83 c4 18          	add    $0x18,%rsp
  400fcd:	c3                   	ret   
End of assembler dump.
```
#### 解决：
比较简单的一道题：
```Assembly
(gdb) x /1sb 0x4025cf
0x4025cf:       "%d %d"
```
实在看不懂，看一遍我标注的注释就可以看懂了。
##### 本题的思考：
由于本题的答案不唯一，我得到了2个答案，其中跳转到`400fac`的情况是`%rax = 5`时，这时在运行到`400fbe`前`%rax`的值都没有被改变，所以第一个答案就是`5 5`，但是这个答案却引爆了炸弹，这个问题我还没有解决，但是走`%rax = 0`这条路可以走通，即`0 207`
#### `phase_4`
```Assembly
(gdb) disas phase_4
000000000040100c <phase_4>:
  40100c:	48 83 ec 18          	sub    $0x18,%rsp
  401010:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx # %rcx = %rsp + 0xc（4字节）
  401015:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx # %rdx = %rsp + 0x8（4字节）
  40101a:	be cf 25 40 00       	mov    $0x4025cf,%esi # %rsi = $0x4025cf
# (gdb) x /1sb 0x4025cf
# 0x4025cf:       "%d %d"
  40101f:	b8 00 00 00 00       	mov    $0x0,%eax # %rax = 0
  401024:	e8 c7 fb ff ff       	call   400bf0 <__isoc99_sscanf@plt>
  401029:	83 f8 02             	cmp    $0x2,%eax # %rax - 2 
  40102c:	75 07                	jne    401035 <phase_4+0x29> # 从这里或者参数数量都可以推出输入了两个值
  40102e:	83 7c 24 08 0e       	cmpl   $0xe,0x8(%rsp) # M[R[%rsp]+0x8] - 0xe
  401033:	76 05                	jbe    40103a <phase_4+0x2e> # 如果输入的第一个变量<=0xe则跳过炸弹
  401035:	e8 00 04 00 00       	call   40143a <explode_bomb>
  40103a:	ba 0e 00 00 00       	mov    $0xe,%edx # %rdx = 0xe
  40103f:	be 00 00 00 00       	mov    $0x0,%esi # %rsi = 0
  401044:	8b 7c 24 08          	mov    0x8(%rsp),%edi # %rdi = M[R[%rsp] + 0x8] # 将输入的第一个变量赋值给%rdi
  401048:	e8 81 ff ff ff       	call   400fce <func4> # fun4(%rdi, %rsi, %rdx, %rcx)
  40104d:	85 c0                	test   %eax,%eax
  40104f:	75 07                	jne    401058 <phase_4+0x4c> # 如果返回值不等于0则触发炸弹
  401051:	83 7c 24 0c 00       	cmpl   $0x0,0xc(%rsp) # M[R[%rsp]+0xc] - 0
  401056:	74 05                	je     40105d <phase_4+0x51> # 如果M[R[%rsp]+0xc] 为 0则不触发炸弹，也就是输入的第二个值为0
  401058:	e8 dd 03 00 00       	call   40143a <explode_bomb>
  40105d:	48 83 c4 18          	add    $0x18,%rsp
  401061:	c3                   	ret    

End of assembler dump.
```
##### 解决1：
第二个值在`40105d`可以看出是0，第一个值决定了`func4`的返回值，并且在`401058`可以得到返回值必须为0，由于`func4`后两个参数固定，第一个参数指定了范围，所以我们可以不去读懂`func4`通过试一遍就可以得到正确答案。
运用`call`获取特定参数下的函数返回值
```
call (int)func4(i, 0, 14)
```
其中`i`从`0-0xe`
```
(gdb) call (int)func4(1, 0, 14)
$1 = 0
(gdb) call (int)func4(2, 0, 14) 
$2 = 4
(gdb) call (int)func4(3, 0, 14) 
$3 = 0
(gdb) call (int)func4(4, 0, 14) 
$4 = 2
(gdb) call (int)func4(5, 0, 14) 
$5 = 2
(gdb) call (int)func4(6, 0, 14)
$6 = 6
(gdb) call (int)func4(7, 0, 14)
$7 = 0
(gdb) call (int)func4(8, 0, 14)
$8 = 1
(gdb) call (int)func4(9, 0, 14)
$9 = 1
(gdb) call (int)func4(10, 0, 14)
$10 = 5
(gdb) call (int)func4(11, 0, 14)
$11 = 1
(gdb) call (int)func4(12, 0, 14)
$12 = 3
(gdb) call (int)func4(13, 0, 14)
$13 = 3
(gdb) call (int)func4(14, 0, 14)
$14 = 7
```
所以答案为`(1,0) (3,0) (7,0)`，对还有`(0,0)`
##### 解决2：
将`func4`转化为c语言程序：
```c
#include<stdio.h>
int func4(int x1,int x2,int x3){
    int rax = x3 - x2;
    int x4 = rax >> 31;//逻辑
    rax = (rax + x4) >> 1;//算数
    x4 = x2 + rax;
    if (x4 <= x1){
        rax = 0;
        if (x4 >= x1){
            return rax;
        }
        else{
            x2 = x4 + 1;
            //printf("func4(%d, %d, %d)\n", x1, x2, x3);
            return 2*func4(x1, x2, x3) + 1;
        }
    }else{
        x3 = x4 -1;
        //printf("func4(%d, %d, %d)\n", x1, x2, x3);
        return  2 * func4(x1, x2, x3);
    }

}
int main(){
    //int i = 1;
    for(int i = 0; i <=14; i++)
        printf("func4(%d, 0, 14)\t:%d\n",i, func4(i, 0, 14));
    return 0;
}
```
结果为：
```
func4(0, 0, 14) :0
func4(1, 0, 14) :0
func4(2, 0, 14) :4
func4(3, 0, 14) :0
func4(4, 0, 14) :2
func4(5, 0, 14) :2
func4(6, 0, 14) :6
func4(7, 0, 14) :0
func4(8, 0, 14) :1
func4(9, 0, 14) :1
func4(10, 0, 14)        :5
func4(11, 0, 14)        :1
func4(12, 0, 14)        :3
func4(13, 0, 14)        :3
func4(14, 0, 14)        :7
```
所以结果为：`(0,0)(1,0)(3,0)(7,0)`
##### 学到的东西：
`call (int)func4(i, 0, 14)`:可以对函数进行调试
#### `phase_5`
```Assembly
(gdb) disas phase_5
0000000000401062 <phase_5>:
  401062:	53                   	push   %rbx
  401063:	48 83 ec 20          	sub    $0x20,%rsp
  401067:	48 89 fb             	mov    %rdi,%rbx # %rbx = %rdi # %rdi 为存放输入字符串的地址
  40106a:	64 48 8b 04 25 28 00 	mov    %fs:0x28,%rax # %rax = 0x933adc9274865d00
  401071:	00 00 
  401073:	48 89 44 24 18       	mov    %rax,0x18(%rsp) # M[R[%rsp]+0x18] = %rax
  401078:	31 c0                	xor    %eax,%eax # %rax = %rax ^ %rax
  40107a:	e8 9c 02 00 00       	call   40131b <string_length>
  40107f:	83 f8 06             	cmp    $0x6,%eax # %rax - 6
  401082:	74 4e                	je     4010d2 <phase_5+0x70> # 这里就要求输入的字符串长度为6了
  401084:	e8 b1 03 00 00       	call   40143a <explode_bomb>
  401089:	eb 47                	jmp    4010d2 <phase_5+0x70>
  40108b:	0f b6 0c 03          	movzbl (%rbx,%rax,1),%ecx # %rcx = %rax+ %rbx
  40108f:	88 0c 24             	mov    %cl,(%rsp) # M[R[%rsp]] = %rcx
  401092:	48 8b 14 24          	mov    (%rsp),%rdx # %rdx = M[R[%rsp]] # %rdx = 输入的字符串的某个字符
  401096:	83 e2 0f             	and    $0xf,%edx # %rdx = %rdx & %0xf # 只将最后4位保留,作为后面寻址的偏移量
  401099:	0f b6 92 b0 24 40 00 	movzbl 0x4024b0(%rdx),%edx # %rdx = M[R[%rdx] + 0x4024b0]
  4010a0:	88 54 04 10          	mov    %dl,0x10(%rsp,%rax,1) # M[R[%rax]+R[%rsp]+0x10] = %rdx
  4010a4:	48 83 c0 01          	add    $0x1,%rax # %rax = %rax + 0x1
  4010a8:	48 83 f8 06          	cmp    $0x6,%rax # %rax - 0x6
  4010ac:	75 dd                	jne    40108b <phase_5+0x29> # 如果%rax~=6则跳转到 40108b （从0开始循环到6结束跳出循环）
  4010ae:	c6 44 24 16 00       	movb   $0x0,0x16(%rsp) # M[R[%rsp]+0x16] = 0 # 后面的截断，只比较0x10-0x15这六个字符
  4010b3:	be 5e 24 40 00       	mov    $0x40245e,%esi # %rsi = $0x40245e ：存的是“flyers”
  4010b8:	48 8d 7c 24 10       	lea    0x10(%rsp),%rdi # %rdi = %rsp + 0x10
  4010bd:	e8 76 02 00 00       	call   401338 <strings_not_equal>
  4010c2:	85 c0                	test   %eax,%eax
  4010c4:	74 13                	je     4010d9 <phase_5+0x77> # 如果 二者不相等则触发炸弹
  4010c6:	e8 6f 03 00 00       	call   40143a <explode_bomb>
  4010cb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1) # 跳过
  4010d0:	eb 07                	jmp    4010d9 <phase_5+0x77> # 跳转
  4010d2:	b8 00 00 00 00       	mov    $0x0,%eax # %rax = 0
  4010d7:	eb b2                	jmp    40108b <phase_5+0x29>
  4010d9:	48 8b 44 24 18       	mov    0x18(%rsp),%rax # %rax = M[R[%rsp] + 0x18]
  4010de:	64 48 33 04 25 28 00 	xor    %fs:0x28,%rax # %rax = %rax ^ M[R[%fs]+0x28]
  4010e5:	00 00 
  4010e7:	74 05                	je     4010ee <phase_5+0x8c> # 若二者相等则退出，炸弹成功解除
  4010e9:	e8 42 fa ff ff       	call   400b30 <__stack_chk_fail@plt>
  4010ee:	48 83 c4 20          	add    $0x20,%rsp
  4010f2:	5b                   	pop    %rbx
  4010f3:	c3                   	ret    
End of assembler dump.
```
##### 解决：
具体步骤看注释即可了解
```Assembly
(gdb) x /1sb 0x40245e
0x40245e:       "flyers"
```

```Assembly
(gdb) x /1sb 0x4024b0
0x4024b0 <array.3449>:  "maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"
```
其中，偏移量是取输入值的二进制后四位，所以范围也就被限定在了`0-f`中
```
maduiersnfotvbylSo
0123456789abcdef
```
进而得到每个字符的偏移量：
9,f,e,5,6,7
但是着还不够，由于他读取输入的字符获取ASCII值，然后截取后四位得到偏移量，然后进而根据偏移量截取`0x4024b0`位置的字符，并与`0x40245e`字符进行比较，所以本题答案不唯一，只要满足其ASCII的后四位与9,f,e,5,6,7对应即可。
#### `phase_6`
```Assembly
(gdb) disas phase_6
00000000004010f4 <phase_6>:
  4010f4:	41 56                	push   %r14
  4010f6:	41 55                	push   %r13
  4010f8:	41 54                	push   %r12
  4010fa:	55                   	push   %rbp
  4010fb:	53                   	push   %rbx 
  4010fc:	48 83 ec 50          	sub    $0x50,%rsp
  # ---------------------(1-start)------------------------------------
  401100:	49 89 e5             	mov    %rsp,%r13 # %r13 = %rsp
  401103:	48 89 e6             	mov    %rsp,%rsi # %rsi = %rsp
  401106:	e8 51 03 00 00       	call   40145c <read_six_numbers> # read_six_numbers(%rdi, %rsi) 存读个数字
  # 数字存在M[R[%rsp]], M[R[%rsp]+0x4], M[R[%rsp]+0x8], M[R[%rsp]+0xc], M[R[%rsp]+0x10], M[R[%rsp]+0x14]中
  40110b:	49 89 e6             	mov    %rsp,%r14 # %r14 = %rsp
  40110e:	41 bc 00 00 00 00    	mov    $0x0,%r12d # %r12d = 0
  401114:	4c 89 ed             	mov    %r13,%rbp # %rbp = %r13
  401117:	41 8b 45 00          	mov    0x0(%r13),%eax # %rax = M[R[%r13]] # %rax是输入的第一个数字
  40111b:	83 e8 01             	sub    $0x1,%eax # %rax = %rax - 0x1
  40111e:	83 f8 05             	cmp    $0x5,%eax # %rax - 0x5
  401121:	76 05                	jbe    401128 <phase_6+0x34> # 如果M[R[%r13]]的值小鱼等于6的话，避开炸弹，这句话表示每个值都要xi<6
  401123:	e8 12 03 00 00       	call   40143a <explode_bomb>
  # ------------------------------(1-end,2-start)--------------------------
  401128:	41 83 c4 01          	add    $0x1,%r12d # %r12d = %r12d + 0x1（计数器）
  40112c:	41 83 fc 06          	cmp    $0x6,%r12d # %r12d - 0x6
  401130:	74 21                	je     401153 <phase_6+0x5f> # 如果%r12d 等于6则跳转，否则进入循环
  401132:	44 89 e3             	mov    %r12d,%ebx # %rbx = %r12d
  401135:	48 63 c3             	movslq %ebx,%rax # %rax = %rbx
  401138:	8b 04 84             	mov    (%rsp,%rax,4),%eax # %rax = M[4*R[%rax] + R[%rsp]]
  40113b:	39 45 00             	cmp    %eax,0x0(%rbp) # M[R[%rbp]] - %rax，M[R[%rbp]]里面存的是第一个输入的数字
  40113e:	75 05                	jne    401145 <phase_6+0x51> # M[R[%rbp]] 与 %rax 不相等则跳过炸弹,即要求每个数字与第一个数字不相等
  401140:	e8 f5 02 00 00       	call   40143a <explode_bomb>
  401145:	83 c3 01             	add    $0x1,%ebx # %rbx = %rbx + 0x1
  401148:	83 fb 05             	cmp    $0x5,%ebx # %rbx - 0x5
  40114b:	7e e8                	jle    401135 <phase_6+0x41> # 若 %rbx <= 0x5则 继续循环
  40114d:	49 83 c5 04          	add    $0x4,%r13 # %r13 = %r13 + 0x4
  401151:	eb c1                	jmp    401114 <phase_6+0x20> # 跳转
  # -------------------------------(2-end,3-start)------------------------------------
  # (以上是个双层循环，通过改变%r12d值，就是要和其他数字进行比较的值，与其他数字进行比较也是一层循环）
  401153:	48 8d 74 24 18       	lea    0x18(%rsp),%rsi # %rsi = %rsp + 0x18
  401158:	4c 89 f0             	mov    %r14,%rax # %rax = %r14(%rsp)
  40115b:	b9 07 00 00 00       	mov    $0x7,%ecx # %rcx = 0x7
  401160:	89 ca                	mov    %ecx,%edx # %rdx = %rcx
  401162:	2b 10                	sub    (%rax),%edx # %rdx = %rdx - M[R[%rax]]
  401164:	89 10                	mov    %edx,(%rax) # M[R[%rax]] = %rdx
  401166:	48 83 c0 04          	add    $0x4,%rax # %rax = %rax + 0x4
  40116a:	48 39 f0             	cmp    %rsi,%rax # %rax - %rsi
  40116d:	75 f1                	jne    401160 <phase_6+0x6c> # 如果%rax 与 %rsi 不相等则进入循环，直到相等退出循环
  # -------------------(3-end,4-start)-------------------------(a[i] = 7 - a[i])
  40116f:	be 00 00 00 00       	mov    $0x0,%esi # %rsi = 0
  401174:	eb 21                	jmp    401197 <phase_6+0xa3> # 跳转
  401176:	48 8b 52 08          	mov    0x8(%rdx),%rdx # %rdx = M[R[%rdx]+0x8]
  40117a:	83 c0 01             	add    $0x1,%eax # %rax = %rax + 1
  40117d:	39 c8                	cmp    %ecx,%eax # %rax - %rcx
  40117f:	75 f5                	jne    401176 <phase_6+0x82> # 如果%rax与%rcx不等则继续循环，否则跳出循环，出循环的时候，rax与读入的>1的数要一致
  401181:	eb 05                	jmp    401188 <phase_6+0x94> # 进入循环a
  401183:	ba d0 32 60 00       	mov    $0x6032d0,%edx # %rdx = 0x6032d0
  401188:	48 89 54 74 20       	mov    %rdx,0x20(%rsp,%rsi,2) # M[R[%rsi]*2+R[%rsp]+0x20] = %rdx
  40118d:	48 83 c6 04          	add    $0x4,%rsi # %rsi = %rsi + 0x4
  401191:	48 83 fe 18          	cmp    $0x18,%rsi # %rsi - 0x18（6次循环）
  401195:	74 14                	je     4011ab <phase_6+0xb7> # 如果 %rsi == 0x18则跳转--》出循环a的方式
  401197:	8b 0c 34             	mov    (%rsp,%rsi,1),%ecx # %rcx = M[R[%rsi] + R[%rsp]] # 对存的数字依次进行读取
  40119a:	83 f9 01             	cmp    $0x1,%ecx # %rcx - 0x1
  40119d:	7e e4                	jle    401183 <phase_6+0x8f> # 如果%rcx<=1则跳转
  40119f:	b8 01 00 00 00       	mov    $0x1,%eax # %rax = 0x1
  4011a4:	ba d0 32 60 00       	mov    $0x6032d0,%edx # %rdx = 0x6032d0 (x/24 0x6032d0)
  4011a9:	eb cb                	jmp    401176 <phase_6+0x82> # 跳转进入循环b
  # ----------------------(4-end,5-start)-----------------------------
  4011ab:	48 8b 5c 24 20       	mov    0x20(%rsp),%rbx # %rbx = M[R[%rsp]+0x20]
  4011b0:	48 8d 44 24 28       	lea    0x28(%rsp),%rax # %rax = %rsp + 0x28
  4011b5:	48 8d 74 24 50       	lea    0x50(%rsp),%rsi # %rsi = %rsp + 0x50
  4011ba:	48 89 d9             	mov    %rbx,%rcx # %rcx = %rbx
  # -----------------------(5-end,6-start)---------------------（上面是变量初始化）
  4011bd:	48 8b 10             	mov    (%rax),%rdx # %rdx = M[R[%rax]]
  4011c0:	48 89 51 08          	mov    %rdx,0x8(%rcx) # M[R[%rcx] + 0x8] = %rdx
  4011c4:	48 83 c0 08          	add    $0x8,%rax # %rax = %rax + 0x8
  4011c8:	48 39 f0             	cmp    %rsi,%rax # %rax - %rsi
  4011cb:	74 05                	je     4011d2 <phase_6+0xde> # 如果%rax与%rsi相等则跳出循环c
  4011cd:	48 89 d1             	mov    %rdx,%rcx # 否则 %rcx = %rdx
  4011d0:	eb eb                	jmp    4011bd <phase_6+0xc9> # 继续循环c
  # ---------------------------------------(6-end,7-start)----------------------------
  4011d2:	48 c7 42 08 00 00 00 	movq   $0x0,0x8(%rdx) # M[R[%rdx]+0x8] = 0 # 链表最后一个元素指针域为NULL
  4011d9:	00 
  4011da:	bd 05 00 00 00       	mov    $0x5,%ebp # %rbp = 0x5
  4011df:	48 8b 43 08          	mov    0x8(%rbx),%rax # %rax = M[R[%rbx]+0x8] # rax代表M[R[%rsp]+0x20]的指针域
  4011e3:	8b 00                	mov    (%rax),%eax # %rax = M[R[%rax]] # 指针值所指元素的值，即指针所在元素的下一个元素的值
  4011e5:	39 03                	cmp    %eax,(%rbx) # M[R[%rbx]] - %rax
  4011e7:	7d 05                	jge    4011ee <phase_6+0xfa> # 如果M[R[%rbx]] >= %rax 则跳过炸弹，说明一个比一个小，链表中的元素
  4011e9:	e8 4c 02 00 00       	call   40143a <explode_bomb>
  4011ee:	48 8b 5b 08          	mov    0x8(%rbx),%rbx # %rbx = M[R[%rbx]+ 0x8] # 这是链表？以此来遍历所有的元素
  4011f2:	83 ed 01             	sub    $0x1,%ebp # %rbp = %rbp - 0x1
  4011f5:	75 e8                	jne    4011df <phase_6+0xeb> # 如果 %rbp 不等于 0则继续循环c，否则退出函数，拆弹成功
  4011f7:	48 83 c4 50          	add    $0x50,%rsp
  4011fb:	5b                   	pop    %rbx
  4011fc:	5d                   	pop    %rbp
  4011fd:	41 5c                	pop    %r12
  4011ff:	41 5d                	pop    %r13
  401201:	41 5e                	pop    %r14
  401203:	c3                   	ret    

```
###### 代码解释：
第1模块：读入输入的索引，并且约束索引值不超过6
第2模块：约束输入的索引两两不等
第3模块：a[i] = 7 - a[i]，输入的索引进行转换
第4模块：根据索引，读取内置的链表中相应位置的值，并将其读入自己的链表中。
第5模块：初始化变量，用来控制链表指针域的构造。
第6模块：构造链表指针域。
第7模块：约束自己创造的链表的值必须依次不增（即相等或递减）。
###### 解决：
从代码的第四部分，我们知道，我们输入的数字其实就是索引，用来截取`0x6032d0`开始的24个字节的数据，所以我们首先用gdb进行查看数据：
```
(gdb) x/24 0x6032d0
0x6032d0 <node1>:       332     1       6304480 0
0x6032e0 <node2>:       168     2       6304496 0
0x6032f0 <node3>:       924     3       6304512 0
0x603300 <node4>:       691     4       6304528 0
0x603310 <node5>:       477     5       6304544 0
0x603320 <node6>:       443     6       0       0

```
作者自定的链表：
```
(gdb) p /d *6304480
$6 = 168
(gdb) p /d *6304496
$7 = 924
(gdb) p /d *6304512
$8 = 691
(gdb) p /d *6304528
$9 = 477
(gdb) p /d *6304544
$10 = 443

```
由于`4011e7`表明，链表中值一个比一个小，所以，正确的索引是：`3，4，5，6，1，2`，在由于，代码第三模块表明，输入值进行了转换：`a[i] = 7 - a[i]`所以，正确的输入索引是：`7-3，7-4，7-5，7-6，7-1，7-2`即`4,3,2,1,6,5`

##### 草稿(这块有很多错误，本模块的唯一作用是为了笔者自己回忆用)
![草稿](/home/trh/文档/VSC/VSC_CPP/jljqbd-CSAPP-Labs-master/CSAPP-Labs/mylabs/2-bomb/caogao.jpg)
#### 思考与感想：
这个实验终于完成了，我知道肯定有很多彩蛋没有挖掘出来，因为在阅读汇编代码的过程中有很多函数并没有用到，这就等我有时间在挖掘吧，总的来说，着6个实验很不容易，锻炼了我的gdb调试能力与耐心，一步步的走，总的来说，感谢作者以及嗯...`Dr. Evil`,使一个单身青年即使在深夜也会因为拆掉一颗雷而兴奋不已。