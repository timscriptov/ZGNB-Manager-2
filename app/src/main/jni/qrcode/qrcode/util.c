
#include <stdlib.h>
#include "util.h"


unsigned qr_isqrt(unsigned _val){
  unsigned g;
  unsigned b;
  int      bshift;
  
  g=0;
  b=0x8000;
  for(bshift=16;bshift-->0;){
    unsigned t;
    t=(g<<1)+b<<bshift;
    if(t<=_val){
      g+=b;
      _val-=t;
    }
    b>>=1;
  }
  return g;
}


unsigned qr_ihypot(int _x,int _y){
  unsigned x;
  unsigned y;
  int      mask;
  int      shift;
  int      u;
  int      v;
  int      i;
  x=_x=abs(_x);
  y=_y=abs(_y);
  mask=-(x>y)&(_x^_y);
  x^=mask;
  y^=mask;
  _y^=mask;
  shift=31-qr_ilog(y);
  shift=QR_MAXI(shift,0);
  x=(unsigned)((x<<shift)*0x9B74EDAAULL>>32);
  _y=(int)((_y<<shift)*0x9B74EDA9LL>>32);
  u=x;
  mask=-(_y<0);
  x+=_y+mask^mask;
  _y-=u+mask^mask;
  u=x+1>>1;
  v=_y+1>>1;
  mask=-(_y<0);
  x+=v+mask^mask;
  _y-=u+mask^mask;
  for(i=1;i<16;i++){
    int r;
    u=x+1>>2;
    r=(1<<2*i)>>1;
    v=_y+r>>2*i;
    mask=-(_y<0);
    x+=v+mask^mask;
    _y=_y-(u+mask^mask)<<1;
  }
  return x+((1U<<shift)>>1)>>shift;
}

#if defined(__GNUC__) && defined(HAVE_FEATURES_H)
# include <features.h>
# if __GNUC_PREREQ(3,4)
#  include <limits.h>
#  if INT_MAX>=2147483647
#   define QR_CLZ0 sizeof(unsigned)*CHAR_BIT
#   define QR_CLZ(_x) (__builtin_clz(_x))
#  elif LONG_MAX>=2147483647L
#   define QR_CLZ0 sizeof(unsigned long)*CHAR_BIT
#   define QR_CLZ(_x) (__builtin_clzl(_x))
#  endif
# endif
#endif

int qr_ilog(unsigned _v){
#if defined(QR_CLZ)

  return QR_CLZ0-QR_CLZ(_v)&-!!_v;
#else
  int ret;
  int m;
  m=!!(_v&0xFFFF0000)<<4;
  _v>>=m;
  ret=m;
  m=!!(_v&0xFF00)<<3;
  _v>>=m;
  ret|=m;
  m=!!(_v&0xF0)<<2;
  _v>>=m;
  ret|=m;
  m=!!(_v&0xC)<<1;
  _v>>=m;
  ret|=m;
  ret|=!!(_v&0x2);
  return ret + !!_v;
#endif
}

#if defined(QR_TEST_SQRT)
#include <math.h>
#include <stdio.h>


int main(void){
  unsigned u;
  u=0;
  do{
    unsigned r;
    unsigned s;
    r=qr_isqrt(u);
    s=(int)sqrt(u);
    if(r!=s)printf("%u: %u!=%u\n",u,r,s);
    u++;
  }
  while(u);
  return 0;
}
#endif
