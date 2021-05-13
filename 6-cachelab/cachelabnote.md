# cachelab
## 干什么？
1. partA实现cache模拟器，使用LRU算法（这个没看着导致花了大量时间就是找不到bug）
2. partB优化程序
## 怎么干？
### Part A: Writing a Cache Simulator
#### 规则：
1. I不考虑
2. M相当于load两次数据。
3. LS相当于load一次数据。
4. size不用考虑，因为只要`tag`和`set_index`对上了，就一定缓存成功,想不明白为啥翻书。
#### 代码：csim.c
```c
#include "cachelab.h"
#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<math.h>
#define MAX 100
/*
*   规则：1.在无需添加参数的项后面加参数不影响结果。
*        2.在本需要加参数的项后面没加参数
*        3. 地址为8个字节。
*
*/


/*----------------------------全局常量-------------------------------*/
int is_v = 0;
int num_e = 0;//组数
int num_s = 0;//每个组行数
int num_b = 0;//块大小
char filepath[MAX];
int need_s = 0;
int need_e = 0;
int need_b = 0;
int need_t = 0;

/*----------------------------最后结果-------------------------------*/
int num_hits = 0;
int num_misses = 0;
int num_evictions = 0;

typedef struct 
{
    long long Valid;//有效位
    long long  tag;//标记位
    long long count;//缓冲次数(错误点，没看清楚要求要求替换算法是LRU最早替换算法,所以该变量的含义为时间戳)
}Cache;
/*----------------------------全局变量-------------------------------*/
char opt;//存放当前行操作符
char address[MAX];//存放当前行地址
int num_m = 64;//地址为64位
char *endptr;
extern char *optarg;
extern int optopt;
extern int opterr;
Cache *cache;
long long address_num;
//int size;//当前长度
/*---------------------------函数声明--------------------------------*/
long long inter_str2num(char *str, int start, int end);
void print_usage();
int is_misscmd(char *cmd);
void finderr1(char ch, char *optarg);
void finderr2();
long long find_evi(Cache *cache, long long start_index);
long long find_index(Cache *cache, long long start_index);
long long atoi64_t(char *str);
void load(Cache *cache, int cache_len, long long count);
void getaddress(char *line, Cache *cache);
extern int getopt(int ___argc, char *const *___argv, const char *__shortopts);
/*--------------------------函数实现---------------------------------*/
long long atoi64_t(char *str)
{
	int len = strlen(str);
    //printf("len:%d\n",len);
    char t[2];
    long long num = 0;
    for(int i = 0; i < len; i++){
        t[0] = str[i];
        t[1] = '\0';
        long temp = strtol (t, &endptr, 16);
        //printf("t:%s\t%ld\n",t,temp);
        num = (num | temp);
        if(i != len-1){
            num = num << 4;
        }
        //printf("num:%llx\n",num);
    }
	return num;
}
//截取start（从1开始）-end（到len-1结束）的比特位
long long inter_str2num(char *str, int start, int end){
    address_num = atoi64_t (str);
    long long mask = 0;
    long long t = 1;
    for(int i = start; i <= end; i++){
        mask += (t<<(64-i));
    }
    //printf("mask:%llx\n",mask);
    unsigned long long un_address_num = address_num & mask;
    return un_address_num>>(64-end);
}
void print_usage(){
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
            "Options:\n"
            "  -h         Print this help message.\n"
            "  -v         Optional verbose flag.\n"
            "  -s <num>   Number of set index bits.\n"
            "  -E <num>   Number of lines per set.\n"
            "  -b <num>   Number of block offset bits.\n"
            "  -t <file>  Trace file.\n\n"
            "Examples:\n"
            "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
            "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}
void finderr2(){
    if(!(need_b&&need_e&&need_s&&need_t)){
        printf("./csim-ref: Missing required command line argument\n");
        print_usage();
        exit(0);
    }
}
long long find_evi(Cache *cache, long long start_index){//寻找最久远的也就是最小的count
    long long min_num = cache[start_index].count;
    long long evi_index = start_index;
    for(long long i = start_index; i <= (start_index + num_e - 1); i++){
        if(cache[i].count < min_num){
            min_num = cache[i].count;
            evi_index = i;
        }
    }
    return evi_index;
}
long long find_index(Cache *cache, long long start_index){
    for(long long i = start_index; i <= (start_index + num_e - 1); i++){
        if(cache[i].Valid==0){
            return i;
        }
    }
    return -1;
}
//操作符为M时，运行两次load
void load(Cache *cache, int cache_len, long long count){

    //int num_m = strlen(address) * 4;
    int num_t = num_m - (num_s + num_b);
    long long address_tag = inter_str2num(address, 1, num_t);//标记位
    long long set_index = inter_str2num(address, num_t + 1, num_t + num_s);//组索引
    long long start_index = set_index * num_e;
    int all_one = 1;//全过一遍都是1，但是不匹配
    //printf("\ntag：%lld\tset_index:%lld\n",address_tag, set_index);
    for(long long index = start_index; index <=(start_index + num_e - 1); index++){
        if(cache[index].Valid==0){
            all_one = 0;
            continue;
        }else{
            if(cache[index].tag == address_tag){
                num_hits++;//命中
                cache[index].count = count;
                if(is_v==1){
                    printf(" hit");
                }
                return;
            }
        }
    }//出循环说明没有找到缓存,或者冷不命中
    if(is_v==1){
        printf(" miss");
    }
    num_misses++;
    long long index;
    if(all_one == 1){//有效位都是1还不命中说明需要驱逐了
        printf(" eviction");
        //寻找需要驱逐的缓存索引
        index = find_evi(cache, start_index);
        num_evictions++;
    }else{//有空位，还可以放
        index = find_index(cache, start_index);
        if(index == -1){
            printf("unknown error!");
            exit(1);
        }
    }
    cache[index].Valid = 1;
    cache[index].tag = address_tag;
    cache[index].count = count;
    return;

}
/*注意：输入的line第一个不是空格*/
void getaddress(char *line, Cache *cache){
    opt = *line++;
    int is_address = 0;
    int address_index = 0;
    while(*line!='\0'){
        if(*line==' '){
            is_address = 1;
            line++;
            continue;
        }
        if(*line==','){
            address[address_index] = '\0';
            //size = atoi(line+1);
            break;
        }
        if(is_address==1){
            address[address_index++] = *line;
        }
        line++;
    }
}
int main(int argc, char **argv)
{
    int ch;
    char line[MAX];
    while((ch = getopt(argc, argv, "hvs:E:b:t:")) != -1){
        switch(ch)
        {
            case 'h':
                print_usage();
                return 0;
            case 'v':
                is_v = 1;
                break;
            case 's':
                need_s = 1;
                num_s = atoi(optarg);
                break;
            case 'E':
                need_e = 1;
                num_e = atoi(optarg);
                break;
            case 'b':
                need_b = 1;
                num_b = atoi(optarg);
                break;
            case 't':
                need_t = 1;
                strcpy(filepath, optarg);
                break;
            default:
                //printf("./csim-ref: invalid option -- '%c'\n",optopt);
                print_usage();
                exit(0);
        }
    }
    finderr2();
    int cache_len = (1<<num_s) * num_e;
    cache = (Cache*)malloc(24 * cache_len);//Cache数组，每一个元素就是一个缓存行
    
    for(int i = 0; i < cache_len; i++){
        cache[i].tag = 0;
        cache[i].count = 0;
        cache[i].Valid = 0;
    }
    FILE *fp = fopen(filepath, "r");
    int line_len;
    long long count = 0;
    //printf("filepath:%s\tfp:%d\n",filepath, fp);
    while(fgets(line, MAX, fp) != NULL){
        count++;
        if(line[0]!=' '){
            continue;
        }
        line_len = strlen(line);
        line[line_len-1] = '\0';
        if(is_v == 1){
            printf("%s",line);//输出每一步的trace
        }
        getaddress(line+1, cache);//更新地址，操作符
        //printf("\naddress:%s\tlen:%d\n",address, num_m);
        if(opt == 'M'){
            load(cache, cache_len, count);
        }
        load(cache, cache_len, count);
        if(is_v == 1){
            printf("\n");
        }
    }
    printSummary(num_hits, num_misses, num_evictions);
    fclose(fp);
    free(cache);
    return 0;
}

```
#### 结果：
```
$ make && ./test-csim
gcc -g -Wall -Werror -std=c99 -m64 -o csim csim.c cachelab.c -lm 
# Generate a handin tar file each time you compile
tar -cvf trh-handin.tar  csim.c trans.c 
csim.c
trans.c
                        Your simulator     Reference simulator
Points (s,E,b)    Hits  Misses  Evicts    Hits  Misses  Evicts
     3 (1,1,1)       9       8       6       9       8       6  traces/yi2.trace
     3 (4,2,4)       4       5       2       4       5       2  traces/yi.trace
     3 (2,1,4)       2       3       1       2       3       1  traces/dave.trace
     3 (2,1,3)     167      71      67     167      71      67  traces/trans.trace
     3 (2,2,3)     201      37      29     201      37      29  traces/trans.trace
     3 (2,4,3)     212      26      10     212      26      10  traces/trans.trace
     3 (5,1,5)     231       7       0     231       7       0  traces/trans.trace
     6 (5,1,5)  265189   21775   21743  265189   21775   21743  traces/long.trace
    27

TEST_CSIM_RESULTS=27
```
### 学习到的东西：
1. linux下vscode调试c语言程序，gcc有参数，程序有参数的情况下如何该`launch.json`和`task.json`来对程序进行调试。
2. 常数为int类型，所以当操作数地址为64位，进行构建`mask`来对操作数进行位操作时不要用常数进行构建`mask`如`1<<56`，这就明显越界了，应该`long long t;t<<56;`
3. 手写了字符串转化为64位长整型的程序（兼容16进制和10进制但需要手动调整）
4. 学习了`getopt`的c语言命令行参数操作函数。
5. 手写了64位长整型，规定范围比特位截取函数。
### 教训：
1. 一定要好好看题，折磨了我一下午找bug，到最后才发现题目要求是LRU（最近最小使用），我写的是LFU（最不常使用）。但是即使替换算法错误我开始也得了21分，只有2个结果不对，可见在此情况下两种替换算法相差较小。
###  Part B: Optimizing Matrix Transpose
首先partb答案是`https://zhuanlan.zhihu.com/p/79058089`这位老哥的，这位老哥讲的非常详细，我是看了很多遍书，很笨没有从中得到启发，知道我看了技术博客才知道有ppt提示这么个东西，从此打开了崭新的大门，ppt讲的非常好，提示了使用分块。下面的这位老哥的代码，就是使用的分块思想，这次实验让我知道了一定要看ppt，可以少踩非常多的坑。
```c
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
   if (M == 32)
{
	int i, j, x1, x2, x3, x4, x5, x6, x7, x8, x, y;
	for (i = 0; i < N; i += 8)
		for (j = 0; j < N; j += 8)
		{
			if (i == j)
			{
				x=i;
				x1=A[x][j];x2=A[x][j+1];x3=A[x][j+2];x4=A[x][j+3];
				x5=A[x][j+4];x6=A[x][j+5];x7=A[x][j+6];x8=A[x][j+7];
 
				B[x][j]=x1;B[x][j+1]=x2;B[x][j+2]=x3;B[x][j+3]=x4;
				B[x][j+4]=x5;B[x][j+5]=x6;B[x][j+6]=x7;B[x][j+7]=x8;
 
				x1=A[x+1][j];x2=A[x+1][j+1];x3=A[x+1][j+2];x4=A[x+1][j+3];
				x5=A[x+1][j+4];x6=A[x+1][j+5];x7=A[x+1][j+6];x8=A[x+1][j+7];
 
				B[x+1][j]=B[x][j+1];B[x][j+1]=x1;
 
				B[x+1][j+1]=x2;B[x+1][j+2]=x3;B[x+1][j+3]=x4;
				B[x+1][j+4]=x5;B[x+1][j+5]=x6;B[x+1][j+6]=x7;B[x+1][j+7]=x8;
 
				x1=A[x+2][j];x2=A[x+2][j+1];x3=A[x+2][j+2];x4=A[x+2][j+3];
				x5=A[x+2][j+4];x6=A[x+2][j+5];x7=A[x+2][j+6];x8=A[x+2][j+7];
 
				B[x+2][j]=B[x][j+2];B[x+2][j+1]=B[x+1][j+2];
				B[x][j+2]=x1;B[x+1][j+2]=x2;B[x+2][j+2]=x3;
				B[x+2][j+3]=x4;B[x+2][j+4]=x5;B[x+2][j+5]=x6;B[x+2][j+6]=x7;B[x+2][j+7]=x8;
 
				x1=A[x+3][j];x2=A[x+3][j+1];x3=A[x+3][j+2];x4=A[x+3][j+3];
				x5=A[x+3][j+4];x6=A[x+3][j+5];x7=A[x+3][j+6];x8=A[x+3][j+7];
 
				B[x+3][j]=B[x][j+3];B[x+3][j+1]=B[x+1][j+3];B[x+3][j+2]=B[x+2][j+3];
				B[x][j+3]=x1;B[x+1][j+3]=x2;B[x+2][j+3]=x3;B[x+3][j+3]=x4;
				B[x+3][j+4]=x5;B[x+3][j+5]=x6;B[x+3][j+6]=x7;B[x+3][j+7]=x8;
 
				x1=A[x+4][j];x2=A[x+4][j+1];x3=A[x+4][j+2];x4=A[x+4][j+3];
				x5=A[x+4][j+4];x6=A[x+4][j+5];x7=A[x+4][j+6];x8=A[x+4][j+7];
 
				B[x+4][j]=B[x][j+4];B[x+4][j+1]=B[x+1][j+4];B[x+4][j+2]=B[x+2][j+4];B[x+4][j+3]=B[x+3][j+4];
				B[x][j+4]=x1;B[x+1][j+4]=x2;B[x+2][j+4]=x3;B[x+3][j+4]=x4;B[x+4][j+4]=x5;
				B[x+4][j+5]=x6;B[x+4][j+6]=x7;B[x+4][j+7]=x8;
 
				x1=A[x+5][j];x2=A[x+5][j+1];x3=A[x+5][j+2];x4=A[x+5][j+3];
				x5=A[x+5][j+4];x6=A[x+5][j+5];x7=A[x+5][j+6];x8=A[x+5][j+7];
 
				B[x+5][j]=B[x][j+5];B[x+5][j+1]=B[x+1][j+5];B[x+5][j+2]=B[x+2][j+5];B[x+5][j+3]=B[x+3][j+5];B[x+5][j+4]=B[x+4][j+5];
				B[x][j+5]=x1;B[x+1][j+5]=x2;B[x+2][j+5]=x3;B[x+3][j+5]=x4;B[x+4][j+5]=x5;B[x+5][j+5]=x6;
				B[x+5][j+6]=x7;B[x+5][j+7]=x8;
 
				x1=A[x+6][j];x2=A[x+6][j+1];x3=A[x+6][j+2];x4=A[x+6][j+3];
				x5=A[x+6][j+4];x6=A[x+6][j+5];x7=A[x+6][j+6];x8=A[x+6][j+7];
 
				B[x+6][j]=B[x][j+6];B[x+6][j+1]=B[x+1][j+6];B[x+6][j+2]=B[x+2][j+6];B[x+6][j+3]=B[x+3][j+6];
				B[x+6][j+4]=B[x+4][j+6];B[x+6][j+5]=B[x+5][j+6];
				B[x][j+6]=x1;B[x+1][j+6]=x2;B[x+2][j+6]=x3;B[x+3][j+6]=x4;B[x+4][j+6]=x5;B[x+5][j+6]=x6;
				B[x+6][j+6]=x7;B[x+6][j+7]=x8;
 
				x1=A[x+7][j];x2=A[x+7][j+1];x3=A[x+7][j+2];x4=A[x+7][j+3];
				x5=A[x+7][j+4];x6=A[x+7][j+5];x7=A[x+7][j+6];x8=A[x+7][j+7];
 
				B[x+7][j]=B[x][j+7];B[x+7][j+1]=B[x+1][j+7];B[x+7][j+2]=B[x+2][j+7];B[x+7][j+3]=B[x+3][j+7];
				B[x+7][j+4]=B[x+4][j+7];B[x+7][j+5]=B[x+5][j+7];B[x+7][j+6]=B[x+6][j+7];
				B[x][j+7]=x1;B[x+1][j+7]=x2;B[x+2][j+7]=x3;B[x+3][j+7]=x4;B[x+4][j+7]=x5;B[x+5][j+7]=x6;B[x+6][j+7]=x7;
				B[x+7][j+7]=x8;
			}
				
			else
			{
				for(x = i; x < (i + 8); ++x)
					for(y = j; y < (j + 8); ++y)
						B[y][x] = A[x][y];
			}
		}
}
    else if (M == 64)
	{
		int i, j, x, y;
		int x1, x2, x3, x4, x5, x6, x7, x8;
		for (i = 0; i < N; i += 8)
			for (j = 0; j < M; j += 8)
			{
				for (x = i; x < i + 4; ++x)
				{
					x1 = A[x][j]; x2 = A[x][j+1]; x3 = A[x][j+2]; x4 = A[x][j+3];
					x5 = A[x][j+4]; x6 = A[x][j+5]; x7 = A[x][j+6]; x8 = A[x][j+7];
					
					B[j][x] = x1; B[j+1][x] = x2; B[j+2][x] = x3; B[j+3][x] = x4;
					B[j][x+4] = x5; B[j+1][x+4] = x6; B[j+2][x+4] = x7; B[j+3][x+4] = x8;
				}
				for (y = j; y < j + 4; ++y)
				{
					x1 = A[i+4][y]; x2 = A[i+5][y]; x3 = A[i+6][y]; x4 = A[i+7][y];
					x5 = B[y][i+4]; x6 = B[y][i+5]; x7 = B[y][i+6]; x8 = B[y][i+7];
					
					B[y][i+4] = x1; B[y][i+5] = x2; B[y][i+6] = x3; B[y][i+7] = x4;
					B[y+4][i] = x5; B[y+4][i+1] = x6; B[y+4][i+2] = x7; B[y+4][i+3] = x8;
				}
				for (x = i + 4; x < i + 8; ++x)
				{
					x1 = A[x][j+4]; x2 = A[x][j+5]; x3 = A[x][j+6]; x4 = A[x][j+7];
					B[j+4][x] = x1; B[j+5][x] = x2; B[j+6][x] = x3; B[j+7][x] = x4;
				}
			}
	}
    else if(M == 61)
	{
		int i, j, v1, v2, v3, v4, v5, v6, v7, v8;
		int n = N / 8 * 8;
		int m = M / 8 * 8;
		for (j = 0; j < m; j += 8)
			for (i = 0; i < n; ++i)
			{
				v1 = A[i][j];
				v2 = A[i][j+1];
				v3 = A[i][j+2];
				v4 = A[i][j+3];
				v5 = A[i][j+4];
				v6 = A[i][j+5];
				v7 = A[i][j+6];
				v8 = A[i][j+7];
				
				B[j][i] = v1;
				B[j+1][i] = v2;
				B[j+2][i] = v3;
				B[j+3][i] = v4;
				B[j+4][i] = v5;
				B[j+5][i] = v6;
				B[j+6][i] = v7;
				B[j+7][i] = v8;
			}
		for (i = n; i < N; ++i)
			for (j = m; j < M; ++j)
			{
				v1 = A[i][j];
				B[j][i] = v1;
			}
		for (i = 0; i < N; ++i)
			for (j = m; j < M; ++j)
			{
				v1 = A[i][j];
				B[j][i] = v1;
			}
		for (i = n; i < N; ++i)
			for (j = 0; j < M; ++j)
			{
				v1 = A[i][j];
				B[j][i] = v1;
			}
	}
}
```