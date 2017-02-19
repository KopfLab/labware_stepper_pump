#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks
#include "application.h"

//#define ENABLE_DISPLAY
//#define ENABLE_LOGGING

// LED
#define LED_pin A2

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

// pump controller
struct StepperPins;
#include "PumpController.h"
StepperPins pins(
  /* dir */     D3,
  /* step */    D4,
  /* enable */  D2,
  /* ms1 */     D5,
  /* ms2 */     D6,
  /* ms3 */     D7
);
PumpSettings settings(
  /* step_angle */         0.35, // 200 steps/rotation with 5.18:1 planetary gear box
  /* step_flow */             0, // undefined
  /* direction */        DIR_CW, // direction
  /* microstep */             1, // step size
  /* max_speed */          1200, // what should be the max speed?
  /* speed */                 0  // start speed
);

// function to update the user interface based on changes in the pump settings
void update_user_interface (PumpSettings settings) {
  // led update
  if (settings.status == STATUS_ON) {
    digitalWrite(LED_pin, HIGH);
  } else {
    // FIXME: ideally blink if it is holding
    digitalWrite(LED_pin, LOW);
  }
  // lcd update
  // FIXME this should be using the PumpController function but I can't access the method from the constant object
  float rpm = settings.speed * settings.step_angle/settings.microstep * 60/360;
  char rpm_buffer[10];
  sprintf(rpm_buffer, "%2.2f", rpm);
  #ifdef ENABLE_DISPLAY
    lcd.print_line(2, String("Speed [rpm]: ") + String(rpm_buffer));
  #endif
  Serial.println("Speed [rpm]: " + String(rpm_buffer));
  Serial.println("Speed [step/s]: " + String(settings.speed));
  Serial.println("Microsteps: " + String(settings.microstep));

  char status[5] = "";
  switch (settings.status) {
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
  }

  char direction[3] = "";
  switch(settings.direction) {
    case DIR_CW:
      strcpy(direction, "cw");
      break;
    case DIR_CC:
      strcpy(direction, "cc");
      break;
  }
  #ifdef ENABLE_DISPLAY
    lcd.print_line(3, String("Status: ") + String( status ));
    lcd.print_line(4, String("Direction: ") + String( direction ));
  #endif
  Serial.println("Status: " + String(status));
  Serial.println("Direction: " + String(direction));
}

// callback function for pump commands
void report_pump_command (const PumpController& pump) {
  // report command on user interfae
  Serial.println("INFO: pump command - " +
    String(pump.command.type) + " " +
    String(pump.command.variable));
  #ifdef ENABLE_DISPLAY
    lcd.show_message(pump.command.type, pump.command.variable); // shown on lcd
  #endif
  update_user_interface(pump.settings);
  #ifdef ENABLE_LOGGING
    // log in google spreadsheet
    gs.send(pump.command.type, pump.command.variable, pump.command.value, pump.command.units, pump.command.msg);
  #endif
}

PumpController pump(pins, settings, report_pump_command);

// device name
char device_name[20];
void name_handler(const char *topic, const char *data) {
  strncpy ( device_name, data, sizeof(device_name) );
  #ifdef ENABLE_DISPLAY
    Serial.println("INFO: device ID " + String(device_name));
    lcd.print_line(1, "ID: " + String(device_name));
  #endif
}


void setup() {

  // time
  Time.zone(-5);

  // serial
  Serial.begin(9600);

  // device name
  Particle.subscribe("spark/", name_handler);
  Particle.publish("spark/device/name");

  // LED init
  pinMode(LED_pin, OUTPUT);
  digitalWrite(LED_pin, LOW);

  // inits
  Serial.println("INFO: initialize stepper");
  pump.init();
  pump.setSpeedRpm(1); // start with 1 rpm

  #ifdef ENABLE_DISPLAY
    Serial.println("INFO: initialize LCD");
    lcd.init();
  #endif
  #ifdef ENABLE_LOGGING
    Serial.println("INFO: initialize gs logger")
    gs.init();
  #endif

  Serial.println("INFO: update user interface");
  update_user_interface(pump.settings);
}


void loop() {
  #ifdef ENABLE_DISPLAY
    lcd.update();
  #endif
  pump.update();
}
