#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks
#include "application.h"

#define ENABLE_DISPLAY
//#define ENABLE_LOGGING

// LED
#define LED_pin A2

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
  /* ms_index */     MS_MODE_AUTO, // start in automatice microstepping mode
  /* status */         STATUS_OFF, // start off
  /* rpm */                     1, // start speed [rpm]
  /* step_flow */ STEP_FLOW_UNDEF, // start undefined how much mass / step
  /* locked */              false  // start with the pump unlocked
);

// function to update the user interface based on changes in the pump state
void update_user_interface (PumpState state) {
  // led update
  if (state.status == STATUS_ON) {
    digitalWrite(LED_pin, HIGH);
  } else {
    // FIXME: ideally blink if it is holding
    digitalWrite(LED_pin, LOW);
  }
  // lcd update
  char rpm_buffer[10];
  sprintf(rpm_buffer, "%2.2f", state.rpm);
  #ifdef ENABLE_DISPLAY
    lcd.print_line(2, "Speed: " + String(rpm_buffer) + " rpm");
  #endif
  Serial.println("@UI - Speed: " + String(rpm_buffer) + " rpm");
  Serial.print("@UI - MS mode: " + String(state.ms_mode));
  if (state.ms_index == MS_MODE_AUTO) Serial.print(" (AUTO)");
  Serial.println();

  char status[5] = "";
  switch (state.status) {
    case STATUS_ON:
      strcpy(status, "on");
      break;
    case STATUS_OFF:
      strcpy(status, "off");
      break;
    case STATUS_HOLD:
      strcpy(status, "hold");
      break;
    case STATUS_ROTATE:
      strcpy(status, "rot");
      break;
    case STATUS_TRIGGER:
      strcpy(status, "trig");
      break;
  }

  char direction[3] = "";
  switch(state.direction) {
    case DIR_CW:
      strcpy(direction, "cw");
      break;
    case DIR_CC:
      strcpy(direction, "cc");
      break;
  }

  char locked[5] = "";
  if (state.locked) {
    strcpy(locked, " LOCK");
  }

  #ifdef ENABLE_DISPLAY
    lcd.print_line(1, "Status: " + String( status ) + " (MS " + String(state.ms_mode) + ")");
    lcd.print_line(3, "Dir: " + String( direction ) + String(locked));
  #endif
  Serial.println("@UI - Status: " + String(status));
  Serial.println("@UI - Direction: " + String(direction));
  Serial.println("@UI - Locked: " + String(locked));
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
  pinMode(LED_pin, OUTPUT);
  digitalWrite(LED_pin, LOW);

  // serial
  Serial.begin(9600);

  // time
  Time.zone(-5);
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
  }

  // stepper
  Serial.println("INFO: initialize stepper");
  pump.init(reset);

  // user interface update
  Serial.println("INFO: updating user interface");
  update_user_interface(pump.state);

  // connect device to cloud and register for listeners
  Serial.println("INFO: registering spark variables and connecting to cloud");
  Particle.subscribe("spark/", name_handler);
  Particle.connect();
}


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
