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
