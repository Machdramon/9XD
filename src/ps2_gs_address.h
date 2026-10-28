#pragma once
#include <cstdint>
// Shared GS local-memory page/block/column addressing (bit offsets).
namespace Ps2Gs {
inline int64_t bitAddress(int x,int y,int width,int bits,bool alternate16=false){
    int pw=bits<=8?128:64,ph=bits==4?128:bits==8||bits==16?64:32;
    int bw=bits==4?32:bits==8||bits==16?16:8,bh=bits<=8?16:8;
    int bx=(x%pw)/bw,by=(y%ph)/bh;int block;
    if(alternate16)block=(by&1)|((bx&1)<<1)|((by&2)<<2)|((bx&2)<<3)|(by&4);
    else if(bits==4||bits==16)block=(by&1)|((bx&1)<<1)|((by&2)<<1)|((bx&2)<<2)|((by&4)<<2);
    else block=(bx&1)|((by&1)<<1)|((bx&2)<<1)|((by&2)<<2)|((bx&4)<<2);
    int xx=x%bw,yy=y%bh,column;
    if(bits==32)column=(xx&1)|((yy&1)<<1)|((xx&6)<<1)|((yy&6)<<3);
    else if(bits==16)column=((xx&8)>>3)|((xx&1)<<1)|((yy&1)<<2)|((xx&6)<<2)|((yy&6)<<4);
    else if(bits==8)column=((yy&2)>>1)|((xx&8)>>2)|((xx&1)<<2)|((yy&1)<<3)|((xx&2)<<3)|((((xx>>2)^(yy>>1)^(yy>>2))&1)<<5)|((yy&12)<<4);
    else column=((yy&2)>>1)|((xx&8)>>2)|((xx&16)>>2)|((xx&1)<<3)|((yy&1)<<4)|((xx&2)<<4)|((((xx>>2)^(yy>>1)^(yy>>2))&1)<<6)|((yy&12)<<5);
    return (int64_t(y/ph)*((width+pw-1)/pw)+x/pw)*8192*8+block*256*8+column*bits;
}
}
