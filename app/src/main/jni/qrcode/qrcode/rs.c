
#include <stdlib.h>
#include <string.h>
#include "rs.h"






void rs_gf256_init(rs_gf256 *_gf,unsigned _ppoly){
  unsigned p;
  int      i;
  
  p=1;
  for(i=0;i<256;i++){
    _gf->exp[i]=_gf->exp[i+255]=p;
    p=((p<<1)^(-(p>>7)&_ppoly))&0xFF;
  }
  
  for(i=0;i<255;i++)_gf->log[_gf->exp[i]]=i;
  
  _gf->log[0]=0;
}


static unsigned rs_gmul(const rs_gf256 *_gf,unsigned _a,unsigned _b){
  return _a==0||_b==0?0:_gf->exp[_gf->log[_a]+_gf->log[_b]];
}


static unsigned rs_gdiv(const rs_gf256 *_gf,unsigned _a,unsigned _b){
  return _a==0?0:_gf->exp[_gf->log[_a]+255-_gf->log[_b]];
}


static unsigned rs_hgmul(const rs_gf256 *_gf,unsigned _a,unsigned _logb){
  return _a==0?0:_gf->exp[_gf->log[_a]+_logb];
}


static unsigned rs_gsqrt(const rs_gf256 *_gf,unsigned _a){
  unsigned loga;
  if(!_a)return 0;
  loga=_gf->log[_a];
  return _gf->exp[loga+(255&-(loga&1))>>1];
}




static int rs_quadratic_solve(const rs_gf256 *_gf,unsigned _b,unsigned _c,
 unsigned char _x[2]){
  unsigned b;
  unsigned logb;
  unsigned logb2;
  unsigned logb4;
  unsigned logb8;
  unsigned logb12;
  unsigned logb14;
  unsigned logc;
  unsigned logc2;
  unsigned logc4;
  unsigned c8;
  unsigned g3;
  unsigned z3;
  unsigned l3;
  unsigned c0;
  unsigned g2;
  unsigned l2;
  unsigned z2;
  int      inc;
  
  if(!_b){
    _x[0]=rs_gsqrt(_gf,_c);
    return 1;
  }
  
  if(!_c){
    _x[0]=0;
    _x[1]=_b;
    return 2;
  }
  logb=_gf->log[_b];
  logc=_gf->log[_c];
  
  inc=logb%(255/15)==0;
  if(inc){
    b=_gf->exp[logb+254];
    logb=_gf->log[b];
    _c=_gf->exp[logc+253];
    logc=_gf->log[_c];
  }
  else b=_b;
  logb2=_gf->log[_gf->exp[logb<<1]];
  logb4=_gf->log[_gf->exp[logb2<<1]];
  logb8=_gf->log[_gf->exp[logb4<<1]];
  logb12=_gf->log[_gf->exp[logb4+logb8]];
  logb14=_gf->log[_gf->exp[logb2+logb12]];
  logc2=_gf->log[_gf->exp[logc<<1]];
  logc4=_gf->log[_gf->exp[logc2<<1]];
  c8=_gf->exp[logc4<<1];
  g3=rs_hgmul(_gf,
   _gf->exp[logb14+logc]^_gf->exp[logb12+logc2]^_gf->exp[logb8+logc4]^c8,logb);
  
  if(_gf->log[g3]%(255/15)!=0)return 0;
  
  z3=rs_gdiv(_gf,g3,_gf->exp[logb8<<1]^b);
  l3=rs_hgmul(_gf,rs_gmul(_gf,z3,z3)^rs_hgmul(_gf,z3,logb)^_c,255-logb2);
  c0=rs_hgmul(_gf,l3,255-2*(255/15));
  
  g2=rs_hgmul(_gf,
   rs_hgmul(_gf,c0,255-2*(255/15))^rs_gmul(_gf,c0,c0),255-255/15);
  z2=rs_gdiv(_gf,g2,_gf->exp[255-(255/15)*4]^_gf->exp[255-(255/15)]);
  l2=rs_hgmul(_gf,
   rs_gmul(_gf,z2,z2)^rs_hgmul(_gf,z2,255-(255/15))^c0,2*(255/15));
  
  _x[0]=_gf->exp[_gf->log[z3^rs_hgmul(_gf,
   rs_hgmul(_gf,l2,255/3)^rs_hgmul(_gf,z2,255/15),logb)]+inc];
  _x[1]=_x[0]^_b;
  return 2;
}


static int rs_cubic_solve(const rs_gf256 *_gf,
 unsigned _a,unsigned _b,unsigned _c,unsigned char _x[3]){
  unsigned k;
  unsigned logd;
  unsigned d2;
  unsigned logd2;
  unsigned logw;
  int      nroots;
  
  if(!_c){
    nroots=rs_quadratic_solve(_gf,_a,_b,_x);
    if(_b)_x[nroots++]=0;
    return nroots;
  }
  
  k=rs_gmul(_gf,_a,_b)^_c;
  d2=rs_gmul(_gf,_a,_a)^_b;
  if(!d2){
    int logx;
    if(!k){
      
      _x[0]=_a;
      return 1;
    }
    logx=_gf->log[k];
    if(logx%3!=0)return 0;
    logx/=3;
    _x[0]=_a^_gf->exp[logx];
    _x[1]=_a^_gf->exp[logx+255/3];
    _x[2]=_a^_x[0]^_x[1];
    return 3;
  }
  logd2=_gf->log[d2];
  logd=logd2+(255&-(logd2&1))>>1;
  k=rs_gdiv(_gf,k,_gf->exp[logd+logd2]);
  
  nroots=rs_quadratic_solve(_gf,k,1,_x);
  if(nroots<1){
    
    return 0;
  }
  
  logw=_gf->log[_x[0]];
  if(logw){
    if(logw%3!=0)return 0;
    logw/=3;
    
    _x[0]=_gf->exp[_gf->log[_gf->exp[logw]^_gf->exp[255-logw]]+logd]^_a;
    logw+=255/3;
    _x[1]=_gf->exp[_gf->log[_gf->exp[logw]^_gf->exp[255-logw]]+logd]^_a;
    _x[2]=_x[0]^_x[1]^_a;
    return 3;
  }
  else{
    _x[0]=_a;
    
    
    return 1;
  }
}


static int rs_quartic_solve(const rs_gf256 *_gf,
 unsigned _a,unsigned _b,unsigned _c,unsigned _d,unsigned char _x[3]){
  unsigned r;
  unsigned s;
  unsigned t;
  unsigned b;
  int      nroots;
  int      i;
  
  if(!_d){
    nroots=rs_cubic_solve(_gf,_a,_b,_c,_x);
    if(_c)_x[nroots++]=0;
    return nroots;
  }
  if(_a){
    unsigned loga;
    
    loga=_gf->log[_a];
    r=rs_hgmul(_gf,_c,255-loga);
    s=rs_gsqrt(_gf,r);
    t=_d^rs_gmul(_gf,_b,r)^rs_gmul(_gf,r,r);
    if(t){
      unsigned logti;
      logti=255-_gf->log[t];
      
      nroots=rs_quartic_solve(_gf,0,rs_hgmul(_gf,_b^rs_hgmul(_gf,s,loga),logti),
       _gf->exp[loga+logti],_gf->exp[logti],_x);
      for(i=0;i<nroots;i++)_x[i]=_gf->exp[255-_gf->log[_x[i]]]^s;
    }
    else{
      
      nroots=rs_quadratic_solve(_gf,_a,_b^r,_x);
      
      if(nroots!=2||_x[0]!=s&&_x[1]!=s)_x[nroots++]=s;
    }
    return nroots;
  }
  
  if(!_c)return rs_quadratic_solve(_gf,rs_gsqrt(_gf,_b),rs_gsqrt(_gf,_d),_x);
  
  nroots=rs_cubic_solve(_gf,0,_b,_c,_x);
  if(nroots<1){
    
    return 0;
  }
  r=_x[0];
  
  b=rs_gdiv(_gf,_c,r);
  nroots=rs_quadratic_solve(_gf,b,_d,_x);
  if(nroots<2)return 0;
  s=_x[0];
  t=_x[1];
  
  nroots=rs_quadratic_solve(_gf,r,s,_x);
  return nroots+rs_quadratic_solve(_gf,r,t,_x+nroots);
}



static void rs_poly_zero(unsigned char *_p,int _dp1){
  memset(_p,0,_dp1*sizeof(*_p));
}

static void rs_poly_copy(unsigned char *_p,const unsigned char *_q,int _dp1){
  memcpy(_p,_q,_dp1*sizeof(*_p));
}


static void rs_poly_mul_x(unsigned char *_p,const unsigned char *_q,int _dp1){
  memmove(_p+1,_q,(_dp1-1)*sizeof(*_p));
  _p[0]=0;
}


static void rs_poly_div_x(unsigned char *_p,const unsigned char *_q,int _dp1){
  memmove(_p,_q+1,(_dp1-1)*sizeof(*_p));
  _p[_dp1-1]=0;
}


static void rs_poly_mult(const rs_gf256 *_gf,unsigned char *_p,int _dp1,
 const unsigned char *_q,int _ep1,const unsigned char *_r,int _fp1){
  int m;
  int i;
  rs_poly_zero(_p,_dp1);
  m=_ep1<_dp1?_ep1:_dp1;
  for(i=0;i<m;i++)if(_q[i]!=0){
    unsigned logqi;
    int      n;
    int      j;
    n=_dp1-i<_fp1?_dp1-i:_fp1;
    logqi=_gf->log[_q[i]];
    for(j=0;j<n;j++)_p[i+j]^=rs_hgmul(_gf,_r[j],logqi);
  }
}




static void rs_calc_syndrome(const rs_gf256 *_gf,int _m0,
 unsigned char *_s,int _npar,const unsigned char *_data,int _ndata){
  int i;
  int j;
  for(j=0;j<_npar;j++){
    unsigned alphaj;
    unsigned sj;
    sj=0;
    alphaj=_gf->log[_gf->exp[j+_m0]];
    for(i=0;i<_ndata;i++)sj=_data[i]^rs_hgmul(_gf,sj,alphaj);
    _s[j]=sj;
  }
}




static void rs_init_lambda(const rs_gf256 *_gf,unsigned char *_lambda,int _npar,
 const unsigned char *_erasures,int _nerasures,int _ndata){
  int i;
  int j;
  rs_poly_zero(_lambda,(_npar<4?4:_npar)+1);
  _lambda[0]=1;
  for(i=0;i<_nerasures;i++)for(j=i+1;j>0;j--){
    _lambda[j]^=rs_hgmul(_gf,_lambda[j-1],_ndata-1-_erasures[i]);
  }
}


static int rs_modified_berlekamp_massey(const rs_gf256 *_gf,
 unsigned char *_lambda,const unsigned char *_s,unsigned char *_omega,int _npar,
 const unsigned char *_erasures,int _nerasures,int _ndata){
  unsigned char tt[256];
  int           n;
  int           l;
  int           k;
  int           i;
  
  rs_init_lambda(_gf,_lambda,_npar,_erasures,_nerasures,_ndata);
  rs_poly_copy(tt,_lambda,_npar+1);
  l=_nerasures;
  k=0;
  for(n=_nerasures+1;n<=_npar;n++){
    unsigned d;
    rs_poly_mul_x(tt,tt,n-k+1);
    d=0;
    for(i=0;i<=l;i++)d^=rs_gmul(_gf,_lambda[i],_s[n-1-i]);
    if(d!=0){
      unsigned logd;
      logd=_gf->log[d];
      if(l<n-k){
        int t;
        for(i=0;i<=n-k;i++){
          unsigned tti;
          tti=tt[i];
          tt[i]=rs_hgmul(_gf,_lambda[i],255-logd);
          _lambda[i]=_lambda[i]^rs_hgmul(_gf,tti,logd);
        }
        t=n-k;
        k=n-l;
        l=t;
      }
      else for(i=0;i<=l;i++)_lambda[i]=_lambda[i]^rs_hgmul(_gf,tt[i],logd);
    }
  }
  rs_poly_mult(_gf,_omega,_npar,_lambda,l+1,_s,_npar);
  return l;
}


static int rs_find_roots(const rs_gf256 *_gf,unsigned char *_epos,
 const unsigned char *_lambda,int _nerrors,int _ndata){
  unsigned alpha;
  int      nroots;
  int      i;
  nroots=0;
  if(_nerrors<=4){
    
    _nerrors=rs_quartic_solve(_gf,_lambda[1],_lambda[2],_lambda[3],_lambda[4],
     _epos);
    for(i=0;i<_nerrors;i++)if(_epos[i]){
      alpha=_gf->log[_epos[i]];
      if((int)alpha<_ndata)_epos[nroots++]=alpha;
    }
    return nroots;
  }
  else for(alpha=0;(int)alpha<_ndata;alpha++){
    unsigned alphai;
    unsigned sum;
    sum=0;
    alphai=0;
    for(i=0;i<=_nerrors;i++){
      sum^=rs_hgmul(_gf,_lambda[_nerrors-i],alphai);
      alphai=_gf->log[_gf->exp[alphai+alpha]];
    }
    if(!sum)_epos[nroots++]=alpha;
  }
  return nroots;
}


int rs_correct(const rs_gf256 *_gf,int _m0,unsigned char *_data,int _ndata,
 int _npar,const unsigned char *_erasures,int _nerasures){
  
  unsigned char lambda[256];
  unsigned char omega[256];
  unsigned char epos[256];
  unsigned char s[256];
  int           i;
  
  if(_nerasures>_npar)return -1;
  
  rs_calc_syndrome(_gf,_m0,s,_npar,_data,_ndata);
  
  for(i=0;i<_npar;i++)if(s[i]){
    int nerrors;
    int j;
    
    nerrors=rs_modified_berlekamp_massey(_gf,lambda,s,omega,_npar,
     _erasures,_nerasures,_ndata);
    
    if(nerrors<=0||nerrors-_nerasures>_npar-_nerasures>>1)return -1;
    
    if(rs_find_roots(_gf,epos,lambda,nerrors,_ndata)<nerrors)return -1;
    
    for(i=0;i<nerrors;i++){
      unsigned a;
      unsigned b;
      unsigned alpha;
      unsigned alphan1;
      unsigned alphan2;
      unsigned alphanj;
      alpha=epos[i];
      
      a=0;
      alphan1=255-alpha;
      alphanj=0;
      for(j=0;j<_npar;j++){
        a^=rs_hgmul(_gf,omega[j],alphanj);
        alphanj=_gf->log[_gf->exp[alphanj+alphan1]];
      }
      
      b=0;
      alphan2=_gf->log[_gf->exp[alphan1<<1]];
      alphanj=alphan1+_m0*alpha%255;
      for(j=1;j<=_npar;j+=2){
        b^=rs_hgmul(_gf,lambda[j],alphanj);
        alphanj=_gf->log[_gf->exp[alphanj+alphan2]];
      }
      
      _data[_ndata-1-alpha]^=rs_gdiv(_gf,a,b);
    }
    return nerrors;
  }
  return 0;
}




void rs_compute_genpoly(const rs_gf256 *_gf,int _m0,
 unsigned char *_genpoly,int _npar){
  int i;
  if(_npar<=0)return;
  rs_poly_zero(_genpoly,_npar);
  _genpoly[0]=1;
  
  for(i=0;i<_npar;i++){
    unsigned alphai;
    int      n;
    int      j;
    n=i+1<_npar-1?i+1:_npar-1;
    alphai=_gf->log[_gf->exp[_m0+i]];
    for(j=n;j>0;j--)_genpoly[j]=_genpoly[j-1]^rs_hgmul(_gf,_genpoly[j],alphai);
    _genpoly[0]=rs_hgmul(_gf,_genpoly[0],alphai);
  }
}


void rs_encode(const rs_gf256 *_gf,unsigned char *_data,int _ndata,
 const unsigned char *_genpoly,int _npar){
  unsigned char *lfsr;
  unsigned       d;
  int            i;
  int            j;
  if(_npar<=0)return;
  lfsr=_data+_ndata-_npar;
  rs_poly_zero(lfsr,_npar);
  for(i=0;i<_ndata-_npar;i++){
    d=_data[i]^lfsr[0];
    if(d){
      unsigned logd;
      logd=_gf->log[d];
      for(j=0;j<_npar-1;j++){
        lfsr[j]=lfsr[j+1]^rs_hgmul(_gf,_genpoly[_npar-1-j],logd);
      }
      lfsr[_npar-1]=rs_hgmul(_gf,_genpoly[0],logd);
    }
    else rs_poly_div_x(lfsr,lfsr,_npar);
  }
}

#if defined(RS_TEST_ENC)
#include <stdio.h>
#include <stdlib.h>

int main(void){
  rs_gf256 gf;
  int      k;
  rs_gf256_init(&gf,QR_PPOLY);
  srand(0);
  for(k=0;k<64*1024;k++){
    unsigned char genpoly[256];
    unsigned char data[256];
    unsigned char epos[256];
    int           ndata;
    int           npar;
    int           nerrors;
    int           i;
    ndata=rand()&0xFF;
    npar=ndata>0?rand()%ndata:0;
    for(i=0;i<ndata-npar;i++)data[i]=rand()&0xFF;
    rs_compute_genpoly(&gf,QR_M0,genpoly,npar);
    rs_encode(&gf,QR_M0,data,ndata,genpoly,npar);
    
    printf("%i %i",ndata,npar);
    for(i=0;i<ndata;i++)printf(" %i",data[i]);
    printf(" 0\n");
    
    fprintf(stderr,"Success!\n",nerrors);
    for(i=0;i<ndata;i++)fprintf(stderr,"%i%s",data[i],i+1<ndata?" ":"\n");
    if(npar>0){
      
      nerrors=rand()%(npar+1);
      if(nerrors>0){
        
        if(nerrors<=npar>>1){
          fprintf(stderr,"Success!\n",nerrors);
          for(i=0;i<ndata;i++){
            fprintf(stderr,"%i%s",data[i],i+1<ndata?" ":"\n");
          }
        }
        else fprintf(stderr,"Failure.\n");
        fprintf(stderr,"Success!\n",nerrors);
        for(i=0;i<ndata;i++)fprintf(stderr,"%i%s",data[i],i+1<ndata?" ":"\n");
        for(i=0;i<ndata;i++)epos[i]=i;
        for(i=0;i<nerrors;i++){
          unsigned char e;
          int           ei;
          ei=rand()%(ndata-i)+i;
          e=epos[ei];
          epos[ei]=epos[i];
          epos[i]=e;
          data[e]^=rand()%255+1;
        }
        
        printf("%i %i",ndata,npar);
        for(i=0;i<ndata;i++)printf(" %i",data[i]);
        printf(" 0\n");
        
        printf("%i %i",ndata,npar);
        for(i=0;i<ndata;i++)printf(" %i",data[i]);
        printf(" %i",nerrors);
        for(i=0;i<nerrors;i++)printf(" %i",epos[i]);
        printf("\n");
      }
    }
  }
  return 0;
}
#endif

#if defined(RS_TEST_DEC)
#include <stdio.h>

int main(void){
  rs_gf256 gf;
  rs_gf256_init(&gf,QR_PPOLY);
  for(;;){
    unsigned char data[255];
    unsigned char erasures[255];
    int idata[255];
    int ierasures[255];
    int ndata;
    int npar;
    int nerasures;
    int nerrors;
    int i;
    if(scanf("%i",&ndata)<1||ndata<0||ndata>255||
     scanf("%i",&npar)<1||npar<0||npar>ndata)break;
    for(i=0;i<ndata;i++){
      if(scanf("%i",idata+i)<1||idata[i]<0||idata[i]>255)break;
      data[i]=idata[i];
    }
    if(i<ndata)break;
    if(scanf("%i",&nerasures)<1||nerasures<0||nerasures>ndata)break;
    for(i=0;i<nerasures;i++){
      if(scanf("%i",ierasures+i)<1||ierasures[i]<0||ierasures[i]>=ndata)break;
      erasures[i]=ierasures[i];
    }
    nerrors=rs_correct(&gf,QR_M0,data,ndata,npar,erasures,nerasures);
    if(nerrors>=0){
      unsigned char data2[255];
      unsigned char genpoly[255];
      for(i=0;i<ndata-npar;i++)data2[i]=data[i];
      rs_compute_genpoly(&gf,QR_M0,genpoly,npar);
      rs_encode(&gf,QR_M0,data2,ndata,genpoly,npar);
      for(i=ndata-npar;i<ndata;i++)if(data[i]!=data2[i]){
        printf("Abject failure! %i!=%i\n",data[i],data2[i]);
      }
      printf("Success!\n",nerrors);
      for(i=0;i<ndata;i++)printf("%i%s",data[i],i+1<ndata?" ":"\n");
    }
    else printf("Failure.\n");
  }
  return 0;
}
#endif

#if defined(RS_TEST_ROOTS)
#include <stdio.h>


int main(void){
  rs_gf256 gf;
  int      a;
  int      b;
  int      c;
  int      d;
  rs_gf256_init(&gf,QR_PPOLY);
  for(a=0;a<256;a++)for(b=0;b<256;b++)for(c=0;c<256;c++)for(d=0;d<256;d++){
    unsigned char x[4];
    unsigned char r[4];
    unsigned      x2;
    unsigned      e[5];
    int           nroots;
    int           mroots;
    int           i;
    int           j;
    nroots=rs_quartic_solve(&gf,a,b,c,d,x);
    for(i=0;i<nroots;i++){
      x2=rs_gmul(&gf,x[i],x[i]);
      e[0]=rs_gmul(&gf,x2,x2)^rs_gmul(&gf,a,rs_gmul(&gf,x[i],x2))^
       rs_gmul(&gf,b,x2)^rs_gmul(&gf,c,x[i])^d;
      if(e[0]){
        printf("Invalid root: (0x%02X)**4 ^ 0x%02X*(0x%02X)**3 ^ "
         "0x%02X*(0x%02X)**2 ^ 0x%02X(0x%02X) ^ 0x%02X = 0x%02X\n",
         x[i],a,x[i],b,x[i],c,x[i],d,e[0]);
      }
      for(j=0;j<i;j++)if(x[i]==x[j]){
        printf("Repeated root %i=%i: (0x%02X)**4 ^ 0x%02X*(0x%02X)**3 ^ "
         "0x%02X*(0x%02X)**2 ^ 0x%02X(0x%02X) ^ 0x%02X = 0x%02X\n",
         i,j,x[i],a,x[i],b,x[i],c,x[i],d,e[0]);
      }
    }
    mroots=0;
    for(j=1;j<256;j++){
      int logx;
      int logx2;
      logx=gf.log[j];
      logx2=gf.log[gf.exp[logx<<1]];
      e[mroots]=d^rs_hgmul(&gf,c,logx)^rs_hgmul(&gf,b,logx2)^
       rs_hgmul(&gf,a,gf.log[gf.exp[logx+logx2]])^gf.exp[logx2<<1];
      if(!e[mroots])r[mroots++]=j;
    }
    
    if(mroots==4)for(j=0;j<mroots;j++){
      for(i=0;i<nroots;i++)if(x[i]==r[j])break;
      if(i>=nroots){
        printf("Missing root: (0x%02X)**4 ^ 0x%02X*(0x%02X)**3 ^ "
         "0x%02X*(0x%02X)**2 ^ 0x%02X(0x%02X) ^ 0x%02X = 0x%02X\n",
         r[j],a,r[j],b,r[j],c,r[j],d,e[j]);
      }
    }
  }
  return 0;
}
#endif
