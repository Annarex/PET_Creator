#include "cfg.h"
#include "libs/GyverOLED.h"
#include "libs/EncButton.h"
#include "libs/GyverPID.h"
#include <Ticker.h>
#define DRIVER_STEP_TIME 6  // меняем задержку на 6 мкс
#include "libs/GyverStepper.h"
#include "MyNTC.h"
#include "beeps.h"
#include "tools.h"
#include "timer.h"
#include "scr.h"
#include <Preferences.h>

Preferences preferences;

#define WIFI_SSID "V{*(Kopitsa)*}V"
#define WIFI_PASS "26031986"
#define BOT_TOKEN "6351327860:AAEF3N0_u1ibzyQrQi4nrUb5Y_-1Xp3rfHg"
#define CHAT_ID "510109188"
#include <FastBot.h>
#define FB_DYNAMIC
FastBot bot(BOT_TOKEN);

#define SPEED_MAX 10
GStepper<STEPPER2WIRE> stepper(200 * CFG_STEP_DIV, CFG_STEP_STEP_PIN, CFG_STEP_DIR_PIN, CFG_STEP_EN_PIN);
#define GEAR_RATIO ((float)CFG_RED_G1 * (float)CFG_RED_G2 * (float)CFG_RED_G3)
#define BOBIN_ROUND_LENGTH ((float)3.1415926 * (float)CFG_BOBIN_DIAM) /* 74 * Pi = 232.478 mm */
const float REDCONST = BOBIN_ROUND_LENGTH /(360 * GEAR_RATIO * 1000);
float SpeedX10 = (float)CFG_SPEED_INIT * 10;

EncButton<EB_TICK, CFG_ENC_A, CFG_ENC_B, CFG_ENC_KEY> enc;// энкодер с кнопкой <A, B, KEY>
GyverOLED<SSD1306_128x32, OLED_NO_BUFFER> oled;
MyNTC therm(CFG_TERM_PIN, CFG_TERM_VALUE, CFG_TERM_B_COEFF, CFG_TERM_SERIAL_R);
GyverPID regulator(CFG_PID_P, CFG_PID_I, CFG_PID_D, 200);

String last_msg = "";
bool Heat = false, runMotor=false, statusFinishWork = false, statusGear = false,  statusWifi = false, minimalMsgTG = true, screen_changed = true;
unsigned long interactive = millis(), prevMillisTime = 0;
float targetTemp = CFG_TEMP_INIT, prePrevTemp = 0,
 prevTemp = 0, curTemp = 1, newTargetTemp = 0,
 fullLenght=0, newSpeedX10 = 0, newfullLenght = 0;
Ticker t_stepper, t_encoder, t_telegrambot;
int value = 0, ti_start=0, ti_end=0;
CHANGE_MODE cmode = CHANGE_NO;
BAR_SCREENS iscreen = MAIN_SCREEN, liscreen = SECOND_SCREEN;

Timer timScreenUpdate;

Scr screen("1.0", cmode, targetTemp, prePrevTemp, prevTemp, curTemp, SpeedX10, REDCONST);

void tickStepper(){
  stepper.tick();
  }

void tickEncoder(){
  enc.tick();
  }

void encRotationToValue (float* value, float inc, float minValue, float maxValue);

void setup() {
  Serial.begin(115200);
  timScreenUpdate.start(200);
  pinMode(CFG_ENDSTOP_PIN, INPUT_PULLUP);
  pinMode(CFG_SOUND_PIN, OUTPUT);
  enc.pullUp();
  enc.setEncReverse(true);
  ledcSetup(0, 50000, 8);
  ledcAttachPin(CFG_HEATER_PIN, 0);
  stepper.setRunMode(KEEP_SPEED);   // режим поддержания скорости
  stepper.autoPower(true);
  stepper.setAcceleration(200);
  stepper.setSpeedDeg(mmStoDeg((float)SPEED_MAX));
  stepper.brake();
  t_stepper.attach_ms(1, tickStepper);
  stepper.reset(); 

  oled.init();              // инициализация
  oled.flipH(true);
  oled.flipV(true);
  Wire.setClock(400000L);   // макс. 800'000
  oled.clear();
   //заставка
  #if defined(CFG_SOUND_START)
    beepE();
  #endif
  screen.DrawStartScreen();
  fullLenght = getFullMilageFromPref();
  screen.DrawMainScreen(fullLenght);

  screen.printLineInfo(statusWifi, iscreen, ti_start, ti_end);
  statusWifi = connectWiFi();
  screen.printLineInfo(statusWifi, iscreen, ti_start, ti_end);

  bot.setChatID(CHAT_ID);
  bot.attach(havingNewMsgInTelegram);
  bot.skipUpdates();
  if(!minimalMsgTG) bot.sendMessage("Станок запущен!");
  regulator.setpoint = targetTemp; 
  t_encoder.attach_ms(20, tickEncoder);     
  }

void loop() {
  bot.tick();
  //Условный оператор изменения рабочего экрана при повороте энкодера,
  //при условии, что на данный момент ничего не изменяется.
  if(cmode == CHANGE_NO){
    liscreen = iscreen;
    if(enc.isLeft()){
      iscreen = BAR_SCREENS((iscreen > MAIN_SCREEN)?(iscreen-1):THIRD_SCREEN);
      screen.printLineBar(iscreen);
      }
    if(enc.isRight()){
      iscreen = BAR_SCREENS((iscreen < THIRD_SCREEN)?(iscreen+1):MAIN_SCREEN);
      screen.printLineBar(iscreen);
      }
    screen_changed = (liscreen != iscreen);
    if(screen_changed){
      oled.clear();
      screen.printLineInfo(statusWifi, iscreen, ti_start, ti_end);     
      }
    }
  //Комментарий
  switch(iscreen){
    case MAIN_SCREEN: 
      if(timScreenUpdate.ready() || screen_changed) showMainScreen(screen_changed);
      handleMainScreen();
      break;
    case SECOND_SCREEN:
      if(timScreenUpdate.ready() || screen_changed) showSettingScreen(screen_changed); 
      handleSettingScreen();
      break;
    case THIRD_SCREEN: 
      if(timScreenUpdate.ready() || screen_changed) showHistoryScreen(screen_changed);
      handleHistoryScreen();
      break;
    }
  mainProccess();    
  }

void mainProccess(){
  if (runMotor) {
    ti_end = millis();
    screen.printTimeWork(ti_start, ti_end);
  }
  curTemp = therm.getTempDenoised();
  if (curTemp > CFG_TEMP_MAX - 10) emStop(OVERHEAT);
  if (curTemp < -10) emStop(THERMISTOR_ERROR);
  regulator.input = curTemp;
  if (curTemp != prevTemp) {
      prePrevTemp = prevTemp;
      prevTemp = curTemp;
    }

  if (Heat) {
      int pidOut = (int) constrain(regulator.getResultTimer(), 0, 255);
      ledcWrite(0, pidOut);
      debugTemp(curTemp, pidOut);
      } 
  else {
      ledcWrite(0, 0);
      debugTemp(curTemp, 0);
      } 
  if(digitalRead(CFG_ENDSTOP_PIN)) {
      if(runMotor) {
        runMotor = false;
        motorCTL(-1);
        Heat = false;
        statusFinishWork = true; 
        ti_end = millis();
        bot.sendMessage("Статус протяжки: \xE2\x9C\x85Выполнена\n\n"+getResultStats());
        saveFullMilageToPref();
        beepI();
        beepI();
       }
      } 
  else statusFinishWork = false; 
}

//Функция для работы на главном экране
void handleMainScreen(){
  newTargetTemp = targetTemp;
  newSpeedX10 = SpeedX10;
  if(enc.click()){
    newfullLenght = fullLenght;
    cmode = CHANGE_MODE((cmode < CHANGE_HIST_LENGHT)? cmode+1 : 0);
    interactiveSet();
    
    }
  if (!isInteractive()) {
      cmode = CHANGE_NO;
    }
  if(cmode == CHANGE_TEMPERATURE) {
      encRotationToValue(&newTargetTemp, 1, CFG_TEMP_MIN, CFG_TEMP_MAX - 10);
      if (enc.isHolded()) changeHeatingState(false);
      if (newTargetTemp != targetTemp) {
        targetTemp = newTargetTemp;
        regulator.setpoint = newTargetTemp;
        screen.printTemps(newTargetTemp, therm.getTemp());
        }
    } else 
  if (cmode == CHANGE_SPEED) {
      encRotationToValue(&newSpeedX10, 1, 0, SPEED_MAX * 10);
      if (enc.isHolded()) changeStepingState(false);
      if (newSpeedX10 != SpeedX10) {
        SpeedX10 = newSpeedX10;
        if (runMotor) motorCTL(newSpeedX10);        // в градусах/сек
        screen.printSpeed(newSpeedX10);
        }
    } else
  if (cmode == CHANGE_HIST_LENGHT) {
      encRotationToValue(&newfullLenght, 0.25, 0, 10000);
      if (newfullLenght != fullLenght) {
        screen.printMilage(stepper.getCurrentDeg(), newfullLenght);
        if (enc.isHolded()) { 
          fullLenght = newfullLenght;
          saveMilageToPref(fullLenght);
          beepI();
          }
        }
    }
}

void showMainScreen(bool screen_changed){
  if (screen_changed){screen.printTemps(targetTemp, curTemp);   }
  
  screen.printMilage(stepper.getCurrentDeg(), fullLenght);
  screen.printSpeed(SpeedX10); // to clear selection
  if (runMotor) {
    screen.printMilage(stepper.getCurrentDeg(), fullLenght);
    screen.printTimeWork(ti_start, ti_end);
    }
  curTemp = therm.getTempDenoised();
  
  if (curTemp != prevTemp) {
      screen.printTemps(targetTemp, curTemp);
    }
  if(digitalRead(CFG_ENDSTOP_PIN)) {
    if(!runMotor) {
      screen.printGearStatus(statusGear);
      } else {
      screen.printHeaterStatus(Heat);
      screen.printWorkStatus(statusFinishWork);
      }
    }
  //вырезано из функции переключения нагрева
  screen.printHeaterStatus(Heat);
}

void handleSettingScreen(){}
void showSettingScreen(bool screen_changed){}
void handleHistoryScreen(){}
void showHistoryScreen(bool screen_changed){}

void havingNewMsgInTelegram(FB_msg& msg) {
  last_msg = msg.text;
  if (msg.text == "/menu") {
    bot.showMenu("Показать данные \t Обновить \t Удалить");
    Serial.println(msg.chatID);
    }
  else if (msg.text == "/stats") {
    String str = "Основные параметры станка:\n";
    str += getStats();
    bot.sendMessage(str);
    }
  else if (msg.text == "/heating") {
    changeHeatingState(true);
    }
  else if (msg.text == "/steping") {
    changeStepingState(true);
    }
  else  if (msg.text == "/reboot") {
    bot.sendMessage("Перезагрузка...");
    ESP.restart();
    }
  else  { Serial.println(msg.text);}
  }
void changeHeatingState(bool withReturnStateMsg){
  cmode = CHANGE_TEMPERATURE;
  Heat = ! Heat;
  if(withReturnStateMsg){
    String mes ="Нагрев ";
    mes += Heat?"включен! \xF0\x9F\x9F\xA2":"выключен! \xF0\x9F\x94\xB4";
    bot.sendMessage(mes); 
  } 
  } 

void changeStepingState(bool withReturnStateMsg){
  cmode == CHANGE_SPEED;
  runMotor = ! runMotor;
  if (runMotor) {
    stepper.setCurrent(0);
    fullLenght = getFullMilageFromPref();
    motorCTL(SpeedX10);
    ti_start = ti_end = millis();
  } else {
    saveFullMilageToPref();
    motorCTL(-1);
    runMotor = false;
  }
  if(withReturnStateMsg){
    String mes ="Двигатель ";
    mes += runMotor?"включен! \xF0\x9F\x9F\xA2":"выключен! \xF0\x9F\x94\xB4";
    bot.sendMessage(mes);
  }
  interactiveSet();
  } 

String getStats(){
    String str = "";
    str += "\xE2\x8C\x9A Время выполнения: "+getFormatedTimeWork(ti_start, ti_end)+"\n";
    str += "\xF0\x9F\x8C\xA1 Температура: "+String(curTemp)+"\n";
    str += "\xF0\x9F\x9A\x80 Скорость: "+String(((float)SpeedX10/SPEED_MAX))+"\n";
    str += "\xF0\x9F\x93\x8F Метраж: "+String(getMilage());
    return str;
  }

String getResultStats(){
    String str = "";
    str += "\xE2\x8C\x9A Время выполнения протяжки: "+getFormatedTimeWork(ti_start, ti_end)+"\n";
    str += "\xF0\x9F\x93\x8F Протянуто метров: - за цикл("+String(getMilage())+") - всего("+getFullMilageFromPref()+")\n";
    return str;
  }

void debugTemp(float temp, int out) {
  #if defined(SERIAL_DEBUG_TEMP)
      static long debug_time;
      if (debug_time < millis() ) {
        debug_time = millis() + 200;
        Serial.print(temp);
  #if defined(SERIAL_DEBUG_TEMP_PID)
        Serial.print(' ');
        Serial.print(out);
  #endif // end SERIAL_DEBUG_TEMP_PID
        Serial.println(' ');
      }
  #endif //end SERIAL_DEBUG_TEMP
  } 

void emStop(int reason) {
  ti_end = millis();
  runMotor = false;
  Serial.print("ErTd:");Serial.println(therm.getTempDenoised());
  motorCTL(-1);
  stepper.disable();
  Heat = false;
  ledcWrite(0, 0);  
  screen.printScreenError(reason);
  String err = "\xF0\x9F\x86\x98 Ошибка: \n";
  err += screen.getTextReason(reason) + "\n";
  err += "\xE2\x8C\x9A Время прерывания: "+getFormatedTimeWork(ti_start, ti_end)+"\n";
  err += "\xF0\x9F\x93\x8F Текущий Метраж: "+String(getMilage());
  bot.sendMessage(err);
  saveFullMilageToPref();
  for(int i=0;i<2;i++){
    beepO();
    delay(60000);
  }
  ESP.restart();
  }

void motorCTL(float setSpeedX10) {
  #if defined(SERIAL_DEBUG_STEPPER)
    Serial.print(stepper.getSpeedDeg());
    Serial.print(",\t");
    Serial.print(stepper.getSpeed());
    Serial.print(",\t");
    Serial.print(stepper.getCurrent());
    Serial.print(",\t");
  #endif // SERIAL_DEBUG_STEPPER
  // oled.setScale(1);
  // oled.setCursor(117, 1);
  /////////////добавить проверку экрана
  if (setSpeedX10 > 0) {
    stepper.setSpeedDeg(mmStoDeg((float)setSpeedX10/10));        // [degree/sec]
    statusGear = true;
  } else {
    statusGear = (setSpeedX10 == 0);
    if(setSpeedX10 == 0){stepper.stop();}  else{stepper.brake();}    
  }
  #if defined(SERIAL_DEBUG_STEPPER)
    Serial.print((float)setSpeedX10/10);
    Serial.print(",\t");
    Serial.print(stepper.getSpeedDeg());
    Serial.print(",\t");
    Serial.print(stepper.getCurrent());
    Serial.println(" ");
  #endif // SERIAL_DEBUG_STEPPE
  }

void encRotationToValue (float* value, float inc, float minValue, float maxValue) {
      if(enc.isFast()){
        if (enc.isRightH()) { *value += inc * 5; interactiveSet(); }    // если был быстрый поворот направо, увеличиваем на 10
        if (enc.isLeftH()) { *value -= inc * 5; interactiveSet(); }    // если был быстрый поворот налево, уменьшаем на на 10
      } else {
        if(enc.isRight()) { *value += inc; interactiveSet(); }     // если был поворот направо, увеличиваем на 1
        if (enc.isLeft())  { *value -= inc; interactiveSet(); }     // если был поворот налево, уменьшаем на 1
      }
      if (*value < minValue) *value = minValue;
      if (*value > maxValue) *value = maxValue;
  }

void interactiveSet() {
  interactive = millis() + 15000;
  }

boolean isInteractive() {
  return millis() < interactive;
  }

float getFullMilageFromPref(){
  preferences.begin("Params", false);
  float full_mill = preferences.getFloat("full_milage", 0);
  preferences.end();
  Serial.print("EEFull Mil:");Serial.println(full_mill);
  return full_mill;
  }

void saveMilageToPref(float full_mill){
  Serial.print("Full Mil:");Serial.println(full_mill);
  preferences.begin("Params", false);
  preferences.putFloat("full_milage", full_mill);
  preferences.end();
  getFullMilageFromPref();
  }

void saveFullMilageToPref(){
  float full_mill = getFullMilageFromPref() + getMilage();
  saveMilageToPref(full_mill);
  }

float getMilage() {
  return stepper.getCurrentDeg() * REDCONST;
  }

long mmStoDeg(float mmS) {
  return mmS / (REDCONST * 1000);
  }

bool connectWiFi() {
  Serial.print ("Wifi ");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  oled.setCursor(10, 3);
  screen.printConnectedStatus(false);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() > 15000) ESP.restart();
  }
  Serial.println("Connected");
  return true;
  }