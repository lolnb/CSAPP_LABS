# perflab
## 要干啥？
重写`kernels.c`中的`rotate.c`与`smooth`函数使之更快。
## 怎么干？
### rotate.c
无法循环展开因为这道题每一步都在寻址，并且寻址的目标每一步都不相同，并且在一个地址的值一次就赋值完毕，不会对同一个地址进行二次寻址，所以我想的改进方向就是使得寻址更加快速。所以我使得寻址的地址更加连续。改进如下：
```c
void rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j, t,n;
    n = dim - 1;
    for (j = 0; j < dim; j++){
        t = (n-j)*dim;
	    for (i = 0; i < dim; i++)
	        dst[t+i] = src[RIDX(i, j, dim)];
    }
}
```
运行`./driver`查看分数：
```
Rotate: Version = rotate: Current working version:
Dim             64      128     256     512     1024    Mean
Your CPEs       2.1     2.3     3.0     4.5     6.3
Baseline CPEs   14.7    40.1    46.4    65.9    94.5
Speedup         6.9     17.8    15.3    14.6    15.1    13.3
```
`13.3`分。我找了一下这位同学的参考答案，得分为：
```
Teamname: Exely
Member 1: Exely
Email 1: Exely@github.com

Rotate: Version = naive_rotate: Naive baseline implementation:
Dim             64      128     256     512     1024    Mean
Your CPEs       2.4     3.3     6.5     11.0    12.6
Baseline CPEs   14.7    40.1    46.4    65.9    94.5
Speedup         6.2     12.1    7.1     6.0     7.5     7.5

Rotate: Version = rotate: Current working version:
Dim             64      128     256     512     1024    Mean
Your CPEs       2.4     2.7     2.8     3.1     5.9
Baseline CPEs   14.7    40.1    46.4    65.9    94.5
Speedup         6.0     15.0    16.4    21.4    16.1    13.8

Smooth: Version = smooth: Current working version:
Dim             32      64      128     256     512     Mean
Your CPEs       15.4    15.7    15.8    16.0    15.5
Baseline CPEs   695.0   698.0   702.0   717.0   722.0
Speedup         45.1    44.5    44.4    44.9    46.5    45.1

Smooth: Version = naive_smooth: Naive baseline implementation:
Dim             32      64      128     256     512     Mean
Your CPEs       61.4    61.5    60.9    62.5    66.2
Baseline CPEs   695.0   698.0   702.0   717.0   722.0
Speedup         11.3    11.3    11.5    11.5    10.9    11.3

Summary of Your Best Scores:
  Rotate: 13.8 (rotate: Current working version)
  Smooth: 45.1 (smooth: Current working version)
```
`13.8`分，虽然相差不到1分但是可以看到，这位同学的改进方式在dim较大时优势较为明显。我看了他的方式，即将矩阵分块，分为`8x8`的小矩阵。
```c
void rotate(int dim, pixel *src, pixel *dst)
{
    int i, j, a, b;
    int sdim = dim - 1;
    for (i = 0; i < dim; i += 8)
    {
        for (j = 0; j < dim; j += 8)
        {
            for (a = i; a < i + 8; a++)
            {
                for (b = j; b < j + 8; b++)
                {
                    dst[RIDX(sdim - b, a, dim)] = src[RIDX(a, b, dim)];
                }
            }
        }
    }
}
```
可以看出他没有用到我的改进方法，所以我将我与他的相结合，变成了最终形式：
```c
void rotate(int dim, pixel *src, pixel *dst){
    int i, j, a, b, t;
    int sdim = dim - 1;
    for (i = 0; i < dim; i += 8)
    {
        for (j = 0; j < dim; j += 8)
        {
            for (b = j; b < j + 8; b++)
            {
                t = (sdim - b)*dim; 
                for (a = i; a < i + 8; a++)
                {
                    dst[t+a] = src[RIDX(a, b, dim)];
                }
            }
        }
    }
}

```
最终分数为：
```
Rotate: Version = rotate: Current working version:
Dim             64      128     256     512     1024    Mean
Your CPEs       2.2     2.5     2.6     3.1     5.4
Baseline CPEs   14.7    40.1    46.4    65.9    94.5
Speedup         6.6     16.2    17.6    21.3    17.5    14.8

```
`14.8`分，相对于这位同学的优化更近一步。
## smooth.c
1. 原本的smooth.c的实现中avg存在大量的预测在max，min，以及边界检测，这会损失大量的时间。
2. 双重for循环的下标有一部分随着外层循环的改变而改变所以可以直接作为定值使用不用运行一次求解一次，avg也一样。
所以我们可以将其分类，避免了让计算机去预测是什么情况。其分类图为：
<img src="/home/trh/文档/VSC/VSC_CPP/jljqbd-CSAPP-Labs-master/CSAPP-Labs/mylabs/5-perflab/classify.png" alt="stack" title="classify.png" style="zoom:200%;" />
```c
void smooth(int dim, pixel *src, pixel *dst)
{
    int i, j, t;
    //边缘点
    //四角：左上角
    i = 0;
    j = 0;
    dst[0].blue = (src[0].blue + src[1].blue + src[dim].blue + src[dim+1].blue)/4;
    dst[0].green = (src[0].green + src[1].green + src[dim].green + src[dim+1].green)/4;
    dst[0].red = (src[0].red + src[1].red + src[dim].red + src[dim+1].red)/4;
    //四角：右上角
    i = 0;
    j = dim - 1;
    dst[dim-1].blue = (src[RIDX(i, j - 1, dim)].blue + src[RIDX(i, j, dim)].blue + src[RIDX(i + 1, j - 1, dim)].blue + src[RIDX(i + 1, j, dim)].blue)/4;
    dst[dim-1].green = (src[RIDX(i, j - 1, dim)].green + src[RIDX(i, j, dim)].green + src[RIDX(i + 1, j - 1, dim)].green + src[RIDX(i + 1, j, dim)].green)/4;
    dst[dim-1].red = (src[RIDX(i, j - 1, dim)].red + src[RIDX(i, j, dim)].red + src[RIDX(i + 1, j - 1, dim)].red + src[RIDX(i + 1, j, dim)].red)/4;
    //四角：左下角
    i = dim - 1;
    j = 0;
    dst[RIDX(i, j, dim)].blue = (src[RIDX(i - 1, j, dim)].blue + src[RIDX(i - 1, j + 1, dim)].blue + src[RIDX(i, j, dim)].blue + src[RIDX(i, j + 1, dim)].blue)/4;
    dst[RIDX(i, j, dim)].green = (src[RIDX(i - 1, j, dim)].green + src[RIDX(i - 1, j + 1, dim)].green + src[RIDX(i, j, dim)].green + src[RIDX(i, j + 1, dim)].green)/4;
    dst[RIDX(i, j, dim)].red = (src[RIDX(i - 1, j, dim)].red + src[RIDX(i - 1, j + 1, dim)].red + src[RIDX(i, j, dim)].red + src[RIDX(i, j + 1, dim)].red)/4;
    //四角：右下角
    i = dim - 1;
    j = dim - 1;
    dst[RIDX(i, j, dim)].blue = (src[RIDX(i - 1, j - 1, dim)].blue + src[RIDX(i - 1, j, dim)].blue + src[RIDX(i, j -1, dim)].blue + src[RIDX(i, j, dim)].blue)/4;
    dst[RIDX(i, j, dim)].green = (src[RIDX(i - 1, j - 1, dim)].green + src[RIDX(i - 1, j, dim)].green + src[RIDX(i, j -1, dim)].green + src[RIDX(i, j, dim)].green)/4;
    dst[RIDX(i, j, dim)].red = (src[RIDX(i - 1, j - 1, dim)].red + src[RIDX(i - 1, j, dim)].red + src[RIDX(i, j -1, dim)].red + src[RIDX(i, j, dim)].red)/4;
    //非四角
    //非四角：上
    i = 0;
    for(j = 1; j <= dim - 2; j++){
        dst[j].blue = (src[RIDX(i, j - 1, dim)].blue + src[RIDX(i, j, dim)].blue + src[RIDX(i, j + 1, dim)].blue + src[RIDX(i + 1, j - 1, dim)].blue + src[RIDX(i + 1, j, dim)].blue + src[RIDX(i + 1, j + 1, dim)].blue)/6;
        dst[j].green = (src[RIDX(i, j - 1, dim)].green + src[RIDX(i, j, dim)].green + src[RIDX(i, j + 1, dim)].green + src[RIDX(i + 1, j - 1, dim)].green + src[RIDX(i + 1, j, dim)].green + src[RIDX(i + 1, j + 1, dim)].green)/6;
        dst[j].red = (src[RIDX(i, j - 1, dim)].red + src[RIDX(i, j, dim)].red + src[RIDX(i, j + 1, dim)].red + src[RIDX(i + 1, j - 1, dim)].red + src[RIDX(i + 1, j, dim)].red + src[RIDX(i + 1, j + 1, dim)].red)/6;
    }
    //非四角：下
    i = dim - 1;
    for (j = 1; j <= dim - 2; j++){
        dst[RIDX(i, j, dim)].blue = (src[RIDX(i-1, j-1, dim)].blue + src[RIDX(i-1, j, dim)].blue + src[RIDX(i - 1, j + 1, dim)].blue + src[RIDX(i, j - 1, dim)].blue + src[RIDX(i, j, dim)].blue + src[RIDX(i, j + 1, dim)].blue)/6;
        dst[RIDX(i, j, dim)].green = (src[RIDX(i-1, j-1, dim)].green + src[RIDX(i-1, j, dim)].green + src[RIDX(i - 1, j + 1, dim)].green + src[RIDX(i, j - 1, dim)].green + src[RIDX(i, j, dim)].green + src[RIDX(i, j + 1, dim)].green)/6;
        dst[RIDX(i, j, dim)].red = (src[RIDX(i-1, j-1, dim)].red + src[RIDX(i-1, j, dim)].red + src[RIDX(i - 1, j + 1, dim)].red + src[RIDX(i, j - 1, dim)].red + src[RIDX(i, j, dim)].red + src[RIDX(i, j + 1, dim)].red)/6;
    }
    //非四角：左
    j = 0;
    for (i = 1; i <= dim - 2; i++){
        dst[RIDX(i, j, dim)].blue = (src[RIDX(i - 1, j, dim)].blue + src[RIDX(i - 1, j + 1, dim)].blue + src[RIDX(i, j, dim)].blue + src[RIDX(i, j + 1, dim)].blue + src[RIDX(i + 1 , j, dim)].blue + src[RIDX(i + 1, j + 1, dim)].blue)/6;
        dst[RIDX(i, j, dim)].green = (src[RIDX(i - 1, j, dim)].green + src[RIDX(i - 1, j + 1, dim)].green + src[RIDX(i, j, dim)].green + src[RIDX(i, j + 1, dim)].green + src[RIDX(i + 1 , j, dim)].green + src[RIDX(i + 1, j + 1, dim)].green)/6;
        dst[RIDX(i, j, dim)].red = (src[RIDX(i - 1, j, dim)].red + src[RIDX(i - 1, j + 1, dim)].red + src[RIDX(i, j, dim)].red + src[RIDX(i, j + 1, dim)].red + src[RIDX(i + 1 , j, dim)].red + src[RIDX(i + 1, j + 1, dim)].red)/6;
    }
    //非四角：右
    j = dim - 1;
    for (i = 1; i <= dim - 2; i++){
        dst[RIDX(i, j, dim)].blue = (src[RIDX(i - 1, j - 1, dim)].blue + src[RIDX(i - 1, j, dim)].blue + src[RIDX(i, j - 1, dim)].blue + src[RIDX(i, j, dim)].blue + src[RIDX(i + 1, j - 1, dim)].blue + src[RIDX(i + 1, j, dim)].blue)/6;
        dst[RIDX(i, j, dim)].green = (src[RIDX(i - 1, j - 1, dim)].green + src[RIDX(i - 1, j, dim)].green + src[RIDX(i, j - 1, dim)].green + src[RIDX(i, j, dim)].green + src[RIDX(i + 1, j - 1, dim)].green + src[RIDX(i + 1, j, dim)].green)/6;
        dst[RIDX(i, j, dim)].red = (src[RIDX(i - 1, j - 1, dim)].red + src[RIDX(i - 1, j, dim)].red + src[RIDX(i, j - 1, dim)].red + src[RIDX(i, j, dim)].red + src[RIDX(i + 1, j - 1, dim)].red + src[RIDX(i + 1, j, dim)].red)/6;
    }
    //中心点
    for (i = 1; i <= dim - 2; i++){
        t = i*dim;
        for (j = 1; j <= dim - 2; j++){
            dst[t+j].blue = (src[RIDX(i - 1, j - 1, dim)].blue + src[RIDX(i - 1, j, dim)].blue + src[RIDX(i - 1, j + 1, dim)].blue + src[RIDX(i, j - 1, dim)].blue + src[RIDX(i, j, dim)].blue + src[RIDX(i, j + 1, dim)].blue + src[RIDX(i + 1, j - 1, dim)].blue + src[RIDX(i + 1, j, dim)].blue + src[RIDX(i + 1, j + 1, dim)].blue)/9;
            dst[t+j].green = (src[RIDX(i - 1, j - 1, dim)].green + src[RIDX(i - 1, j, dim)].green + src[RIDX(i - 1, j + 1, dim)].green + src[RIDX(i, j - 1, dim)].green + src[RIDX(i, j, dim)].green + src[RIDX(i, j + 1, dim)].green + src[RIDX(i + 1, j - 1, dim)].green + src[RIDX(i + 1, j, dim)].green + src[RIDX(i + 1, j + 1, dim)].green)/9;
            dst[t+j].red = (src[RIDX(i - 1, j - 1, dim)].red + src[RIDX(i - 1, j, dim)].red + src[RIDX(i - 1, j + 1, dim)].red + src[RIDX(i, j - 1, dim)].red + src[RIDX(i, j, dim)].red + src[RIDX(i, j + 1, dim)].red + src[RIDX(i + 1, j - 1, dim)].red + src[RIDX(i + 1, j, dim)].red + src[RIDX(i + 1, j + 1, dim)].red)/9;
        }
    }
}
```
最终的分数为：
```
Teamname: jljqbd
Member 1: jljqbd
Email 1: 1927978923@qq.com

Rotate: Version = naive_rotate: Naive baseline implementation:
Dim             64      128     256     512     1024    Mean
Your CPEs       2.2     3.2     6.2     11.4    11.3
Baseline CPEs   14.7    40.1    46.4    65.9    94.5
Speedup         6.6     12.5    7.5     5.8     8.3     7.8

Rotate: Version = rotate: Current working version:
Dim             64      128     256     512     1024    Mean
Your CPEs       2.2     2.4     2.7     3.2     5.3
Baseline CPEs   14.7    40.1    46.4    65.9    94.5
Speedup         6.7     16.6    17.3    20.7    17.7    14.8

Smooth: Version = smooth: Current working version:
Dim             32      64      128     256     512     Mean
Your CPEs       14.3    14.5    14.7    14.7    15.2
Baseline CPEs   695.0   698.0   702.0   717.0   722.0
Speedup         48.5    48.0    47.8    48.6    47.6    48.1

Smooth: Version = naive_smooth: Naive baseline implementation:
Dim             32      64      128     256     512     Mean
Your CPEs       48.5    48.4    48.3    48.2    50.0
Baseline CPEs   695.0   698.0   702.0   717.0   722.0
Speedup         14.3    14.4    14.5    14.9    14.4    14.5

Summary of Your Best Scores:
  Rotate: 14.8 (rotate: Current working version)
  Smooth: 48.1 (smooth: Current working version)

```
分数每一次运行都会有变化，仅为参考。


