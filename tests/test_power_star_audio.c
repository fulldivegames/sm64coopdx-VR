#define MA_NO_DEVICE_IO
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_VORBIS
#include "../src/pc/utils/miniaudio.c"
#include <stdio.h>
int main(int argc,char**argv){
 if(argc!=2)return 2;
 ma_decoder d;ma_decoder_config c=ma_decoder_config_init(ma_format_f32,2,48000);
 if(ma_decoder_init_file(argv[1],&c,&d)!=MA_SUCCESS)return 3;
 float samples[8192],peak=0;ma_uint64 total=0,count;
 for(;;){ma_result r=ma_decoder_read_pcm_frames(&d,samples,4096,&count);
  for(ma_uint64 i=0;i<count*2;i++){float v=fabsf(samples[i]);if(!isfinite(v))return 4;if(v>peak)peak=v;}
  total+=count;if(r==MA_AT_END||count==0)break;if(r!=MA_SUCCESS)return 5;}
 ma_decoder_uninit(&d);
 printf("Decoded Power Star: %.2f seconds, peak %.3f\n",(double)total/48000,peak);
 return total<48000||peak<0.001f;
}
