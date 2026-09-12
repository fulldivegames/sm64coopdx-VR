#include <assert.h>
#include <stdio.h>
#include "../src/pc/djui/djui_tutorial_pages.h"
int main(void){
 const char* m="First instruction. Second instruction. Third instruction. Fourth instruction. Fifth instruction. Sixth instruction. Seventh instruction. Eighth instruction. Ninth instruction. Tenth instruction. Eleventh instruction. Twelfth instruction. Thirteenth instruction. Fourteenth instruction. Fifteenth instruction.";
 size_t start=0, n=strlen(m), count=0;char out[512];
 while(start<n){size_t end=djui_tutorial_next(m,start);assert(end>start&&end<=n&&end-start<=220);
 djui_tutorial_format(out,sizeof(out),m,start,end);assert(strlen(out)>0);assert(out[0]==m[start]);
 start=end;while(m[start]==' ')start++;count++;assert(count<10);}
 assert(count>=2);assert(djui_tutorial_next(m,n)==n);
 puts("PASS: tutorial page progress, word boundaries, spacing and final page.");
}
