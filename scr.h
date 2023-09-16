#ifndef _Screen_h
#define _Screen_h

#include "libs/GyverOLED.h"
#include "cfg.h"
/* Статусы интерактивности*/
enum CHANGE_MODE {CHANGE_NO, CHANGE_TEMPERATURE, CHANGE_SPEED, CHANGE_HIST_LENGHT};
enum BAR_SCREENS {MAIN_SCREEN, SECOND_SCREEN, THIRD_SCREEN};
enum ERRORS {OVERHEAT=1, THERMISTOR_ERROR=2};

MyNTC therm1(CFG_TERM_PIN, CFG_TERM_VALUE, CFG_TERM_B_COEFF, CFG_TERM_SERIAL_R);

class Scr{
  private:
  String ver;
  CHANGE_MODE *whatToChange;
  float REDCONST, *prePrevTemp, *prevTemp, *curTemp, *targetTemp, *SpeedX10;
  GyverOLED<SSD1306_128x32, OLED_NO_BUFFER> oled;
  public:
  Scr(String ver, CHANGE_MODE &whatToChange, float &targetTemp, float &prePrevTemp, float &prevTemp,float &curTemp, float &SpeedX10,float redconst){
    this->ver = ver;
    this->whatToChange = &whatToChange;    
    this->targetTemp = &targetTemp;
    this->prePrevTemp = &prePrevTemp;
    this->prevTemp = &prevTemp;
    this->curTemp = &curTemp;
    this->SpeedX10 = &SpeedX10;
    this->REDCONST = redconst;
    }
  
  void DrawStartScreen(int del = 1500){
    oled.setScale(2);
    oled.setCursor(0, 0);
    oled.println("PET_Creator");
    oled.setScale(1);
    oled.setCursor(0, 2);
    oled.print("ver ");
    oled.print(ver);
    delay(del);
    };
  void DrawMainScreen(float pref_milage){
    oled.clear();
    oled.setScale(1);
    printTemps(*targetTemp, therm1.getTemp());
    printSpeed(*SpeedX10);
    printMilage(0.0, pref_milage);
  };

  void printScreenError(int reason){
    oled.clear();
    oled.invertText(true);
    oled.setScale(3);
    oled.setCursorXY(3,2);
    oled.println("*STOP!*");
    oled.invertText(false);
    oled.setScale(1);
    oled.setCursor(40,3);
    oled.println(getTextReason(reason));
    }
  String getTextReason(int reason){
    switch (reason) { 
      case OVERHEAT:
        return "Overheat";
        break;
      case THERMISTOR_ERROR:
        return "Thermistor";
        break;
      }
      return "";
    }
  void printTemps(float target_temp, float cur_temp){
    oled.setScale(1); 
    oled.setCursor(2,0);
    oled.print("Температ");
    oled.setCursor(55,0 );
    if((int)*curTemp != (int)*prePrevTemp){
      oled.print("   ");
      oled.setCursor(55,0 );
      if(cur_temp>20)oled.print((int)cur_temp, 1); 
      else oled.print("000");
    }
    oled.setCursor(72,0 );
    oled.println(" >");
    if(*whatToChange == CHANGE_TEMPERATURE)  oled.invertText(true);
    oled.setCursor(86, 0);
    oled.println((int)target_temp, 1);
    oled.invertText(false);
    oled.setCursor(105,0 );
    printSymbol(degreeSymbol);
    }

  void printHeaterStatus(boolean status) {
    oled.setCursor(117, 0);
    if(status) 
      printSymbol(symbolPlay);
    else
      printSymbol(symbolPause);
    }

  void printGearStatus(boolean status) {
    oled.setCursor(117, 1);
    printSymbol(status?symbolPlay:symbolPause);
    }
  void printWorkStatus(boolean status) {
    oled.setCursor(117, 2);
    printSymbol(status?symbolChecked:symbolUnChecked);
    }
   
  void printLineBar(int i_screen) {
    for(int i=0;i<3;i++){
      oled.setCursor(30+(i*10),3);
      printSymbol(i==i_screen?symbolActive:symbolNoActive);
      }
    }
  void printTimeWork(int ti_start, int ti_end) {
      oled.setCursor(80,3);
      oled.println(getFormatedTimeWork(ti_start, ti_end));
    }
  void printLineInfo(boolean statusWifi, BAR_SCREENS iscreen, int ti_start, int ti_end){
      printConnectedStatus(statusWifi);
      printLineBar(iscreen);
      printTimeWork(ti_start, ti_end);
    } 
  void printConnectedStatus(boolean status) {
    oled.setCursor(0, 3);
    printSymbol(status?symbolConnected:symbolNotConnected);
    }

  void printSpeed(float s){
    // s -speed in mm/s * 10, print in mm/s
    oled.setScale(1);      
    oled.setCursor(2,1);
    oled.print("Скорость");
    
    oled.setCursor(55, 1);
    if(*whatToChange == CHANGE_SPEED)  oled.invertText(true);
    oled.print((float)s/10, 1);
    if (s<100) oled.print(" "); //fix display garbage 
    oled.invertText(false);
    oled.setCursor(80,1);
    oled.print("mm/s");
    }

  void printMilage(float m, float fm){
    // m - current stepper position in degree, output to display in meters
    oled.setScale(1);
    oled.setCursor(2,2);
    oled.print("Метраж ");
    // oled.setCursor(55, 2);
    char str[40];
    sprintf(str, "%02.1f",  (m * REDCONST));
    oled.print(str);
    
    oled.print("(");
    if(*whatToChange == CHANGE_HIST_LENGHT)  oled.invertText(true); 
    sprintf(str, "%.1f",  (fm));
    oled.print(str);
    oled.invertText(false); 
    oled.print(") m");  
    }

  
  String getFormatedTimeWork(int ti_start, int ti_end){
  uint32_t sec = (ti_end-ti_start)/ 1000ul;
  char str[10];
  sprintf(str, "%02d:%02d:%02d",  (sec / 3600ul), ((sec % 3600ul) / 60ul), ((sec % 3600ul) % 60ul));
  return String(str);
  }
  unsigned char degreeSymbol[7] = {0x02, 0x05, 0x02, 0x38, 0x44, 0x44, 0x28}; 
  unsigned char symbolPlay[7] = {0x7f, 0x7f, 0x3e, 0x3e, 0x1c, 0x1c, 0x08}; 
  unsigned char symbolPause[7] = {0x00, 0x7f, 0x7f, 0x00, 0x7f, 0x7f, 0x00}; 
  unsigned char symbolNoActive[7] = {0x1c, 0x3e, 0x77, 0x77, 0x77, 0x3e, 0x1c}; 
  unsigned char symbolActive[7] = {0x3e, 0x41, 0x5d, 0x5d, 0x5d, 0x41, 0x3e};
  unsigned char symbolChecked[7] ={0x3e, 0x41, 0x49, 0x51, 0x49, 0x44, 0x32};
  unsigned char symbolUnChecked[7] ={0x3e, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3e};
  unsigned char symbolConnected[7] ={0x60, 0x70, 0x78, 0x7c, 0x7e, 0x7f, 0x7f};
  unsigned char symbolNotConnected[7] ={0x23, 0x36, 0x3c, 0x38, 0x36, 0x6f, 0x5f};
  
  void printSymbol(unsigned char *symbol){
    for(unsigned int i = 0; i < 7; i++) {
      oled.drawByte(pgm_read_byte(&(symbol[i])));
    }
    }
};
#endif
