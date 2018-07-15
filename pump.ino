#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks
#include "application.h"

// debugging options
#define CLOUD_DEBUG_ON
//#define WEBHOOKS_DEBUG_ON
#define STATE_DEBUG_ON
//#define DATA_DEBUG_ON
//#define SERIAL_DEBUG_ON
//#define LCD_DEBUG_ON
#define STEPPER_DEBUG_ON

// keep track of installed version
#define STATE_VERSION    1 // update whenver structure changes
#define DEVICE_VERSION  "pump 0.2.0" // update with every code update

// M800 controller
#include "StepperController.h"

// lcd
DeviceDisplay* lcd = &LCD_20x4;

// board
StepperBoard* board = &PHOTON_STEPPER_BOARD;

// driver
StepperDriver* driver = &DRV8825;

// motor
StepperMotor* motor = &WM114ST;

// initial state
StepperState* state = new StepperState(
  /* locked */                    false,
  /* state_logging */             true,
  /* data_logging */              false,
  /* data_logging_period */       600, // in seconds
  /* data_logging_type */         LOG_BY_TIME, // log by time
  /* direction */                 DIR_CW, // start clockwise
  /* status */                    STATUS_OFF, // start off
  /* rpm */                       1 // start speed [rpm]
  // no specification of microstepping mode = automatic mode
);

// controller
StepperController* pump = new StepperController(
  /* reset pin */         A5,
  /* lcd screen */        lcd,
  /* pointer to board */  board,
  /* pointer to driver */ driver,
  /* pointer to motor */  motor,
  /* pointer to state */  state
);

/*

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
  if (state->status == STATUS_ON) {
    digitalWrite(LED_pin, HIGH);
  } else {
    // FIXME: ideally blink if it is holding
    digitalWrite(LED_pin, LOW);
  }

  // user interface update text
  getPumpStateStatusInfo(state->status, status_lcd, sizeof(status_lcd), status_web, sizeof(status_web));
  getPumpStateSpeedInfo(state->rpm, rpm_lcd, sizeof(rpm_lcd), rpm_web, sizeof(rpm_web));
  getStepperStateMSInfo(state->ms_auto, state->ms_mode, ms_mode_lcd, sizeof(ms_mode_lcd), ms_mode_web, sizeof(ms_mode_web));
  getPumpStateDirectionInfo(state->direction, dir_lcd, sizeof(dir_lcd), dir_web, sizeof(dir_web) );
  get_pump_stat_locked_info(state->locked, locked_lcd, sizeof(locked_lcd), locked_web, sizeof(locked_web));

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
void report_pump_command (const StepperController& pump) {
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
*/

// using system threading to improve timely stepper stepping
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {

  // serial
  Serial.begin(9600);

  // lcd temporary messages
  lcd->setTempTextShowTime(3); // how many seconds temp time

  // controller
  pump->init();

  // connect device to cloud
  Serial.println("INFO: connecting to cloud");
  Particle.connect();

}

void loop() {
  pump->update();
}


/*
// maybe test this more
// might need a system where every line of LCD screen gets stored in a String
// and then only updated in the separate threat whenever necessary
led1Thread = new Thread("sample", test_lcd);
Thread* led1Thread;

os_thread_return_t test_lcd(){
    for(;;) {
        lcd.update();
    }
}*/
