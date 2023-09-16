#include "cfg.h"
void beepE() {
  digitalWrite(CFG_SOUND_PIN, 1);
  delay(50);
  digitalWrite(CFG_SOUND_PIN, 0);
  delay(50);
}
void beepI() {
  beepE();
  beepE();
}
void beepT() {
  digitalWrite(CFG_SOUND_PIN, 1);
  delay(600);
  digitalWrite(CFG_SOUND_PIN, 0);
  delay(200);
}
void beepO() {
  beepT();
  beepT();
  beepT();
}
void beep(int k, int j){
  for(int k=0;k<=j;k++){
   beepI();
  }
}
