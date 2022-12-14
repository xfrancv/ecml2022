/* NOTE: Compiling MEX under Windows requires compiler option -Za */

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "libadag.hpp"


int printf0(const char *format, ...)
{
    return(0);
}

/*================================================================================================*/

#define Printf printf0
//#define Printf printf
#define Error(s) { Printf("\nERROR on line %i: %s\n",__LINE__,s); exit(1); }
#define Bug(s) { Printf("\nBUG on line %i: %s\n",__LINE__,s); exit(1); }
#define Warning(s) { Printf("\nWARNING on line %i: %s\n",__LINE__,s); }
static void *Alloc( unsigned n )
{
  void *m= malloc(n);
  if ( !m ) Error("Out of memory.");
  return m;
}
#define Free(m) free(m)

/*================================================================================================*/



/*================================================================================================*/
typedef unsigned char boolean;
typedef unsigned char type_n;
//static type_n ALIVE= ((type_n)-1), NONMAX= (ALIVE-1);
#define ALIVE ((type_n)-1)
#define NONMAX (ALIVE-1)
#define type_g int
#define MAX_type_g  ( (type_g)(((unsigned type_g)-1)/2) )
typedef type_g type_df;
#define MAX_type_df MAX_type_g 
typedef unsigned short type_k;
#define BUMPER ((type_k)-1)

typedef struct {
  unsigned t;
  type_k k;
} type_tk;

typedef struct {
  type_k kk;
  type_g gg;
} type_kgg;
/*================================================================================================*/


/*==================================================================================================*/
class Stack {
public:
  unsigned len; /* max number of elements in the stack */
  unsigned top; /* index of the first free element */
  type_tk *data;
  
  Stack();
  void realloc( unsigned );
  ~Stack() { Free(data); }
  char *print();

  void pop( unsigned *t, type_k *k )
  {
    type_tk *_data= data + --top;
    *t= _data->t;
    *k= _data->k;
  }
  
  void push( unsigned t, type_k k )
  {
    type_tk *_data= data + top;
    _data->t= t;
    _data->k= k;
    if ( ++top == len )
      realloc(2*len);
  }
};

Stack::Stack()
{
  len= 100;
  data= (type_tk*)Alloc(len*sizeof(type_tk));
  top= 0;
}

void Stack::realloc( unsigned _len )
{
  if ( _len <= len ) Bug("Smaller length when reallocating stack.");
  len= _len;
  type_tk *_data= (type_tk*)Alloc(len*sizeof(type_tk));
  if ( _data == 0 )  Error("Out of memory.");
  for ( unsigned i=0; i<top; i++ )
    _data[i]= data[i];
  Free(data);
  data= _data;
}

char *Stack::print()
{
  static char str[1000];
  *str= 0;
  for ( unsigned i=0; i<top; i++ )
    sprintf(str,"%s(%i %i) ",str,(int)data[i].t,(int)data[i].k);
  sprintf(str,"%s\n",str);
  return str;
}
/*================================================================================================*/


/*================================================================================================*/
class Queue {
public:
  unsigned len;  /* max number of elements in the queue */
  unsigned tail; /* index of the oldest element in data */
  unsigned head; /* index of the first free element in data */
  type_tk *data;
  
  Queue();
  void realloc( unsigned );
  ~Queue() { Free(data); };
  unsigned length() { return head>=tail ? head-tail : head+len-tail; };
  char *print();

  void get( unsigned *t, type_k *k )
  {
    type_tk *_data= data + tail;
    *t= _data->t;
    *k= _data->k;
    if ( ++tail == len ) tail= 0;
  }
  
  void put( unsigned t, type_k k )
  {
    type_tk *_data= data + head;
    _data->t= t;
    _data->k= k;
    if ( ++head == len ) head= 0;
    if ( head == tail )  realloc(2*len);
  }
};

Queue::Queue()
{
  len= 100;
  data= (type_tk*)Alloc(len*sizeof(type_tk));
  tail= head= 0;
}

void Queue::realloc( unsigned _len )
{
  if ( _len <= len ) Bug("Smaller length when reallocating queue.");
  type_tk *_data= (type_tk*)Alloc(_len*sizeof(type_tk));
  if ( _data == 0 ) Error("Out of memory.");
  unsigned i= tail, j=0;
  do {
    _data[j++]= data[i];
    if ( ++i == len ) i=0;
  } while ( i != head );
  Free(data);
  data= _data;
  tail= 0;
  head= j;
  len= _len;
}

char *Queue::print()
{
  static char str[1000];
  *str= 0;
  unsigned i= tail;
  while ( i != head ) {
    sprintf(str,"%s(%i %i) ",str,(int)data[i].t,(int)data[i].k);
    if ( ++i == len ) i=0;
  };
  sprintf(str,"%s\n",str);
  return str;
}
/*================================================================================================*/


/*================================================================================================
  This class stores a sparse representation of compatibility functions g_{t,tt}(k,kk).
  This representation is suitable for fast access during energy minimization.
  
  There is ngg different functions g_{t,tt}(k,kk). Typically, ngg is smaller than the number of
  object pairs {t,t'}. The different functions are indexed by index c as g_c(k,kk).
  
  Functions g_c(k,kk) typically have many edges with value -infty, which have to be ignored
  during energy minimization.

  Functions g_c(k,kk) can be accessed via the array PP of length 2*ngg as follows:
  (1) For a fixed c and k, the finite values of g_c(k,kk) can be accessed as
        for ( type_kgg *P= PP[0+2*c][k]; P->kk!=BUMBER; P++ ) { kk=P->kk; gg=P->gg; ... }.
  (2) For a fixed c and kk, the finite values of g_c(k,kk) can be accessed as
        for ( type_kgg *P= PP[1+2*c][kk]; P->kk!=BUMBER; P++ ) { k=P->kk; gg=P->gg; ... }.
  
  Values accessed in the loop represent pencil of edges (c,k) (option (1)) or (c,kk) (option (2)).
  The index of the 2*ngg sets of pencils is denoted by cs.
==============================================================================================*/
class Compat {
public:
  unsigned ngg; // number of different functions g_{t,tt}(k,kk)
  type_k *nK;    // vector of length 2*ngg; nK[cs] is number of labels in cs-th pencil set
  type_kgg ***PP, **_PP, *__PP;
    /* PP[cs][k][i] is i-th edge of pencil (cs,k). Each edge set (cs,k) is terminated with PP[cs][k][i].kk==BUMBER.
       PP[cs] is pointer to element in auxilliary array _PP.
       _PP[PP[cs]+k] (which equals PP[cs][k]) is pointer to an element 
       in aux. array __PP, where the edges are actually stored. */
  
  void init( const unsigned, const int *,  const unsigned, const unsigned *, const type_k * );
  void translate( const unsigned, const int * );
  ~Compat();
  void print();
};


/*
  Computes ngg.
  Compute array nK. nK has length 2*ngg, where nK[s+2*c] is the number of labels for
  direction s of the c-th compatibility function g_{t,tt}. For c-th function,
  s==0 means function g_{t,tt} (tail) and s==1 means g_{tt,t} (head).
  Values nK[] are computed from Omega and TnK.
*/
void Compat::init(
  const unsigned nGG, const int *GG,
  const unsigned nOmega, const unsigned *Omega,
  const type_k *TnK /* Array of length nT; TnK[t] is the number of labels of object t. */
)
{
  /* Compute ngg= max(GG(0,:))+1, the number of different compatibility functions g_{t,tt}. */
  ngg= 0;
  for ( unsigned i=0; i<nGG; i++ ) if ( (unsigned)GG[0+4*i]+1 > ngg ) ngg= (unsigned)GG[0+4*i]+1;
  //Printf("ngg=%i\n",ngg);
  
  /* Compute nK. */
  nK= (type_k*)Alloc(2*ngg*sizeof(type_k));
  for ( unsigned c=0; c<2*ngg; nK[c++]=0 );
  const unsigned *Om= Omega;
  for ( unsigned e=0;  e<nOmega;  e++, Om+=3 ) {
    const unsigned t0= Om[0], t1= Om[1], c=Om[2];
    if ( TnK[t0] > nK[0+2*c] ) nK[0+2*c]= TnK[t0];
    if ( TnK[t1] > nK[1+2*c] ) nK[1+2*c]= TnK[t1];
  }

  /* Check if no label index in GG is greater than the corresponding value in nK. */
  const int *GGi= GG;
  for ( unsigned i=0;  i<nGG;  i++, GGi+=4 )
    if ( GGi[0+1] > nK[0+2*GGi[0]] ||
         GGi[1+1] > nK[1+2*GGi[0]] )
      Error("Some index in GG greater than corresponding value in nK.");
}


/*
  Translates the representation of compatibility function by array GG (supplied by user)
  to internal representation of class Compat, suitable to fast access.

  GG is a linearly stored 4-by-nGG array with the following format:
  GG = [c k kk gg; c k kk gg; ...] where gg is value of g_c(k,kk).
*/
void Compat::translate( const unsigned nGG, const int *GG )
{
  //for ( unsigned c=0; c<4; c++ ) { for ( unsigned i=0; i<nGG; i++ ) Printf("%i ",GG[c+4*i]); Printf("\n"); } Printf("\n");
  
  /* Compute number of all pencils, nP=sum(nK). */
  unsigned nP= 0;
  for ( unsigned cs=0; cs<2*ngg; cs++ ) nP+= nK[cs];
  
  /* Allocate arrays nKK and _nKK.
     nKK[cs][k] is the number of finite edges in pencil (cs,k).
     nKK[cs] is pointer to auxilliary array _nKK where the numbers are actually stored.
     It holds that sum(_nKK)==2*nGG. */
  type_k **nKK= (type_k**)Alloc(2*ngg*sizeof(type_k*));
  type_k *_nKK= (type_k*)Alloc(nP*sizeof(type_k));
  
  /* Allocate arrays PP, _PP and __PP. */
  PP= (type_kgg***)Alloc(2*ngg*sizeof(type_kgg**));
  _PP= (type_kgg**)Alloc(nP*sizeof(type_kgg*));
  __PP= (type_kgg*)Alloc((2*nGG+nP)*sizeof(type_kgg));
  
  /* Fill arrays nKK and PP. */
  nP= 0;
  for ( unsigned cs=0; cs<2*ngg; cs++ ) {
    nKK[cs]= _nKK + nP;
    PP[cs]= _PP + nP;
    nP+= nK[cs];
  }
  
  /* Fill array _nKK (pointed to by nKK[]). */
  for ( unsigned p=0; p<nP; _nKK[p++]=0 );
  for ( unsigned i=0; i<nGG; i++ ) {
    const int *GGi= GG + 4*i;
    for ( unsigned s=0; s<2; s++ ) {
      unsigned cs= s + 2*GGi[0];
      type_k k= GGi[s+1];
      nKK[cs][k]++;
    }
  }
  /*for ( unsigned cs=0; cs<2*ngg; cs++ ) {
    for ( type_k k=0; k<nK[cs]; k++ )
      Printf("%i ",nKK[cs][k]);
    Printf("\n");
}*/
  
  /* Fill array _PP. Fill BUMPERs in array __PP. */
  unsigned i= 0;
  for ( unsigned p=0; p<nP; p++ ) {
    _PP[p]= __PP + i;
    i+= _nKK[p];
    __PP[i++].kk= BUMPER;
  }
  
  /* Fill array __PP (pointed to by PP[][]). */
  for ( unsigned p=0; p<nP; _nKK[p++]=0 );
  for ( unsigned i=0; i<nGG; i++ ) {
    const int *GGi= GG + 4*i;
    for ( unsigned s=0; s<2; s++ ) {
      unsigned cs= s + 2*GGi[0];
      type_k k= GGi[s+1], kk= GGi[(s^1)+1];
      type_kgg *kgg= PP[cs][k] + nKK[cs][k]++;
      kgg->kk= kk;
      kgg->gg= GGi[3];
    }
  }
  //for ( unsigned i=0; i<2*nGG+nP; i++ ) Printf("[%i %i] ",__PP[i].kk,__PP[i].gg); Printf("\n");
  
  Free(_nKK);
  Free(nKK);
}


Compat::~Compat()
{
  Free(__PP);
  Free(_PP);
  Free(PP);
  Free(nK);
}

void Compat::print()
{
  for ( unsigned cs=0; cs<2*ngg; cs++ ) {
    Printf("cs=%i:\n",cs);
    for ( type_k k=0; k<nK[cs]; k++ ) {
      Printf("  k=%i:  ",k);
      for ( type_kgg *P= PP[cs][k]; P->kk!=BUMPER; P++ )
        Printf("[%i %i] ",P->kk,P->gg);
      Printf("\n");
    }
  }
}
/*================================================================================================*/


/*================================================================================================*/
class Maxsum {
public:

  typedef struct {
    const type_kgg **P;
    /* Pointer to pencil set.
       It provides access to K_{t,tt}(k) and to set { g_{t,tt}(k,kk) | k \in K_{t,tt}(k) }.
       Usage idiom: for ( type_kgg *P= N.P[k]; P->kk!=BUMPER; P++ ) { kk=P->kk; gg=P->gg; ... } */
    type_g theta;   /* \theta_{t,tt} */
    type_g *f;      /* .f[k] is potential \phi_{t,tt}(k) */
    type_df *df;    /* .df[k] is potential direction \Delta\phi_{t,tt}(k) */
    unsigned t;     /* neighboring object */
    type_n n;
      /* `reverse neighbor index', defined for each ordered pair (t,tt) by equalities
         tt == T[t].N[n].t
         nn == T[t].N[n].n
         t == T[tt].N[nn].t
         n == T[tt].N[nn].n */
  } Neighbor;
  
  typedef struct {
    unsigned short v;
      /* In function 'direction', v stores value d_t(k) being indegree of node (t,k).
         Otherwise, v is a boolean indicating whether node (t,k) is in queue V. */
    type_n p;  /* index of a neighbor (index to T[t].N), or ALIVE, or NONMAX */
  } Node;
  
  typedef struct {
    type_g h;      /* height */
    const type_g *g;  /* .g[k] is value of g_t(k), k=0..nK-1 */
    type_g theta;  /* threshold */
    Neighbor *N;   /* array of neighbors */
    Node *K;       /* array of labels */
    type_n nN;     /* number of neighbours */
    type_k nK;     /* number of labels */
    type_k n;      /* number of live labels */
  } Object;
  
  /* Aux. data type for (de-)allocating class Maxsum.
     Contains pointers to allocated memory blocks. */
  typedef struct {
    Neighbor *N;
    Node *K;
    type_df *df;
  } type_alloc;
  
  unsigned nT, iter, step_iter;
  Object *T;
  Queue V;
  Stack S0, S;
  Compat C;
  type_alloc A;
  
  Maxsum( const unsigned, const unsigned *, const type_k *, const unsigned, const int *, const type_g *, type_g * );  
  ~Maxsum();
  double minimize( type_g );
  void unique_labels( type_k * );
  void check() { check_gg(); check_h(); check_n(); check_p(); check_consist(); };
  
private:
  void init( type_g );
  unsigned relax();
  unsigned direction( unsigned );
  void step( unsigned, type_g*, int*, type_g*, unsigned*, type_k* );
  void update( type_g );
  void repair( unsigned, type_g );
  void thupdate( unsigned, type_g, unsigned, type_k );
  void ressurect();
  
  char str[1000]; // just for printing during debugging
  char *print_gf( unsigned );
  char *print_ggf( unsigned, type_n );
  char *print_f( unsigned );
  void check_gg();
  void check_h();
  void check_n();
  void check_p();
  void check_consist();
  void check_df( Stack*, unsigned );
};
/*================================================================================================*/


/*======== PRINT FUNCTIONS FOR DEBUGGING =========================================================*/
char *Maxsum::print_gf( unsigned t )
{
  if ( t<0 || t>=nT ) return 0;
  sprintf(str,"\ngf( t=%i, h=%g, th=%g )=\n",t,(double)T[t].h,(double)T[t].theta);
  for ( int k=0;  k<T[t].nK;  k++ ) {
    type_g gf= T[t].g[k];
    for ( type_n n=0;  n<T[t].nN;  n++ )
      gf+= T[t].N[n].f[k];
    sprintf(str,"%s  %g  ",str,(double)gf);
    if ( T[t].K[k].p == ALIVE ) sprintf(str,"%sALIVE\n",str);
    else if ( T[t].K[k].p == NONMAX ) sprintf(str,"%sNONMAX\n",str);
    else sprintf(str,"%st=%i(n=%i)\n",str,(int)T[t].N[T[t].K[k].p].t,(int)T[t].K[k].p);
  }
  return str;
}

char *Maxsum::print_ggf( unsigned t, type_n n )
{
  if ( t>=nT || n>=T[t].nN ) Bug("t or n out of range.");
  Neighbor *tn= T[t].N + n;
  sprintf(str,"\nggf( t=%i, tt=%i(n=%i), th=%i )=\n",t,tn->t,n,tn->theta);
  type_kgg kg[1000];
  for ( type_k k=0; k<T[t].nK; k++ ) {
    for ( type_k kk=0; kk<T[tn->t].nK; kg[kk++].kk=BUMPER );
    for ( const type_kgg *P= tn->P[k]; P->kk!=BUMPER; P++ ) kg[P->kk]= *P;
    for ( type_k kk=0; kk<T[tn->t].nK; kk++ ) {
      if ( kg[kk].kk != BUMPER ) {
        type_g ggf= kg[kk].gg - (T[t].N[n].f[k] + T[tn->t].N[tn->n].f[kk]);
        sprintf(str,"%s  %4i%s ",str,ggf,ggf>=-tn->theta?"*":" ");
      }
      else
        sprintf(str,"%s    x   ",str);
    }
    sprintf(str,"%s\n",str);
  }
  return str;
}

char *Maxsum::print_f( unsigned t )
{
  if ( t<0 || t>=nT ) return 0;
  sprintf(str,"\nf/df( t=%i )=\n",t);
  for ( type_n n=0; n<T[t].nN; n++ ) {
    sprintf(str,"%s  tt=%i(n=%i): ",str,T[t].N[n].t,n);
    for ( type_k k=0; k<T[t].nK; k++ )
      sprintf(str,"%s%g/%i  ",str,(double)T[t].N[n].f[k],T[t].N[n].df[k]);
    sprintf(str,"%s\n",str);
  }
  Printf("%s", str);
  return str;
}
/*================================================================================================*/


/*========= TESTING FUNCTIONS FOR DEBUGGING ======================================================*/
void Maxsum::check_gg()
{
  for ( unsigned t=0; t<nT; t++ )
    for ( type_n n=0; n<T[t].nN; n++ ) {
      Neighbor *tn= T[t].N + n;
      for ( type_k k=0;  k<T[t].nK;  k++ )
        for ( const type_kgg *P= tn->P[k]; P->kk!=BUMPER; P++ )
          if ( P->gg - (T[t].N[n].f[k] + T[tn->t].N[tn->n].f[P->kk]) > 0 )
            Error("Infeasible potential.");
    }
}

void Maxsum::check_h()
{
  for ( unsigned t=0;  t<nT;  t++ ) {
    type_g h= -MAX_type_g;
    for ( type_k k=0;  k<T[t].nK;  k++ ) {
      type_g gf= T[t].g[k];  for ( type_n n=0; n<T[t].nN; n++ )  gf+= T[t].N[n].f[k];
      if ( gf > h ) h= gf;
    }
    if ( h > T[t].h )
      Bug("Wrong height.");
  }
}

void Maxsum::check_n()
{
  for ( unsigned t=0;  t<nT;  t++ ) {
    type_n n= 0;
    for ( type_k k=0; k<T[t].nK; k++ )
      if ( T[t].K[k].p == ALIVE )
        n++;
    if ( n != T[t].n )
      Bug("Wrong n.");
  }
}

void Maxsum::check_p()
{
  /* Check that -
    - p_t(k)==NONMAX iff h_t-g^f_t(k)>theta_t
    - p_t(k)==tt  ===>  (p_tt(kk)!=ALIVE && p_tt(kk)!=t) for all kk such that gg_{t,tt}(k,kk)>=-theta_{t,tt}
  */
  for ( unsigned t=0; t<nT; t++ )
    for ( type_k k=0; k<T[t].nK; k++ ) {
      Node *tk= T[t].K + k;
      type_g gf= T[t].g[k];  for ( type_n n=0; n<T[t].nN; n++ )  gf+= T[t].N[n].f[k];
      if ( (T[t].h - gf > T[t].theta) ^ (tk->p == NONMAX) )
        Bug("p==NONMAX inconsistent with h.");
      type_n n= tk->p;
      if ( n!=ALIVE && n!=NONMAX ) {
        Neighbor *tn= T[t].N + n;
        for ( const type_kgg *P= tn->P[k]; P->kk!=BUMPER; P++ )
          if ( P->gg - (tn->f[k] + T[tn->t].N[tn->n].f[P->kk]) >= -tn->theta ) {
            if ( T[tn->t].K[P->kk].p == ALIVE )
              Bug("Pointer to live node.");
            if ( T[tn->t].K[P->kk].p == tn->n )
              Bug("Two nodes pointing at each other in DAG.");
          }
      }
    }
}

/*void Maxsum::check_Vv()
{
  // Set v[t][k] to TRUE iff (t,k) is in V.
  for ( unsigned t=0; t<nT; t++ )  for ( type_k k=0; k<T[t].nK; k++ )  v[t][k]= 0;
  for ( unsigned i=V.tail;  i!=V.head;  i=(i+1)%V.len )
    v[V.data[i].t][V.data[i].k]= 1;

  // Check if node (t,k) is in V iff v_t(k)==TRUE.
  for ( unsigned t=0; t<nT; t++ )
    for ( type_k k=0; k<T[t].nK; k++ ) {
      Node *tk= T[t].K + k;
      if ( v[t][k] != tk->v )
        Bug("Different v and V.");
    }
}*/

void Maxsum::check_consist()
{
  for ( unsigned t=0; t<nT; t++ )
    for ( type_k k=0; k<T[t].nK; k++ ) {
      Node *tk= T[t].K + k;

      /* Check if node in V is alive. */
      if ( tk->v && (tk->p!=ALIVE) )
        Bug("Dead node in V.");

      /* Check if node not in V is consistent. */
      if ( !tk->v && tk->p==ALIVE )
        for ( type_n n=0;  n<T[t].nN;  n++ ) {
          Neighbor *tn= T[t].N + n;
          int alive= 0;
          for ( const type_kgg *P=tn->P[k]; P->kk!=BUMPER; P++ )
            if ( T[tn->t].K[P->kk].p == ALIVE &&
                 P->gg - (tn->f[k] + T[tn->t].N[tn->n].f[P->kk]) >= -tn->theta ) {
              alive= 1;
            }
          if ( !alive )
            Bug("Inconsistent node not in V.");
        }
    }
}

void Maxsum::check_df( Stack *S0, unsigned t0 )
{
  for ( unsigned i=0;  i<S0->top; i++ ) {
    type_tk *data= S0->data + i;
    unsigned t= data->t;
    type_k k =data->k;
    Node *tk= T[t].K + k;

    type_df df= 0;  for ( type_n n=0; n<T[t].nN; n++ )  df+= T[t].N[n].df[k];
    if ( (tk->p!=ALIVE && tk->p!=NONMAX && t==t0 && df!=-1) ||
         (tk->p!=ALIVE && tk->p!=NONMAX && t!=t0 && df>0) ||
         (tk->p==ALIVE                           && df>0) ) {
      Bug("Wrong df on a node.");
    }

    /* Check that for each saturated edge {(t,k),(tt,kk)},
      it is \Delta\phi_{t,tt}(k) + \Delta\phi_{tt,t}(kk) >= 0. */
    unsigned n= tk->p;
    if ( n!=NONMAX && n!=ALIVE ) {
      Neighbor *tn= T[t].N + n;
      for ( const type_kgg *P= tn->P[k]; P->kk!=BUMPER; P++ )
        if ( P->gg - (tn->f[k] + T[tn->t].N[tn->n].f[P->kk]) >= -tn->theta  &&
             tn->df[k] + T[tn->t].N[tn->n].df[P->kk] < 0 ) {
          //Printf("Elem %i of S0. Edge {(%i %i),(%i %i)}. ",i,t,(int)k,tn->t,(int)P->kk);
          Bug("Wrong df on an edge.");
        }
    }
  }
}
/*================================================================================================*/


/*================================================================================================*/
void Maxsum::init( type_g theta )
{
  /* Set all thresholds theta_t and theta_{t,tt} to theta. */
  for ( unsigned t=0; t<nT; t++ ) {
    T[t].theta= theta;
    for ( type_n n=0; n<T[t].nN; n++ ) T[t].N[n].theta= T[t].theta;
  }

  V.tail= V.head= S0.top= S.top= 0;

  for ( unsigned t=0;  t<nT;  t++ ) {
    Object *Tt= T + t;
    Tt->n= 0;
    type_g *h= &Tt->h;
    *h= -MAX_type_g;
    
    for ( type_k k=0;  k<Tt->nK;  k++ ) {
      type_g gf= Tt->g[k];
      for ( type_n n=0; n<Tt->nN; n++ ) {
        gf+= Tt->N[n].f[k];
        Tt->N[n].df[k]= 0;
      }
      if ( gf > *h ) *h= gf;
    }
  }

  for ( unsigned t=0;  t<nT;  t++ ) {
    Object *Tt= T + t;
    Tt->n= 0;
    for ( type_k k=0;  k<Tt->nK;  k++ ) {
      type_g gf= Tt->g[k];
      for ( type_n n=0;  n<Tt->nN;  n++ )
        gf+= Tt->N[n].f[k];
      if ( Tt->h - gf <= Tt->theta ) {
        Tt->K[k].p= ALIVE;
        Tt->n++;
        V.put(t,k);
        Tt->K[k].v= 1;
      }
      else {
        Tt->K[k].p= NONMAX;
        Tt->K[k].v= 0;
      }
    }
  }
}
/*================================================================================================*/


/*================================================================================================*/
unsigned Maxsum::relax()
{
  while ( V.tail != V.head ) {
    unsigned t;
    type_k k;
    V.get(&t,&k);
    Object *Tt= T + t;
    Node *tk= Tt->K + k;
    tk->v= 0;
    if ( tk->p != ALIVE ) Bug("Dead node in V.");

    Neighbor *tn= Tt->N;
    for ( type_n n=0;  n<Tt->nN;  n++, tn++ ) {
      boolean alive= 0;
      for ( const type_kgg *P=tn->P[k]; P->kk!=BUMPER; P++ ) {
        if ( T[tn->t].K[P->kk].p == ALIVE &&
             P->gg - (tn->f[k] + T[tn->t].N[tn->n].f[P->kk]) >= -tn->theta ) {
          alive= 1;
          break;
        }
      }
      if ( !alive ) {
        tk->p= n;
        Tt->n--;
        Neighbor *tn= Tt->N;
        for ( type_n n=0;  n<Tt->nN;  n++, tn++ ) {
          Object *Tn= T + tn->t;
          for ( const type_kgg *P=tn->P[k]; P->kk!=BUMPER; P++ ) {
            Node *ttkk= Tn->K + P->kk;
            if ( ttkk->p==ALIVE &&
                 ttkk->v==0 &&
                 P->gg - (tn->f[k] + Tn->N[tn->n].f[P->kk]) >= -tn->theta ) {
              V.put(tn->t,P->kk);
              ttkk->v= 1;
            }
          }
        }
        if ( T[t].n == 0 )  return t;
        break;
      }
    }
  }
  return 0;
}
/*================================================================================================*/


/*================================================================================================*/
unsigned Maxsum::direction( unsigned t0 )
{
  S.top= 0;
  for ( type_k k=0;  k<T[t0].nK;  k++ )  S.push(t0,k);
  while ( S.top != 0 ) {
    unsigned t;
    type_k k;
    S.pop(&t,&k);
    Object *Tt= T + t;
    type_n n= Tt->K[k].p;
    if ( n != NONMAX ) {
      if ( n == ALIVE ) Bug("Alive e.");
      Neighbor *tn= Tt->N + n;
      type_g fth= tn->f[k] - tn->theta;
      for ( const type_kgg *P=tn->P[k]; P->kk!=BUMPER; P++ ) {
        type_k kk= P->kk;
        if ( P->gg - T[tn->t].N[tn->n].f[kk] >= fth ) {
          if ( (T[tn->t].K[kk].v++) == 0 &&
               tn->t != t0 )
            S.push(tn->t,kk);
        }
      }
    }
  }

  unsigned max_df= 0;
  S.top= 0;
  for ( type_k k=0;  k<T[t0].nK;  k++ ) {
    if ( T[t0].K[k].v == 0 ) S.push(t0,k);
    type_n n= T[t0].K[k].p;
    if ( n != NONMAX ) T[t0].N[n].df[k]= -1;
  }
  S0.top= 0;
  while ( S.top != 0 ) {
    unsigned t;
    type_k k;
    S.pop(&t,&k);
    S0.push(t,k);
    Object *Tt= T + t;
    type_n n= Tt->K[k].p;
    if ( n != NONMAX ) {
      if ( n == ALIVE ) Bug("Alive e.");
      Neighbor *tn= Tt->N + n, *nt= T[tn->t].N + tn->n;

      type_df *df0= tn->df + k;
      for ( type_n nn=0;  nn<Tt->nN;  nn++ )
        if ( nn != n )
          *df0-= Tt->N[nn].df[k];

      unsigned abs_df0= *df0>0 ? *df0 : -*df0;
      if ( abs_df0 > max_df ) max_df= abs_df0;

      type_g fth= tn->f[k] - tn->theta;
      for ( const type_kgg *P=tn->P[k]; P->kk!=BUMPER; P++ ) {
        type_k kk= P->kk;
        if ( P->gg - T[tn->t].N[tn->n].f[kk] >= fth ) {
          if ( (--T[tn->t].K[kk].v)==0 )
            S.push(tn->t,kk);
          type_df *df1= nt->df + kk;
          if ( *df0 + *df1 < 0 )
            *df1= -*df0;
        }
      }
    }
  }
  return max_df; /* maximum abs value of \Delta\phi_{t,tt}(k) */
}
/*================================================================================================*/


/*================================================================================================
  If *lambda>0, then height of t0 can be decreased by *lambda.
  If *lambda==0 && *df_valid, then T[].N[].df are valid but the step is smaller than 1.
  If !*df_valid, then *theta==0 and T[].N[].df are invalid due to an overflow.

  Parameter df_valid is only for debugging purposes; see minimize(...) and check_df(...).
================================================================================================*/
void Maxsum::step(
  unsigned t0,
  type_g *lambda,
  int *df_valid,
  type_g *theta, unsigned *t_theta, type_k *k_theta
)
{
  *df_valid= 1;
  *lambda= *theta= MAX_type_g;
  type_tk *data= S0.data;
  for ( unsigned i=0;  i<S0.top;  i++, data++ ) {
    unsigned t= data->t;
    type_k k= data->k;
    Object *Tt= T + t;
    type_n n= Tt->K[k].p;
    if ( n != NONMAX ) {
      if ( n==ALIVE ) Bug("Alive e.");
      Neighbor
        *tn= Tt->N + n,
        *nt= T[tn->t].N + tn->n;
      for ( const type_kgg *P=tn->P[k]; P->kk!=BUMPER; P++ ) {
        type_df df= tn->df[k] + nt->df[P->kk];
        if ( df < 0 ) {
          type_g
            ggf= P->gg - (tn->f[k] + nt->f[P->kk]),
            llambda= ggf/df;
          if ( llambda < *lambda )
            *lambda= llambda;
          if ( ggf < -tn->theta ) {
            if ( -ggf < *theta ) {
              if ( (*theta= -ggf) == 0 ) Bug("Zero threshold found for an object pair.");
              *t_theta= t;
              *k_theta= k;
            }
          }
          else {
            *lambda= 0;
            *df_valid= 0;
          }
        }
      }
    }
    else {
      type_g
        hgf= Tt->h - Tt->g[k],
        df= t==t0 ? 1 : 0;
      Neighbor *tn= Tt->N;
      for ( type_n n=0;  n<Tt->nN;  n++, tn++ ) {
        hgf-= tn->f[k];
        df+= tn->df[k];
      }
      if ( df > 0 ) {
        type_g llambda= hgf/df;
        if ( llambda < *lambda )
          *lambda= llambda;
        if ( hgf < *theta ) {
          if ( (*theta= hgf) == 0 ) Bug("Zero threshold found for an object.");
          *t_theta= t;
          *k_theta= k;
        }
      }
    }
  }
  if ( *lambda == MAX_type_g ) Error("Dual problem unbounded (E=-infty).");
  if ( *theta==MAX_type_g ) Bug("Minimal threshold not found.");
}
/*================================================================================================*/


/*================================================================================================*/
void Maxsum::update( type_g lambda )
{
  type_tk *data= S0.data;
  for ( unsigned i=0;  i<S0.top; i++, data++ ) {
    unsigned t= data->t;
    type_k k =data->k;
    Object *Tt= T + t;
    type_n n= Tt->K[k].p;
    if ( n != NONMAX ) {
      if ( n==ALIVE ) Bug("Alive e.");
      Neighbor
        *tn= Tt->N + n,
        *nt= T[tn->t].N + tn->n;

      tn->f[k]+= lambda*tn->df[k];
      tn->df[k]= 0;
      
      type_g fth= tn->f[k] - tn->theta;
      for ( const type_kgg *P=tn->P[k]; P->kk!=BUMPER; P++ ) {
        type_k kk= P->kk;
        if ( P->gg - T[tn->t].N[tn->n].f[kk] >= fth ) {
          nt->f[kk]+= lambda*nt->df[kk];
          nt->df[kk]= 0;
        }
      }
    }
  }
}
/*================================================================================================*/


/*================================================================================================*/
void Maxsum::repair( unsigned t0, type_g lambda )
{
  S.top= 0;
  type_tk *data= S0.data;
  for ( unsigned i=0;  i<S0.top;  i++, data++ ) {
    unsigned t= data->t;
    type_k k =data->k;
    Object *Tt= T + t;
    type_n n= Tt->K[k].p;
    if ( n != NONMAX ) {
      if ( n == ALIVE ) Bug("Alive e.");
      Neighbor *tn= Tt->N + n;
      Object *Tn= T + tn->t;
      Neighbor *nt= Tn->N + tn->n;
      for ( const type_kgg *P= tn->P[k]; P->kk!=BUMPER; P++ ) {
        type_k kk= P->kk;
        type_g ggf= P->gg - (tn->f[k] + nt->f[kk]);
        if ( ggf >= -tn->theta )

          for ( const type_kgg *PP= nt->P[kk]; PP->kk!=BUMPER; PP++ ) {
            type_k kkk= PP->kk;
            if ( Tt->K[kkk].p==ALIVE && !Tt->K[kkk].v ) {
              type_g gft= PP->gg - (tn->f[kkk] + nt->f[kk]) + tn->theta;
              if ( 0 > gft  &&  gft >= lambda*(tn->df[kkk] + nt->df[kk]) ) {
                V.put(t,kkk);
                Tt->K[kk].v= 1;
              }
            }
          }

        else if ( ggf - lambda*(tn->df[k] + nt->df[kk]) >= -tn->theta ) {

          type_g gf= Tn->g[kk], df= (tn->t==t0);
          for ( type_n n=0;  n<Tn->nN;  n++ ) {
            gf+= Tn->N[n].f[kk];
            df+= Tn->N[n].df[kk];
          }
          if ( Tn->h - gf - lambda*df <= Tn->theta )
            S.push(t,k);

        }
      }
    }
    else {

      type_g gf= Tt->g[k], df= (t==t0);
      for ( type_n n=0;  n<Tt->nN;  n++ ) {
        gf+= Tt->N[n].f[k];
        df+= Tt->N[n].df[k];
      }
      if ( Tt->h - gf -lambda*df <= Tt->theta )
        S.push(t,k);

    }
  }
}
/*================================================================================================*/


/*================================================================================================*/
void Maxsum::thupdate( unsigned t0, type_g theta, unsigned t, type_k k )
{
  S.top= 0;
  Object *Tt= T + t;
  type_n n= Tt->K[k].p;
  if ( n != NONMAX ) {

    Neighbor *tn= T[t].N + n;
    Object *Tn= T + tn->t;
    Neighbor *nt= Tn->N + tn->n;
    for ( type_k k=0; k<Tt->nK; k++ ) {
      for ( const type_kgg *P=tn->P[k]; P->kk!=BUMPER; P++ ) {
        type_k kk= P->kk;
        if ( Tt->K[k].p==n && Tn->K[kk].p!=NONMAX ) {
          type_g ggf= (tn->f[k] + nt->f[kk]) - P->gg;
          if ( tn->theta < ggf  &&  ggf <= theta )
            S.push(t,k);
        }
        else if ( Tt->K[k].p!=NONMAX && Tn->K[kk].p==tn->n ) {
          type_g ggf= (tn->f[k] + nt->f[kk]) - P->gg;
          if ( tn->theta < ggf  &&  ggf <= theta )
            S.push(tn->t,kk);
        }
      }
    }
    tn->theta= nt->theta= theta;
  
  }
  else {
  
    for ( type_k k=0; k<Tt->nK; k++ )
      if ( Tt->K[k].p == NONMAX ) {
        type_g hgf= Tt->h - Tt->g[k];
        for ( type_n n=0;  n<Tt->nN;  n++ )
          hgf-= Tt->N[n].f[k];
        if ( Tt->theta < hgf  &&  hgf <= theta )
          S.push(t,k);
      }
    Tt->theta= theta;

  }
}
/*================================================================================================*/


/*================================================================================================*/
void Maxsum::ressurect()
{
  while ( S.top ) {
    unsigned t;
    type_k k;
    S.pop(&t,&k);
    Object *Tt= T + t;
    Node *tk= Tt->K + k;
    if ( tk->p != ALIVE ) {
      tk->p= ALIVE;
      Tt->n++;
      if ( tk->v == 0 ) {
        V.put(t,k);
        tk->v= 1;
      }
      Neighbor *tn= Tt->N;
      for ( type_n n=0;  n<Tt->nN;  n++, tn++ ) {
        type_g fth= tn->f[k] - tn->theta;
        for ( const type_kgg *P=tn->P[k]; P->kk!=BUMPER; P++ ) {
          type_k kk= P->kk;
          if ( P->gg - T[tn->t].N[tn->n].f[kk] >= fth ) {
            if ( T[tn->t].K[kk].p == tn->n )
              S.push(tn->t,kk);
          }
        }
      }
    }
  }
}
/*================================================================================================*/


/*================================================================================================*/
#define REPORT  (step_iter>0 && iter%step_iter==0)
double Maxsum::minimize( type_g theta )
{
  if (step_iter>0) Printf("Initializing with theta=%d.....", theta);
  init(theta);
  check_gg(); // check feasibility of edges
  if (step_iter>0) Printf("done.\n");

  double E= 0; for ( unsigned t=0;  t<nT;  t++ ) {  E+= T[t].h;  }
  if (step_iter>0) Printf("[E=%.16g][V=%8i]\n",E,V.length());

  while ( V.tail != V.head ) {

    unsigned Vlen= V.length(), t0= relax();
    if (REPORT) Printf("[RELAX: [V-=%5i]] ",Vlen-V.length());
    //check_consist(); check_n(); check_p();
    int WAS_RELAX= 1;

    while ( T[t0].n == 0 ) {
      if (REPORT && !WAS_RELAX) Printf("                    ");
      WAS_RELAX= 0;
      if (REPORT) Printf("[EQT#%7i: ",iter);

      unsigned max_df= direction(t0);
      if (REPORT) Printf("[DAG=%5i df=%9.3g]",S0.top,(double)max_df);

      type_g lambda, theta;
      unsigned t_theta;
      type_k k_theta;
      int df_valid;
      step(t0,&lambda,&df_valid,&theta,&t_theta,&k_theta);
      //if ( df_valid ) check_df(&S0,t0);

      Vlen= V.length();
      if ( lambda > 0 ) {
        if (REPORT) Printf("[lam=%8i        ]",lambda);
        repair(t0,lambda);
      }
      else {
        if ( lambda < 0 ) Bug("lambda<0.");
        if (REPORT) Printf("[lam=%8i th=%4i]",lambda,theta);
        lambda= 0;
        thupdate(t0,theta,t_theta,k_theta);
      }

      update(lambda);
      //check_gg();
      type_g *h= &T[t0].h, h0= *h;
      *h= -MAX_type_g;
      for ( type_k k=0;  k<T[t0].nK;  k++ ) {
        type_g gf= T[t0].g[k];
        for ( type_n n=0;  n<T[t0].nN;  n++ )
          gf+= T[t0].N[n].f[k];
        if ( gf > *h )  *h= gf;
      }
      if ( h0 - T[t0].h != lambda ) Bug("Energy did not decrease by lambda.");
      //check_h();

      ressurect();
      if (REPORT) Printf("[V+=%5i]] [V=%8i]",V.length()-Vlen,V.length());
      //check_consist(); check_n(); check_p();

      if (REPORT) Printf("\n");
      iter++;
      #ifdef MATLAB
        if (REPORT) mexEvalString("drawnow"); /* force mexPrintf to flush even in Java desktop */
      #endif
    }
  }
  if (REPORT) Printf("\n");
  
  E= 0; for ( unsigned t=0;  t<nT;  t++ )  E+= T[t].h;
  if (step_iter>0) Printf("[E=%.16g]\n",E);
  return E;
}
/*================================================================================================*/


/*================================================================================================*/
Maxsum::Maxsum(
  const unsigned nOmega,  // # of edges
  const unsigned *Omega,
    /* Array 3-by-nOmega of format [t tt c; t tt c; ...]
       where (t,tt) are end objects of edge and c is index of function g_{t,tt}.
       Array Omega defines number of objects as nT=max(max(Omega(0:1,:))). */
  const type_k *nK, /* Array of length nT; nK[t] is number of labels of object t. */
  const unsigned nGG,
  const int *GG,
  const type_g *g,  // numbers g_t(k); linear array, index k is changing first, t second
  type_g *f
) {
  unsigned e;
  const unsigned *Om;

  /* Compute number of objects, nT. Allocate T. */
  nT= 0;
  for ( e=0, Om= Omega;  e<nOmega;  e++, Om+=3 ) {
    if ( Om[0]+1 > nT )  nT= Om[0]+1;
    if ( Om[1]+1 > nT )  nT= Om[1]+1;
  }
  if ( nT == 0 ) Error("No objects.");
  T= (Object*)Alloc(nT*sizeof(Object));

  //for ( unsigned t=0; t<nT; T[t++].nK= nK[t] );
  for( unsigned t=0; t < nT; t++) {
    T[t].nK = nK[t];
  }

  C.init(nGG,GG,nOmega,Omega,nK);

  /* Allocate T[].K. It is done in a block, similarly as T[].N. */
  { unsigned n= 0;
    for ( unsigned t=0; t<nT; n+= T[t++].nK );
    A.K= (Node*)Alloc(n*sizeof(Node));
  }
  for ( unsigned t=0, n=0; t<nT; n+= T[t++].nK ) {
    T[t].K= A.K + n;
    T[t].g= g + n;
  }

  /* Compute T[].nN. */
  for ( unsigned t=0;  t<nT;  T[t++].nN= 0 );
  for ( e=0, Om= Omega;  e<nOmega;  e++, Om+=3 ) {
    T[Om[0]].nN++;
    T[Om[1]].nN++;
  }
  for ( unsigned t=0; t<nT; t++ ) if ( T[t].nN == 0 ) Error("An object has no neighbors.");

  /* Allocate T[].N.
     Memory for all neighbors is allocated in one block of length 2*nOmega.
     T[].N are pointers to this block. */
  A.N= (Neighbor*)Alloc(2*nOmega*sizeof(Neighbor));
  for ( unsigned t=0, n=0; t<nT; n+= T[t++].nN ) T[t].N= A.N + n;

  C.translate(nGG,GG);

  /* Allocate A.df. */
  { unsigned n= 0;
    for ( e=0, Om= Omega;  e<nOmega;  e++, Om+=3 )  n+= T[Om[0]].nK + T[Om[1]].nK;
    A.df= (type_df*)Alloc(n*sizeof(type_df));
  }

  /* Fill T[].N[].t, .n, .P, .f, .df. */
  for ( unsigned t=0; t<nT; T[t++].nN= 0 );
  type_df *df= A.df;
  for ( e=0, Om= Omega;  e<nOmega;  e++, Om+=3 ) {
    const unsigned t0= Om[0], t1= Om[1], c= Om[2];
    Neighbor *N0= T[t0].N + T[t0].nN,
             *N1= T[t1].N + T[t1].nN;

    N0->t= t1;  N0->n= T[t1].nN++;
    N1->t= t0;  N1->n= T[t0].nN++;

    N0->P= (const type_kgg**)C.PP[0+2*c];
    N1->P= (const type_kgg**)C.PP[1+2*c];

    N0->f= f;  f+= T[t0].nK;
    N1->f= f;  f+= T[t1].nK;

    N0->df= df;  df+= T[t0].nK;
    N1->df= df;  df+= T[t1].nK;
  }
}
/*================================================================================================*/

  
/*================================================================================================*/
Maxsum::~Maxsum()
{
  Free(A.df);
  Free(A.N);
  Free(A.K);
  Free(T);
}
/*================================================================================================*/


/*================================================================================================*/
void Maxsum::unique_labels( type_k *I )
{
  for ( unsigned t=0; t<nT; t++ ) {
    unsigned n= 0;
    for ( type_k k=0; k<T[t].nK; k++ )
      if ( T[t].K[k].p == ALIVE ) {
        if ( ++n == 1 ) I[t]= k;
        if ( n > 1 ) {
          I[t]= T[t].nK;
          break;
        }
      }
  }
}

/*================================================================================================*/


int adag_maxsum( unsigned char* labels, double *energy, unsigned int nT, unsigned short nK,
                  unsigned short nOmega, unsigned int *Omega, 
                 unsigned short nG, int *G, int *Q, int *f, unsigned int theta)
{

  /*
    printf("verb=%d", verb);
    printf("nT=%d\n", nT );
    printf("nK=%d\n", nK );
    printf("nOmega=%d\n", nOmega );
    printf("nG=%d\n", nG );
    printf("theta=%d\n", theta );
    printf("Omega:\n");

    for( int i = 0; i < nOmega*3; i ++ ) {
      printf("%d ", Omega[i] );
    }
    printf("\n");
    printf("G:\n");
    for( int i = 0 ; i < 4*nG; i++ ) {
      printf("%d ", G[i] );
    }
    printf("\n");
    printf("Q:\n");
    for( unsigned i = 0 ; i < nT*nK; i++ ) {
      printf("%d ", Q[i] );
    }
    printf("\n");
    printf("f:\n");
    for( unsigned i = 0 ; i < nK*unsigned(2)*nOmega; i++ ) {
      printf("%d ", f[i] );
    }
    printf("\n");
  */

  
  type_k *_nK = (type_k*) Alloc( nT*sizeof(type_k) );
  if( !_nK ) { return -1; }
  
  for( unsigned i=0; i < nT; i++ ) { _nK[i] = nK; }
    
  Maxsum M( nOmega, Omega, _nK, nG, G, Q, f );

  unsigned ng= 0, nf= 0;
  for ( unsigned t=0; t<M.nT; t++ ) {
    ng+= M.T[t].nK;
    nf+= M.T[t].nK * M.T[t].nN;
  }

  M.iter= 0;
  M.step_iter= 1000;
  *energy = M.minimize( theta );
  Printf("Check....."); M.check(); Printf("passed.\n");
                       
  for ( unsigned t=0, n=0;  t<M.nT;  t++ ) {
    for ( type_k k=0; k<M.T[t].nK; k++ ) {
      labels[n++]= M.T[t].K[k].p == ALIVE;
    }
  }

  Free( _nK );
  
  return( 0 );  
}


