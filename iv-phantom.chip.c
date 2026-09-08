// Synthetic transfer model, NOT AD5940 / MAX30208 / IMU device emulation.
// Only raw measurements cross the UART; no diagnosis or scenario ID is sent.
#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
typedef struct { uart_dev_t uart; pin_t buttons[6]; uint32_t attrs[6];
 buffer_t fb; uint8_t pixels[220*110*4]; int mode; float fluid; unsigned seq; uint64_t lastButton; char frame[180]; } chip_state_t;

// Read-only visualisation of phantom conditions, never the MCU decision.
static void paint(chip_state_t *s,float motion,float contact,float shift,float wave) {
 for(int y=0;y<110;y++)for(int x=0;x<220;x++) {
  int r=15,g=23,b=42;
  int cy=47+(int)(motion*.04f*wave);
  if(y>cy-25&&y<cy+25&&x>8&&x<210){r=192;g=146;b=121;}
  float dx=(x-112)/1.6f,dy=y-cy;
  if(dx*dx+dy*dy<25+s->fluid*5){r=71;g=172;b=207;}
  if(x>62&&x<160&&y>cy-19&&y<cy+19&&(x<65||x>157||y<cy-16||y>cy+16)){r=226;g=232;b=240;}
  if(y>cy-3&&y<cy+2&&x>14&&x<112+(int)(shift*.12f)){r=245;g=245;b=250;}
  if(((x>68&&x<80)||(x>144&&x<156))&&y>cy-10&&y<cy+10){r=contact<65?240:24;g=contact<65?150:210;b=contact<65?40:170;}
  // Bottom bars: fluid (cyan), motion (violet), contact (green).
  if(y>=83&&y<89&&x>10&&x<210){r=30;g=41;b=59;if(x<10+2*s->fluid){r=56;g=189;b=248;}}
  if(y>=92&&y<98&&x>10&&x<210){r=30;g=41;b=59;if(x<10+2*motion){r=167;g=139;b=250;}}
  if(y>=101&&y<107&&x>10&&x<210){r=30;g=41;b=59;if(x<10+2*contact){r=52;g=211;b=153;}}
  int n=(y*220+x)*4;s->pixels[n]=r;s->pixels[n+1]=g;s->pixels[n+2]=b;s->pixels[n+3]=255;
 }
 buffer_write(s->fb,0,s->pixels,sizeof(s->pixels));
}

static void command(void *data, uint8_t b) {
 chip_state_t *s=data; const char *keys="NMLSIF"; const char *k=strchr(keys,b);
 if(k && b) s->mode=k-keys;
}
static void pressed(void *data,pin_t pin,uint32_t value) {
 chip_state_t *s=data; uint64_t now=get_sim_nanos();
 if(now-s->lastButton<150000000) return;
 for(int i=0;i<6;i++) if(pin==s->buttons[i]) s->mode=i;
 s->lastButton=now;
}
static void tick(void *data) {
 chip_state_t *s=data; float t=get_sim_nanos()/1e9;
 float f=0,m=0,c=100,h=0,e=0,temp=36.5;
 if(s->mode==1) m=85;
 if(s->mode==2) c=15;
 if(s->mode==3) h=85;
 if(s->mode==4) f=85;
 if(s->mode==5) { f=attr_read_float(s->attrs[0]); m=attr_read_float(s->attrs[1]);
 c=attr_read_float(s->attrs[2]);h=attr_read_float(s->attrs[3]);
 e=attr_read_float(s->attrs[4]);temp=attr_read_float(s->attrs[5]); }
 // Slow tissue accumulation/washout, immediate contact and movement.
 s->fluid+=(f-s->fluid)*0.065f;
 float wave=sinf(t*11.7f), n=0.12f*sinf(t*2.3f);
 float lf=1000*(1-.0026f*s->fluid-.00035f*h+.0013f*m*wave)+n;
 float hf=650*(1-.0013f*s->fluid-.00012f*h+.0007f*m*wave)+n;
 float strain=.145f*s->fluid+.10f*h+e+.08f*m*wave+n;
 temp+=.004f*s->fluid+.01f*sinf(t*.7f);
 if(c<50) { lf=2800+600*wave; hf=1900+350*wave; }
 paint(s,m,c,h,wave);
 int len=snprintf(s->frame,sizeof(s->frame),"%u,%.2f,%.2f,%.3f,%.3f,%.2f,%.2f\n",++s->seq,lf,hf,strain,temp,m/100,c/100);
 uart_write(s->uart,(uint8_t*)s->frame,len);
}
void chip_init(void) {
 chip_state_t *s=calloc(1,sizeof(chip_state_t));
 uint32_t width,height;s->fb=framebuffer_init(&width,&height);
 const char *names[]={"NORMAL","MOVE","LIFT","SHIFT","FLUID","FREE"};
 pin_watch_config_t w={.edge=FALLING,.pin_change=pressed,.user_data=s};
 for(int i=0;i<6;i++) {s->buttons[i]=pin_init(names[i],INPUT_PULLUP);pin_watch(s->buttons[i],&w);}
 const char *attrs[]={"fluid","motion","contact","shift","expansion","temperature"};
 float defaults[]={0,0,100,0,0,36.5};
 for(int i=0;i<6;i++) s->attrs[i]=attr_init_float(attrs[i],defaults[i]);
 uart_config_t u={.rx=pin_init("RX",INPUT),.tx=pin_init("TX",INPUT_PULLUP),.baud_rate=115200,.rx_data=command,.user_data=s};
 s->uart=uart_init(&u);
 timer_config_t tc={.callback=tick,.user_data=s};timer_t tm=timer_init(&tc);timer_start(tm,100000,true);
}
