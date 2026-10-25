/* 9XD memory adapter. libmspack is separately distributed under LGPL-2.1. */
#include "system.h"
#include "lzx.h"
#include <stdlib.h>
#include <string.h>
struct memory_file {unsigned char *data;int size,position;};
static int mem_read(struct mspack_file *f,void *p,int n){struct memory_file *m=(struct memory_file *)f;if(n<0)return -1;if(n>m->size-m->position)n=m->size-m->position;memcpy(p,m->data+m->position,n);m->position+=n;return n;}
static int mem_write(struct mspack_file *f,void *p,int n){struct memory_file *m=(struct memory_file *)f;if(n<0||n>m->size-m->position)return -1;memcpy(m->data+m->position,p,n);m->position+=n;return n;}
static void *mem_alloc(struct mspack_system *s,size_t n){return calloc(1,n);}
static void mem_free(void *p){free(p);}
static void mem_copy(void *a,void *b,size_t n){memcpy(b,a,n);}
#ifdef _WIN32
__declspec(dllexport)
#endif
int nines_lzx_decode(const unsigned char *source,int size,unsigned char *destination,int decoded,int bits){
 struct mspack_system sys={0};struct memory_file input,output;struct lzxd_stream *state;int result;
 if(!source||!destination||size<1||decoded<1||bits<15||bits>21)return -1;
 input.data=(unsigned char *)source;input.size=size;input.position=0;output.data=destination;output.size=decoded;output.position=0;
 sys.read=mem_read;sys.write=mem_write;sys.alloc=mem_alloc;sys.free=mem_free;sys.copy=mem_copy;
 state=lzxd_init(&sys,(struct mspack_file *)&input,(struct mspack_file *)&output,bits,0,4096,decoded,0);if(!state)return -2;
 result=lzxd_decompress(state,decoded);lzxd_free(state);return result?result:output.position==decoded?0:-3;
}
