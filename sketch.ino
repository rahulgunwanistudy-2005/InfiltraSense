/* INFILTRASENSE | executable digital twin v1.0
 * Real target: AD5940 + strain + MAX30208 + LSM6DSV80X -> MAX78002 -> nRF5340.
 * This ESP32-S3 is an EXECUTION SURROGATE. UART carries synthetic measurements.
 * Demonstration classifier uses hand-authored parameters, not clinical training.
 * No scenario identifier is available to the inference/safety pipeline.
 */
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
Adafruit_SSD1306 oled(128,64,&Wire,-1);
HardwareSerial phantom(1);
const int GREEN=10, AMBER=11, RED=12, BUZZ=13, ACK=14;
struct Sample {unsigned seq; float lf,hf,strain,temp,motion,contact;} x;
enum State { NORMAL, WATCH, INVALID, CHECK };
const char* names[]={"NORMAL","WATCH","SENSOR INVALID","CHECK IV"};
State state=WATCH, previous=INVALID;
const char *reason="ACQUIRE BASELINE";
float baseLF=1000,baseHF=650,baseS=0,baseT=36.5;
float sumLF=0,sumHF=0,sumS=0,sumT=0;
float dz=0,dh=0,ds=0,dt=0,pSite=0,pArt=0,pNorm=0,quality=0;
float persist=0,clearTime=0,baselineTime=0,quietTime=0,zSlope=0,sSlope=0;
float oldZ=0,oldS=0;
bool baseline=false,anchor=false,abstain=true,latched=false,acknowledged=false,buzzerOn=false;
bool zEvidence=false,sEvidence=false,modelValid=false,oledOK=false,candidateEvidence=false;
bool graphMode=true;
unsigned lastSeq=0, samples=0,receiptId=0;
uint32_t lastFrame=0,lastReceipt=0,lastUI=0;
String line;
void resetAcquisition(){baselineTime=0;samples=0;sumLF=sumHF=sumS=sumT=0;}
void requestReacquisition(){
 baseline=false;resetAcquisition();persist=0;clearTime=0;quietTime=0;
 modelValid=false;abstain=true;state=WATCH;reason="REACQUISITION SETTLING";
}
// Transparent three-class softmax; signed int8 coefficients in units of 1/8.
// Inputs: LF decrease /20%, strain increase /12%, |temperature| /1C, motion.
// This small embedded classifier demonstrates inference; scores are NOT calibrated probabilities.
void classify(){
 const int8_t W[3][4]={{-8,-8,0,-8},{0,0,0,64},{24,24,2,-48}};
 const int8_t B[3]={16,-16,-24};
 float f[4]={constrain(-dz/20,0.f,2.f),constrain(ds/12,0.f,2.f),constrain(fabsf(dt),0.f,2.f),x.motion};
 float logits[3],peak=-100;for(int k=0;k<3;k++){logits[k]=B[k]/8.f;for(int j=0;j<4;j++)logits[k]+=W[k][j]*f[j]/8.f;peak=fmaxf(peak,logits[k]);}
 float total=0;for(int k=0;k<3;k++){logits[k]=expf(logits[k]-peak);total+=logits[k];}
 pNorm=logits[0]/total;pArt=logits[1]/total;pSite=logits[2]/total;
}
struct GraphPoint {float lf,hf,strain,motion,temp,threshold,candidate,check;};
GraphPoint graphPoint(){
 // Every live trace is expressed as a percentage of its own policy threshold.
 // This keeps unlike units visible on one plot: 100 means "threshold reached".
 return {
  constrain(fmaxf(-dz,0.f)/8.f*100.f,0.f,200.f),
  constrain(fmaxf(-dh,0.f)/3.f*100.f,0.f,200.f),
  constrain(fmaxf(ds,0.f)/4.f*100.f,0.f,200.f),
  constrain(x.motion/.35f*100.f,0.f,200.f),
  constrain(fabsf(dt)/.8f*100.f,0.f,200.f),
  100.f,candidateEvidence?110.f:0.f,latched?125.f:0.f
 };
}
void plotGraph(){
 GraphPoint g=graphPoint();
 Serial.printf("LF_Z:%0.1f\tHF_Z:%0.1f\tStrain:%0.1f\tMotion:%0.1f\tTemperature:%0.1f\tThreshold:%0.1f\tCandidate:%0.1f\tCHECK_IV:%0.1f\n",
  g.lf,g.hf,g.strain,g.motion,g.temp,g.threshold,g.candidate,g.check);
}
void banner(){
 Serial.println("\nINFILTRASENSE | EXECUTABLE DIGITAL TWIN v1.1");
 Serial.println("AD5940 + strain + MAX30208 + LSM6DSV80X -> MAX78002 -> nRF5340 (REAL TARGET)");
 Serial.println("ESP32-S3 = execution surrogate; UART phantom = synthetic measurements, NOT device emulation.");
 Serial.println("Demo hand-authored int8 classifier. No clinical validation or diagnostic claims.");
 Serial.println("Wait ~5s for baseline. Buttons or serial: N normal, M motion, L lift, S shift, I fluid, F free sliders.");
 Serial.println("A acknowledges/mutes; R requests guarded reacquisition; ? prints evidence; P returns to live graphs.");
}
void receipt(){
 Serial.printf("\n=== EVIDENCE RECEIPT %u | t=%.1fs ===\n",++receiptId,millis()/1000.f);
 Serial.printf("RAW LF=%.1fohm HF=%.1fohm strain=%.2f%% T=%.2fC motion=%.2f contact=%.2f\n",x.lf,x.hf,x.strain,x.temp,x.motion,x.contact);
 Serial.printf("INTEGRITY quality=%.2f baseline=%s acquisition=%.1f/4s abstain=%s\n",quality,baseline?"VERIFIED":"PENDING",baselineTime,abstain?"YES":"NO");
 Serial.printf("EVIDENCE dLF=%+.1f%% dHF=%+.1f%% strain=%+.1f%% dT=%+.2fC slopes(Z,S)=%+.2f,%+.2f/s\n",dz,dh,ds,dt,zSlope,sSlope);
 if(modelValid)Serial.printf("MODEL demo-int8-v1 scores normal=%.3f artifact=%.3f site=%.3f (uncalibrated)\n",pNorm,pArt,pSite);
 else Serial.println("MODEL WITHHELD: untrusted or unbaselined input");
 Serial.printf("POLICY bioZ=%d strain=%d persistence=%.1f/4s latch=%d ack=%d clear=%.1f/4s\n",zEvidence,sEvidence,persist,latched,acknowledged,clearTime);
 Serial.printf("DECISION %s | %s | audible=%s\n",names[state],reason,latched&&!acknowledged?"ON":"OFF");
}
void process(float elapsed){
 quality=constrain(x.contact*(1-.75f*x.motion),0.f,1.f);
 dz=100*(x.lf/baseLF-1);dh=100*(x.hf/baseHF-1);ds=x.strain-baseS;dt=x.temp-baseT;
 zSlope=.75f*zSlope+.25f*(dz-oldZ)/elapsed;sSlope=.75f*sSlope+.25f*(ds-oldS)/elapsed;oldZ=dz;oldS=ds;
 zEvidence=dz < -8 && dh < -3;sEvidence=ds>4;candidateEvidence=false;
 modelValid=false;abstain=true;
 bool range=x.lf>100&&x.lf<4000&&x.hf>100&&x.hf<3000&&x.temp>=30&&x.temp<=42&&fabsf(x.strain)<40&&x.motion>=0&&x.motion<=1&&x.contact>=0&&x.contact<=1;
 if(!range||x.contact<.65f){
 baseline=false;resetAcquisition();persist=0;clearTime=0;quietTime=0;
 state=INVALID;reason="CONTACT / RANGE FAIL";
 }else if(x.motion>.35f||quality<.70f){
 persist=0;clearTime=0;quietTime=0;resetAcquisition();state=WATCH;reason="MOTION: INFER WITHHELD";
 }else{
 quietTime+=elapsed;
 if(quietTime<1){persist=0;state=WATCH;reason="ARTIFACT SETTLING";}
 else if(!baseline){
 // Guarded demo envelope/retained reference prevents learning an abnormal new baseline.
 bool safe=fabsf(dz)<3&&fabsf(dh)<3&&fabsf(ds)<2&&fabsf(dt)<.5f;
 if(!safe){resetAcquisition();state=WATCH;reason="BASELINE GUARD: RESTORE";}
 else {
 baselineTime+=elapsed;sumLF+=x.lf;sumHF+=x.hf;sumS+=x.strain;sumT+=x.temp;samples++;
 state=WATCH;reason=anchor?"REACQUIRING BASELINE":"ACQUIRING BASELINE";
 if(baselineTime>=4){baseLF=sumLF/samples;baseHF=sumHF/samples;baseS=sumS/samples;baseT=sumT/samples;baseline=true;anchor=true;oldZ=oldS=zSlope=sSlope=0;}
 }
 }else{
 classify();modelValid=true;
 bool uncertain=fmaxf(pNorm,fmaxf(pArt,pSite))<.60f;
 abstain=uncertain;
 candidateEvidence=zEvidence&&sEvidence&&pSite>=.75f&&!uncertain;
 if(candidateEvidence)persist+=elapsed;else persist=0;
 bool changed=zEvidence||sEvidence||fabsf(dt)>.8f||fabsf(dz)>5;
 if(persist>=4){latched=true;state=CHECK;reason="CORROBORATED + PERSISTENT";}
 else if(changed||uncertain){state=WATCH;reason=candidateEvidence?"ACCUMULATING EVIDENCE":uncertain?"UNCERTAIN: ABSTAIN":"INSUFFICIENT CORROBORATION";}
 else{state=NORMAL;reason="TRUSTED / STABLE";}
 bool normalRecovery=!changed&&!uncertain&&pNorm>.75f;
 if(normalRecovery)clearTime+=elapsed;else clearTime=0;
 if(latched&&acknowledged&&clearTime>=4){latched=false;acknowledged=false;}
 }
 }
 if(latched&&state!=INVALID){state=CHECK;if(abstain)reason="LATCHED / INFERENCE HELD";else if(persist<4)reason="LATCHED: RESTORE + ACK";}
 if(state!=previous||millis()-lastReceipt>=2000){if(!graphMode)receipt();lastReceipt=millis();previous=state;}
 if(graphMode)plotGraph();
}
void render(){
 digitalWrite(GREEN,state==NORMAL&&!latched);
 digitalWrite(AMBER,state==WATCH||(state==INVALID&&(millis()/350)%2));
 digitalWrite(RED,latched||state==CHECK);
 bool wantBuzz=latched&&!acknowledged&&(millis()/500)%2;
 if(wantBuzz&&!buzzerOn)ledcWriteTone(0,1800);
 else if(!wantBuzz&&buzzerOn)ledcWriteTone(0,0);
 buzzerOn=wantBuzz;
 if(!oledOK)return;
 oled.clearDisplay();oled.setTextColor(SSD1306_WHITE);oled.setTextSize(1);oled.setCursor(0,0);oled.print("INFILTRASENSE | TWIN");oled.drawLine(0,10,127,10,SSD1306_WHITE);
 oled.setCursor(0,13);oled.setTextSize(state==INVALID?1:2);oled.print(names[state]);oled.setTextSize(1);
 oled.setCursor(0,31);oled.printf("Z%+.0f%% S%+.1f%% T%.1f",dz,ds,x.temp);
 oled.setCursor(0,41);oled.printf("Q%.2f M%.2f %s",quality,x.motion,abstain?"HOLD":"LIVE");
 oled.setCursor(0,51);
 if(!baseline)oled.printf("BASE %.1f/4s",baselineTime);else oled.printf("PERSIST %.1f/4s",persist);
 if(latched){oled.setCursor(91,51);oled.print(acknowledged?"MUTE":"ALRM");}
 oled.display();
}
void setup(){
 Serial.begin(115200);phantom.begin(115200,SERIAL_8N1,18,17);line.reserve(180);
 for(int pin:{GREEN,AMBER,RED,BUZZ})pinMode(pin,OUTPUT);pinMode(ACK,INPUT_PULLUP);
 ledcSetup(0,1800,8);ledcAttachPin(BUZZ,0);ledcWriteTone(0,0);
 Wire.begin(8,9);oledOK=oled.begin(SSD1306_SWITCHCAPVCC,0x3c);
 if(!graphMode)banner();
}
void loop(){
 while(Serial.available()) {char c=toupper(Serial.read());if(c&&strchr("NMLSIF",c))phantom.write(c);
 if(c=='A'&&latched)acknowledged=true;
 if(c=='R')requestReacquisition();
 if(c=='P'){graphMode=!graphMode;if(!graphMode){banner();receipt();}}
 if(c=='?'&&!graphMode)receipt();}
 if(!digitalRead(ACK)&&latched)acknowledged=true;
 while(phantom.available()){
 char c=phantom.read();if(c=='\n'){
 Sample next;int consumed=0;
 if(sscanf(line.c_str(),"%u,%f,%f,%f,%f,%f,%f%n",&next.seq,&next.lf,&next.hf,&next.strain,&next.temp,&next.motion,&next.contact,&consumed)==7&&consumed==static_cast<int>(line.length())
 &&isfinite(next.lf)&&isfinite(next.hf)&&isfinite(next.strain)&&isfinite(next.temp)&&isfinite(next.motion)&&isfinite(next.contact)&&next.seq>lastSeq){
 uint32_t now=millis();float elapsed=lastFrame?constrain((now-lastFrame)/1000.f,.001f,.2f):.1f;
 if(lastFrame&&now-lastFrame>300){persist=0;quietTime=0;resetAcquisition();}
 x=next;lastSeq=x.seq;lastFrame=now;process(elapsed);
 }line="";
 }else if(line.length()<175)line+=c;else line="";
 }
 if(millis()-lastFrame>700){state=INVALID;reason="SENSOR STREAM TIMEOUT";abstain=true;modelValid=false;baseline=false;persist=clearTime=quietTime=0;resetAcquisition();
 if(millis()-lastReceipt>2000){receipt();lastReceipt=millis();}}
 if(millis()-lastUI>=100){render();lastUI=millis();}delay(1);
}
