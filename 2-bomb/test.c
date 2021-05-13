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