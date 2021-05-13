# 实验1：DataLab

## 要干啥？
按照要求填写bitsc.c内函数。
## 怎么干？

### bitAnd(int x, int y):
只使用～与|符号来实现x&y的效果，离散数学的知识，即
答案：~((~x)|(~y))
### getByte(int x, int n):
只使用 ! ~ & ^ | + << >>符号，且最多只使用6个，来实现获取x中第n个字节信息，n从0开始。
基本思想就是让x右移f（n）个比特位，然后与0xff进行&运算，为什么是0xff呢因为题目只要一个字节。
答案：(x>>(n<<3))&0xff;
### logicalShift(int x, int n):
只使用! ~ & ^ | + << >>符号，且最多只使用20个来实现x逻辑右移n位。对于无符号数来说<< >>都是逻辑移位，而c语言中对于有符号数 >> 为算数移位。而<<对于有符号数是逻辑移位。所以不考虑符号约束的情况下一般逻辑为：（x>>n）&[m>>(n-1)] , int m = 0x80, 00, 00, 00,但符号约束中不允许出现减号，根据csapp第二章补码加法，我们可以得到error答案1：
error答案1：(x>>n）&～(m >> (n-1)
#### 注意：(x>>n）&(～m >> (n-1)与(x>>n）&～(0x80000000>> (n-1)不同，如果事先定义int m则右移新位为1,若0x80000000则右移新位为0.
btest 错误提示为：
```
ERROR: Test logicalShift(-2147483648[0x80000000],0[0x0]) failed...
...Gives 0[0x0]. Should be -2147483648[0x80000000]
```
这个问题出现在当为0时，会出现bug，而题目禁用了if所以只能寻找某种范式来合并两种情况，实在想不到了，于是看了一下答案。
```
int logicalShift(int x, int n) {
  int mask=((0x1<<(32+~n))+~0)|(0x1<<(32+~n));
  return (x>>n)&mask;
}
```
秒啊～～～～～
#### 注意 return 中mask不能直接用((0x1<<(32+~n))+~0)|(0x1<<(32+~n))代替，原因上面讲过了。
#### 总结：
1.  m - n 等价于 m + ~n，当n不为0时成立
2. ~0等于-1
3. x >> -n 等价于 x << n
### bitCount(int x)：
x的二进制位求和，符号约束：! ~ & ^ | + << >>，最多使用符号： 40
这道题也非常难，如果没有最多符号书，则扫一遍就可以得到结果，当时只想到了分而治之思想但是没有得到一个范式来求解4位或8位的子问题。
我也是完全没有思路，最终借鉴的答案：
```
int bitCount(int x) {
  int _mask1=(0x55)|((0x55)<<8);
  int _mask2=(0x33)|((0x33)<<8);
  int _mask3=(0x0f)|((0x0f)<<8);
  int mask1=(_mask1)|(_mask1<<16);//0101...
  int mask2=(_mask2)|(_mask2<<16);//00110011.....
  int mask3=(_mask3)|(_mask3<<16);//0000111100001111....
  int mask4=(0xff)|(0xff<<16);//0*8 1*8 0*8 1*8
  int mask5=(0xff)|(0xff<<8);//0*8 0*8 1*8 1*8
  int ans=(x&mask1)+((x>>1)&mask1);
  ans=(ans&mask2)+((ans>>2)&mask2);
  ans=(ans&mask3)+((ans>>4)&mask3);
  ans=(ans&mask4)+((ans>>8)&mask4);
  ans=(ans&mask5)+((ans>>16)&mask5);
  return ans;
}
```
其中前8行都是为了构建5种mask，只是作者比较懒，你直接写mask1 = 0x0101010101010...可以吗，完全可以。只要看懂即可。解决问题是由于找到了(x&mask)+((x>>n)&mask);的范式。
基本思想为，该范式求和当前基本元素的比特和。基本元素位被合并扩大为1,2,4,8,16位。以求解1111的比特和为例。
  1111             0111           0101
&  0101       &   0101      &  0101
____________  __________  __________
  0101             0101           1010
  1111的基本元素长度为1,结果中基本元素长度为2,即01,01作为一个不可分的原子看待。[ 01] [ 01]分别代表第三位与第一位的元素为多少。第二个式子相同，向右移动一位是为了得到第四位与第二位的元素为多少，二者相加，得到的[10]_前 [10]_后 意义为[10]_前 代表3,4位的比特和，[10]_后 代表1,2位的比特和。进入下一次迭代
   1010          0010
 &  0011      & 0011
__________  ___________
   0010          0010
1010的基本元素长度为2,结果基本长度为4,&0011获取输入的第一个基本元素，和第二个基本元素。此时上式的两个结果的意义为第一个0010代表1111中3,4位的比特和，第二个0010代表1111中1,2位的比特和，二者相加即为1111，1,2,3,4位的比特和。4位的比特求和需要log2(4)=2次迭代32位的比特求和则需要log2(32)=5次迭代，这也是为什么需要5个mask的原因。
### bang(int x)：
代替！符号，符号约束为： ~ & ^ | + << >>，允许最大符号数量：12,本题运用了折叠的思想。x中的比特1会被在每次折叠中保留下来。答案为：
```
int bang(int x) {
  x = x | (x >> 16);
  x = x | (x >> 8);
  x = x | (x >> 4);
  x = x | (x >> 2);
  x = x | (x >> 1);
  return ~x & 0x1;
}
```
### tmin（void）：
返回int最小值，我的答案是0x80000000，参考答案为：0x1<<31
### fitsBits(int x, int n)：
这题题目意思是判断 x 能否用一个 n 位的补码表示,能的话就返回 1，符号约束为：! ~ & ^ | + << >>，符号最大数量约束为：15,直观的写出答案是：
```
int fitsBits(int x, int n) {
  int min = (~(1<<(n+~0)))+1;
  int max = (1<<(n+~0))+~0;
  return !((x+~min)>>31) & ((!((max+~x)>>31)) | (!((max+~x)^0xffffffff) ) );
}
```
这个答案面临两个问题：
1. 超过Max ops 
2. 出现错误
```
ERROR: Test fitsBits(0[0x0],32[0x20]) failed...
...Gives 1[0x1]. Should be 0[0x0]
Total points: 0/2
```
提示中的fitBits(0,32)=0,明显是错误答案，此外我在使用参考答案时也会出现error，
```
Score   Rating  Errors  Function
ERROR: Test fitsBits(-2147483648[0x80000000],32[0x20]) failed...
...Gives 1[0x1]. Should be 0[0x0]
Total points: 0/2
```
这也算是btest.c 的bug之一。算了，直接看答案把。
```
int fitsBits(int x, int n) {
	int c=33+~n;
	int t=(x<<c)>>c;
	return !(x^t);
}

```
其答案的思想是如果x可以由n位的补码表示，则x左移32-n位，在右移32-n位，其结果是不变的。
### int divpwr2(int x, int n)：
计算x/(2^n)，并且向0取整，我们知道在c语言中右移运算符是向下取整的，而我们要实现的是在结果大于0时向下取整，在结果小于0时向上取整。并且结合CS::APP中p73提到过的方法，（x+(1<<k)-1）>>k 来实现向上取整的思想，我们可以选取一个flag来决定x加什么。所以答案为：
```
int divpwr2(int x, int n) {
  int flag = (x>>31)&((1<<n)+~0);
    return (x+flag)>>n;
}
```
### negate(int x)：
实现-x,符号约束：! ~ & ^ | + << >>，Max ops：8
答案运用公式就可以：
```
int negate(int x) {
  return ~x+1;
}
```
### isPositive(int x)：
实现eturn 1 if x > 0, return 0 otherwise ，符号约束：! ~ & ^ | + << >>，Max ops: 8
```
int isPositive(int x) {
  int is_zero = !(x^0x0);
  return !is_zero & (((x>>31)&(0x00000001))^0x1);
}
```
### isLessOrEqual(int x, int y)：
用来实现if x <= y  then return 1, else return 0 ，符号约束：! ~ & ^ | + << >>，Max ops：24
主要思想就是，首先x<=y不等价于x-y<=0, 当然在不溢出的情况下是等价的，所以我将问题分为
溢出和不溢出的情况，x为负数y为正，可能溢出但结果肯定为1,x为正y为负数可能溢出，但结果肯定为0,除此之外的情况就可以用x-y<=0判断了，但是要实现这个逻辑，我是一个个组合试出来的，不知道参考答案的作者是根据什么技巧想出来的佩服！下面是我的答案：
```
int isLessOrEqual(int x, int y) {
  int x_f = !(x>>31);//1为正，0为负
  int y_f = !(y>>31);//1为正，0为负
  return ( !x_f | y_f) & ((!x_f&y_f)|(!((y+(~x+1))>>31)&0x00000001));
}
```
### ilog2(int x):
实现：return floor(log base 2 of x), where x > 0，符号约束：! ~ & ^ | + << >>，Max ops:90
首先在不考虑Max ops的情况下，可以使用暴力求解的思想：
```
int mask_i = ~(x>>i)^0x1; i=0,1,2,...31
return 31&mask_31 | 30&mask_30 | ...|0 & mask_0;
```
但显然ops超了，参考答案的方法，十分巧妙：
```
int ilog2(int x) {
	int ans=0;
	ans=(!!(x>>(16)))<<4;
	ans=ans+((!!(x>>(8+ans)))<<3);
	ans=ans+((!!(x>>(4+ans)))<<2);
	ans=ans+((!!(x>>(2+ans)))<<1);
	ans=ans+((!!(x>>(1+ans)))<<0);
	return ans;
}
```
log2(x) = y，其中y在题目中是一个0<=y<=31的数字，所以y = 16*a+8*b+4*c+2*d+e，其中a,b,c,d,e都是0 or 1，所以我们可以通过x>>16来通过是否为0来判断a是否为0,然后在x>>8时将16部分的权重给剔除即是16还是0，也就是加上ans变为x>>8+ans是否为0就是b是否为0,以此类推。
得到最终答案。
### 题外话：
这是data-lab中给出的u2f与f2u转换程序，非常巧妙，很有意思。注意这个和强制类型转换不一样，以 unsigned x = 121为例：强制类型转换变为了121.000000,而u2f则变为了非规格化的表示接近于0的数字。二者要搞清楚。
```
/* Convert from bit level representation to floating point number */
float u2f(unsigned u) {
  union {
    unsigned u;
    float f;
  } a;
  a.u = u;
  return a.f;
}

/* Convert from floating point number to bit-level representation */
unsigned f2u(float f) {
  union {
    unsigned u;
    float f;
  } a;
  a.f = f;
  return a.u;
}

```
此外：
```
  unsigned x = 0x251;
  float y = x;//强制类型转换。
  float y1 = 0x251;//换类别解释
```
y与y1的值也是不同的在%f输出下。并且在函数return的类型和函数定义的类型冲突时，与赋值相同return的值就是左值，函数定义返回值类型就是右值的定义类型。
### float_neg(unsigned uf)：
这题计算 -f ,f 是浮点数,这里直接改浮点数的符号位。可以使用if就简单了很多，但是要注意上面提到的强制类型转换和让计算机以另一种数据类型来解释这4个字节的区别。
```
unsigned float_neg(unsigned uf) {
  int uuf = (uf&0x7fffffff);
  int bz = 0x7f800000;
  int  isnan = uuf > bz;
  if(isnan)
    return uf;
  else
    return uf + 0x80000000;
}
```
### float_i2f(int x)：
将整型转化为浮点数的格式，注意逻辑表达式两边都应该是整形，浮点型会报错。
```
unsigned float_i2f(int x) {
int n=0xffffffff;
int e=0; /* exp */
int tmp=0;
int tmp2=0;
int cp=0;
int cp2=0;
int sign=x&0x80000000; /* 0x80000000 or 0x0 */
 if(x==0x80000000){
return 0xcf000000;
}
if(x==0){
return 0;
}
if(sign){
x=-x;
}
 x=x&0x7fffffff; /* remove sign */
tmp=x;
while(tmp){
tmp=tmp>>1;
n++;
}
 x=x-(0x1<<n); /* remove highest bit */
if(n<24){
x=x<<(23-n);
}else{
tmp2=x>>(n-23);
cp2=0x1<<(n-24);
cp=x&((cp2<<1)-1);
if(cp<cp2){
x=tmp2;
}else{
if(tmp2==0x7fffff){
x=0;
n++;
}else{
if(cp==cp2){
x=((tmp2)&0x1)+tmp2;
}else{
x=tmp2+1;
}
}
}
}
e=(127+n)<<23;
return sign|e|x;
}

```
本题参考书中浮点数的运算规则，单独考虑规格化，非规格化，无穷大和NAN情况。
### float_twice(unsigned uf)：
题计算浮点数的两倍，
```
unsigned float_twice(unsigned uf) {
int tmp=uf;
int sign=((uf>>31)<<31); /* 0x80000000 or 0x0 */
int exp=uf&0x7f800000;
int f=uf&0x7fffff;
tmp=tmp&0x7fffffff; /* remove sign */
if((tmp>>23)==0x0){
tmp=tmp<<1|sign;
return tmp;
} else if((tmp>>23)==0xff){
return uf;
} else{
if((exp>>23)+1==0xff){
return sign|0x7f800000;
}else{
return sign|(((exp>>23)+1)<<23)|f;
}
}
return tmp;
}

```
后面两题的符号约束减小了他就偏向于一般的考验你编程能力与逻辑，以及卡你特殊值，只要考虑好这几点后面两题挺好做的。