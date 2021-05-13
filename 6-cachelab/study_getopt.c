#include "cachelab.h"
#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#define MAX 100
int is_v = 0;
int num_e = 0;//组数
int num_s = 0;//每个组行数
int num_b = 0;//块大小
int num_t = 0;//标记位，t值
char filepath[MAX];
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
void finderr(char ch, char *optarg){
    if(optarg == NULL){
            printf("./csim-ref: Missing required command line argument\n");
            print_usage();
            exit(0);
    }
    if(ch == '?'){
        if(strchr("hvsEbt",ch)!=NULL){//没加参数
            printf("./csim-ref: option requires an argument -- '%c'\n",ch);
            print_usage();
            exit(0);
        }else{//输入了未知参数
            printf("./csim-ref: invalid option -- '%c'\n",ch);
            print_usage();
            exit(0);
        }
    }
}
char *endptr;
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

long long inter_str2num(char *str, int start, int end){
    long address_num = atoi64_t (str);
    long long mask = 0;
    long long t = 1;
    for(int i = start; i <= end; i++){
        mask += (t<<(64-i));
    }
    printf("mask:%llx\n",mask);
    unsigned long long un_address_num = address_num & mask;
    return un_address_num>>(64-end);
}

typedef struct 
{
    long long Valid;//有效位
    long long  tag;//标记位
    long long count;//缓冲次数
}Cache;
int main(int argc, char **argv)
{
    Cache c1;
    c1.count = 1;
    c1.tag = 1;
    c1.Valid = 1;
    printf("%d\n",11);
}
