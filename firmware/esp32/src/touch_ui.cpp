#include "touch_ui.h"
#include "board_config.h"

#if KILN_BOARD_PROFILE == KILN_BOARD_CYD_28_ILI9341_XPT2046
#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <math.h>
#include <stdio.h>

namespace {
constexpr int16_t W=320, H=240, NAV_Y=210;
constexpr int16_t TCS=Cyd28ClassicPins::TOUCH_CS, TIRQ=Cyd28ClassicPins::TOUCH_IRQ;
constexpr int16_t TSCLK=Cyd28ClassicPins::TOUCH_CLK, TMISO=Cyd28ClassicPins::TOUCH_MISO;
constexpr int16_t TMOSI=Cyd28ClassicPins::TOUCH_MOSI, BACKLIGHT=Cyd28ClassicPins::TFT_BACKLIGHT;
constexpr uint16_t BG=0x1082, PANEL=0x2124, PANEL2=0x2945, MUTED=0xA514;
constexpr uint16_t ACCENT=0x07FF, GOOD=0x3666, WARN=0xFD20, BAD=0xD104, DISABLED=0x4208;
TFT_eSPI tft;
SPIClass touchBus(HSPI);
XPT2046_Touchscreen touch(TCS,TIRQ);
Preferences prefs;

const char* stateName(uint8_t s) {
  switch(s) {
    case kiln::STATE_BOOT:return "BOOT"; case kiln::STATE_SELF_TEST:return "SELF TEST";
    case kiln::STATE_IDLE:return "IDLE"; case kiln::STATE_ARMED:return "ARMED";
    case kiln::STATE_FIRING:return "FIRING"; case kiln::STATE_PAUSED:return "PAUSED";
    case kiln::STATE_COMPLETE:return "COMPLETE"; case kiln::STATE_FAULT:return "FAULT";
    default:return "UNKNOWN";
  }
}
const char* modeName(uint8_t m) {
  switch(m) { case 0:return "OFF"; case 1:return "LOG ONLY";
    case 2:return "LOG + PID"; case 3:return "LOG + SAFETY"; default:return "UNKNOWN"; }
}
const char* typeName(uint8_t t) {
  static const char* names[]={"K","J","N","R","S","T","E","B"};
  return t<8?names[t]:"?";
}
const char* faultName(uint8_t f) {
  switch(f) { case 0:return "OK"; case 1:return "OPEN"; case 2:return "SHORT";
    case 3:return "CONVERSION"; case 4:return "STALE"; case 5:return "RANGE";
    case 6:return "RATE"; case 7:return "NOT CONFIGURED"; default:return "UNKNOWN"; }
}
void tempText(char* out,size_t n,int16_t d) {
  const int v=static_cast<int>(d), a=abs(v);
  snprintf(out,n,"%s%d.%d C",v<0?"-":"",a/10,a%10);
}
}

extern void requestEmergencyShutdown();

void TouchUi::begin(ControllerLink& link) {
  link_=&link;
  pinMode(BACKLIGHT,OUTPUT); digitalWrite(BACKLIGHT,HIGH);
  tft.init(); tft.setRotation(1); tft.fillScreen(BG);
  touchBus.begin(TSCLK,TMISO,TMOSI,TCS);
  touch.begin(touchBus); touch.setRotation(1);
  prefs.begin("kiln-ui",false); loadSettings();
  if(calibrationValid_) screen_=HOME; else startCalibration();
  redraw_=true;
}

void TouchUi::update() {
  if(!link_) return;
  const bool online=link_->connected() && link_->statusFresh();
  if(wasConnected_ && !online) {
    profileUploaded_=false;
    setNotice("Link lost: re-send the profile before starting.");
  }
  wasConnected_=online;

  const bool down=touch.touched();
  if(down && !touchDown_) {
    const TS_Point p=touch.getPoint();
    if(screen_==CALIBRATION) handleCalibrationTap(p.x,p.y);
    else { int x=0,y=0; if(rawToScreen(p.x,p.y,x,y)) handleTap(x,y); }
  }
  touchDown_=down;
  const uint32_t now=millis();
  if(redraw_ || static_cast<uint32_t>(now-lastDrawMs_)>=500) {
    render(); lastDrawMs_=now; redraw_=false;
  }
}

void TouchUi::drawButton(int x,int y,int w,int h,const char* label,uint16_t fill,bool enabled) {
  const uint16_t c=enabled?fill:DISABLED;
  tft.fillRoundRect(x,y,w,h,6,c);
  tft.drawRoundRect(x,y,w,h,6,enabled?MUTED:PANEL2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(enabled?TFT_WHITE:MUTED,c);
  tft.drawString(label,x+w/2,y+h/2,2);
  tft.setTextDatum(TL_DATUM);
}

void TouchUi::drawHeader(const char* title) {
  tft.fillRect(0,0,W,38,TFT_BLACK);
  tft.setTextColor(TFT_WHITE,TFT_BLACK); tft.drawString(title,9,10,4);
  const bool online=link_ && link_->connected() && link_->statusFresh();
  tft.setTextColor(online?GOOD:WARN,TFT_BLACK);
  tft.drawRightString(online?"CAN ONLINE":"CAN OFFLINE",244,14,1);
  drawButton(255,5,59,27,"E-STOP",BAD,true);
}
void TouchUi::drawNavigation() {
  tft.fillRect(0,NAV_Y,W,H-NAV_Y,TFT_BLACK);
  const char* labels[]={"HOME","PROFILE","SENSOR","ZONES","INFO"};
  for(uint8_t i=0;i<5;i++) {
    const bool active=(i==0&&screen_==HOME)||(i==1&&screen_==PROFILE)||
      (i==2&&screen_==SENSORS)||(i==3&&screen_==ZONES)||(i==4&&screen_==INFO);
    drawButton(3+i*64,214,60,23,labels[i],active?ACCENT:PANEL,true);
  }
}

void TouchUi::drawHome() {
  tft.fillRoundRect(8,43,304,23,5,PANEL);
  tft.setTextColor(TFT_WHITE,PANEL);
  char line[64];
  if(link_->hasStatus()) snprintf(line,sizeof(line),"%s  |  FAULT 0x%04X",
      stateName(link_->controllerState()),link_->faultFlags());
  else snprintf(line,sizeof(line),"WAITING FOR CONTROLLER STATUS");
  tft.drawString(line,14,49,2);

  for(uint8_t i=0;i<4;i++) {
    const int x=(i%2)?164:8, y=(i/2)?114:70;
    const bool active=link_->hasStatus()&&link_->sensorMode(i)!=0;
    const bool valid=link_->hasStatus()&&(link_->sensorValidMask()&(1u<<i));
    const uint16_t panel=valid?PANEL:PANEL2;
    tft.fillRoundRect(x,y,148,40,5,panel);
    snprintf(line,sizeof(line),"TC%u  %s",i+1,link_->hasStatus()?modeName(link_->sensorMode(i)):"NO DATA");
    tft.setTextColor(MUTED,panel); tft.drawString(line,x+6,y+3,1);
    char temperature[24];
    if(valid) tempText(temperature,sizeof(temperature),link_->temperatureDeciC(i));
    else snprintf(temperature,sizeof(temperature),"---.- C");
    tft.setTextColor(valid?TFT_WHITE:WARN,panel); tft.drawString(temperature,x+6,y+17,4);
    tft.setTextColor(!active?MUTED:(valid?GOOD:BAD),panel);
    tft.drawRightString(link_->hasStatus()?faultName(link_->sensorFault(i)):"NO LINK",x+142,y+25,1);
  }

  const uint8_t state=link_->hasStatus()?link_->controllerState():kiln::STATE_BOOT;
  const bool firing=state==kiln::STATE_FIRING, paused=state==kiln::STATE_PAUSED;
  const char* reason=nullptr;
  const bool ready=readyToStart(reason);
  drawButton(8,161,98,40,"START",GOOD,!firing&&!paused&&ready);
  drawButton(111,161,98,40,paused?"RESUME":"PAUSE",WARN,firing||paused);
  drawButton(214,161,98,40,"STOP",BAD,firing||paused);
  if(notice_) { tft.setTextColor(WARN,BG); tft.drawString(notice_,10,202,1); }
  else if(!ready&&!firing&&!paused&&reason) {
    tft.setTextColor(MUTED,BG); tft.drawString(reason,10,202,1);
  }
}

void TouchUi::drawProfile() {
  drawHeader("FIRING PROFILE");
  tft.setTextColor(profileUploaded_?GOOD:WARN,BG);
  tft.drawString(profileUploaded_?"PROFILE SENT TO CONTROLLER":"EDIT SEGMENTS, THEN SEND TO S3",9,43,1);
  char title[24]; snprintf(title,sizeof(title),"SEGMENT %u / %u",selectedSegment_+1,profileCount_);
  drawButton(9,59,48,24,"<",PANEL); drawButton(263,59,48,24,">",PANEL);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_WHITE,BG);
  tft.drawString(title,160,71,2); tft.setTextDatum(TL_DATUM);

  const ProfileSegment& s=profile_[selectedSegment_];
  char value[32];
  snprintf(value,sizeof(value),"%d C",s.targetDeciC/10);
  tft.setTextColor(TFT_WHITE,BG); tft.drawString("TARGET",12,94,2);
  tft.drawRightString(value,210,94,2);
  drawButton(219,89,42,28,"-",PANEL); drawButton(267,89,42,28,"+",PANEL);
  snprintf(value,sizeof(value),"%d C/h",s.rampDeciCPerHour/10);
  tft.drawString("RAMP RATE",12,127,2); tft.drawRightString(value,210,127,2);
  drawButton(219,122,42,28,"-",PANEL); drawButton(267,122,42,28,"+",PANEL);
  snprintf(value,sizeof(value),"%lu min",static_cast<unsigned long>(s.holdSeconds/60));
  tft.drawString("SOAK",12,160,2); tft.drawRightString(value,210,160,2);
  drawButton(219,155,42,28,"-",PANEL); drawButton(267,155,42,28,"+",PANEL);
  drawButton(8,184,72,22,"ADD",PANEL,profileCount_<4);
  drawButton(86,184,72,22,"REMOVE",PANEL,profileCount_>1);
  drawButton(164,184,148,22,"SEND PROFILE",ACCENT);
}

void TouchUi::drawSensors() {
  drawHeader("SENSOR SETUP");
  for(uint8_t i=0;i<4;i++) {
    char label[8]; snprintf(label,sizeof(label),"TC%u",i+1);
    drawButton(8+i*78,48,72,28,label,selectedSensor_==i?ACCENT:PANEL);
  }
  const uint8_t mode=link_->hasStatus()?link_->sensorMode(selectedSensor_):0;
  const bool valid=link_->hasStatus()&&(link_->sensorValidMask()&(1u<<selectedSensor_));
  char line[56]; snprintf(line,sizeof(line),"SENSOR %u   %s",selectedSensor_+1,valid?"READING VALID":"NO VALID READING");
  tft.setTextColor(MUTED,BG); tft.drawString(line,10,84,1);
  drawButton(9,96,302,35,modeName(mode),PANEL);
  tft.setTextColor(MUTED,BG); tft.drawCentreString("Tap to cycle: OFF / LOG / PID / SAFETY",160,135,1);
  snprintf(line,sizeof(line),"THERMOCOUPLE TYPE: %s",typeName(thermocoupleTypes_[selectedSensor_]));
  tft.setTextColor(TFT_WHITE,BG); tft.drawString(line,10,151,2);
  drawButton(225,145,38,27,"-",PANEL); drawButton(269,145,42,27,"+",PANEL);
  snprintf(line,sizeof(line),"CAL OFFSET: %+.1f C",sensorOffsetsDeciC_[selectedSensor_]/10.0f);
  tft.drawString(line,10,181,2);
  drawButton(225,175,38,27,"-",PANEL); drawButton(269,175,42,27,"+",PANEL);
}

void TouchUi::drawZones() {
  drawHeader("HEATER ZONES");
  for(uint8_t i=0;i<4;i++) {
    char label[8]; snprintf(label,sizeof(label),"Z%u",i+1);
    drawButton(8+i*78,49,72,27,label,selectedZone_==i?ACCENT:PANEL);
  }
  char value[40]; tft.setTextColor(TFT_WHITE,BG);
  snprintf(value,sizeof(value),"ZONE %u CONTROL",selectedZone_+1); tft.drawString(value,10,84,2);
  snprintf(value,sizeof(value),"HYSTERESIS: %.1f C",zones_[selectedZone_].hysteresisDeciC/10.0f);
  tft.drawString(value,10,119,2); drawButton(220,111,40,29,"-",PANEL); drawButton(267,111,42,29,"+",PANEL);
  snprintf(value,sizeof(value),"SETPOINT OFFSET: %+.1f C",zones_[selectedZone_].offsetDeciC/10.0f);
  tft.drawString(value,10,153,2); drawButton(220,145,40,29,"-",PANEL); drawButton(267,145,42,29,"+",PANEL);
  drawButton(10,180,300,25,"APPLY ZONE SETTINGS",ACCENT);
}

void TouchUi::drawInfo() {
  drawHeader("CONTROLLER INFO");
  char line[64]; const bool online=link_->connected()&&link_->statusFresh();
  tft.setTextColor(TFT_WHITE,BG);
  snprintf(line,sizeof(line),"CAN: %s",online?"ONLINE":"OFFLINE"); tft.drawString(line,10,48,2);
  snprintf(line,sizeof(line),"HEARTBEAT AGE: %lu ms",static_cast<unsigned long>(link_->heartbeatAgeMs())); tft.drawString(line,10,73,2);
  snprintf(line,sizeof(line),"SENSOR VALID MASK: 0x%02X",link_->sensorValidMask()); tft.drawString(line,10,98,2);
  snprintf(line,sizeof(line),"OUTPUT COMMAND MASK: 0x%02X",link_->outputCommandMask()); tft.drawString(line,10,123,2);
  snprintf(line,sizeof(line),"FAULT FLAGS: 0x%04X",link_->faultFlags()); tft.drawString(line,10,148,2);
  drawButton(10,177,300,26,"RECALIBRATE TOUCH",PANEL);
  tft.setTextColor(WARN,BG); tft.drawCentreString("Touch E-STOP is software only.",160,205,1);
}

void TouchUi::drawStartConfirm() {
  drawHeader("CONFIRM START");
  tft.setTextColor(TFT_WHITE,BG); tft.drawCentreString("START THIS KILN PROFILE?",160,78,4);
  tft.setTextColor(MUTED,BG); tft.drawCentreString("The S3 remains the control authority.",160,111,2);
  const char* reason=nullptr; const bool ready=readyToStart(reason);
  tft.setTextColor(ready?GOOD:WARN,BG);
  tft.drawCentreString(ready?"Controller reports ready":(reason?reason:"Not ready"),160,143,1);
  drawButton(20,170,125,34,"CANCEL",PANEL);
  drawButton(175,170,125,34,"SEND START",GOOD,ready);
}

void TouchUi::drawCalibration() {
  tft.fillScreen(BG); tft.fillRect(0,0,W,38,TFT_BLACK);
  tft.setTextColor(TFT_WHITE,TFT_BLACK); tft.drawString("TOUCH CALIBRATION",9,10,4);
  char text[40]; snprintf(text,sizeof(text),"Tap target %u of 3",calibrationStep_+1);
  tft.setTextColor(MUTED,BG); tft.drawCentreString(text,160,80,2);
  const int tx[]={28,292,28}, ty[]={52,52,188};
  const int x=tx[calibrationStep_], y=ty[calibrationStep_];
  tft.drawCircle(x,y,13,ACCENT); tft.drawFastHLine(x-19,y,38,ACCENT);
  tft.drawFastVLine(x,y-19,38,ACCENT);
  tft.setTextColor(WARN,BG); tft.drawCentreString("Keep kiln power disconnected.",160,202,1);
}

void TouchUi::render() {
  if(screen_==CALIBRATION) { drawCalibration(); return; }
  tft.fillScreen(BG);
  switch(screen_) {
    case HOME: drawHeader("KILN CONTROL"); drawHome(); break;
    case PROFILE: drawProfile(); break;
    case SENSORS: drawSensors(); break;
    case ZONES: drawZones(); break;
    case INFO: drawInfo(); break;
    case START_CONFIRM: drawStartConfirm(); break;
    default: break;
  }
  drawNavigation();
}

void TouchUi::handleTap(int x,int y) {
  if(y<38&&x>=248) {
    requestEmergencyShutdown(); profileUploaded_=false;
    setNotice("Software E-STOP sent. Use the hardwired E-stop too.");
    screen_=HOME; return;
  }
  if(y>=NAV_Y) {
    if(x<64) screen_=HOME; else if(x<128) screen_=PROFILE;
    else if(x<192) screen_=SENSORS; else if(x<256) screen_=ZONES; else screen_=INFO;
    redraw_=true; return;
  }
  switch(screen_) {
    case HOME: {
      if(y>=161&&y<=203) {
        const uint8_t state=link_->hasStatus()?link_->controllerState():kiln::STATE_BOOT;
        if(x<108) {
          if(state==kiln::STATE_FIRING||state==kiln::STATE_PAUSED) break;
          const char* reason=nullptr;
          if(readyToStart(reason)) { screen_=START_CONFIRM; notice_=nullptr; }
          else setNotice(reason?reason:"Controller is not ready.");
        } else if(x<213) {
          if(state==kiln::STATE_FIRING) link_->sendCommand(kiln::CMD_PAUSE);
          else if(state==kiln::STATE_PAUSED) link_->sendCommand(kiln::CMD_RESUME);
        } else if(state==kiln::STATE_FIRING||state==kiln::STATE_PAUSED) {
          link_->sendCommand(kiln::CMD_STOP); setNotice("Stop request sent to S3.");
        }
      }
      break;
    }
    case START_CONFIRM:
      if(y>=165&&y<=207) {
        if(x<160) screen_=HOME;
        else {
          const char* reason=nullptr;
          if(readyToStart(reason)&&link_->sendCommand(kiln::CMD_START)) {
            setNotice("Start request sent; waiting for S3 state update."); screen_=HOME;
          } else setNotice(reason?reason:"Start request could not be sent.");
        }
      }
      break;
    case PROFILE:
      if(y>=59&&y<=84) {
        if(x<64&&selectedSegment_>0) --selectedSegment_;
        if(x>255&&selectedSegment_+1<profileCount_) ++selectedSegment_;
      } else if(y>=88&&y<=119) {
        int16_t& v=profile_[selectedSegment_].targetDeciC;
        v=static_cast<int16_t>(constrain(v+(x>=250?250:-250),0,18000)); profileUploaded_=false;
      } else if(y>=121&&y<=151) {
        int16_t& v=profile_[selectedSegment_].rampDeciCPerHour;
        v=static_cast<int16_t>(constrain(v+(x>=250?50:-50),10,6000)); profileUploaded_=false;
      } else if(y>=154&&y<=184) {
        uint32_t& v=profile_[selectedSegment_].holdSeconds;
        if(x>=250) v=(v<=86100UL)?v+300UL:86400UL; else v=v>=300UL?v-300UL:0UL;
        profileUploaded_=false;
      } else if(y>=184&&y<=208) {
        if(x<82&&profileCount_<4) {
          ++profileCount_; selectedSegment_=profileCount_-1; profile_[selectedSegment_]={0,500,0}; profileUploaded_=false;
        } else if(x<160&&profileCount_>1) {
          --profileCount_; if(selectedSegment_>=profileCount_) selectedSegment_=profileCount_-1; profileUploaded_=false;
        } else if(x>=160) {
          setNotice(sendProfile()?"Profile sent to S3; check controller response.":"Profile send failed or profile values are invalid.");
        }
      }
      break;
    case SENSORS:
      if(y>=48&&y<=78) selectedSensor_=static_cast<uint8_t>(constrain((x-8)/78,0,3));
      else if(y>=96&&y<=132) {
        if(link_->connected()&&link_->hasStatus()) {
          const uint8_t next=static_cast<uint8_t>((link_->sensorMode(selectedSensor_)+1)%4);
          setNotice(link_->configureSensorMode(selectedSensor_,next)?"Sensor mode request sent.":"Could not send sensor mode.");
        } else setNotice("Connect to the S3 before changing sensor modes.");
      } else if(y>=143&&y<=174&&x>=220) {
        const uint8_t delta=x>=265?1:7;
        thermocoupleTypes_[selectedSensor_]=static_cast<uint8_t>((thermocoupleTypes_[selectedSensor_]+delta)%8);
        setNotice(link_->configureSensor(selectedSensor_,thermocoupleTypes_[selectedSensor_],sensorOffsetsDeciC_[selectedSensor_])?"Thermocouple configuration sent.":"Could not send thermocouple configuration.");
        saveSettings();
      } else if(y>=174&&y<=205&&x>=220) {
        int16_t& off=sensorOffsetsDeciC_[selectedSensor_];
        off=static_cast<int16_t>(constrain(off+(x>=265?5:-5),-500,500));
        setNotice(link_->configureSensor(selectedSensor_,thermocoupleTypes_[selectedSensor_],off)?"Calibration offset sent.":"Could not send calibration offset.");
        saveSettings();
      }
      break;
    case ZONES:
      if(y>=49&&y<=78) selectedZone_=static_cast<uint8_t>(constrain((x-8)/78,0,3));
      else if(y>=111&&y<=140) {
        int16_t& h=zones_[selectedZone_].hysteresisDeciC;
        h=static_cast<int16_t>(constrain(h+(x>=260?5:-5),5,100));
      } else if(y>=145&&y<=175) {
        int16_t& o=zones_[selectedZone_].offsetDeciC;
        o=static_cast<int16_t>(constrain(o+(x>=260?10:-10),-1000,1000));
      } else if(y>=180&&y<=207) {
        setNotice(link_->configureZone(selectedZone_,zones_[selectedZone_].hysteresisDeciC,
          10000UL,10000UL,zones_[selectedZone_].offsetDeciC)?"Zone configuration sent to S3.":"Could not send zone configuration.");
      }
      break;
    case INFO:
      if(y>=177&&y<=207) {
        const uint8_t state=link_->hasStatus()?link_->controllerState():kiln::STATE_BOOT;
        if(state==kiln::STATE_FIRING||state==kiln::STATE_PAUSED) setNotice("Cannot recalibrate during a firing.");
        else startCalibration();
      }
      break;
    default: break;
  }
  redraw_=true;
}

void TouchUi::handleCalibrationTap(int rawX,int rawY) {
  calibrationRaw_[calibrationStep_][0]=static_cast<float>(rawX);
  calibrationRaw_[calibrationStep_][1]=static_cast<float>(rawY);
  ++calibrationStep_;
  if(calibrationStep_>=3) {
    if(solveCalibration()) {
      calibrationValid_=true; prefs.putBytes("touchAffine",touchMatrix_,sizeof(touchMatrix_));
      screen_=HOME; setNotice("Touch calibration saved.");
    } else calibrationStep_=0;
  }
  redraw_=true;
}

void TouchUi::startCalibration() { calibrationStep_=0; screen_=CALIBRATION; redraw_=true; }

bool TouchUi::solveCalibration() {
  const float x1=calibrationRaw_[0][0], y1=calibrationRaw_[0][1];
  const float x2=calibrationRaw_[1][0], y2=calibrationRaw_[1][1];
  const float x3=calibrationRaw_[2][0], y3=calibrationRaw_[2][1];
  const float det=x1*(y2-y3)+x2*(y3-y1)+x3*(y1-y2);
  if(fabsf(det)<1000.0f) return false;
  const float sx[3]={28.0f,292.0f,28.0f}, sy[3]={52.0f,52.0f,188.0f};
  touchMatrix_[0]=(sx[0]*(y2-y3)+sx[1]*(y3-y1)+sx[2]*(y1-y2))/det;
  touchMatrix_[1]=(sx[0]*(x3-x2)+sx[1]*(x1-x3)+sx[2]*(x2-x1))/det;
  touchMatrix_[2]=(sx[0]*(x2*y3-x3*y2)+sx[1]*(x3*y1-x1*y3)+sx[2]*(x1*y2-x2*y1))/det;
  touchMatrix_[3]=(sy[0]*(y2-y3)+sy[1]*(y3-y1)+sy[2]*(y1-y2))/det;
  touchMatrix_[4]=(sy[0]*(x3-x2)+sy[1]*(x1-x3)+sy[2]*(x2-x1))/det;
  touchMatrix_[5]=(sy[0]*(x2*y3-x3*y2)+sy[1]*(x3*y1-x1*y3)+sy[2]*(x1*y2-x2*y1))/det;
  return true;
}

bool TouchUi::rawToScreen(int rx,int ry,int& x,int& y) const {
  if(!calibrationValid_) return false;
  x=static_cast<int>(lroundf(touchMatrix_[0]*rx+touchMatrix_[1]*ry+touchMatrix_[2]));
  y=static_cast<int>(lroundf(touchMatrix_[3]*rx+touchMatrix_[4]*ry+touchMatrix_[5]));
  x=constrain(x,0,W-1); y=constrain(y,0,H-1); return true;
}

bool TouchUi::readyToStart(const char*& reason) const {
  if(!link_||!link_->connected()||!link_->hasStatus()||!link_->statusFresh()) {
    reason="CAN link or S3 status is not fresh"; return false;
  }
  if(link_->faultFlags()!=0||link_->controllerState()==kiln::STATE_FAULT) {
    reason="A controller fault is active"; return false;
  }
  if(!profileUploaded_) { reason="Send a valid firing profile first"; return false; }
  if(link_->controllerState()!=kiln::STATE_IDLE&&link_->controllerState()!=kiln::STATE_ARMED) {
    reason="Controller is not in IDLE or ARMED"; return false;
  }
  uint8_t required=0;
  for(uint8_t i=0;i<4;i++) if(link_->sensorMode(i)!=0) required|=static_cast<uint8_t>(1u<<i);
  if(required==0) { reason="No active safety-monitored sensors"; return false; }
  if((link_->sensorValidMask()&required)!=required) {
    reason="An enabled safety sensor is not valid"; return false;
  }
  reason=nullptr; return true;
}

bool TouchUi::sendProfile() {
  if(!link_||!link_->connected()||profileCount_==0) return false;
  for(uint8_t i=0;i<profileCount_;i++) {
    if(profile_[i].targetDeciC<=0||profile_[i].rampDeciCPerHour<=0||
       profile_[i].targetDeciC>18000||profile_[i].rampDeciCPerHour>6000||
       profile_[i].holdSeconds>86400UL) return false;
  }
  if(!link_->beginProfileConfig(profileCount_)) return false;
  for(uint8_t i=0;i<profileCount_;i++)
    if(!link_->sendProfileSegment(i,profile_[i].targetDeciC,
       profile_[i].rampDeciCPerHour,profile_[i].holdSeconds)) return false;
  if(!link_->commitProfileConfig()) return false;
  profileUploaded_=true; saveSettings(); return true;
}

void TouchUi::setNotice(const char* message) { notice_=message; redraw_=true; }
void TouchUi::saveSettings() {
  if(calibrationValid_) prefs.putBytes("touchAffine",touchMatrix_,sizeof(touchMatrix_));
  prefs.putBytes("profile",profile_,sizeof(profile_));
  prefs.putUChar("profileCount",profileCount_);
  prefs.putBytes("tcTypes",thermocoupleTypes_,sizeof(thermocoupleTypes_));
  prefs.putBytes("tcOffsets",sensorOffsetsDeciC_,sizeof(sensorOffsetsDeciC_));
}
void TouchUi::loadSettings() {
  if(prefs.getBytesLength("touchAffine")==sizeof(touchMatrix_)) {
    prefs.getBytes("touchAffine",touchMatrix_,sizeof(touchMatrix_)); calibrationValid_=true;
  }
  if(prefs.getBytesLength("profile")==sizeof(profile_)) prefs.getBytes("profile",profile_,sizeof(profile_));
  profileCount_=constrain(prefs.getUChar("profileCount",1),1,4);
  if(prefs.getBytesLength("tcTypes")==sizeof(thermocoupleTypes_))
    prefs.getBytes("tcTypes",thermocoupleTypes_,sizeof(thermocoupleTypes_));
  if(prefs.getBytesLength("tcOffsets")==sizeof(sensorOffsetsDeciC_))
    prefs.getBytes("tcOffsets",sensorOffsetsDeciC_,sizeof(sensorOffsetsDeciC_));
}

#else

void TouchUi::begin(ControllerLink& link) {
  link_=&link;
  Serial.println("Touch UI disabled: this CYD display/touch profile is not verified.");
}
void TouchUi::update() {}

#endif
