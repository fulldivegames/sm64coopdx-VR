#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef float f32,Vec3f[3]; typedef int s32;typedef unsigned char u8;typedef signed char s8;
typedef struct {struct {short ob[3];s8 n[3];} n;} Vtx;
typedef struct {float ob[3];s8 n[3];} Vtx_Interp;
struct Painting {struct {Vtx_Interp *sVerticesPrev,*sVerticesCur;Vtx *sVerticesPtr[2];unsigned sVerticesPrevTimestamp,sVertexSwaps;int sVerticesCount,sVerticesFirstCount;} ripples;};
struct Painting_List_Item {struct Painting *painting;struct Painting_List_Item *nextPaintingItem;} paintingZero;
static unsigned gGlobalTimer=100;
static void delta_interpolate_vec3f(Vec3f d,Vec3f a,Vec3f b,float t){for(int i=0;i<3;i++)d[i]=a[i]+(b[i]-a[i])*t;}
static void delta_interpolate_normal(s8*d,s8*a,s8*b,float t){for(int i=0;i<3;i++)d[i]=a[i]+(b[i]-a[i])*t;}
#include "../build/test_painting_interpolation.inc"
int main(void){
 Vtx_Interp prev[8]={0},cur[8]={0};
 struct {Vtx left[3];unsigned guard;Vtx right[5];unsigned guard2;} buffers={.guard=0x12345678,.guard2=0x87654321};
 struct Painting p={.ripples={.sVerticesPrev=prev,.sVerticesCur=cur,.sVerticesPtr={buffers.left,buffers.right},.sVerticesPrevTimestamp=100,.sVertexSwaps=3,.sVerticesCount=8,.sVerticesFirstCount=3}};
 paintingZero.painting=&p;paintingZero.nextPaintingItem=&paintingZero;
 for(int i=0;i<8;i++)cur[i].ob[0]=20+i*2;
 patch_paintings_interpolated(.5f);
 assert(buffers.guard==0x12345678 && buffers.guard2==0x87654321);
 assert(buffers.left[2].n.ob[0]==12 && buffers.right[0].n.ob[0]==13 && buffers.right[4].n.ob[0]==17);
 // Frame-arena memory may now belong to text: an old painting cannot write it.
 gGlobalTimer++;buffers.left[0].n.ob[0]=123;
 patch_paintings_interpolated(1);assert(buffers.left[0].n.ob[0]==123);
 puts("PASS: actual painting interpolation, unequal section sizes, guarded HUD neighbors and stale-frame rejection");
}
