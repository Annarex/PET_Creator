/*   Stepper driver microstep devision */
#define CFG_STEP_DIV 8
/* Which pin stepper driver STEP pin connected */
#define CFG_STEP_STEP_PIN 14
/* Which pin stepper driver DIR pin connected */
#define CFG_STEP_DIR_PIN 27
/* Which pin stepper driver EN pin connected */
#define CFG_STEP_EN_PIN 12
/* Invert stepper rotation direction (comment out to disable invertion)*/
#define CFG_STEP_INVERT 
/* Which pin encoder DT pin connected */
#define CFG_ENC_KEY 15
/* Which pin encoder DT pin connected */
#define CFG_ENC_A 16
/* Which pin encoder SW pin connected */
#define CFG_ENC_B 17
/*  Type of encoder: TYPE1 or TYPE2 */
#define CFG_ENC_TYPE TYPE2
/* Initial target temperature [degree C]*/
#define CFG_TEMP_INIT 200
/* Maximum allowed temperature [degree C], allowed to set to 10 degree less */
#define CFG_TEMP_MAX 270
/* Minimum allowed temperature to set [degree C] */
#define CFG_TEMP_MIN 120
/* Which pin termistor connected to*/
#define CFG_TERM_PIN 34
/* Thermistor resistance at 25 degrees C [Om] */
#define CFG_TERM_VALUE 100000
/* Thermistor temperature for nominal resistance (almost always 25 C) [degree C] */
#define CFG_TERM_VALUE_TEMP 25
/* The beta coefficient of the thermistor (usually 3000-4000) */
#define CFG_TERM_B_COEFF 3950
/* the value of the 'other' resistor [Om] */
#define CFG_TERM_SERIAL_R 4700
/* Which pin endstop connected to */
#define CFG_ENDSTOP_PIN 23
/* Extra length to pull after end stop triggered [m] */
#define CFG_PULL_EXTRA_LENGTH 0//0.07
/* Which pin emergency endstop connected to */
#define CFG_EMENDSTOP_PIN 11
/* PID regulator coefficients */
//PID p: 12.69  PID i: 0.71 PID d: 57.11
#define CFG_PID_P 30
#define CFG_PID_I 0.065
#define CFG_PID_D 20
/* Which pin heater MOSFET connected to */
#define CFG_HEATER_PIN 25
/* Target filament bobin diameter [mm] */
#define CFG_BOBIN_DIAM 74
/* Initial pull speed [mm/s] */
#define CFG_SPEED_INIT 2.5
/* Buzzer pin connection */
#define CFG_SOUND_PIN 13
/* Enable startup sound (comment to disable) */
#define CFG_SOUND_START
/*  Chouse reductor type. 
/* PETPull-2 Zneipas reductor variant (1:65.68(18) ratio)*/
#define CFG_RED_PP2

/* DON'T CHANGE ANYTHING AFTER THIS LINE IF YOU NOT SHURE TO 146% */

/* 
  enable/disable serial debug output
*/
//#define SERIAL_DEBUG_TEMP 
//#define SERIAL_DEBUG_TEMP_PID 
#define SERIAL_DEBUG_STEPPER

/* Gear ratio for PETPull-2 Zneipas reductor variant */
/* 
  8 teeth gear on stepper shaft interact with
  34 teeth gear of 1-st gear.
  11 teeth of 1-st gear interact with 
  34teeth gear of 2-nd gear.
  11 teeth of 2-nd gear interact with 
  55 teeth of target bobin

  reduction ratio 65.68(18)
*/
#if defined(CFG_RED_PP2)
#define CFG_RED_G1 34/8
#define CFG_RED_G2 34/11
#define CFG_RED_G3 55/11
#endif //CFG_RED_PP2