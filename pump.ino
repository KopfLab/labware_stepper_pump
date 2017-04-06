#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks
#include "application.h"

#define ENABLE_DISPLAY
//#define ENABLE_LOGGING

// LED
#define LED_pin A2

#define POT_pin A4

// reset
#define RESET_pin A5

// display
#ifdef ENABLE_DISPLAY
#include "Display.h"
Display lcd (
  /* i2c address */  0x27,
  /* lcd width */      20,
  /* lcd height */      4,
  /* message width */   7,
  /* sg show time */ 3000);
#endif

// google spreadsheets webhook
#ifdef ENABLE_LOGGING
#include "GsWebhook.h"
GsWebhook gs ("post_to_gs");
#endif

/*** pump controller ***/
#include "PumpController.h"

// pins of the stepper
StepperPins pins(
  /* dir */     D2,
  /* step */    D3,
  /* enable */  D7,
  /* ms1 */     D6,
  /* ms2 */     D5,
  /* ms3 */     D4
);

// microstep modes of the chip (DRV8825)
const int microstep_modes_n = 6;
MicrostepMode microstep_modes[microstep_modes_n] =
  {
    /* n_steps, MS1, MS2, MS3 */
    {1,  LOW,  LOW,  LOW},  // full step
    {2,  HIGH, LOW,  LOW},  // half step
    {4,  LOW,  HIGH, LOW},  // quarter step
    {8,  HIGH, HIGH, LOW},  // eight step
    {16, LOW,  LOW,  HIGH}, // sixteenth step
    {32, HIGH, LOW,  HIGH},  // 1/32 step
  };

// settings of the pump and microcontroller
PumpSettings settings(
  /* steps */      200, // 200 steps/rotation
  /* gearing */     1, // 5.18:1 planetary gear box
  /* max_speed */  1000  // max steps/s (how often particle can reliably call update)
);

// initial state of the pump
PumpState state(
  /* direction */          DIR_CW, // start clockwise
  /* status */         STATUS_OFF, // start off
  /* rpm */                     1, // start speed [rpm]
  /* step_flow */ STEP_FLOW_UNDEF, // start undefined how much mass / step
  /* locked */              false  // start with the pump unlocked
  // no specification of microstepping mode = automatic mode
);

// state information & function to update the user interface(s) based on changes in the pump state
char state_information[600] = "";

char rpm_lcd[10];
char ms_mode_lcd[5];
char status_lcd[5];
char dir_lcd[3];
char locked_lcd[6];

char rpm_web[15];
char ms_mode_web[10];
char status_web[50];
char dir_web[20];
char locked_web[20];

void update_user_interface (PumpState state) {
  // led update
  if (state.status == STATUS_ON) {
    digitalWrite(LED_pin, HIGH);
  } else {
    // FIXME: ideally blink if it is holding
    digitalWrite(LED_pin, LOW);
  }

  // user interface update text
  get_pump_state_status_info(state.status, status_lcd, sizeof(status_lcd), status_web, sizeof(status_web));
  get_pump_state_speed_info(state.rpm, rpm_lcd, sizeof(rpm_lcd), rpm_web, sizeof(rpm_web));
  get_pump_state_ms_info(state.ms_auto, state.ms_mode, ms_mode_lcd, sizeof(ms_mode_lcd), ms_mode_web, sizeof(ms_mode_web));
  get_pump_state_direction_info(state.direction, dir_lcd, sizeof(dir_lcd), dir_web, sizeof(dir_web) );
  get_pump_stat_locked_info(state.locked, locked_lcd, sizeof(locked_lcd), locked_web, sizeof(locked_web));

  // serial (for debugging)
  Serial.println("@UI - Status: " + String(status_lcd));
  Serial.println("@UI - Speed: " + String(rpm_lcd) + " rpm");
  Serial.println("@UI - MS mode: " + String(ms_mode_lcd));
  Serial.println("@UI - Direction: " + String(dir_lcd));
  Serial.println("@UI - Locked: " + String(locked_lcd));

  // lcd
  #ifdef ENABLE_DISPLAY
    lcd.print_line(1, "Status: " + String(status_lcd) + " (MS" + String(ms_mode_lcd) + ")");
    lcd.print_line(2, "Speed: " + String(rpm_lcd) + " rpm");
    lcd.print_line(3, "Dir: " + String(dir_lcd) + " " + String(locked_lcd));
  #endif

  // web
  snprintf(state_information, sizeof(state_information),
    "{\"status\":\"%s\", \"rpm\":\"%s\", \"ms\":\"%s\", \"dir\":\"%s\", \"lock\":\"%s\"}",
    status_web, rpm_web, ms_mode_web, dir_web, locked_web);
}

// callback function for pump commands
void report_pump_command (const PumpController& pump) {
  // report command on user interface
  Serial.println("INFO: pump command - " +
    String(pump.command.type) + " " +
    String(pump.command.variable) + " " +
    String(pump.command.value));
  #ifdef ENABLE_DISPLAY
    lcd.show_message(pump.command.type, pump.command.variable); // shown on lcd
  #endif
  update_user_interface(pump.state);
  #ifdef ENABLE_LOGGING
    // log in google spreadsheet
    gs.send(pump.command.type, pump.command.variable, pump.command.value, pump.command.units, pump.command.msg);
  #endif
}

PumpController pump(pins, microstep_modes, microstep_modes_n, settings, state, report_pump_command);

// device name
char device_name[20];
void name_handler(const char *topic, const char *data) {
  strncpy ( device_name, data, sizeof(device_name) );
  Serial.println("INFO: device ID " + String(device_name));
  #ifdef ENABLE_DISPLAY
    lcd.print_line(4, "ID: " + String(device_name));
  #endif
}


// using system threading to improve timely stepper stepping
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);
bool name_handler_registered = false;

void setup() {

  // pins
  pinMode(RESET_pin, INPUT_PULLDOWN);
  pinMode(POT_pin, INPUT);
  pinMode(LED_pin, OUTPUT);
  digitalWrite(LED_pin, LOW);

  // serial
  Serial.begin(9600);

  // time
  Time.zone(-6); // Mountain Daylight Time -6 / Mountain Standard Time -7
  Serial.print("INFO: starting up at time ");
  Serial.println(Time.format("%Y-%m-%d %H:%M:%S"));

  // inits
  #ifdef ENABLE_DISPLAY
    Serial.println("INFO: initialize LCD");
    lcd.init();
    lcd.print_line(1, "Initializing...");
    lcd.print_line(4, "ID: waiting...");
  #endif
  #ifdef ENABLE_LOGGING
    Serial.println("INFO: initialize gs logger")
    gs.init();
  #endif

  // check for reset
  bool reset = FALSE;
  if(digitalRead(RESET_pin) == HIGH) {
    reset = TRUE;
    Serial.println("INFO: reset request detected");
    #ifdef ENABLE_DISPLAY
      lcd.print_line(1, "Resetting...");
      delay(1000);
    #endif
  }

  // stepper
  Serial.println("INFO: initialize stepper");
  pump.init(reset);

  // user interface update
  Serial.println("INFO: updating user interface");
  update_user_interface(pump.state);

  // connect device to cloud and register for listeners
  Serial.println("INFO: registering spark variables and connecting to cloud");
  Particle.variable("state", state_information);
  Particle.subscribe("spark/", name_handler);
  Particle.connect();
}


// manual speed control

#define MANUAL_read_interval 500 // [ms]
long last_manual_read = 0;
float last_manual_rpm = -1;
float current_manual_rpm = 0;

#define MANUAL_POT_DEBOUNCE 50    // the debounce time in ms



void loop() {
  if (!name_handler_registered && Particle.connected()){
    // running this here becaus we're in system thread mode and it won't work until connected
    name_handler_registered = Particle.publish("spark/device/name");
    Serial.println("INFO: name handler registered");
  }



  #ifdef ENABLE_DISPLAY
    lcd.update();
  #endif
  pump.update();
}
