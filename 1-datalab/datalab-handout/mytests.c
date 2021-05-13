#include<stdio.h>
#include<math.h>
int zz_fitsBits(int x, int n) {
  int c=33+~n;
  int t=(x<<c)>>c;
  return !(x^t);
}

int test_fitsBits(int x, int n)
{
  int TMin_n = -(1 << (n-1));
  int TMax_n = (1 << (n-1)) - 1;
  return x >= TMin_n && x <= TMax_n;
}
int fitsBits(int x, int n) {
  int min = (~(1<<(n+~0)))+1;
  int max = (1<<(n+~0))+~0;
  return !((x+~min)>>31) & ((!((max+~x)>>31)) | (!((max+~x)^0xffffffff) ) );
}
int negate(int x) {
  int x_z = ((~x)&0x7fffffff) + 1;
  int x_f = ~(x+~0)&0x7fffffff;
  int flag = x>>31;
  int fmin = !(x^0x80000000);
  return ((fmin) & (~flag) & x_z) | ((fmin) & (~flag) & x_f) | ((fmin) & 0x80000000);
}
int isLessOrEqual(int x, int y) {
  int x_f = !(x>>31);//1为正，0为负
  int y_f = !(y>>31);//1为正，0为负
  return ( !x_f | y_f) & ((!x_f&y_f)|(!((y+(~x+1))>>31)&0x00000001));
}
int test_ilog2(int x) {
  int mask, result;
  /* find the leftmost bit */
  result = 31;
  mask = 1 << result;
  while (!(x & mask)) {
    printf("0x%x\t%d\n", (x & mask), !(x & mask));
    result--;
    mask = 1 << result;
  }
  return result;
}
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
unsigned test_float_neg1(unsigned uf) {
    float f = u2f(uf);
    float nf = -f;
    if (isnan(f))
 return uf;
    else
 return f2u(nf);
}
unsigned test_float_neg2(unsigned uf) {
    float f = u2f(uf);
    float nf = -f;
    if (isnan(f))
 return uf;
    else
 return nf;
}
int main(){
    /* logicalShift */
    /*1w
    int n = 0xfffffffe;
    int n1 = 0x7ffffffe;
    int m = n>>1;
    int m1 = n1>>1;
    printf("0x%x\n",m);
    printf("0x%x\n",m1);
    1*/
   /*2 
   int n = 0x1;
   int m = n<<3;
   printf("0x%x\n",m);
   */
  /*3
  int m = 0x80000000;
  int n = 0x70000000;
  int m1 = 0x1;
  int n1 = 0x2;
  printf("0x%x\n",~(m >> (4 + 0xffffffff)));
  printf("0x%x\n", m>>1);
  printf("0x%x\n", n>>1);
  printf("0x%x\n", m1<<1);
  printf("%d\n", 8>>-1);
  printf("0x%x\n", n1<<1);
  //(x>>n) & ~(0x80000000 >> (n + 0xffffffff));
  */
 /* fitsBits*/
 /*1
 printf("%d\n", 0x80000000);
 printf("%d\n",0x80000000-1);
 */
/*2

    int x = -2147483648;
    int n = 32;
    int min = (~(1<<(n+~0)))+1;
    int max = (1<<(n+~0))+~0;
    //printf("%d \t %d\n", min, max);
    //printf("%d\t%d\t%d\n",!((x+~min)>>31), !((max+~x)>>31),!((max+~x)^0xffffffff));
    //printf("%d\n", max+~x);
    printf("%d\n",test_fitsBits(0, 32));
    printf("%d\n",fitsBits(0, 32));
    printf("%d\n",zz_fitsBits(0, 32));
    //!(x+~min)>>31 & !(max+~x)>>31;
    //!((x+~min)>>31) & (!((max+~x)>>31)|((max+~x)^0xffffffff))
    */
   /*negate 
   int x = 0x80000000;
   //int x = 12;
   printf("0x%x\t 0x%x\n",1-x, -x);
   printf("%d\t %d\t %d\n",1-x,!-2,!0);
   //printf("0x%x\n", negate(x));
   */
 /* 
  int x = -2147483648;
  int y = -2147483648;
  isLessOrEqual(x, y);
  */
  unsigned x = 0x80000000;
  float y = x;
  float y1 = x & 0xffffffff;
  printf("0x%x\t 0x%x\t 0x%x\n", x, test_float_neg1(x), test_float_neg2(x));
  printf("%f\t %f\t %f\n",x,y, y1);
  /*
  printf("0x%x\t 0x%x\n", x, y);
  printf("%f\t %f\t %f\n",x, y, y1);
  printf("%u\t %f\n", x, y);
  printf("%f\t %u\n",u2f(x), f2u(u2f(x)));
  printf("%f\t %u\n", (float)x, (unsigned)((float)x));
 */
}