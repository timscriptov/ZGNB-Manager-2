
#include <config.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include "qrcode.h"
#include "qrdec.h"
#include "bch15_5.h"
#include "rs.h"
#include "isaac.h"
#include "util.h"
#include "binarize.h"
#include "image.h"
#include "error.h"
#include "svg.h"

typedef int qr_line[3];

typedef struct qr_finder_cluster qr_finder_cluster;
typedef struct qr_finder_edge_pt  qr_finder_edge_pt;
typedef struct qr_finder_center   qr_finder_center;

typedef struct qr_aff qr_aff;
typedef struct qr_hom qr_hom;

typedef struct qr_finder qr_finder;

typedef struct qr_hom_cell      qr_hom_cell;
typedef struct qr_sampling_grid qr_sampling_grid;
typedef struct qr_pack_buf      qr_pack_buf;


#define QR_INT_BITS    ((int)sizeof(int)*CHAR_BIT)
#define QR_INT_LOGBITS (QR_ILOG(QR_INT_BITS))


#define QR_HOM_BITS (14)


#define QR_ALIGN_SUBPREC (2)



typedef struct qr_finder_lines {
    qr_finder_line *lines;
    int nlines, clines;
} qr_finder_lines;


struct qr_reader {
    
    rs_gf256  gf;
    
    isaac_ctx isaac;
    
    qr_finder_lines finder_lines[2];
};



static void qr_reader_init (qr_reader *reader)
{
    
    isaac_init(&reader->isaac, NULL, 0);
    rs_gf256_init(&reader->gf, QR_PPOLY);
}


qr_reader *_zbar_qr_create (void)
{
    qr_reader *reader = (qr_reader*)calloc(1, sizeof(*reader));
    qr_reader_init(reader);
    return(reader);
}


void _zbar_qr_destroy (qr_reader *reader)
{
    zprintf(1, "max finder lines = %dx%d\n",
            reader->finder_lines[0].clines,
            reader->finder_lines[1].clines);
    if(reader->finder_lines[0].lines)
        free(reader->finder_lines[0].lines);
    if(reader->finder_lines[1].lines)
        free(reader->finder_lines[1].lines);
    free(reader);
}


void _zbar_qr_reset (qr_reader *reader)
{
    reader->finder_lines[0].nlines = 0;
    reader->finder_lines[1].nlines = 0;
}



struct qr_finder_cluster{
  
  qr_finder_line **lines;
  
  int              nlines;
};



struct qr_finder_edge_pt{
  
  qr_point pos;
  
  int      edge;
  
  int      extent;
};



struct qr_finder_center{
  
  qr_point           pos;
  
  qr_finder_edge_pt *edge_pts;
  
  int                nedge_pts;
};


static int qr_finder_vline_cmp(const void *_a,const void *_b){
  const qr_finder_line *a;
  const qr_finder_line *b;
  a=(const qr_finder_line *)_a;
  b=(const qr_finder_line *)_b;
  return ((a->pos[0]>b->pos[0])-(a->pos[0]<b->pos[0])<<1)+
   (a->pos[1]>b->pos[1])-(a->pos[1]<b->pos[1]);
}


static int qr_finder_cluster_lines(qr_finder_cluster *_clusters,
 qr_finder_line **_neighbors,qr_finder_line *_lines,int _nlines,int _v){
  unsigned char   *mark;
  qr_finder_line **neighbors;
  int              nneighbors;
  int              nclusters;
  int              i;
  
  mark=(unsigned char *)calloc(_nlines,sizeof(*mark));
  neighbors=_neighbors;
  nclusters=0;
  for(i=0;i<_nlines-1;i++)if(!mark[i]){
    int len;
    int j;
    nneighbors=1;
    neighbors[0]=_lines+i;
    len=_lines[i].len;
    for(j=i+1;j<_nlines;j++)if(!mark[j]){
      const qr_finder_line *a;
      const qr_finder_line *b;
      int                   thresh;
      a=neighbors[nneighbors-1];
      b=_lines+j;
      
      thresh=a->len+7>>2;
      if(abs(a->pos[1-_v]-b->pos[1-_v])>thresh)break;
      if(abs(a->pos[_v]-b->pos[_v])>thresh)continue;
      if(abs(a->pos[_v]+a->len-b->pos[_v]-b->len)>thresh)continue;
      if(a->boffs>0&&b->boffs>0&&
       abs(a->pos[_v]-a->boffs-b->pos[_v]+b->boffs)>thresh){
        continue;
      }
      if(a->eoffs>0&&b->eoffs>0&&
       abs(a->pos[_v]+a->len+a->eoffs-b->pos[_v]-b->len-b->eoffs)>thresh){
        continue;
      }
      neighbors[nneighbors++]=_lines+j;
      len+=b->len;
    }
    
    if(nneighbors<3)continue;
    
    len=((len<<1)+nneighbors)/(nneighbors<<1);
    if(nneighbors*(5<<QR_FINDER_SUBPREC)>=len){
      _clusters[nclusters].lines=neighbors;
      _clusters[nclusters].nlines=nneighbors;
      for(j=0;j<nneighbors;j++)mark[neighbors[j]-_lines]=1;
      neighbors+=nneighbors;
      nclusters++;
    }
  }
  free(mark);
  return nclusters;
}


static int qr_finder_edge_pts_fill(qr_finder_edge_pt *_edge_pts,int _nedge_pts,
 qr_finder_cluster **_neighbors,int _nneighbors,int _v){
  int i;
  for(i=0;i<_nneighbors;i++){
    qr_finder_cluster *c;
    int                j;
    c=_neighbors[i];
    for(j=0;j<c->nlines;j++){
      qr_finder_line *l;
      l=c->lines[j];
      if(l->boffs>0){
        _edge_pts[_nedge_pts].pos[0]=l->pos[0];
        _edge_pts[_nedge_pts].pos[1]=l->pos[1];
        _edge_pts[_nedge_pts].pos[_v]-=l->boffs;
        _nedge_pts++;
      }
      if(l->eoffs>0){
        _edge_pts[_nedge_pts].pos[0]=l->pos[0];
        _edge_pts[_nedge_pts].pos[1]=l->pos[1];
        _edge_pts[_nedge_pts].pos[_v]+=l->len+l->eoffs;
        _nedge_pts++;
      }
    }
  }
  return _nedge_pts;
}

static int qr_finder_center_cmp(const void *_a,const void *_b){
  const qr_finder_center *a;
  const qr_finder_center *b;
  a=(const qr_finder_center *)_a;
  b=(const qr_finder_center *)_b;
  return ((b->nedge_pts>a->nedge_pts)-(b->nedge_pts<a->nedge_pts)<<2)+
   ((a->pos[1]>b->pos[1])-(a->pos[1]<b->pos[1])<<1)+
   (a->pos[0]>b->pos[0])-(a->pos[0]<b->pos[0]);
}


static int qr_finder_lines_are_crossing(const qr_finder_line *_hline,
 const qr_finder_line *_vline){
  return
   _hline->pos[0]<=_vline->pos[0]&&_vline->pos[0]<_hline->pos[0]+_hline->len&&
   _vline->pos[1]<=_hline->pos[1]&&_hline->pos[1]<_vline->pos[1]+_vline->len;
}


static int qr_finder_find_crossings(qr_finder_center *_centers,
 qr_finder_edge_pt *_edge_pts,qr_finder_cluster *_hclusters,int _nhclusters,
 qr_finder_cluster *_vclusters,int _nvclusters){
  qr_finder_cluster **hneighbors;
  qr_finder_cluster **vneighbors;
  unsigned char      *hmark;
  unsigned char      *vmark;
  int                 ncenters;
  int                 i;
  int                 j;
  hneighbors=(qr_finder_cluster **)malloc(_nhclusters*sizeof(*hneighbors));
  vneighbors=(qr_finder_cluster **)malloc(_nvclusters*sizeof(*vneighbors));
  hmark=(unsigned char *)calloc(_nhclusters,sizeof(*hmark));
  vmark=(unsigned char *)calloc(_nvclusters,sizeof(*vmark));
  ncenters=0;
  
  for(i=0;i<_nhclusters;i++)if(!hmark[i]){
    qr_finder_line *a;
    qr_finder_line *b;
    int             nvneighbors;
    int             nedge_pts;
    int             y;
    a=_hclusters[i].lines[_hclusters[i].nlines>>1];
    y=nvneighbors=0;
    for(j=0;j<_nvclusters;j++)if(!vmark[j]){
      b=_vclusters[j].lines[_vclusters[j].nlines>>1];
      if(qr_finder_lines_are_crossing(a,b)){
        vmark[j]=1;
        y+=(b->pos[1]<<1)+b->len;
        if(b->boffs>0&&b->eoffs>0)y+=b->eoffs-b->boffs;
        vneighbors[nvneighbors++]=_vclusters+j;
      }
    }
    if(nvneighbors>0){
      qr_finder_center *c;
      int               nhneighbors;
      int               x;
      x=(a->pos[0]<<1)+a->len;
      if(a->boffs>0&&a->eoffs>0)x+=a->eoffs-a->boffs;
      hneighbors[0]=_hclusters+i;
      nhneighbors=1;
      j=nvneighbors>>1;
      b=vneighbors[j]->lines[vneighbors[j]->nlines>>1];
      for(j=i+1;j<_nhclusters;j++)if(!hmark[j]){
        a=_hclusters[j].lines[_hclusters[j].nlines>>1];
        if(qr_finder_lines_are_crossing(a,b)){
          hmark[j]=1;
          x+=(a->pos[0]<<1)+a->len;
          if(a->boffs>0&&a->eoffs>0)x+=a->eoffs-a->boffs;
          hneighbors[nhneighbors++]=_hclusters+j;
        }
      }
      c=_centers+ncenters++;
      c->pos[0]=(x+nhneighbors)/(nhneighbors<<1);
      c->pos[1]=(y+nvneighbors)/(nvneighbors<<1);
      c->edge_pts=_edge_pts;
      nedge_pts=qr_finder_edge_pts_fill(_edge_pts,0,
       hneighbors,nhneighbors,0);
      nedge_pts=qr_finder_edge_pts_fill(_edge_pts,nedge_pts,
       vneighbors,nvneighbors,1);
      c->nedge_pts=nedge_pts;
      _edge_pts+=nedge_pts;
    }
  }
  free(vmark);
  free(hmark);
  free(vneighbors);
  free(hneighbors);
  
  qsort(_centers,ncenters,sizeof(*_centers),qr_finder_center_cmp);
  return ncenters;
}


static int qr_finder_centers_locate(qr_finder_center **_centers,
 qr_finder_edge_pt **_edge_pts, qr_reader *reader,
 int _width,int _height){
  qr_finder_line     *hlines = reader->finder_lines[0].lines;
  int                 nhlines = reader->finder_lines[0].nlines;
  qr_finder_line     *vlines = reader->finder_lines[1].lines;
  int                 nvlines = reader->finder_lines[1].nlines;

  qr_finder_line    **hneighbors;
  qr_finder_cluster  *hclusters;
  int                 nhclusters;
  qr_finder_line    **vneighbors;
  qr_finder_cluster  *vclusters;
  int                 nvclusters;
  int                 ncenters;

  
  hneighbors=(qr_finder_line **)malloc(nhlines*sizeof(*hneighbors));
  
  hclusters=(qr_finder_cluster *)malloc((nhlines>>1)*sizeof(*hclusters));
  nhclusters=qr_finder_cluster_lines(hclusters,hneighbors,hlines,nhlines,0);
  
  qsort(vlines,nvlines,sizeof(*vlines),qr_finder_vline_cmp);
  vneighbors=(qr_finder_line **)malloc(nvlines*sizeof(*vneighbors));
  
  vclusters=(qr_finder_cluster *)malloc((nvlines>>1)*sizeof(*vclusters));
  nvclusters=qr_finder_cluster_lines(vclusters,vneighbors,vlines,nvlines,1);
  
  if(nhclusters>=3&&nvclusters>=3){
    qr_finder_edge_pt  *edge_pts;
    qr_finder_center   *centers;
    int                 nedge_pts;
    int                 i;
    nedge_pts=0;
    for(i=0;i<nhclusters;i++)nedge_pts+=hclusters[i].nlines;
    for(i=0;i<nvclusters;i++)nedge_pts+=vclusters[i].nlines;
    nedge_pts<<=1;
    edge_pts=(qr_finder_edge_pt *)malloc(nedge_pts*sizeof(*edge_pts));
    centers=(qr_finder_center *)malloc(
     QR_MINI(nhclusters,nvclusters)*sizeof(*centers));
    ncenters=qr_finder_find_crossings(centers,edge_pts,
     hclusters,nhclusters,vclusters,nvclusters);
    *_centers=centers;
    *_edge_pts=edge_pts;
  }
  else ncenters=0;
  free(vclusters);
  free(vneighbors);
  free(hclusters);
  free(hneighbors);
  return ncenters;
}



static void qr_point_translate(qr_point _point,int _dx,int _dy){
  _point[0]+=_dx;
  _point[1]+=_dy;
}

static unsigned qr_point_distance2(const qr_point _p1,const qr_point _p2){
  return (_p1[0]-_p2[0])*(_p1[0]-_p2[0])+(_p1[1]-_p2[1])*(_p1[1]-_p2[1]);
}


static int qr_point_ccw(const qr_point _p0,
 const qr_point _p1,const qr_point _p2){
  return (_p1[0]-_p0[0])*(_p2[1]-_p0[1])-(_p1[1]-_p0[1])*(_p2[0]-_p0[0]);
}




static int qr_line_eval(qr_line _line,int _x,int _y){
  return _line[0]*_x+_line[1]*_y+_line[2];
}


static void qr_line_fit(qr_line _l,int _x0,int _y0,
 int _sxx,int _sxy,int _syy,int _res){
  int dshift;
  int dround;
  int u;
  int v;
  int w;
  u=abs(_sxx-_syy);
  v=-_sxy<<1;
  w=qr_ihypot(u,v);
  
  dshift=QR_MAXI(0,QR_MAXI(qr_ilog(u),qr_ilog(abs(v)))+1-(_res+1>>1));
  dround=(1<<dshift)>>1;
  if(_sxx>_syy){
    _l[0]=v+dround>>dshift;
    _l[1]=u+w+dround>>dshift;
  }
  else{
    _l[0]=u+w+dround>>dshift;
    _l[1]=v+dround>>dshift;
  }
  _l[2]=-(_x0*_l[0]+_y0*_l[1]);
}


static void qr_line_fit_points(qr_line _l,qr_point *_p,int _np,int _res){
  int sx;
  int sy;
  int xmin;
  int xmax;
  int ymin;
  int ymax;
  int xbar;
  int ybar;
  int dx;
  int dy;
  int sxx;
  int sxy;
  int syy;
  int sshift;
  int sround;
  int i;
  sx=sy=0;
  ymax=xmax=INT_MIN;
  ymin=xmin=INT_MAX;
  for(i=0;i<_np;i++){
    sx+=_p[i][0];
    xmin=QR_MINI(xmin,_p[i][0]);
    xmax=QR_MAXI(xmax,_p[i][0]);
    sy+=_p[i][1];
    ymin=QR_MINI(ymin,_p[i][1]);
    ymax=QR_MAXI(ymax,_p[i][1]);
  }
  xbar=(sx+(_np>>1))/_np;
  ybar=(sy+(_np>>1))/_np;
  sshift=QR_MAXI(0,qr_ilog(_np*QR_MAXI(QR_MAXI(xmax-xbar,xbar-xmin),
   QR_MAXI(ymax-ybar,ybar-ymin)))-(QR_INT_BITS-1>>1));
  sround=(1<<sshift)>>1;
  sxx=sxy=syy=0;
  for(i=0;i<_np;i++){
    dx=_p[i][0]-xbar+sround>>sshift;
    dy=_p[i][1]-ybar+sround>>sshift;
    sxx+=dx*dx;
    sxy+=dx*dy;
    syy+=dy*dy;
  }
  qr_line_fit(_l,xbar,ybar,sxx,sxy,syy,_res);
}

static void qr_line_orient(qr_line _l,int _x,int _y){
  if(qr_line_eval(_l,_x,_y)<0){
    _l[0]=-_l[0];
    _l[1]=-_l[1];
    _l[2]=-_l[2];
  }
}

static int qr_line_isect(qr_point _p,const qr_line _l0,const qr_line _l1){
  int d;
  int x;
  int y;
  d=_l0[0]*_l1[1]-_l0[1]*_l1[0];
  if(d==0)return -1;
  x=_l0[1]*_l1[2]-_l1[1]*_l0[2];
  y=_l1[0]*_l0[2]-_l0[0]*_l1[2];
  if(d<0){
    x=-x;
    y=-y;
    d=-d;
  }
  _p[0]=QR_DIVROUND(x,d);
  _p[1]=QR_DIVROUND(y,d);
  return 0;
}




struct qr_aff{
  int fwd[2][2];
  int inv[2][2];
  int x0;
  int y0;
  int res;
  int ires;
};


static void qr_aff_init(qr_aff *_aff,
 const qr_point _p0,const qr_point _p1,const qr_point _p2,int _res){
  int det;
  int ires;
  int dx1;
  int dy1;
  int dx2;
  int dy2;
  
  dx1=_p1[0]-_p0[0];
  dx2=_p2[0]-_p0[0];
  dy1=_p1[1]-_p0[1];
  dy2=_p2[1]-_p0[1];
  det=dx1*dy2-dy1*dx2;
  ires=QR_MAXI((qr_ilog(abs(det))>>1)-2,0);
  _aff->fwd[0][0]=dx1;
  _aff->fwd[0][1]=dx2;
  _aff->fwd[1][0]=dy1;
  _aff->fwd[1][1]=dy2;
  _aff->inv[0][0]=QR_DIVROUND(dy2<<_res,det>>ires);
  _aff->inv[0][1]=QR_DIVROUND(-dx2<<_res,det>>ires);
  _aff->inv[1][0]=QR_DIVROUND(-dy1<<_res,det>>ires);
  _aff->inv[1][1]=QR_DIVROUND(dx1<<_res,det>>ires);
  _aff->x0=_p0[0];
  _aff->y0=_p0[1];
  _aff->res=_res;
  _aff->ires=ires;
}


static void qr_aff_unproject(qr_point _q,const qr_aff *_aff,
 int _x,int _y){
  _q[0]=_aff->inv[0][0]*(_x-_aff->x0)+_aff->inv[0][1]*(_y-_aff->y0)
   +(1<<_aff->ires>>1)>>_aff->ires;
  _q[1]=_aff->inv[1][0]*(_x-_aff->x0)+_aff->inv[1][1]*(_y-_aff->y0)
   +(1<<_aff->ires>>1)>>_aff->ires;
}


static void qr_aff_project(qr_point _p,const qr_aff *_aff,
 int _u,int _v){
  _p[0]=(_aff->fwd[0][0]*_u+_aff->fwd[0][1]*_v+(1<<_aff->res-1)>>_aff->res)
   +_aff->x0;
  _p[1]=(_aff->fwd[1][0]*_u+_aff->fwd[1][1]*_v+(1<<_aff->res-1)>>_aff->res)
   +_aff->y0;
}




struct qr_hom{
  int fwd[3][2];
  int inv[3][2];
  int fwd22;
  int inv22;
  int x0;
  int y0;
  int res;
};


static void qr_hom_init(qr_hom *_hom,int _x0,int _y0,
 int _x1,int _y1,int _x2,int _y2,int _x3,int _y3,int _res){
  int dx10;
  int dx20;
  int dx30;
  int dx31;
  int dx32;
  int dy10;
  int dy20;
  int dy30;
  int dy31;
  int dy32;
  int a20;
  int a21;
  int a22;
  int b0;
  int b1;
  int b2;
  int s1;
  int s2;
  int r1;
  int r2;
  dx10=_x1-_x0;
  dx20=_x2-_x0;
  dx30=_x3-_x0;
  dx31=_x3-_x1;
  dx32=_x3-_x2;
  dy10=_y1-_y0;
  dy20=_y2-_y0;
  dy30=_y3-_y0;
  dy31=_y3-_y1;
  dy32=_y3-_y2;
  a20=dx32*dy10-dx10*dy32;
  a21=dx20*dy31-dx31*dy20;
  a22=dx32*dy31-dx31*dy32;
  
  b0=qr_ilog(QR_MAXI(abs(dx10),abs(dy10)))+qr_ilog(abs(a20+a22));
  b1=qr_ilog(QR_MAXI(abs(dx20),abs(dy20)))+qr_ilog(abs(a21+a22));
  b2=qr_ilog(QR_MAXI(QR_MAXI(abs(a20),abs(a21)),abs(a22)));
  s1=QR_MAXI(0,_res+QR_MAXI(QR_MAXI(b0,b1),b2)-(QR_INT_BITS-2));
  r1=(1<<s1)>>1;
  
  _hom->fwd[0][0]=QR_FIXMUL(dx10,a20+a22,r1,s1);
  _hom->fwd[0][1]=QR_FIXMUL(dx20,a21+a22,r1,s1);
  _hom->x0=_x0;
  _hom->fwd[1][0]=QR_FIXMUL(dy10,a20+a22,r1,s1);
  _hom->fwd[1][1]=QR_FIXMUL(dy20,a21+a22,r1,s1);
  _hom->y0=_y0;
  _hom->fwd[2][0]=a20+r1>>s1;
  _hom->fwd[2][1]=a21+r1>>s1;
  _hom->fwd22=s1>_res?a22+(r1>>_res)>>s1-_res:a22<<_res-s1;
  
  b0=qr_ilog(QR_MAXI(QR_MAXI(abs(dx10),abs(dx20)),abs(dx30)))+
   qr_ilog(QR_MAXI(abs(_hom->fwd[0][0]),abs(_hom->fwd[1][0])));
  b1=qr_ilog(QR_MAXI(QR_MAXI(abs(dy10),abs(dy20)),abs(dy30)))+
   qr_ilog(QR_MAXI(abs(_hom->fwd[0][1]),abs(_hom->fwd[1][1])));
  b2=qr_ilog(abs(a22))-s1;
  s2=QR_MAXI(0,QR_MAXI(b0,b1)+b2-(QR_INT_BITS-3));
  r2=(1<<s2)>>1;
  s1+=s2;
  r1<<=s2;
  
  _hom->inv[0][0]=QR_FIXMUL(_hom->fwd[1][1],a22,r1,s1);
  _hom->inv[0][1]=QR_FIXMUL(-_hom->fwd[0][1],a22,r1,s1);
  _hom->inv[1][0]=QR_FIXMUL(-_hom->fwd[1][0],a22,r1,s1);
  _hom->inv[1][1]=QR_FIXMUL(_hom->fwd[0][0],a22,r1,s1);
  _hom->inv[2][0]=QR_FIXMUL(_hom->fwd[1][0],_hom->fwd[2][1],
   -QR_EXTMUL(_hom->fwd[1][1],_hom->fwd[2][0],r2),s2);
  _hom->inv[2][1]=QR_FIXMUL(_hom->fwd[0][1],_hom->fwd[2][0],
   -QR_EXTMUL(_hom->fwd[0][0],_hom->fwd[2][1],r2),s2);
  _hom->inv22=QR_FIXMUL(_hom->fwd[0][0],_hom->fwd[1][1],
   -QR_EXTMUL(_hom->fwd[0][1],_hom->fwd[1][0],r2),s2);
  _hom->res=_res;
}



static int qr_hom_unproject(qr_point _q,const qr_hom *_hom,int _x,int _y){
  int x;
  int y;
  int w;
  _x-=_hom->x0;
  _y-=_hom->y0;
  x=_hom->inv[0][0]*_x+_hom->inv[0][1]*_y;
  y=_hom->inv[1][0]*_x+_hom->inv[1][1]*_y;
  w=_hom->inv[2][0]*_x+_hom->inv[2][1]*_y
   +_hom->inv22+(1<<_hom->res-1)>>_hom->res;
  if(w==0){
    _q[0]=x<0?INT_MIN:INT_MAX;
    _q[1]=y<0?INT_MIN:INT_MAX;
    return -1;
  }
  else{
    if(w<0){
      x=-x;
      y=-y;
      w=-w;
    }
    _q[0]=QR_DIVROUND(x,w);
    _q[1]=QR_DIVROUND(y,w);
  }
  return 0;
}


static void qr_hom_fproject(qr_point _p,const qr_hom *_hom,
 int _x,int _y,int _w){
  if(_w==0){
    _p[0]=_x<0?INT_MIN:INT_MAX;
    _p[1]=_y<0?INT_MIN:INT_MAX;
  }
  else{
    if(_w<0){
      _x=-_x;
      _y=-_y;
      _w=-_w;
    }
    _p[0]=QR_DIVROUND(_x,_w)+_hom->x0;
    _p[1]=QR_DIVROUND(_y,_w)+_hom->y0;
  }
}

#if defined(QR_DEBUG)

static void qr_hom_project(qr_point _p,const qr_hom *_hom,
 int _u,int _v){
  qr_hom_fproject(_p,_hom,
   _hom->fwd[0][0]*_u+_hom->fwd[0][1]*_v,
   _hom->fwd[1][0]*_u+_hom->fwd[1][1]*_v,
   _hom->fwd[2][0]*_u+_hom->fwd[2][1]*_v+_hom->fwd22);
}
#endif




struct qr_finder{
  
  int                size[2];
  
  int                eversion[2];
  
  qr_finder_edge_pt *edge_pts[4];
  
  int                nedge_pts[4];
  
  int                ninliers[4];
  
  qr_point           o;
  
  qr_finder_center  *c;
};


static int qr_cmp_edge_pt(const void *_a,const void *_b){
  const qr_finder_edge_pt *a;
  const qr_finder_edge_pt *b;
  a=(const qr_finder_edge_pt *)_a;
  b=(const qr_finder_edge_pt *)_b;
  return ((a->edge>b->edge)-(a->edge<b->edge)<<1)+
   (a->extent>b->extent)-(a->extent<b->extent);
}


static void qr_finder_edge_pts_aff_classify(qr_finder *_f,const qr_aff *_aff){
  qr_finder_center *c;
  int               i;
  int               e;
  c=_f->c;
  for(e=0;e<4;e++)_f->nedge_pts[e]=0;
  for(i=0;i<c->nedge_pts;i++){
    qr_point q;
    int      d;
    qr_aff_unproject(q,_aff,c->edge_pts[i].pos[0],c->edge_pts[i].pos[1]);
    qr_point_translate(q,-_f->o[0],-_f->o[1]);
    d=abs(q[1])>abs(q[0]);
    e=d<<1|(q[d]>=0);
    _f->nedge_pts[e]++;
    c->edge_pts[i].edge=e;
    c->edge_pts[i].extent=q[d];
  }
  qsort(c->edge_pts,c->nedge_pts,sizeof(*c->edge_pts),qr_cmp_edge_pt);
  _f->edge_pts[0]=c->edge_pts;
  for(e=1;e<4;e++)_f->edge_pts[e]=_f->edge_pts[e-1]+_f->nedge_pts[e-1];
}


static void qr_finder_edge_pts_hom_classify(qr_finder *_f,const qr_hom *_hom){
  qr_finder_center *c;
  int               i;
  int               e;
  c=_f->c;
  for(e=0;e<4;e++)_f->nedge_pts[e]=0;
  for(i=0;i<c->nedge_pts;i++){
    qr_point q;
    int      d;
    if(qr_hom_unproject(q,_hom,
     c->edge_pts[i].pos[0],c->edge_pts[i].pos[1])>=0){
      qr_point_translate(q,-_f->o[0],-_f->o[1]);
      d=abs(q[1])>abs(q[0]);
      e=d<<1|(q[d]>=0);
      _f->nedge_pts[e]++;
      c->edge_pts[i].edge=e;
      c->edge_pts[i].extent=q[d];
    }
    else{
      c->edge_pts[i].edge=4;
      c->edge_pts[i].extent=q[0];
    }
  }
  qsort(c->edge_pts,c->nedge_pts,sizeof(*c->edge_pts),qr_cmp_edge_pt);
  _f->edge_pts[0]=c->edge_pts;
  for(e=1;e<4;e++)_f->edge_pts[e]=_f->edge_pts[e-1]+_f->nedge_pts[e-1];
}




#define QR_SMALL_VERSION_SLACK (1)

#define QR_LARGE_VERSION_SLACK (3)


static int qr_finder_estimate_module_size_and_version(qr_finder *_f,
 int _width,int _height){
  qr_point offs;
  int      sums[4];
  int      nsums[4];
  int      usize;
  int      nusize;
  int      vsize;
  int      nvsize;
  int      uversion;
  int      vversion;
  int      e;
  offs[0]=offs[1]=0;
  for(e=0;e<4;e++)if(_f->nedge_pts[e]>0){
    qr_finder_edge_pt *edge_pts;
    int                sum;
    int                mean;
    int                n;
    int                i;
    
    edge_pts=_f->edge_pts[e];
    n=_f->nedge_pts[e];
    sum=0;
    for(i=(n>>2);i<n-(n>>2);i++)sum+=edge_pts[i].extent;
    n=n-((n>>2)<<1);
    mean=QR_DIVROUND(sum,n);
    offs[e>>1]+=mean;
    sums[e]=sum;
    nsums[e]=n;
  }
  else nsums[e]=sums[e]=0;
  
  if(_f->nedge_pts[0]>0&&_f->nedge_pts[1]>0){
    _f->o[0]-=offs[0]>>1;
    sums[0]-=offs[0]*nsums[0]>>1;
    sums[1]-=offs[0]*nsums[1]>>1;
  }
  if(_f->nedge_pts[2]>0&&_f->nedge_pts[3]>0){
    _f->o[1]-=offs[1]>>1;
    sums[2]-=offs[1]*nsums[2]>>1;
    sums[3]-=offs[1]*nsums[3]>>1;
  }
  
  nusize=nsums[0]+nsums[1];
  if(nusize<=0)return -1;
  
  nusize*=3;
  usize=sums[1]-sums[0];
  usize=((usize<<1)+nusize)/(nusize<<1);
  if(usize<=0)return -1;
  
  uversion=(_width-8*usize)/(usize<<2);
  if(uversion<1||uversion>40+QR_LARGE_VERSION_SLACK)return -1;
  
  nvsize=nsums[2]+nsums[3];
  if(nvsize<=0)return -1;
  nvsize*=3;
  vsize=sums[3]-sums[2];
  vsize=((vsize<<1)+nvsize)/(nvsize<<1);
  if(vsize<=0)return -1;
  vversion=(_height-8*vsize)/(vsize<<2);
  if(vversion<1||vversion>40+QR_LARGE_VERSION_SLACK)return -1;
  
  if(abs(uversion-vversion)>QR_LARGE_VERSION_SLACK)return -1;
  _f->size[0]=usize;
  _f->size[1]=vsize;
  
  _f->eversion[0]=uversion;
  _f->eversion[1]=vversion;
  return 0;
}


static void qr_finder_ransac(qr_finder *_f,const qr_aff *_hom,
 isaac_ctx *_isaac,int _e){
  qr_finder_edge_pt *edge_pts;
  int                best_ninliers;
  int                n;
  edge_pts=_f->edge_pts[_e];
  n=_f->nedge_pts[_e];
  best_ninliers=0;
  if(n>1){
    int max_iters;
    int i;
    int j;
    
    max_iters=17;
    for(i=0;i<max_iters;i++){
      qr_point  q0;
      qr_point  q1;
      int       ninliers;
      int       thresh;
      int       p0i;
      int       p1i;
      int      *p0;
      int      *p1;
      int       j;
      
      p0i=isaac_next_uint(_isaac,n);
      p1i=isaac_next_uint(_isaac,n-1);
      if(p1i>=p0i)p1i++;
      p0=edge_pts[p0i].pos;
      p1=edge_pts[p1i].pos;
      
      qr_aff_unproject(q0,_hom,p0[0],p0[1]);
      qr_aff_unproject(q1,_hom,p1[0],p1[1]);
      qr_point_translate(q0,-_f->o[0],-_f->o[1]);
      qr_point_translate(q1,-_f->o[0],-_f->o[1]);
      if(abs(q0[_e>>1]-q1[_e>>1])>abs(q0[1-(_e>>1)]-q1[1-(_e>>1)]))continue;
      
      thresh=qr_isqrt(qr_point_distance2(p0,p1)<<2*QR_FINDER_SUBPREC+1);
      ninliers=0;
      for(j=0;j<n;j++){
        if(abs(qr_point_ccw(p0,p1,edge_pts[j].pos))<=thresh){
          edge_pts[j].extent|=1;
          ninliers++;
        }
        else edge_pts[j].extent&=~1;
      }
      if(ninliers>best_ninliers){
        for(j=0;j<n;j++)edge_pts[j].extent<<=1;
        best_ninliers=ninliers;
        
        if(ninliers>n>>1)max_iters=(67*n-63*ninliers-1)/(n<<1);
      }
    }
    
    for(i=j=0;j<best_ninliers;i++)if(edge_pts[i].extent&2){
      if(j<i){
        qr_finder_edge_pt tmp;
        *&tmp=*(edge_pts+i);
        *(edge_pts+j)=*(edge_pts+i);
        *(edge_pts+i)=*&tmp;
      }
      j++;
    }
  }
  _f->ninliers[_e]=best_ninliers;
}


static int qr_line_fit_finder_edge(qr_line _l,
 const qr_finder *_f,int _e,int _res){
  qr_finder_edge_pt *edge_pts;
  qr_point          *pts;
  int                npts;
  int                i;
  npts=_f->ninliers[_e];
  if(npts<2)return -1;
  
  pts=(qr_point *)malloc(npts*sizeof(*pts));
  edge_pts=_f->edge_pts[_e];
  for(i=0;i<npts;i++){
    pts[i][0]=edge_pts[i].pos[0];
    pts[i][1]=edge_pts[i].pos[1];
  }
  qr_line_fit_points(_l,pts,npts,_res);
  
  qr_line_orient(_l,_f->c->pos[0],_f->c->pos[1]);
  free(pts);
  return 0;
}


static void qr_line_fit_finder_pair(qr_line _l,const qr_aff *_aff,
 const qr_finder *_f0,const qr_finder *_f1,int _e){
  qr_point          *pts;
  int                npts;
  qr_finder_edge_pt *edge_pts;
  qr_point           q;
  int                n0;
  int                n1;
  int                i;
  n0=_f0->ninliers[_e];
  n1=_f1->ninliers[_e];
  
  npts=QR_MAXI(n0,1)+QR_MAXI(n1,1);
  pts=(qr_point *)malloc(npts*sizeof(*pts));
  if(n0>0){
    edge_pts=_f0->edge_pts[_e];
    for(i=0;i<n0;i++){
      pts[i][0]=edge_pts[i].pos[0];
      pts[i][1]=edge_pts[i].pos[1];
    }
  }
  else{
    q[0]=_f0->o[0];
    q[1]=_f0->o[1];
    q[_e>>1]+=_f0->size[_e>>1]*(2*(_e&1)-1);
    qr_aff_project(pts[0],_aff,q[0],q[1]);
    n0++;
  }
  if(n1>0){
    edge_pts=_f1->edge_pts[_e];
    for(i=0;i<n1;i++){
      pts[n0+i][0]=edge_pts[i].pos[0];
      pts[n0+i][1]=edge_pts[i].pos[1];
    }
  }
  else{
    q[0]=_f1->o[0];
    q[1]=_f1->o[1];
    q[_e>>1]+=_f1->size[_e>>1]*(2*(_e&1)-1);
    qr_aff_project(pts[n0],_aff,q[0],q[1]);
    n1++;
  }
  qr_line_fit_points(_l,pts,npts,_aff->res);
  
  qr_line_orient(_l,_f0->c->pos[0],_f0->c->pos[1]);
  free(pts);
}

static int qr_finder_quick_crossing_check(const unsigned char *_img,
 int _width,int _height,int _x0,int _y0,int _x1,int _y1,int _v){
  
  if(_x0<0||_x0>=_width||_y0<0||_y0>=_height||
   _x1<0||_x1>=_width||_y1<0||_y1>=_height){
    return -1;
  }
  if(!_img[_y0*_width+_x0]!=_v||!_img[_y1*_width+_x1]!=_v)return 1;
  if(!_img[(_y0+_y1>>1)*_width+(_x0+_x1>>1)]==_v)return -1;
  return 0;
}


static int qr_finder_locate_crossing(const unsigned char *_img,
 int _width,int _height,int _x0,int _y0,int _x1,int _y1,int _v,qr_point _p){
  qr_point x0;
  qr_point x1;
  qr_point dx;
  int      step[2];
  int      steep;
  int      err;
  int      derr;
  
  x0[0]=_x0;
  x0[1]=_y0;
  x1[0]=_x1;
  x1[1]=_y1;
  dx[0]=abs(_x1-_x0);
  dx[1]=abs(_y1-_y0);
  steep=dx[1]>dx[0];
  err=0;
  derr=dx[1-steep];
  step[0]=((_x0<_x1)<<1)-1;
  step[1]=((_y0<_y1)<<1)-1;
  
  for(;;){
    
    if(x0[steep]==x1[steep])return -1;
    x0[steep]+=step[steep];
    err+=derr;
    if(err<<1>dx[steep]){
      x0[1-steep]+=step[1-steep];
      err-=dx[steep];
    }
    if(!_img[x0[1]*_width+x0[0]]!=_v)break;
  }
  
  err=0;
  for(;;){
    if(x0[steep]==x1[steep])break;
    x1[steep]-=step[steep];
    err+=derr;
    if(err<<1>dx[steep]){
      x1[1-steep]-=step[1-steep];
      err-=dx[steep];
    }
    if(!_img[x1[1]*_width+x1[0]]!=_v)break;
  }
  
  _p[0]=(x0[0]+x1[0]+1<<QR_FINDER_SUBPREC)>>1;
  _p[1]=(x0[1]+x1[1]+1<<QR_FINDER_SUBPREC)>>1;
  return 0;
}

static int qr_aff_line_step(const qr_aff *_aff,qr_line _l,
 int _v,int _du,int *_dv){
  int shift;
  int round;
  int dv;
  int n;
  int d;
  n=_aff->fwd[0][_v]*_l[0]+_aff->fwd[1][_v]*_l[1];
  d=_aff->fwd[0][1-_v]*_l[0]+_aff->fwd[1][1-_v]*_l[1];
  if(d<0){
    n=-n;
    d=-d;
  }
  shift=QR_MAXI(0,qr_ilog(_du)+qr_ilog(abs(n))+3-QR_INT_BITS);
  round=(1<<shift)>>1;
  n=n+round>>shift;
  d=d+round>>shift;
  
  if(abs(n)>=d)return -1;
  n=-_du*n;
  dv=QR_DIVROUND(n,d);
  if(abs(dv)>=_du)return -1;
  *_dv=dv;
  return 0;
}


static int qr_hamming_dist(unsigned _y1,unsigned _y2,int _maxdiff){
  unsigned y;
  int      ret;
  y=_y1^_y2;
  for(ret=0;ret<_maxdiff&&y;ret++)y&=y-1;
  return ret;
}


static int qr_img_get_bit(const unsigned char *_img,int _width,int _height,
 int _x,int _y){
  _x>>=QR_FINDER_SUBPREC;
  _y>>=QR_FINDER_SUBPREC;
  return _img[QR_CLAMPI(0,_y,_height-1)*_width+QR_CLAMPI(0,_x,_width-1)]!=0;
}

#if defined(QR_DEBUG)
#include "image.h"

static void qr_finder_dump_aff_undistorted(qr_finder *_ul,qr_finder *_ur,
 qr_finder *_dl,qr_aff *_aff,const unsigned char *_img,int _width,int _height){
  unsigned char *gimg;
  FILE          *fout;
  int            lpsz;
  int            pixel_size;
  int            dim;
  int            min;
  int            max;
  int            u;
  int            y;
  int            i;
  int            j;
  lpsz=qr_ilog(_ur->size[0]+_ur->size[1]+_dl->size[0]+_dl->size[1])-6;
  pixel_size=1<<lpsz;
  dim=(1<<_aff->res-lpsz)+128;
  gimg=(unsigned char *)malloc(dim*dim*sizeof(*gimg));
  for(i=0;i<dim;i++)for(j=0;j<dim;j++){
    qr_point p;
    qr_aff_project(p,_aff,(j-64)<<lpsz,(i-64)<<lpsz);
    gimg[i*dim+j]=_img[
     QR_CLAMPI(0,p[1]>>QR_FINDER_SUBPREC,_height-1)*_width+
     QR_CLAMPI(0,p[0]>>QR_FINDER_SUBPREC,_width-1)];
  }
  {
    min=(_ur->o[0]-7*_ur->size[0]>>lpsz)+64;
    if(min<0)min=0;
    max=(_ur->o[0]+7*_ur->size[0]>>lpsz)+64;
    if(max>dim)max=dim;
    for(y=-7;y<=7;y++){
      i=(_ur->o[1]+y*_ur->size[1]>>lpsz)+64;
      if(i<0||i>=dim)continue;
      for(j=min;j<max;j++)gimg[i*dim+j]=0x7F;
    }
    min=(_ur->o[1]-7*_ur->size[1]>>lpsz)+64;
    if(min<0)min=0;
    max=(_ur->o[1]+7*_ur->size[1]>>lpsz)+64;
    if(max>dim)max=dim;
    for(u=-7;u<=7;u++){
      j=(_ur->o[0]+u*_ur->size[0]>>lpsz)+64;
      if(j<0||j>=dim)continue;
      for(i=min;i<max;i++)gimg[i*dim+j]=0x7F;
    }
  }
  {
    min=(_dl->o[0]-7*_dl->size[0]>>lpsz)+64;
    if(min<0)min=0;
    max=(_dl->o[0]+7*_dl->size[0]>>lpsz)+64;
    if(max>dim)max=dim;
    for(y=-7;y<=7;y++){
      i=(_dl->o[1]+y*_dl->size[1]>>lpsz)+64;
      if(i<0||i>=dim)continue;
      for(j=min;j<max;j++)gimg[i*dim+j]=0x7F;
    }
    min=(_dl->o[1]-7*_dl->size[1]>>lpsz)+64;
    if(min<0)min=0;
    max=(_dl->o[1]+7*_dl->size[1]>>lpsz)+64;
    if(max>dim)max=dim;
    for(u=-7;u<=7;u++){
      j=(_dl->o[0]+u*_dl->size[0]>>lpsz)+64;
      if(j<0||j>=dim)continue;
      for(i=min;i<max;i++)gimg[i*dim+j]=0x7F;
    }
  }
  fout=fopen("undistorted_aff.png","wb");
  image_write_png(gimg,dim,dim,fout);
  fclose(fout);
  free(gimg);
}

static void qr_finder_dump_hom_undistorted(qr_finder *_ul,qr_finder *_ur,
 qr_finder *_dl,qr_hom *_hom,const unsigned char *_img,int _width,int _height){
  unsigned char *gimg;
  FILE          *fout;
  int            lpsz;
  int            pixel_size;
  int            dim;
  int            min;
  int            max;
  int            u;
  int            v;
  int            i;
  int            j;
  lpsz=qr_ilog(_ur->size[0]+_ur->size[1]+_dl->size[0]+_dl->size[1])-6;
  pixel_size=1<<lpsz;
  dim=(1<<_hom->res-lpsz)+256;
  gimg=(unsigned char *)malloc(dim*dim*sizeof(*gimg));
  for(i=0;i<dim;i++)for(j=0;j<dim;j++){
    qr_point p;
    qr_hom_project(p,_hom,(j-128)<<lpsz,(i-128)<<lpsz);
    gimg[i*dim+j]=_img[
     QR_CLAMPI(0,p[1]>>QR_FINDER_SUBPREC,_height-1)*_width+
     QR_CLAMPI(0,p[0]>>QR_FINDER_SUBPREC,_width-1)];
  }
  {
    min=(_ur->o[0]-7*_ur->size[0]>>lpsz)+128;
    if(min<0)min=0;
    max=(_ur->o[0]+7*_ur->size[0]>>lpsz)+128;
    if(max>dim)max=dim;
    for(v=-7;v<=7;v++){
      i=(_ur->o[1]+v*_ur->size[1]>>lpsz)+128;
      if(i<0||i>=dim)continue;
      for(j=min;j<max;j++)gimg[i*dim+j]=0x7F;
    }
    min=(_ur->o[1]-7*_ur->size[1]>>lpsz)+128;
    if(min<0)min=0;
    max=(_ur->o[1]+7*_ur->size[1]>>lpsz)+128;
    if(max>dim)max=dim;
    for(u=-7;u<=7;u++){
      j=(_ur->o[0]+u*_ur->size[0]>>lpsz)+128;
      if(j<0||j>=dim)continue;
      for(i=min;i<max;i++)gimg[i*dim+j]=0x7F;
    }
  }
  {
    min=(_dl->o[0]-7*_dl->size[0]>>lpsz)+128;
    if(min<0)min=0;
    max=(_dl->o[0]+7*_dl->size[0]>>lpsz)+128;
    if(max>dim)max=dim;
    for(v=-7;v<=7;v++){
      i=(_dl->o[1]+v*_dl->size[1]>>lpsz)+128;
      if(i<0||i>=dim)continue;
      for(j=min;j<max;j++)gimg[i*dim+j]=0x7F;
    }
    min=(_dl->o[1]-7*_dl->size[1]>>lpsz)+128;
    if(min<0)min=0;
    max=(_dl->o[1]+7*_dl->size[1]>>lpsz)+128;
    if(max>dim)max=dim;
    for(u=-7;u<=7;u++){
      j=(_dl->o[0]+u*_dl->size[0]>>lpsz)+128;
      if(j<0||j>=dim)continue;
      for(i=min;i<max;i++)gimg[i*dim+j]=0x7F;
    }
  }
  fout=fopen("undistorted_hom.png","wb");
  image_write_png(gimg,dim,dim,fout);
  fclose(fout);
  free(gimg);
}
#endif




struct qr_hom_cell{
  int fwd[3][3];
  int x0;
  int y0;
  int u0;
  int v0;
};


static void qr_hom_cell_init(qr_hom_cell *_cell,int _u0,int _v0,
 int _u1,int _v1,int _u2,int _v2,int _u3,int _v3,int _x0,int _y0,
 int _x1,int _y1,int _x2,int _y2,int _x3,int _y3){
  int du10;
  int du20;
  int du30;
  int du31;
  int du32;
  int dv10;
  int dv20;
  int dv30;
  int dv31;
  int dv32;
  int dx10;
  int dx20;
  int dx30;
  int dx31;
  int dx32;
  int dy10;
  int dy20;
  int dy30;
  int dy31;
  int dy32;
  int a00;
  int a01;
  int a02;
  int a10;
  int a11;
  int a12;
  int a20;
  int a21;
  int a22;
  int i00;
  int i01;
  int i10;
  int i11;
  int i20;
  int i21;
  int i22;
  int b0;
  int b1;
  int b2;
  int shift;
  int round;
  int x;
  int y;
  int w;
  
  du10=_u1-_u0;
  du20=_u2-_u0;
  du30=_u3-_u0;
  du31=_u3-_u1;
  du32=_u3-_u2;
  dv10=_v1-_v0;
  dv20=_v2-_v0;
  dv30=_v3-_v0;
  dv31=_v3-_v1;
  dv32=_v3-_v2;
  
  a20=du32*dv10-du10*dv32;
  a21=du20*dv31-du31*dv20;
  if(a20||a21)a22=du32*dv31-du31*dv32;
  
  else a22=1;
  a00=du10*(a20+a22);
  a01=du20*(a21+a22);
  a10=dv10*(a20+a22);
  a11=dv20*(a21+a22);
  
  i00=a11*a22;
  i01=-a01*a22;
  i10=-a10*a22;
  i11=a00*a22;
  i20=a10*a21-a11*a20;
  i21=a01*a20-a00*a21;
  i22=a00*a11-a01*a10;
  
  if(i00)i00=QR_FLIPSIGNI(QR_DIVROUND(i22,abs(i00)),i00);
  if(i01)i01=QR_FLIPSIGNI(QR_DIVROUND(i22,abs(i01)),i01);
  if(i10)i10=QR_FLIPSIGNI(QR_DIVROUND(i22,abs(i10)),i10);
  if(i11)i11=QR_FLIPSIGNI(QR_DIVROUND(i22,abs(i11)),i11);
  if(i20)i20=QR_FLIPSIGNI(QR_DIVROUND(i22,abs(i20)),i20);
  if(i21)i21=QR_FLIPSIGNI(QR_DIVROUND(i22,abs(i21)),i21);
  
  dx10=_x1-_x0;
  dx20=_x2-_x0;
  dx30=_x3-_x0;
  dx31=_x3-_x1;
  dx32=_x3-_x2;
  dy10=_y1-_y0;
  dy20=_y2-_y0;
  dy30=_y3-_y0;
  dy31=_y3-_y1;
  dy32=_y3-_y2;
  a20=dx32*dy10-dx10*dy32;
  a21=dx20*dy31-dx31*dy20;
  a22=dx32*dy31-dx31*dy32;
  
  b0=qr_ilog(QR_MAXI(abs(dx10),abs(dy10)))+qr_ilog(abs(a20+a22));
  b1=qr_ilog(QR_MAXI(abs(dx20),abs(dy20)))+qr_ilog(abs(a21+a22));
  b2=qr_ilog(QR_MAXI(QR_MAXI(abs(a20),abs(a21)),abs(a22)));
  shift=QR_MAXI(0,QR_MAXI(QR_MAXI(b0,b1),b2)-(QR_INT_BITS-3-QR_ALIGN_SUBPREC));
  round=(1<<shift)>>1;
  
  a00=QR_FIXMUL(dx10,a20+a22,round,shift);
  a01=QR_FIXMUL(dx20,a21+a22,round,shift);
  a10=QR_FIXMUL(dy10,a20+a22,round,shift);
  a11=QR_FIXMUL(dy20,a21+a22,round,shift);
  
  _cell->fwd[0][0]=(i00?QR_DIVROUND(a00,i00):0)+(i10?QR_DIVROUND(a01,i10):0);
  _cell->fwd[0][1]=(i01?QR_DIVROUND(a00,i01):0)+(i11?QR_DIVROUND(a01,i11):0);
  _cell->fwd[1][0]=(i00?QR_DIVROUND(a10,i00):0)+(i10?QR_DIVROUND(a11,i10):0);
  _cell->fwd[1][1]=(i01?QR_DIVROUND(a10,i01):0)+(i11?QR_DIVROUND(a11,i11):0);
  _cell->fwd[2][0]=(i00?QR_DIVROUND(a20,i00):0)+(i10?QR_DIVROUND(a21,i10):0)
   +(i20?QR_DIVROUND(a22,i20):0)+round>>shift;
  _cell->fwd[2][1]=(i01?QR_DIVROUND(a20,i01):0)+(i11?QR_DIVROUND(a21,i11):0)
   +(i21?QR_DIVROUND(a22,i21):0)+round>>shift;
  _cell->fwd[2][2]=a22+round>>shift;
  
  x=_cell->fwd[0][0]*du10+_cell->fwd[0][1]*dv10;
  y=_cell->fwd[1][0]*du10+_cell->fwd[1][1]*dv10;
  w=_cell->fwd[2][0]*du10+_cell->fwd[2][1]*dv10+_cell->fwd[2][2];
  a02=dx10*w-x;
  a12=dy10*w-y;
  x=_cell->fwd[0][0]*du20+_cell->fwd[0][1]*dv20;
  y=_cell->fwd[1][0]*du20+_cell->fwd[1][1]*dv20;
  w=_cell->fwd[2][0]*du20+_cell->fwd[2][1]*dv20+_cell->fwd[2][2];
  a02+=dx20*w-x;
  a12+=dy20*w-y;
  x=_cell->fwd[0][0]*du30+_cell->fwd[0][1]*dv30;
  y=_cell->fwd[1][0]*du30+_cell->fwd[1][1]*dv30;
  w=_cell->fwd[2][0]*du30+_cell->fwd[2][1]*dv30+_cell->fwd[2][2];
  a02+=dx30*w-x;
  a12+=dy30*w-y;
  _cell->fwd[0][2]=a02+2>>2;
  _cell->fwd[1][2]=a12+2>>2;
  _cell->x0=_x0;
  _cell->y0=_y0;
  _cell->u0=_u0;
  _cell->v0=_v0;
}


static void qr_hom_cell_fproject(qr_point _p,const qr_hom_cell *_cell,
 int _x,int _y,int _w){
  if(_w==0){
    _p[0]=_x<0?INT_MIN:INT_MAX;
    _p[1]=_y<0?INT_MIN:INT_MAX;
  }
  else{
    if(_w<0){
      _x=-_x;
      _y=-_y;
      _w=-_w;
    }
    _p[0]=QR_DIVROUND(_x,_w)+_cell->x0;
    _p[1]=QR_DIVROUND(_y,_w)+_cell->y0;
  }
}

static void qr_hom_cell_project(qr_point _p,const qr_hom_cell *_cell,
 int _u,int _v,int _res){
  _u-=_cell->u0<<_res;
  _v-=_cell->v0<<_res;
  qr_hom_cell_fproject(_p,_cell,
   _cell->fwd[0][0]*_u+_cell->fwd[0][1]*_v+(_cell->fwd[0][2]<<_res),
   _cell->fwd[1][0]*_u+_cell->fwd[1][1]*_v+(_cell->fwd[1][2]<<_res),
   _cell->fwd[2][0]*_u+_cell->fwd[2][1]*_v+(_cell->fwd[2][2]<<_res));
}




static unsigned qr_alignment_pattern_fetch(qr_point _p[5][5],int _x0,int _y0,
 const unsigned char *_img,int _width,int _height){
  unsigned v;
  int      i;
  int      j;
  int      k;
  int      dx;
  int      dy;
  dx=_x0-_p[2][2][0];
  dy=_y0-_p[2][2][1];
  v=0;
  for(k=i=0;i<5;i++)for(j=0;j<5;j++,k++){
    v|=qr_img_get_bit(_img,_width,_height,_p[i][j][0]+dx,_p[i][j][1]+dy)<<k;
  }
  return v;
}


static int qr_alignment_pattern_search(qr_point _p,const qr_hom_cell *_cell,
 int _u,int _v,int _r,const unsigned char *_img,int _width,int _height){
  qr_point c[4];
  int      nc[4];
  qr_point p[5][5];
  qr_point pc;
  unsigned best_match;
  int      best_dist;
  int      bestx;
  int      besty;
  unsigned match;
  int      dist;
  int      u;
  int      v;
  int      x0;
  int      y0;
  int      w0;
  int      x;
  int      y;
  int      w;
  int      dxdu;
  int      dydu;
  int      dwdu;
  int      dxdv;
  int      dydv;
  int      dwdv;
  int      dx;
  int      dy;
  int      i;
  int      j;
  
  u=(_u-2)-_cell->u0;
  v=(_v-2)-_cell->v0;
  x0=_cell->fwd[0][0]*u+_cell->fwd[0][1]*v+_cell->fwd[0][2];
  y0=_cell->fwd[1][0]*u+_cell->fwd[1][1]*v+_cell->fwd[1][2];
  w0=_cell->fwd[2][0]*u+_cell->fwd[2][1]*v+_cell->fwd[2][2];
  dxdu=_cell->fwd[0][0];
  dydu=_cell->fwd[1][0];
  dwdu=_cell->fwd[2][0];
  dxdv=_cell->fwd[0][1];
  dydv=_cell->fwd[1][1];
  dwdv=_cell->fwd[2][1];
  for(i=0;i<5;i++){
    x=x0;
    y=y0;
    w=w0;
    for(j=0;j<5;j++){
      qr_hom_cell_fproject(p[i][j],_cell,x,y,w);
      x+=dxdu;
      y+=dydu;
      w+=dwdu;
    }
    x0+=dxdv;
    y0+=dydv;
    w0+=dwdv;
  }
  bestx=p[2][2][0];
  besty=p[2][2][1];
  best_match=qr_alignment_pattern_fetch(p,bestx,besty,_img,_width,_height);
  best_dist=qr_hamming_dist(best_match,0x1F8D63F,25);
  if(best_dist>0){
    u=_u-_cell->u0;
    v=_v-_cell->v0;
    x=_cell->fwd[0][0]*u+_cell->fwd[0][1]*v+_cell->fwd[0][2]<<QR_ALIGN_SUBPREC;
    y=_cell->fwd[1][0]*u+_cell->fwd[1][1]*v+_cell->fwd[1][2]<<QR_ALIGN_SUBPREC;
    w=_cell->fwd[2][0]*u+_cell->fwd[2][1]*v+_cell->fwd[2][2]<<QR_ALIGN_SUBPREC;
    
    for(i=1;i<_r<<QR_ALIGN_SUBPREC;i++){
      int side_len;
      side_len=(i<<1)-1;
      x-=dxdu+dxdv;
      y-=dydu+dydv;
      w-=dwdu+dwdv;
      for(j=0;j<4*side_len;j++){
        int      dir;
        qr_hom_cell_fproject(pc,_cell,x,y,w);
        match=qr_alignment_pattern_fetch(p,pc[0],pc[1],_img,_width,_height);
        dist=qr_hamming_dist(match,0x1F8D63F,best_dist+1);
        if(dist<best_dist){
          best_match=match;
          best_dist=dist;
          bestx=pc[0];
          besty=pc[1];
        }
        if(j<2*side_len){
          dir=j>=side_len;
          x+=_cell->fwd[0][dir];
          y+=_cell->fwd[1][dir];
          w+=_cell->fwd[2][dir];
        }
        else{
          dir=j>=3*side_len;
          x-=_cell->fwd[0][dir];
          y-=_cell->fwd[1][dir];
          w-=_cell->fwd[2][dir];
        }
        if(!best_dist)break;
      }
      if(!best_dist)break;
    }
  }
  
  if(best_dist>6){
    _p[0]=p[2][2][0];
    _p[1]=p[2][2][1];
    return -1;
  }
  
  dx=bestx-p[2][2][0];
  dy=besty-p[2][2][1];
  memset(nc,0,sizeof(nc));
  memset(c,0,sizeof(c));
  
  for(i=0;i<8;i++){
    static const unsigned MASK_TESTS[8][2]={
      {0x1040041,0x1000001},{0x0041040,0x0001000},
      {0x0110110,0x0100010},{0x0011100,0x0001000},
      {0x0420084,0x0400004},{0x0021080,0x0001000},
      {0x0006C00,0x0004400},{0x0003800,0x0001000},
    };
    static const unsigned char MASK_COORDS[8][2]={
      {0,0},{1,1},{4,0},{3,1},{2,0},{2,1},{0,2},{1,2}
    };
    if((best_match&MASK_TESTS[i][0])==MASK_TESTS[i][1]){
      int x0;
      int y0;
      int x1;
      int y1;
      x0=p[MASK_COORDS[i][1]][MASK_COORDS[i][0]][0]+dx>>QR_FINDER_SUBPREC;
      if(x0<0||x0>=_width)continue;
      y0=p[MASK_COORDS[i][1]][MASK_COORDS[i][0]][1]+dy>>QR_FINDER_SUBPREC;
      if(y0<0||y0>=_height)continue;
      x1=p[4-MASK_COORDS[i][1]][4-MASK_COORDS[i][0]][0]+dx>>QR_FINDER_SUBPREC;
      if(x1<0||x1>=_width)continue;
      y1=p[4-MASK_COORDS[i][1]][4-MASK_COORDS[i][0]][1]+dy>>QR_FINDER_SUBPREC;
      if(y1<0||y1>=_height)continue;
      if(!qr_finder_locate_crossing(_img,_width,_height,x0,y0,x1,y1,i&1,pc)){
        int w;
        int cx;
        int cy;
        cx=pc[0]-bestx;
        cy=pc[1]-besty;
        if(i&1){
          
          w=3;
          cx+=cx<<1;
          cy+=cy<<1;
        }
        else w=1;
        nc[i>>1]+=w;
        c[i>>1][0]+=cx;
        c[i>>1][1]+=cy;
      }
    }
  }
  
  for(i=0;i<2;i++){
    int a;
    int b;
    a=nc[i<<1];
    b=nc[i<<1|1];
    if(a&&b){
      int w;
      w=QR_MAXI(a,b);
      c[i<<1][0]=QR_DIVROUND(w*(b*c[i<<1][0]+a*c[i<<1|1][0]),a*b);
      c[i<<1][1]=QR_DIVROUND(w*(b*c[i<<1][1]+a*c[i<<1|1][1]),a*b);
      nc[i<<1]=w<<1;
    }
    else{
      c[i<<1][0]+=c[i<<1|1][0];
      c[i<<1][1]+=c[i<<1|1][1];
      nc[i<<1]+=b;
    }
  }
  
  c[0][0]+=c[2][0];
  c[0][1]+=c[2][1];
  nc[0]+=nc[2];
  
  if(nc[0]){
    dx=QR_DIVROUND(c[0][0],nc[0]);
    dy=QR_DIVROUND(c[0][1],nc[0]);
    
    match=qr_alignment_pattern_fetch(p,bestx+dx,besty+dy,_img,_width,_height);
    dist=qr_hamming_dist(match,0x1F8D63F,best_dist+1);
    if(dist<=best_dist+1){
      bestx+=dx;
      besty+=dy;
    }
  }
  _p[0]=bestx;
  _p[1]=besty;
  return 0;
}

static int qr_hom_fit(qr_hom *_hom,qr_finder *_ul,qr_finder *_ur,
 qr_finder *_dl,qr_point _p[4],const qr_aff *_aff,isaac_ctx *_isaac,
 const unsigned char *_img,int _width,int _height){
  qr_point *b;
  int       nb;
  int       cb;
  qr_point *r;
  int       nr;
  int       cr;
  qr_line   l[4];
  qr_point  q;
  qr_point  p;
  int       ox;
  int       oy;
  int       ru;
  int       rv;
  int       dru;
  int       drv;
  int       bu;
  int       bv;
  int       dbu;
  int       dbv;
  int       rx;
  int       ry;
  int       drxi;
  int       dryi;
  int       drxj;
  int       dryj;
  int       rdone;
  int       nrempty;
  int       rlastfit;
  int       bx;
  int       by;
  int       dbxi;
  int       dbyi;
  int       dbxj;
  int       dbyj;
  int       bdone;
  int       nbempty;
  int       blastfit;
  int       shift;
  int       round;
  int       version4;
  int       brx;
  int       bry;
  int       i;
  
  
  qr_finder_ransac(_ul,_aff,_isaac,0);
  qr_finder_ransac(_dl,_aff,_isaac,0);
  qr_line_fit_finder_pair(l[0],_aff,_ul,_dl,0);
  if(qr_line_eval(l[0],_dl->c->pos[0],_dl->c->pos[1])<0||
   qr_line_eval(l[0],_ur->c->pos[0],_ur->c->pos[1])<0){
    return -1;
  }
  qr_finder_ransac(_ul,_aff,_isaac,2);
  qr_finder_ransac(_ur,_aff,_isaac,2);
  qr_line_fit_finder_pair(l[2],_aff,_ul,_ur,2);
  if(qr_line_eval(l[2],_dl->c->pos[0],_dl->c->pos[1])<0||
   qr_line_eval(l[2],_ur->c->pos[0],_ur->c->pos[1])<0){
    return -1;
  }
  
  drv=_ur->size[1]>>1;
  qr_finder_ransac(_ur,_aff,_isaac,1);
  if(qr_line_fit_finder_edge(l[1],_ur,1,_aff->res)>=0){
    if(qr_line_eval(l[1],_ul->c->pos[0],_ul->c->pos[1])<0||
     qr_line_eval(l[1],_dl->c->pos[0],_dl->c->pos[1])<0){
      return -1;
    }
    
    if(qr_aff_line_step(_aff,l[1],1,drv,&dru)<0)return -1;
  }
  else dru=0;
  ru=_ur->o[0]+3*_ur->size[0]-2*dru;
  rv=_ur->o[1]-2*drv;
  dbu=_dl->size[0]>>1;
  qr_finder_ransac(_dl,_aff,_isaac,3);
  if(qr_line_fit_finder_edge(l[3],_dl,3,_aff->res)>=0){
    if(qr_line_eval(l[3],_ul->c->pos[0],_ul->c->pos[1])<0||
     qr_line_eval(l[3],_ur->c->pos[0],_ur->c->pos[1])<0){
      return -1;
    }
    
    if(qr_aff_line_step(_aff,l[3],0,dbu,&dbv)<0)return -1;
  }
  else dbv=0;
  bu=_dl->o[0]-2*dbu;
  bv=_dl->o[1]+3*_dl->size[1]-2*dbv;
  
  nr=rlastfit=_ur->ninliers[1];
  cr=nr+(_dl->o[1]-rv+drv-1)/drv;
  r=(qr_point *)malloc(cr*sizeof(*r));
  for(i=0;i<_ur->ninliers[1];i++){
    memcpy(r[i],_ur->edge_pts[1][i].pos,sizeof(r[i]));
  }
  nb=blastfit=_dl->ninliers[3];
  cb=nb+(_ur->o[0]-bu+dbu-1)/dbu;
  b=(qr_point *)malloc(cb*sizeof(*b));
  for(i=0;i<_dl->ninliers[3];i++){
    memcpy(b[i],_dl->edge_pts[3][i].pos,sizeof(b[i]));
  }
  
  ox=(_aff->x0<<_aff->res)+(1<<_aff->res-1);
  oy=(_aff->y0<<_aff->res)+(1<<_aff->res-1);
  rx=_aff->fwd[0][0]*ru+_aff->fwd[0][1]*rv+ox;
  ry=_aff->fwd[1][0]*ru+_aff->fwd[1][1]*rv+oy;
  drxi=_aff->fwd[0][0]*dru+_aff->fwd[0][1]*drv;
  dryi=_aff->fwd[1][0]*dru+_aff->fwd[1][1]*drv;
  drxj=_aff->fwd[0][0]*_ur->size[0];
  dryj=_aff->fwd[1][0]*_ur->size[0];
  bx=_aff->fwd[0][0]*bu+_aff->fwd[0][1]*bv+ox;
  by=_aff->fwd[1][0]*bu+_aff->fwd[1][1]*bv+oy;
  dbxi=_aff->fwd[0][0]*dbu+_aff->fwd[0][1]*dbv;
  dbyi=_aff->fwd[1][0]*dbu+_aff->fwd[1][1]*dbv;
  dbxj=_aff->fwd[0][1]*_dl->size[1];
  dbyj=_aff->fwd[1][1]*_dl->size[1];
  
  nrempty=nbempty=0;
  for(;;){
    int ret;
    int x0;
    int y0;
    int x1;
    int y1;
    
    rdone=rv>=QR_MINI(bv,_dl->o[1]+bv>>1)||nrempty>14;
    bdone=bu>=QR_MINI(ru,_ur->o[0]+ru>>1)||nbempty>14;
    if(!rdone&&(bdone||rv<bu)){
      x0=rx+drxj>>_aff->res+QR_FINDER_SUBPREC;
      y0=ry+dryj>>_aff->res+QR_FINDER_SUBPREC;
      x1=rx-drxj>>_aff->res+QR_FINDER_SUBPREC;
      y1=ry-dryj>>_aff->res+QR_FINDER_SUBPREC;
      if(nr>=cr){
        cr=cr<<1|1;
        r=(qr_point *)realloc(r,cr*sizeof(*r));
      }
      ret=qr_finder_quick_crossing_check(_img,_width,_height,x0,y0,x1,y1,1);
      if(!ret){
        ret=qr_finder_locate_crossing(_img,_width,_height,x0,y0,x1,y1,1,r[nr]);
      }
      if(ret>=0){
        if(!ret){
          qr_aff_unproject(q,_aff,r[nr][0],r[nr][1]);
          
          ru=ru+q[0]>>1;
          
          if(q[1]+drv>rv)rv=rv+q[1]>>1;
          rx=_aff->fwd[0][0]*ru+_aff->fwd[0][1]*rv+ox;
          ry=_aff->fwd[1][0]*ru+_aff->fwd[1][1]*rv+oy;
          nr++;
          
          if(nr>QR_MAXI(1,rlastfit+(rlastfit>>2))){
            qr_line_fit_points(l[1],r,nr,_aff->res);
            if(qr_aff_line_step(_aff,l[1],1,drv,&dru)>=0){
              drxi=_aff->fwd[0][0]*dru+_aff->fwd[0][1]*drv;
              dryi=_aff->fwd[1][0]*dru+_aff->fwd[1][1]*drv;
            }
            rlastfit=nr;
          }
        }
        nrempty=0;
      }
      else nrempty++;
      ru+=dru;
      
      if(rv+drv>rv)rv+=drv;
      else nrempty=INT_MAX;
      rx+=drxi;
      ry+=dryi;
    }
    else if(!bdone){
      x0=bx+dbxj>>_aff->res+QR_FINDER_SUBPREC;
      y0=by+dbyj>>_aff->res+QR_FINDER_SUBPREC;
      x1=bx-dbxj>>_aff->res+QR_FINDER_SUBPREC;
      y1=by-dbyj>>_aff->res+QR_FINDER_SUBPREC;
      if(nb>=cb){
        cb=cb<<1|1;
        b=(qr_point *)realloc(b,cb*sizeof(*b));
      }
      ret=qr_finder_quick_crossing_check(_img,_width,_height,x0,y0,x1,y1,1);
      if(!ret){
        ret=qr_finder_locate_crossing(_img,_width,_height,x0,y0,x1,y1,1,b[nb]);
      }
      if(ret>=0){
        if(!ret){
          qr_aff_unproject(q,_aff,b[nb][0],b[nb][1]);
          
          
          if(q[0]+dbu>bu)bu=bu+q[0]>>1;
          bv=bv+q[1]>>1;
          bx=_aff->fwd[0][0]*bu+_aff->fwd[0][1]*bv+ox;
          by=_aff->fwd[1][0]*bu+_aff->fwd[1][1]*bv+oy;
          nb++;
          
          if(nb>QR_MAXI(1,blastfit+(blastfit>>2))){
            qr_line_fit_points(l[3],b,nb,_aff->res);
            if(qr_aff_line_step(_aff,l[3],0,dbu,&dbv)>=0){
              dbxi=_aff->fwd[0][0]*dbu+_aff->fwd[0][1]*dbv;
              dbyi=_aff->fwd[1][0]*dbu+_aff->fwd[1][1]*dbv;
            }
            blastfit=nb;
          }
        }
        nbempty=0;
      }
      else nbempty++;
      
      if(bu+dbu>bu)bu+=dbu;
      else nbempty=INT_MAX;
      bv+=dbv;
      bx+=dbxi;
      by+=dbyi;
    }
    else break;
  }
  
  if(nr>1)qr_line_fit_points(l[1],r,nr,_aff->res);
  else{
    qr_aff_project(p,_aff,_ur->o[0]+3*_ur->size[0],_ur->o[1]);
    shift=QR_MAXI(0,
     qr_ilog(QR_MAXI(abs(_aff->fwd[0][1]),abs(_aff->fwd[1][1])))
     -(_aff->res+1>>1));
    round=(1<<shift)>>1;
    l[1][0]=_aff->fwd[1][1]+round>>shift;
    l[1][1]=-_aff->fwd[0][1]+round>>shift;
    l[1][2]=-(l[1][0]*p[0]+l[1][1]*p[1]);
  }
  free(r);
  if(nb>1)qr_line_fit_points(l[3],b,nb,_aff->res);
  else{
    qr_aff_project(p,_aff,_dl->o[0],_dl->o[1]+3*_dl->size[1]);
    shift=QR_MAXI(0,
     qr_ilog(QR_MAXI(abs(_aff->fwd[0][1]),abs(_aff->fwd[1][1])))
     -(_aff->res+1>>1));
    round=(1<<shift)>>1;
    l[3][0]=_aff->fwd[1][0]+round>>shift;
    l[3][1]=-_aff->fwd[0][0]+round>>shift;
    l[3][2]=-(l[1][0]*p[0]+l[1][1]*p[1]);
  }
  free(b);
  for(i=0;i<4;i++){
    if(qr_line_isect(_p[i],l[i&1],l[2+(i>>1)])<0)return -1;
    
    if(_p[i][0]<-_width<<QR_FINDER_SUBPREC||
     _p[i][0]>=_width<<QR_FINDER_SUBPREC+1||
     _p[i][1]<-_height<<QR_FINDER_SUBPREC||
     _p[i][1]>=_height<<QR_FINDER_SUBPREC+1){
      return -1;
    }
  }
  
  brx=_p[3][0];
  bry=_p[3][1];
  
  version4=_ul->eversion[0]+_ul->eversion[1]+_ur->eversion[0]+_dl->eversion[1];
  if(version4>4){
    qr_hom_cell cell;
    qr_point    p3;
    int         dim;
    dim=17+version4;
    qr_hom_cell_init(&cell,0,0,dim-1,0,0,dim-1,dim-1,dim-1,
     _p[0][0],_p[0][1],_p[1][0],_p[1][1],
     _p[2][0],_p[2][1],_p[3][0],_p[3][1]);
    if(qr_alignment_pattern_search(p3,&cell,dim-7,dim-7,4,
     _img,_width,_height)>=0){
      long long w;
      long long mask;
      int       c21;
      int       dx21;
      int       dy21;
      
      
      c21=_p[2][0]*_p[1][1]-_p[2][1]*_p[1][0];
      dx21=_p[2][0]-_p[1][0];
      dy21=_p[2][1]-_p[1][1];
      w=QR_EXTMUL(dim-7,c21,
       QR_EXTMUL(dim-13,_p[0][0]*dy21-_p[0][1]*dx21,
       QR_EXTMUL(6,p3[0]*dy21-p3[1]*dx21,0)));
      
      if(w==0)return -1;
      mask=QR_SIGNMASK(w);
      w=w+mask^mask;
      brx=(int)QR_DIVROUND(QR_EXTMUL((dim-7)*_p[0][0],p3[0]*dy21,
       QR_EXTMUL((dim-13)*p3[0],c21-_p[0][1]*dx21,
       QR_EXTMUL(6*_p[0][0],c21-p3[1]*dx21,0)))+mask^mask,w);
      bry=(int)QR_DIVROUND(QR_EXTMUL((dim-7)*_p[0][1],-p3[1]*dx21,
       QR_EXTMUL((dim-13)*p3[1],c21+_p[0][0]*dy21,
       QR_EXTMUL(6*_p[0][1],c21+p3[0]*dy21,0)))+mask^mask,w);
    }
  }
  
  qr_hom_init(_hom,_p[0][0],_p[0][1],_p[1][0],_p[1][1],
   _p[2][0],_p[2][1],brx,bry,QR_HOM_BITS);
  return 0;
}




static const unsigned BCH18_6_CODES[34]={
                                                          0x07C94,
  0x085BC,0x09A99,0x0A4D3,0x0BBF6,0x0C762,0x0D847,0x0E60D,0x0F928,
  0x10B78,0x1145D,0x12A17,0x13532,0x149A6,0x15683,0x168C9,0x177EC,
  0x18EC4,0x191E1,0x1AFAB,0x1B08E,0x1CC1A,0x1D33F,0x1ED75,0x1F250,
  0x209D5,0x216F0,0x228BA,0x2379F,0x24B0B,0x2542E,0x26A64,0x27541,
  0x28C69
};


static int bch18_6_correct(unsigned *_y){
  unsigned x;
  unsigned y;
  int      nerrs;
  y=*_y;
  
  x=y>>12;
  if(x>=7&&x<=40){
    nerrs=qr_hamming_dist(y,BCH18_6_CODES[x-7],4);
    if(nerrs<4){
      *_y=BCH18_6_CODES[x-7];
      return nerrs;
    }
  }
  
  for(x=0;x<34;x++)if(x+7!=y>>12){
    nerrs=qr_hamming_dist(y,BCH18_6_CODES[x],4);
    if(nerrs<4){
      *_y=BCH18_6_CODES[x];
      return nerrs;
    }
  }
  return -1;
}

#if 0
static unsigned bch18_6_encode(unsigned _x){
  return (-(_x&1)&0x01F25)^(-(_x>>1&1)&0x0216F)^(-(_x>>2&1)&0x042DE)^
   (-(_x>>3&1)&0x085BC)^(-(_x>>4&1)&0x10B78)^(-(_x>>5&1)&0x209D5);
}
#endif


static int qr_finder_version_decode(qr_finder *_f,const qr_hom *_hom,
 const unsigned char *_img,int _width,int _height,int _dir){
  qr_point q;
  unsigned v;
  int      x0;
  int      y0;
  int      w0;
  int      dxi;
  int      dyi;
  int      dwi;
  int      dxj;
  int      dyj;
  int      dwj;
  int      ret;
  int      i;
  int      j;
  int      k;
  v=0;
  q[_dir]=_f->o[_dir]-7*_f->size[_dir];
  q[1-_dir]=_f->o[1-_dir]-3*_f->size[1-_dir];
  x0=_hom->fwd[0][0]*q[0]+_hom->fwd[0][1]*q[1];
  y0=_hom->fwd[1][0]*q[0]+_hom->fwd[1][1]*q[1];
  w0=_hom->fwd[2][0]*q[0]+_hom->fwd[2][1]*q[1]+_hom->fwd22;
  dxi=_hom->fwd[0][1-_dir]*_f->size[1-_dir];
  dyi=_hom->fwd[1][1-_dir]*_f->size[1-_dir];
  dwi=_hom->fwd[2][1-_dir]*_f->size[1-_dir];
  dxj=_hom->fwd[0][_dir]*_f->size[_dir];
  dyj=_hom->fwd[1][_dir]*_f->size[_dir];
  dwj=_hom->fwd[2][_dir]*_f->size[_dir];
  for(k=i=0;i<6;i++){
    int x;
    int y;
    int w;
    x=x0;
    y=y0;
    w=w0;
    for(j=0;j<3;j++,k++){
      qr_point p;
      qr_hom_fproject(p,_hom,x,y,w);
      v|=qr_img_get_bit(_img,_width,_height,p[0],p[1])<<k;
      x+=dxj;
      y+=dyj;
      w+=dwj;
    }
    x0+=dxi;
    y0+=dyi;
    w0+=dwi;
  }
  ret=bch18_6_correct(&v);
  
#if 0
  if(ret<0){
    
    v=0;
    for(k=i=0;i<3;i++){
      p[_dir]=_f->o[_dir]+_f->size[_dir]*(-5-i);
      for(j=0;j<6;j++,k++){
        qr_point q;
        p[1-_dir]=_f->o[1-_dir]+_f->size[1-_dir]*(2-j);
        qr_hom_project(q,_hom,p[0],p[1]);
        v|=qr_img_get_bit(_img,_width,_height,q[0],q[1])<<k;
      }
    }
    ret=bch18_6_correct(&v);
  }
#endif
  return ret>=0?(int)(v>>12):ret;
}


static int qr_finder_fmt_info_decode(qr_finder *_ul,qr_finder *_ur,
 qr_finder *_dl,const qr_hom *_hom,
 const unsigned char *_img,int _width,int _height){
  qr_point p;
  unsigned lo[2];
  unsigned hi[2];
  int      u;
  int      v;
  int      x;
  int      y;
  int      w;
  int      dx;
  int      dy;
  int      dw;
  int      fmt_info[4];
  int      count[4];
  int      nerrs[4];
  int      nfmt_info;
  int      besti;
  int      imax;
  int      di;
  int      i;
  int      k;
  
  lo[0]=0;
  u=_ul->o[0]+5*_ul->size[0];
  v=_ul->o[1]-3*_ul->size[1];
  x=_hom->fwd[0][0]*u+_hom->fwd[0][1]*v;
  y=_hom->fwd[1][0]*u+_hom->fwd[1][1]*v;
  w=_hom->fwd[2][0]*u+_hom->fwd[2][1]*v+_hom->fwd22;
  dx=_hom->fwd[0][1]*_ul->size[1];
  dy=_hom->fwd[1][1]*_ul->size[1];
  dw=_hom->fwd[2][1]*_ul->size[1];
  for(k=i=0;;i++){
    
    if(i!=6){
      qr_hom_fproject(p,_hom,x,y,w);
      lo[0]|=qr_img_get_bit(_img,_width,_height,p[0],p[1])<<k++;
      
      if(i>=8)break;
    }
    x+=dx;
    y+=dy;
    w+=dw;
  }
  hi[0]=0;
  dx=-_hom->fwd[0][0]*_ul->size[0];
  dy=-_hom->fwd[1][0]*_ul->size[0];
  dw=-_hom->fwd[2][0]*_ul->size[0];
  while(i-->0){
    x+=dx;
    y+=dy;
    w+=dw;
    
    if(i!=6){
      qr_hom_fproject(p,_hom,x,y,w);
      hi[0]|=qr_img_get_bit(_img,_width,_height,p[0],p[1])<<k++;
    }
  }
  
  lo[1]=0;
  u=_ur->o[0]+3*_ur->size[0];
  v=_ur->o[1]+5*_ur->size[1];
  x=_hom->fwd[0][0]*u+_hom->fwd[0][1]*v;
  y=_hom->fwd[1][0]*u+_hom->fwd[1][1]*v;
  w=_hom->fwd[2][0]*u+_hom->fwd[2][1]*v+_hom->fwd22;
  dx=-_hom->fwd[0][0]*_ur->size[0];
  dy=-_hom->fwd[1][0]*_ur->size[0];
  dw=-_hom->fwd[2][0]*_ur->size[0];
  for(k=0;k<8;k++){
    qr_hom_fproject(p,_hom,x,y,w);
    lo[1]|=qr_img_get_bit(_img,_width,_height,p[0],p[1])<<k;
    x+=dx;
    y+=dy;
    w+=dw;
  }
  
  hi[1]=0;
  u=_dl->o[0]+5*_dl->size[0];
  v=_dl->o[1]-3*_dl->size[1];
  x=_hom->fwd[0][0]*u+_hom->fwd[0][1]*v;
  y=_hom->fwd[1][0]*u+_hom->fwd[1][1]*v;
  w=_hom->fwd[2][0]*u+_hom->fwd[2][1]*v+_hom->fwd22;
  dx=_hom->fwd[0][1]*_dl->size[1];
  dy=_hom->fwd[1][1]*_dl->size[1];
  dw=_hom->fwd[2][1]*_dl->size[1];
  for(k=8;k<15;k++){
    qr_hom_fproject(p,_hom,x,y,w);
    hi[1]|=qr_img_get_bit(_img,_width,_height,p[0],p[1])<<k;
    x+=dx;
    y+=dy;
    w+=dw;
  }
  
  imax=2<<(hi[0]!=hi[1]);
  di=1+(lo[0]==lo[1]);
  nfmt_info=0;
  for(i=0;i<imax;i+=di){
    unsigned v;
    int      ret;
    int      j;
    v=(lo[i&1]|hi[i>>1])^0x5412;
    ret=bch15_5_correct(&v);
    v>>=10;
    if(ret<0)ret=4;
    for(j=0;;j++){
      if(j>=nfmt_info){
        fmt_info[j]=v;
        count[j]=1;
        nerrs[j]=ret;
        nfmt_info++;
        break;
      }
      if(fmt_info[j]==(int)v){
        count[j]++;
        if(ret<nerrs[j])nerrs[j]=ret;
        break;
      }
    }
  }
  besti=0;
  for(i=1;i<nfmt_info;i++){
    if(nerrs[besti]>3&&nerrs[i]<=3||
     count[i]>count[besti]||count[i]==count[besti]&&nerrs[i]<nerrs[besti]){
      besti=i;
    }
  }
  return nerrs[besti]<4?fmt_info[besti]:-1;
}




struct qr_sampling_grid{
  qr_hom_cell    *cells[6];
  unsigned       *fpmask;
  int             cell_limits[6];
  int             ncells;
};



static void qr_sampling_grid_fp_mask_rect(qr_sampling_grid *_grid,int _dim,
 int _u,int _v,int _w,int _h){
  int i;
  int j;
  int stride;
  stride=_dim+QR_INT_BITS-1>>QR_INT_LOGBITS;
  
  for(j=_u;j<_u+_w;j++)for(i=_v;i<_v+_h;i++){
    _grid->fpmask[j*stride+(i>>QR_INT_LOGBITS)]|=1<<(i&QR_INT_BITS-1);
  }
}


static int qr_sampling_grid_is_in_fp(const qr_sampling_grid *_grid,int _dim,
 int _u,int _v){
  return _grid->fpmask[_u*(_dim+QR_INT_BITS-1>>QR_INT_LOGBITS)
   +(_v>>QR_INT_LOGBITS)]>>(_v&QR_INT_BITS-1)&1;
}


static const unsigned char QR_ALIGNMENT_SPACING[34]={
  16,18,20,22,24,26,28,
  20,22,24,24,26,28,28,
  22,24,24,26,26,28,28,
  24,24,26,26,26,28,28,
  24,26,26,26,28,28
};

static inline void qr_svg_points(const char *cls,
                                 qr_point *p,
                                 int n)
{
    int i;
    svg_path_start(cls, 1, 0, 0);
    for(i = 0; i < n; i++, p++)
        svg_path_moveto(SVG_ABS, p[0][0], p[0][1]);
    svg_path_end();
}


static void qr_sampling_grid_init(qr_sampling_grid *_grid,int _version,
 const qr_point _ul_pos,const qr_point _ur_pos,const qr_point _dl_pos,
 qr_point _p[4],const unsigned char *_img,int _width,int _height){
  qr_hom_cell          base_cell;
  int                  align_pos[7];
  int                  dim;
  int                  nalign;
  int                  i;
  dim=17+(_version<<2);
  nalign=(_version/7)+2;
  
  qr_hom_cell_init(&base_cell,0,0,dim-1,0,0,dim-1,dim-1,dim-1,
   _p[0][0],_p[0][1],_p[1][0],_p[1][1],_p[2][0],_p[2][1],_p[3][0],_p[3][1]);
  
  _grid->ncells=nalign-1;
  _grid->cells[0]=(qr_hom_cell *)malloc(
   (nalign-1)*(nalign-1)*sizeof(*_grid->cells[0]));
  for(i=1;i<_grid->ncells;i++)_grid->cells[i]=_grid->cells[i-1]+_grid->ncells;
  
  _grid->fpmask=(unsigned *)calloc(dim,
   (dim+QR_INT_BITS-1>>QR_INT_LOGBITS)*sizeof(*_grid->fpmask));
  
  qr_sampling_grid_fp_mask_rect(_grid,dim,0,0,9,9);
  qr_sampling_grid_fp_mask_rect(_grid,dim,0,dim-8,9,8);
  qr_sampling_grid_fp_mask_rect(_grid,dim,dim-8,0,8,9);
  
  if(_version>6){
    qr_sampling_grid_fp_mask_rect(_grid,dim,0,dim-11,6,3);
    qr_sampling_grid_fp_mask_rect(_grid,dim,dim-11,0,3,6);
  }
  
  qr_sampling_grid_fp_mask_rect(_grid,dim,9,6,dim-17,1);
  qr_sampling_grid_fp_mask_rect(_grid,dim,6,9,1,dim-17);
  
  if(_version<2)memcpy(_grid->cells[0],&base_cell,sizeof(base_cell));
  else{
    qr_point *q;
    qr_point *p;
    int       j;
    int       k;
    q=(qr_point *)malloc(nalign*nalign*sizeof(*q));
    p=(qr_point *)malloc(nalign*nalign*sizeof(*p));
    
    align_pos[0]=6;
    align_pos[nalign-1]=dim-7;
    if(_version>6){
      int d;
      d=QR_ALIGNMENT_SPACING[_version-7];
      for(i=nalign-1;i-->1;)align_pos[i]=align_pos[i+1]-d;
    }
    
    q[0][0]=3;
    q[0][1]=3;
    p[0][0]=_ul_pos[0];
    p[0][1]=_ul_pos[1];
    q[nalign-1][0]=dim-4;
    q[nalign-1][1]=3;
    p[nalign-1][0]=_ur_pos[0];
    p[nalign-1][1]=_ur_pos[1];
    q[(nalign-1)*nalign][0]=3;
    q[(nalign-1)*nalign][1]=dim-4;
    p[(nalign-1)*nalign][0]=_dl_pos[0];
    p[(nalign-1)*nalign][1]=_dl_pos[1];
    
    for(k=1;k<2*nalign-1;k++){
      int jmin;
      int jmax;
      jmax=QR_MINI(k,nalign-1)-(k==nalign-1);
      jmin=QR_MAXI(0,k-(nalign-1))+(k==nalign-1);
      for(j=jmin;j<=jmax;j++){
        qr_hom_cell *cell;
        int          u;
        int          v;
        int          k;
        i=jmax-(j-jmin);
        k=i*nalign+j;
        u=align_pos[j];
        v=align_pos[i];
        q[k][0]=u;
        q[k][1]=v;
        
        qr_sampling_grid_fp_mask_rect(_grid,dim,u-2,v-2,5,5);
        
        if(i>1&&j>1){
          qr_point p0;
          qr_point p1;
          qr_point p2;
          
          qr_hom_cell_project(p0,_grid->cells[i-2]+j-1,u,v,0);
          qr_hom_cell_project(p1,_grid->cells[i-2]+j-2,u,v,0);
          qr_hom_cell_project(p2,_grid->cells[i-1]+j-2,u,v,0);
          
          QR_SORT2I(p0[0],p1[0]);
          QR_SORT2I(p0[1],p1[1]);
          QR_SORT2I(p1[0],p2[0]);
          QR_SORT2I(p1[1],p2[1]);
          QR_SORT2I(p0[0],p1[0]);
          QR_SORT2I(p0[1],p1[1]);
          
          cell=_grid->cells[i-1]+j-1;
          qr_hom_cell_init(cell,
           q[k-nalign-1][0],q[k-nalign-1][1],q[k-nalign][0],q[k-nalign][1],
           q[k-1][0],q[k-1][1],q[k][0],q[k][1],
           p[k-nalign-1][0],p[k-nalign-1][1],p[k-nalign][0],p[k-nalign][1],
           p[k-1][0],p[k-1][1],p1[0],p1[1]);
        }
        else if(i>1&&j>0)cell=_grid->cells[i-2]+j-1;
        else if(i>0&&j>1)cell=_grid->cells[i-1]+j-2;
        else cell=&base_cell;
        
        qr_alignment_pattern_search(p[k],cell,u,v,2,_img,_width,_height);
        if(i>0&&j>0){
          qr_hom_cell_init(_grid->cells[i-1]+j-1,
           q[k-nalign-1][0],q[k-nalign-1][1],q[k-nalign][0],q[k-nalign][1],
           q[k-1][0],q[k-1][1],q[k][0],q[k][1],
           p[k-nalign-1][0],p[k-nalign-1][1],p[k-nalign][0],p[k-nalign][1],
           p[k-1][0],p[k-1][1],p[k][0],p[k][1]);
        }
      }
    }
    qr_svg_points("align", p, nalign * nalign);
    free(q);
    free(p);
  }
  
  memcpy(_grid->cell_limits,align_pos+1,
   (_grid->ncells-1)*sizeof(*_grid->cell_limits));
  _grid->cell_limits[_grid->ncells-1]=dim;
  
  qr_hom_cell_project(_p[0],_grid->cells[0]+0,-1,-1,1);
  qr_hom_cell_project(_p[1],_grid->cells[0]+_grid->ncells-1,(dim<<1)-1,-1,1);
  qr_hom_cell_project(_p[2],_grid->cells[_grid->ncells-1]+0,-1,(dim<<1)-1,1);
  qr_hom_cell_project(_p[3],_grid->cells[_grid->ncells-1]+_grid->ncells-1,
   (dim<<1)-1,(dim<<1)-1,1);
  
  for(i=0;i<4;i++){
    _p[i][0]=QR_CLAMPI(-_width<<QR_FINDER_SUBPREC,_p[i][0],
     _width<<QR_FINDER_SUBPREC+1);
    _p[i][1]=QR_CLAMPI(-_height<<QR_FINDER_SUBPREC,_p[i][1],
     _height<<QR_FINDER_SUBPREC+1);
  }
  
}

static void qr_sampling_grid_clear(qr_sampling_grid *_grid){
  free(_grid->fpmask);
  free(_grid->cells[0]);
}



#if defined(QR_DEBUG)
static void qr_sampling_grid_dump(qr_sampling_grid *_grid,int _version,
 const unsigned char *_img,int _width,int _height){
  unsigned char *gimg;
  FILE          *fout;
  int            dim;
  int            u;
  int            v;
  int            x;
  int            y;
  int            w;
  int            i;
  int            j;
  int            r;
  int            s;
  dim=17+(_version<<2)+8<<QR_ALIGN_SUBPREC;
  gimg=(unsigned char *)malloc(dim*dim*sizeof(*gimg));
  for(i=0;i<dim;i++)for(j=0;j<dim;j++){
    qr_hom_cell *cell;
    if(i>=(4<<QR_ALIGN_SUBPREC)&&i<=dim-(5<<QR_ALIGN_SUBPREC)&&
     j>=(4<<QR_ALIGN_SUBPREC)&&j<=dim-(5<<QR_ALIGN_SUBPREC)&&
     ((!(i&(1<<QR_ALIGN_SUBPREC)-1))^(!(j&(1<<QR_ALIGN_SUBPREC)-1)))){
      gimg[i*dim+j]=0x7F;
    }
    else{
      qr_point p;
      u=(j>>QR_ALIGN_SUBPREC)-4;
      v=(i>>QR_ALIGN_SUBPREC)-4;
      for(r=0;r<_grid->ncells-1;r++)if(u<_grid->cell_limits[r])break;
      for(s=0;s<_grid->ncells-1;s++)if(v<_grid->cell_limits[s])break;
      cell=_grid->cells[s]+r;
      u=j-(cell->u0+4<<QR_ALIGN_SUBPREC);
      v=i-(cell->v0+4<<QR_ALIGN_SUBPREC);
      x=cell->fwd[0][0]*u+cell->fwd[0][1]*v+(cell->fwd[0][2]<<QR_ALIGN_SUBPREC);
      y=cell->fwd[1][0]*u+cell->fwd[1][1]*v+(cell->fwd[1][2]<<QR_ALIGN_SUBPREC);
      w=cell->fwd[2][0]*u+cell->fwd[2][1]*v+(cell->fwd[2][2]<<QR_ALIGN_SUBPREC);
      qr_hom_cell_fproject(p,cell,x,y,w);
      gimg[i*dim+j]=_img[
       QR_CLAMPI(0,p[1]>>QR_FINDER_SUBPREC,_height-1)*_width+
       QR_CLAMPI(0,p[0]>>QR_FINDER_SUBPREC,_width-1)];
    }
  }
  for(v=0;v<17+(_version<<2);v++)for(u=0;u<17+(_version<<2);u++){
    if(qr_sampling_grid_is_in_fp(_grid,17+(_version<<2),u,v)){
      j=u+4<<QR_ALIGN_SUBPREC;
      i=v+4<<QR_ALIGN_SUBPREC;
      gimg[(i-1)*dim+j-1]=0x7F;
      gimg[(i-1)*dim+j]=0x7F;
      gimg[(i-1)*dim+j+1]=0x7F;
      gimg[i*dim+j-1]=0x7F;
      gimg[i*dim+j+1]=0x7F;
      gimg[(i+1)*dim+j-1]=0x7F;
      gimg[(i+1)*dim+j]=0x7F;
      gimg[(i+1)*dim+j+1]=0x7F;
    }
  }
  fout=fopen("grid.png","wb");
  image_write_png(gimg,dim,dim,fout);
  fclose(fout);
  free(gimg);
}
#endif


static void qr_data_mask_fill(unsigned *_mask,int _dim,int _pattern){
  int stride;
  int i;
  int j;
  stride=_dim+QR_INT_BITS-1>>QR_INT_LOGBITS;
  
  switch(_pattern){
    
    case 0:{
      int m;
      m=0x55;
      for(j=0;j<_dim;j++){
        memset(_mask+j*stride,m,stride*sizeof(*_mask));
        m^=0xFF;
      }
    }break;
    
    case 1:memset(_mask,0x55,_dim*stride*sizeof(*_mask));break;
    
    case 2:{
      unsigned m;
      m=0xFF;
      for(j=0;j<_dim;j++){
        memset(_mask+j*stride,m&0xFF,stride*sizeof(*_mask));
        m=m<<8|m>>16;
      }
    }break;
    
    case 3:{
      unsigned mi;
      unsigned mj;
      mj=0;
      for(i=0;i<(QR_INT_BITS+2)/3;i++)mj|=1<<3*i;
      for(j=0;j<_dim;j++){
        mi=mj;
        for(i=0;i<stride;i++){
          _mask[j*stride+i]=mi;
          mi=mi>>QR_INT_BITS%3|mi<<3-QR_INT_BITS%3;
        }
        mj=mj>>1|mj<<2;
      }
    }break;
    
    case 4:{
      unsigned m;
      m=7;
      for(j=0;j<_dim;j++){
        memset(_mask+j*stride,(0xCC^-(m&1))&0xFF,stride*sizeof(*_mask));
        m=m>>1|m<<5;
      }
    }break;
    
    case 5:{
      for(j=0;j<_dim;j++){
        unsigned m;
        m=0;
        for(i=0;i<6;i++)m|=!((i*j)%6)<<i;
        for(i=6;i<QR_INT_BITS;i<<=1)m|=m<<i;
        for(i=0;i<stride;i++){
          _mask[j*stride+i]=m;
          m=m>>QR_INT_BITS%6|m<<6-QR_INT_BITS%6;
        }
      }
    }break;
    
    case 6:{
      for(j=0;j<_dim;j++){
        unsigned m;
        m=0;
        for(i=0;i<6;i++)m|=((i*j)%3+i*j+1&1)<<i;
        for(i=6;i<QR_INT_BITS;i<<=1)m|=m<<i;
        for(i=0;i<stride;i++){
          _mask[j*stride+i]=m;
          m=m>>QR_INT_BITS%6|m<<6-QR_INT_BITS%6;
        }
      }
    }break;
    
    default:{
      for(j=0;j<_dim;j++){
        unsigned m;
        m=0;
        for(i=0;i<6;i++)m|=((i*j)%3+i+j+1&1)<<i;
        for(i=6;i<QR_INT_BITS;i<<=1)m|=m<<i;
        for(i=0;i<stride;i++){
          _mask[j*stride+i]=m;
          m=m>>QR_INT_BITS%6|m<<6-QR_INT_BITS%6;
        }
      }
    }break;
  }
}

static void qr_sampling_grid_sample(const qr_sampling_grid *_grid,
 unsigned *_data_bits,int _dim,int _fmt_info,
 const unsigned char *_img,int _width,int _height){
  int stride;
  int u0;
  int u1;
  int j;
  
  qr_data_mask_fill(_data_bits,_dim,_fmt_info&7);
  stride=_dim+QR_INT_BITS-1>>QR_INT_LOGBITS;
  u0=0;
  svg_path_start("sampling-grid", 1, 0, 0);
  
  for(j=0;j<_grid->ncells;j++){
    int i;
    int v0;
    int v1;
    u1=_grid->cell_limits[j];
    v0=0;
    for(i=0;i<_grid->ncells;i++){
      qr_hom_cell *cell;
      int          x0;
      int          y0;
      int          w0;
      int          u;
      int          du;
      int          dv;
      v1=_grid->cell_limits[i];
      cell=_grid->cells[i]+j;
      du=u0-cell->u0;
      dv=v0-cell->v0;
      x0=cell->fwd[0][0]*du+cell->fwd[0][1]*dv+cell->fwd[0][2];
      y0=cell->fwd[1][0]*du+cell->fwd[1][1]*dv+cell->fwd[1][2];
      w0=cell->fwd[2][0]*du+cell->fwd[2][1]*dv+cell->fwd[2][2];
      for(u=u0;u<u1;u++){
        int x;
        int y;
        int w;
        int v;
        x=x0;
        y=y0;
        w=w0;
        for(v=v0;v<v1;v++){
          
          if(!qr_sampling_grid_is_in_fp(_grid,_dim,u,v)){
            qr_point p;
            qr_hom_cell_fproject(p,cell,x,y,w);
            _data_bits[u*stride+(v>>QR_INT_LOGBITS)]^=
             qr_img_get_bit(_img,_width,_height,p[0],p[1])<<(v&QR_INT_BITS-1);
            svg_path_moveto(SVG_ABS, p[0], p[1]);
          }
          x+=cell->fwd[0][1];
          y+=cell->fwd[1][1];
          w+=cell->fwd[2][1];
        }
        x0+=cell->fwd[0][0];
        y0+=cell->fwd[1][0];
        w0+=cell->fwd[2][0];
      }
      v0=v1;
    }
    u0=u1;
  }
  svg_path_end();
}


static void qr_samples_unpack(unsigned char **_blocks,int _nblocks,
 int _nshort_data,int _nshort_blocks,const unsigned *_data_bits,
 const unsigned *_fp_mask,int _dim){
  unsigned bits;
  int      biti;
  int      stride;
  int      blocki;
  int      blockj;
  int      i;
  int      j;
  stride=_dim+QR_INT_BITS-1>>QR_INT_LOGBITS;
  
  if(_nshort_blocks>=_nblocks)_nshort_blocks=0;
  
  bits=0;
  for(blocki=blockj=biti=0,j=_dim-1;j>0;j-=2){
    unsigned data1;
    unsigned data2;
    unsigned fp_mask1;
    unsigned fp_mask2;
    int      nbits;
    int      l;
    
    nbits=(_dim-1&QR_INT_BITS-1)+1;
    l=j*stride;
    for(i=stride;i-->0;){
      data1=_data_bits[l+i];
      fp_mask1=_fp_mask[l+i];
      data2=_data_bits[l+i-stride];
      fp_mask2=_fp_mask[l+i-stride];
      while(nbits-->0){
        
        if(!(fp_mask1>>nbits&1)){
          bits=bits<<1|data1>>nbits&1;
          biti++;
        }
        
        if(!(fp_mask2>>nbits&1)){
          bits=bits<<1|data2>>nbits&1;
          biti++;
        }
        
        if(biti>=8){
          biti-=8;
          *_blocks[blocki++]++=(unsigned char)(bits>>biti);
          
          if(blocki>=_nblocks)blocki=++blockj==_nshort_data?_nshort_blocks:0;
        }
      }
      nbits=QR_INT_BITS;
    }
    j-=2;
    
    if(j==6)j--;
    
    l=j*stride;
    for(i=0;i<stride;i++){
      data1=_data_bits[l+i];
      fp_mask1=_fp_mask[l+i];
      data2=_data_bits[l+i-stride];
      fp_mask2=_fp_mask[l+i-stride];
      nbits=QR_MINI(_dim-(i<<QR_INT_LOGBITS),QR_INT_BITS);
      while(nbits-->0){
        
        if(!(fp_mask1&1)){
          bits=bits<<1|data1&1;
          biti++;
        }
        data1>>=1;
        fp_mask1>>=1;
        
        if(!(fp_mask2&1)){
          bits=bits<<1|data2&1;
          biti++;
        }
        data2>>=1;
        fp_mask2>>=1;
        
        if(biti>=8){
          biti-=8;
          *_blocks[blocki++]++=(unsigned char)(bits>>biti);
          
          if(blocki>=_nblocks)blocki=++blockj==_nshort_data?_nshort_blocks:0;
        }
      }
    }
  }
}



struct qr_pack_buf{
  const unsigned char *buf;
  int                  endbyte;
  int                  endbit;
  int                  storage;
};


static void qr_pack_buf_init(qr_pack_buf *_b,
 const unsigned char *_data,int _ndata){
  _b->buf=_data;
  _b->storage=_ndata;
  _b->endbyte=_b->endbit=0;
}


static int qr_pack_buf_read(qr_pack_buf *_b,int _bits){
  const unsigned char *p;
  unsigned             ret;
  int                  m;
  int                  d;
  m=16-_bits;
  _bits+=_b->endbit;
  d=_b->storage-_b->endbyte;
  if(d<=2){
    
    if(d*8<_bits){
      _b->endbyte+=_bits>>3;
      _b->endbit=_bits&7;
      return -1;
    }
    
    else if(!_bits)return 0;
  }
  p=_b->buf+_b->endbyte;
  ret=p[0]<<8+_b->endbit;
  if(_bits>8){
    ret|=p[1]<<_b->endbit;
    if(_bits>16)ret|=p[2]>>8-_b->endbit;
  }
  _b->endbyte+=_bits>>3;
  _b->endbit=_bits&7;
  return (ret&0xFFFF)>>m;
}

static int qr_pack_buf_avail(const qr_pack_buf *_b){
  return (_b->storage-_b->endbyte<<3)-_b->endbit;
}



static const unsigned char QR_ALNUM_TABLE[45]={
  '0','1','2','3','4','5','6','7','8','9',
  'A','B','C','D','E','F','G','H','I','J',
  'K','L','M','N','O','P','Q','R','S','T',
  'U','V','W','X','Y','Z',' ','$','%','*',
  '+','-','.','/',':'
};

static int qr_code_data_parse(qr_code_data *_qrdata,int _version,
 const unsigned char *_data,int _ndata){
  qr_pack_buf qpb;
  unsigned    self_parity;
  int         centries;
  int         len_bits_idx;
  
  _qrdata->entries=NULL;
  _qrdata->nentries=0;
  _qrdata->sa_size=0;
  self_parity=0;
  centries=0;
  
  len_bits_idx=(_version>9)+(_version>26);
  qr_pack_buf_init(&qpb,_data,_ndata);
  
  while(qr_pack_buf_avail(&qpb)>=4){
    qr_code_data_entry *entry;
    int                 mode;
    mode=qr_pack_buf_read(&qpb,4);
    
    if(!mode)break;
    if(_qrdata->nentries>=centries){
      centries=centries<<1|1;
      _qrdata->entries=(qr_code_data_entry *)realloc(_qrdata->entries,
       centries*sizeof(*_qrdata->entries));
    }
    entry=_qrdata->entries+_qrdata->nentries++;
    entry->mode=mode;
    
    entry->payload.data.buf=NULL;
    switch(mode){
      
      static const unsigned char LEN_BITS[3][4]={
        {10, 9, 8, 8},
        {12,11,16,10},
        {14,13,16,12}
      };
      case QR_MODE_NUM:{
        unsigned char *buf;
        unsigned       bits;
        unsigned       c;
        int            len;
        int            count;
        int            rem;
        len=qr_pack_buf_read(&qpb,LEN_BITS[len_bits_idx][0]);
        if(len<0)return -1;
        
        count=len/3;
        rem=len%3;
        if(qr_pack_buf_avail(&qpb)<10*count+7*(rem>>1&1)+4*(rem&1))return -1;
        entry->payload.data.buf=buf=(unsigned char *)malloc(len*sizeof(*buf));
        entry->payload.data.len=len;
        
        while(count-->0){
          bits=qr_pack_buf_read(&qpb,10);
          if(bits>=1000)return -1;
          c='0'+bits/100;
          self_parity^=c;
          *buf++=(unsigned char)c;
          bits%=100;
          c='0'+bits/10;
          self_parity^=c;
          *buf++=(unsigned char)c;
          c='0'+bits%10;
          self_parity^=c;
          *buf++=(unsigned char)c;
        }
        
        if(rem>1){
          bits=qr_pack_buf_read(&qpb,7);
          if(bits>=100)return -1;
          c='0'+bits/10;
          self_parity^=c;
          *buf++=(unsigned char)c;
          c='0'+bits%10;
          self_parity^=c;
          *buf++=(unsigned char)c;
        }
        
        else if(rem){
          bits=qr_pack_buf_read(&qpb,4);
          if(bits>=10)return -1;
          c='0'+bits;
          self_parity^=c;
          *buf++=(unsigned char)c;
        }
      }break;
      case QR_MODE_ALNUM:{
        unsigned char *buf;
        unsigned       bits;
        unsigned       c;
        int            len;
        int            count;
        int            rem;
        len=qr_pack_buf_read(&qpb,LEN_BITS[len_bits_idx][1]);
        if(len<0)return -1;
        
        count=len>>1;
        rem=len&1;
        if(qr_pack_buf_avail(&qpb)<11*count+6*rem)return -1;
        entry->payload.data.buf=buf=(unsigned char *)malloc(len*sizeof(*buf));
        entry->payload.data.len=len;
        
        while(count-->0){
          bits=qr_pack_buf_read(&qpb,11);
          if(bits>=2025)return -1;
          c=QR_ALNUM_TABLE[bits/45];
          self_parity^=c;
          *buf++=(unsigned char)c;
          c=QR_ALNUM_TABLE[bits%45];
          self_parity^=c;
          *buf++=(unsigned char)c;
          len-=2;
        }
        
        if(rem){
          bits=qr_pack_buf_read(&qpb,6);
          if(bits>=45)return -1;
          c=QR_ALNUM_TABLE[bits];
          self_parity^=c;
          *buf++=(unsigned char)c;
        }
      }break;
      
      case QR_MODE_STRUCT:{
        int bits;
        bits=qr_pack_buf_read(&qpb,16);
        if(bits<0)return -1;
        
        if(_qrdata->sa_size==0){
          _qrdata->sa_index=entry->payload.sa.sa_index=
           (unsigned char)(bits>>12&0xF);
          _qrdata->sa_size=entry->payload.sa.sa_size=
           (unsigned char)((bits>>8&0xF)+1);
          _qrdata->sa_parity=entry->payload.sa.sa_parity=
           (unsigned char)(bits&0xFF);
        }
      }break;
      case QR_MODE_BYTE:{
        unsigned char *buf;
        unsigned       c;
        int            len;
        len=qr_pack_buf_read(&qpb,LEN_BITS[len_bits_idx][2]);
        if(len<0)return -1;
        
        if(qr_pack_buf_avail(&qpb)<len<<3)return -1;
        entry->payload.data.buf=buf=(unsigned char *)malloc(len*sizeof(*buf));
        entry->payload.data.len=len;
        while(len-->0){
          c=qr_pack_buf_read(&qpb,8);
          self_parity^=c;
          *buf++=(unsigned char)c;
        }
      }break;
      
      case QR_MODE_FNC1_1ST:break;
      
      case QR_MODE_ECI:{
        unsigned val;
        int      bits;
        
        bits=qr_pack_buf_read(&qpb,8);
        if(bits<0)return -1;
        
        if(!(bits&0x80))val=bits;
        
        else if(!(bits&0x40)){
          val=bits&0x3F<<8;
          bits=qr_pack_buf_read(&qpb,8);
          if(bits<0)return -1;
          val|=bits;
        }
        
        else if(!(bits&0x20)){
          val=bits&0x1F<<16;
          bits=qr_pack_buf_read(&qpb,16);
          if(bits<0)return -1;
          val|=bits;
          
          if(val>=1000000)return -1;
        }
        
        else return -1;
        entry->payload.eci=val;
      }break;
      case QR_MODE_KANJI:{
        unsigned char *buf;
        unsigned       bits;
        int            len;
        len=qr_pack_buf_read(&qpb,LEN_BITS[len_bits_idx][3]);
        if(len<0)return -1;
        
        if(qr_pack_buf_avail(&qpb)<13*len)return -1;
        entry->payload.data.buf=buf=(unsigned char *)malloc(2*len*sizeof(*buf));
        entry->payload.data.len=2*len;
        
        while(len-->0){
          bits=qr_pack_buf_read(&qpb,13);
          bits=(bits/0xC0<<8|bits%0xC0)+0x8140;
          if(bits>=0xA000)bits+=0x4000;
          
          self_parity^=bits;
          *buf++=(unsigned char)(bits>>8);
          *buf++=(unsigned char)(bits&0xFF);
        }
      }break;
      
      case QR_MODE_FNC1_2ND:{
        int bits;
        
        bits=qr_pack_buf_read(&qpb,8);
        if(!(bits>=0&&bits<100||bits>=165&&bits<191||bits>=197&&bits<223)){
          return -1;
        }
        entry->payload.ai=bits;
      }break;
      
      default:{
        
        return -1;
      }break;
    }
  }
  
  _qrdata->self_parity=((self_parity>>8)^self_parity)&0xFF;
  
  _qrdata->entries=(qr_code_data_entry *)realloc(_qrdata->entries,
   _qrdata->nentries*sizeof(*_qrdata->entries));
  return 0;
}

static void qr_code_data_clear(qr_code_data *_qrdata){
  int i;
  for(i=0;i<_qrdata->nentries;i++){
    if(QR_MODE_HAS_DATA(_qrdata->entries[i].mode)){
      free(_qrdata->entries[i].payload.data.buf);
    }
  }
  free(_qrdata->entries);
}


void qr_code_data_list_init(qr_code_data_list *_qrlist){
  _qrlist->qrdata=NULL;
  _qrlist->nqrdata=_qrlist->cqrdata=0;
}

void qr_code_data_list_clear(qr_code_data_list *_qrlist){
  int i;
  for(i=0;i<_qrlist->nqrdata;i++)qr_code_data_clear(_qrlist->qrdata+i);
  free(_qrlist->qrdata);
  qr_code_data_list_init(_qrlist);
}

static void qr_code_data_list_add(qr_code_data_list *_qrlist,
 qr_code_data *_qrdata){
  if(_qrlist->nqrdata>=_qrlist->cqrdata){
    _qrlist->cqrdata=_qrlist->cqrdata<<1|1;
    _qrlist->qrdata=(qr_code_data *)realloc(_qrlist->qrdata,
     _qrlist->cqrdata*sizeof(*_qrlist->qrdata));
  }
  memcpy(_qrlist->qrdata+_qrlist->nqrdata++,_qrdata,sizeof(*_qrdata));
}

#if 0
static const unsigned short QR_NCODEWORDS[40]={
    26,  44,  70, 100, 134, 172, 196, 242, 292, 346,
   404, 466, 532, 581, 655, 733, 815, 901, 991,1085,
  1156,1258,1364,1474,1588,1706,1828,1921,2051,2185,
  2323,2465,2611,2761,2876,3034,3196,3362,3532,3706
};
#endif


static int qr_code_ncodewords(unsigned _version){
  unsigned nalign;
  
  if(_version==1)return 26;
  nalign=(_version/7)+2;
  return (_version<<4)*(_version+8)
   -(5*nalign)*(5*nalign-2)+36*(_version<7)+83>>3;
}

#if 0

static const unsigned char QR_RS_NPAR[40][4]={
  { 7,10,13,17},{10,16,22,28},{15,26,18,22},{20,18,26,16},
  {26,24,18,22},{18,16,24,28},{20,18,18,26},{24,22,22,26},
  {30,22,20,24},{18,26,24,28},{20,30,28,24},{24,22,26,28},
  {26,22,24,22},{30,24,20,24},{22,24,30,24},{24,28,24,30},
  {28,28,28,28},{30,26,28,28},{28,26,26,26},{28,26,30,28},
  {28,26,28,30},{28,28,30,24},{30,28,30,30},{30,28,30,30},
  {26,28,30,30},{28,28,28,30},{30,28,30,30},{30,28,30,30},
  {30,28,30,30},{30,28,30,30},{30,28,30,30},{30,28,30,30},
  {30,28,30,30},{30,28,30,30},{30,28,30,30},{30,28,30,30},
  {30,28,30,30},{30,28,30,30},{30,28,30,30},{30,28,30,30}
};
#endif


static const unsigned char QR_RS_NPAR_VALS[71]={
   7,10,13,17,
  10,16,22, 28,26,26, 26,22, 24,22,22, 26,24,18,22,
  15,26,18, 22,24, 30,24,20,24,
  18,16,24, 28, 28, 28,28,30,24,
  20,18, 18,26, 24,28,24, 30,26,28, 28, 26,28,30, 30,22,20,24,
  20,18,26,16,
  20,30,28, 24,22,26, 28,26, 30,28,30,30
};


static const unsigned char QR_RS_NPAR_OFFS[40]={
   0, 4,19,55,15,28,37,12,51,39,
  59,62,10,24,22,41,31,44, 7,65,
  47,33,67,67,48,32,67,67,67,67,
  67,67,67,67,67,67,67,67,67,67
};


static const unsigned char QR_RS_NBLOCKS[40][4]={
  { 1, 1, 1, 1},{ 1, 1, 1, 1},{ 1, 1, 2, 2},{ 1, 2, 2, 4},
  { 1, 2, 4, 4},{ 2, 4, 4, 4},{ 2, 4, 6, 5},{ 2, 4, 6, 6},
  { 2, 5, 8, 8},{ 4, 5, 8, 8},{ 4, 5, 8,11},{ 4, 8,10,11},
  { 4, 9,12,16},{ 4, 9,16,16},{ 6,10,12,18},{ 6,10,17,16},
  { 6,11,16,19},{ 6,13,18,21},{ 7,14,21,25},{ 8,16,20,25},
  { 8,17,23,25},{ 9,17,23,34},{ 9,18,25,30},{10,20,27,32},
  {12,21,29,35},{12,23,34,37},{12,25,34,40},{13,26,35,42},
  {14,28,38,45},{15,29,40,48},{16,31,43,51},{17,33,45,54},
  {18,35,48,57},{19,37,51,60},{19,38,53,63},{20,40,56,66},
  {21,43,59,70},{22,45,62,74},{24,47,65,77},{25,49,68,81}
};


static int qr_code_decode(qr_code_data *_qrdata,const rs_gf256 *_gf,
 const qr_point _ul_pos,const qr_point _ur_pos,const qr_point _dl_pos,
 int _version,int _fmt_info,
 const unsigned char *_img,int _width,int _height){
  qr_sampling_grid   grid;
  unsigned          *data_bits;
  unsigned char    **blocks;
  unsigned char     *block_data;
  int                nblocks;
  int                nshort_blocks;
  int                ncodewords;
  int                block_sz;
  int                ecc_level;
  int                ndata;
  int                npar;
  int                dim;
  int                ret;
  int                i;
  
  qr_sampling_grid_init(&grid,_version,_ul_pos,_ur_pos,_dl_pos,_qrdata->bbox,
   _img,_width,_height);
#if defined(QR_DEBUG)
  qr_sampling_grid_dump(&grid,_version,_img,_width,_height);
#endif
  dim=17+(_version<<2);
  data_bits=(unsigned *)malloc(
   dim*(dim+QR_INT_BITS-1>>QR_INT_LOGBITS)*sizeof(*data_bits));
  qr_sampling_grid_sample(&grid,data_bits,dim,_fmt_info,_img,_width,_height);
  
  ecc_level=(_fmt_info>>3)^1;
  nblocks=QR_RS_NBLOCKS[_version-1][ecc_level];
  npar=*(QR_RS_NPAR_VALS+QR_RS_NPAR_OFFS[_version-1]+ecc_level);
  ncodewords=qr_code_ncodewords(_version);
  block_sz=ncodewords/nblocks;
  nshort_blocks=nblocks-(ncodewords%nblocks);
  blocks=(unsigned char **)malloc(nblocks*sizeof(*blocks));
  block_data=(unsigned char *)malloc(ncodewords*sizeof(*block_data));
  blocks[0]=block_data;
  for(i=1;i<nblocks;i++)blocks[i]=blocks[i-1]+block_sz+(i>nshort_blocks);
  qr_samples_unpack(blocks,nblocks,block_sz-npar,nshort_blocks,
   data_bits,grid.fpmask,dim);
  qr_sampling_grid_clear(&grid);
  free(blocks);
  free(data_bits);
  
  ndata=0;
  ncodewords=0;
  ret=0;
  for(i=0;i<nblocks;i++){
    int block_szi;
    int ndatai;
    block_szi=block_sz+(i>=nshort_blocks);
    ret=rs_correct(_gf,QR_M0,block_data+ncodewords,block_szi,npar,NULL,0);
    
    if(ret<0||_version==1&&ret>ecc_level+1<<1||
     _version==2&&ecc_level==0&&ret>4){
      ret=-1;
      break;
    }
    ndatai=block_szi-npar;
    memmove(block_data+ndata,block_data+ncodewords,ndatai*sizeof(*block_data));
    ncodewords+=block_szi;
    ndata+=ndatai;
  }
  
  if(ret>=0){
    ret=qr_code_data_parse(_qrdata,_version,block_data,ndata);
    
    if(ret<0)qr_code_data_clear(_qrdata);
    _qrdata->version=_version;
    _qrdata->ecc_level=ecc_level;
  }
  free(block_data);
  return ret;
}


static int qr_reader_try_configuration(qr_reader *_reader,
 qr_code_data *_qrdata,const unsigned char *_img,int _width,int _height,
 qr_finder_center *_c[3]){
  int      ci[7];
  unsigned maxd;
  int      ccw;
  int      i0;
  int      i;
  
  ccw=qr_point_ccw(_c[0]->pos,_c[1]->pos,_c[2]->pos);
  
  if(!ccw)return -1;
  
  ci[6]=ci[3]=ci[0]=0;
  ci[4]=ci[1]=1+(ccw<0);
  ci[5]=ci[2]=2-(ccw<0);
  
  maxd=qr_point_distance2(_c[1]->pos,_c[2]->pos);
  i0=0;
  for(i=1;i<3;i++){
    unsigned d;
    d=qr_point_distance2(_c[ci[i+1]]->pos,_c[ci[i+2]]->pos);
    if(d>maxd){
      i0=i;
      maxd=d;
    }
  }
  
  for(i=i0;i<i0+3;i++){
    qr_aff    aff;
    qr_hom    hom;
    qr_finder ul;
    qr_finder ur;
    qr_finder dl;
    qr_point  bbox[4];
    int       res;
    int       ur_version;
    int       dl_version;
    int       fmt_info;
    ul.c=_c[ci[i]];
    ur.c=_c[ci[i+1]];
    dl.c=_c[ci[i+2]];
    
    res=QR_INT_BITS-2-QR_FINDER_SUBPREC-qr_ilog(QR_MAXI(_width,_height)-1);
    qr_aff_init(&aff,ul.c->pos,ur.c->pos,dl.c->pos,res);
    qr_aff_unproject(ur.o,&aff,ur.c->pos[0],ur.c->pos[1]);
    qr_finder_edge_pts_aff_classify(&ur,&aff);
    if(qr_finder_estimate_module_size_and_version(&ur,1<<res,1<<res)<0)continue;
    qr_aff_unproject(dl.o,&aff,dl.c->pos[0],dl.c->pos[1]);
    qr_finder_edge_pts_aff_classify(&dl,&aff);
    if(qr_finder_estimate_module_size_and_version(&dl,1<<res,1<<res)<0)continue;
    
    if(abs(ur.eversion[1]-dl.eversion[0])>QR_LARGE_VERSION_SLACK)continue;
    qr_aff_unproject(ul.o,&aff,ul.c->pos[0],ul.c->pos[1]);
    qr_finder_edge_pts_aff_classify(&ul,&aff);
    if(qr_finder_estimate_module_size_and_version(&ul,1<<res,1<<res)<0||
     abs(ul.eversion[1]-ur.eversion[1])>QR_LARGE_VERSION_SLACK||
     abs(ul.eversion[0]-dl.eversion[0])>QR_LARGE_VERSION_SLACK){
      continue;
    }
#if defined(QR_DEBUG)
    qr_finder_dump_aff_undistorted(&ul,&ur,&dl,&aff,_img,_width,_height);
#endif
    
    if(qr_hom_fit(&hom,&ul,&ur,&dl,bbox,&aff,
     &_reader->isaac,_img,_width,_height)<0){
      continue;
    }
    memcpy(_qrdata->bbox,bbox,sizeof(bbox));
    qr_hom_unproject(ul.o,&hom,ul.c->pos[0],ul.c->pos[1]);
    qr_hom_unproject(ur.o,&hom,ur.c->pos[0],ur.c->pos[1]);
    qr_hom_unproject(dl.o,&hom,dl.c->pos[0],dl.c->pos[1]);
    qr_finder_edge_pts_hom_classify(&ur,&hom);
    if(qr_finder_estimate_module_size_and_version(&ur,
     ur.o[0]-ul.o[0],ur.o[0]-ul.o[0])<0){
      continue;
    }
    qr_finder_edge_pts_hom_classify(&dl,&hom);
    if(qr_finder_estimate_module_size_and_version(&dl,
     dl.o[1]-ul.o[1],dl.o[1]-ul.o[1])<0){
      continue;
    }
#if defined(QR_DEBUG)
    qr_finder_dump_hom_undistorted(&ul,&ur,&dl,&hom,_img,_width,_height);
#endif
    
    if(ur.eversion[1]==dl.eversion[0]&&ur.eversion[1]<7){
      
#if 0
      static const signed char LINE_TESTS[12][6]={
        
        {2,0,0, 1,1, 1},
        
        {2,1,0, 1,1,-1},
        
        {1,2,0, 1,2, 1},
        
        {1,3,0, 1,2,-1},
        
        {1,0,2,-1,0,-1},
        
        {1,1,2, 1,0, 1},
        
        {2,2,1,-1,0,-1},
        
        {2,3,1, 1,0, 1},
        
        {0,0,2, 1,1, 1},
        
        {0,1,2, 1,1,-1},
        
        {0,2,1, 1,2, 1},
        
        {0,3,1, 1,2,-1}
      };
      qr_finder *f[3];
      int        j;
      
      fmt_info=qr_finder_fmt_info_decode(&ul,&ur,&dl,&aff,_img,_width,_height);
      if(fmt_info<0)continue;
      
      f[0]=&ul;
      f[1]=&ur;
      f[2]=&dl;
      for(j=0;j<12;j++){
        const signed char *t;
        qr_line            l0;
        int               *p;
        t=LINE_TESTS[j];
        qr_finder_ransac(f[t[0]],&aff,&_reader->isaac,t[1]);
        
        if(qr_line_fit_finder_edge(l0,f[t[0]],t[1],res)<0)continue;
        p=f[t[2]]->c->pos;
        if(qr_line_eval(l0,p[0],p[1])*t[3]<0)break;
        p=f[t[4]]->c->pos;
        if(qr_line_eval(l0,p[0],p[1])*t[5]<0)break;
      }
      if(j<12)continue;
      
#endif
      ur_version=ur.eversion[1];
    }
    else{
      
      if(abs(ur.eversion[1]-dl.eversion[0])>QR_LARGE_VERSION_SLACK)continue;
      
      if(ur.eversion[1]>=7-QR_LARGE_VERSION_SLACK){
        ur_version=qr_finder_version_decode(&ur,&hom,_img,_width,_height,0);
        if(abs(ur_version-ur.eversion[1])>QR_LARGE_VERSION_SLACK)ur_version=-1;
      }
      else ur_version=-1;
      if(dl.eversion[0]>=7-QR_LARGE_VERSION_SLACK){
        dl_version=qr_finder_version_decode(&dl,&hom,_img,_width,_height,1);
        if(abs(dl_version-dl.eversion[0])>QR_LARGE_VERSION_SLACK)dl_version=-1;
      }
      else dl_version=-1;
      
      if(ur_version>=0){
        if(dl_version>=0&&dl_version!=ur_version)continue;
      }
      else if(dl_version<0)continue;
      else ur_version=dl_version;
    }
    qr_finder_edge_pts_hom_classify(&ul,&hom);
    if(qr_finder_estimate_module_size_and_version(&ul,
     ur.o[0]-dl.o[0],dl.o[1]-ul.o[1])<0||
     abs(ul.eversion[1]-ur.eversion[1])>QR_SMALL_VERSION_SLACK||
     abs(ul.eversion[0]-dl.eversion[0])>QR_SMALL_VERSION_SLACK){
      continue;
    }
    fmt_info=qr_finder_fmt_info_decode(&ul,&ur,&dl,&hom,_img,_width,_height);
    if(fmt_info<0||
     qr_code_decode(_qrdata,&_reader->gf,ul.c->pos,ur.c->pos,dl.c->pos,
     ur_version,fmt_info,_img,_width,_height)<0){
      
      QR_SWAP2I(hom.inv[0][0],hom.inv[1][0]);
      QR_SWAP2I(hom.inv[0][1],hom.inv[1][1]);
      QR_SWAP2I(hom.fwd[0][0],hom.fwd[0][1]);
      QR_SWAP2I(hom.fwd[1][0],hom.fwd[1][1]);
      QR_SWAP2I(hom.fwd[2][0],hom.fwd[2][1]);
      QR_SWAP2I(ul.o[0],ul.o[1]);
      QR_SWAP2I(ul.size[0],ul.size[1]);
      QR_SWAP2I(ur.o[0],ur.o[1]);
      QR_SWAP2I(ur.size[0],ur.size[1]);
      QR_SWAP2I(dl.o[0],dl.o[1]);
      QR_SWAP2I(dl.size[0],dl.size[1]);
#if defined(QR_DEBUG)
      qr_finder_dump_hom_undistorted(&ul,&dl,&ur,&hom,_img,_width,_height);
#endif
      fmt_info=qr_finder_fmt_info_decode(&ul,&dl,&ur,&hom,_img,_width,_height);
      if(fmt_info<0)continue;
      QR_SWAP2I(bbox[1][0],bbox[2][0]);
      QR_SWAP2I(bbox[1][1],bbox[2][1]);
      memcpy(_qrdata->bbox,bbox,sizeof(bbox));
      if(qr_code_decode(_qrdata,&_reader->gf,ul.c->pos,dl.c->pos,ur.c->pos,
       ur_version,fmt_info,_img,_width,_height)<0){
        continue;
      }
    }
    return ur_version;
  }
  return -1;
}

void qr_reader_match_centers(qr_reader *_reader,qr_code_data_list *_qrlist,
 qr_finder_center *_centers,int _ncenters,
 const unsigned char *_img,int _width,int _height){
  
  unsigned char *mark;
  int            nfailures_max;
  int            nfailures;
  int            i;
  int            j;
  int            k;
  mark=(unsigned char *)calloc(_ncenters,sizeof(*mark));
  nfailures_max=QR_MAXI(8192,_width*_height>>9);
  nfailures=0;
  for(i=0;i<_ncenters;i++){
    
    for(j=i+1;!mark[i]&&j<_ncenters;j++){
      for(k=j+1;!mark[j]&&k<_ncenters;k++)if(!mark[k]){
        qr_finder_center *c[3];
        qr_code_data      qrdata;
        int               version;
        c[0]=_centers+i;
        c[1]=_centers+j;
        c[2]=_centers+k;
        version=qr_reader_try_configuration(_reader,&qrdata,
         _img,_width,_height,c);
        if(version>=0){
          int ninside;
          int l;
          
          qr_code_data_list_add(_qrlist,&qrdata);
          
          for(l=0;l<4;l++){
            _qrlist->qrdata[_qrlist->nqrdata-1].bbox[l][0]>>=QR_FINDER_SUBPREC;
            _qrlist->qrdata[_qrlist->nqrdata-1].bbox[l][1]>>=QR_FINDER_SUBPREC;
          }
          
          mark[i]=mark[j]=mark[k]=1;
          
          for(l=ninside=0;l<_ncenters;l++)if(!mark[l]){
            if(qr_point_ccw(qrdata.bbox[0],qrdata.bbox[1],_centers[l].pos)>=0&&
             qr_point_ccw(qrdata.bbox[1],qrdata.bbox[3],_centers[l].pos)>=0&&
             qr_point_ccw(qrdata.bbox[3],qrdata.bbox[2],_centers[l].pos)>=0&&
             qr_point_ccw(qrdata.bbox[2],qrdata.bbox[0],_centers[l].pos)>=0){
              mark[l]=2;
              ninside++;
            }
          }
          if(ninside>=3){
            
            qr_finder_center *inside;
            inside=(qr_finder_center *)malloc(ninside*sizeof(*inside));
            for(l=ninside=0;l<_ncenters;l++){
              if(mark[l]==2)*&inside[ninside++]=*&_centers[l];
            }
            qr_reader_match_centers(_reader,_qrlist,inside,ninside,
             _img,_width,_height);
            free(inside);
          }
          
          for(l=0;l<_ncenters;l++)if(mark[l]==2)mark[l]=1;
          nfailures=0;
        }
        else if(++nfailures>nfailures_max){
          
          i=j=k=_ncenters;
        }
      }
    }
  }
  free(mark);
}

int _zbar_qr_found_line (qr_reader *reader,
                         int dir,
                         const qr_finder_line *line)
{
    
    qr_finder_lines *lines = &reader->finder_lines[dir];

    if(lines->nlines >= lines->clines) {
        lines->clines *= 2;
        lines->lines = realloc(lines->lines,
                               ++lines->clines * sizeof(*lines->lines));
    }

    memcpy(lines->lines + lines->nlines++, line, sizeof(*line));

    return(0);
}

static inline void qr_svg_centers (const qr_finder_center *centers,
                                   int ncenters)
{
    int i, j;
    svg_path_start("centers", 1, 0, 0);
    for(i = 0; i < ncenters; i++)
        svg_path_moveto(SVG_ABS, centers[i].pos[0], centers[i].pos[1]);
    svg_path_end();

    svg_path_start("edge-pts", 1, 0, 0);
    for(i = 0; i < ncenters; i++) {
        const qr_finder_center *cen = centers + i;
        for(j = 0; j < cen->nedge_pts; j++)
            svg_path_moveto(SVG_ABS,
                            cen->edge_pts[j].pos[0], cen->edge_pts[j].pos[1]);
    }
    svg_path_end();
}

int _zbar_qr_decode (qr_reader *reader,
                     zbar_image_scanner_t *iscn,
                     zbar_image_t *img)
{
    int nqrdata = 0, ncenters;
    qr_finder_edge_pt *edge_pts = NULL;
    qr_finder_center *centers = NULL;

    if(reader->finder_lines[0].nlines < 9 ||
       reader->finder_lines[1].nlines < 9)
        return(0);

    svg_group_start("finder", 0, 1. / (1 << QR_FINDER_SUBPREC), 0, 0, 0);

    ncenters = qr_finder_centers_locate(&centers, &edge_pts, reader, 0, 0);

    zprintf(14, "%dx%d finders, %d centers:\n",
            reader->finder_lines[0].nlines,
            reader->finder_lines[1].nlines,
            ncenters);
    qr_svg_centers(centers, ncenters);

    if(ncenters >= 3) {
        void *bin = qr_binarize(img->data, img->width, img->height);

        qr_code_data_list qrlist;
        qr_code_data_list_init(&qrlist);

        qr_reader_match_centers(reader, &qrlist, centers, ncenters,
                                bin, img->width, img->height);

        if(qrlist.nqrdata > 0)
            nqrdata = qr_code_data_list_extract_text(&qrlist, iscn, img);

        qr_code_data_list_clear(&qrlist);
        free(bin);
    }
    svg_group_end();

    if(centers)
        free(centers);
    if(edge_pts)
        free(edge_pts);
    return(nqrdata);
}
