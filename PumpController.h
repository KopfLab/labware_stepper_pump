/*
 * PumpController.h
 * Created on: February 17, 2017
 * Author: Sebastian Kopf <seb.kopf@gmail.com>
 * Pump controller
 TODO implement set acceleration
separate the mass/s aspects from the rest? probably not at the pump class level
 */

#pragma once
#include <AccelStepper.h>
#include "PumpCommands.h"

// pin constants and wiring structure
#define PIN_DIR_CW LOW // is clockwise on LOW or HIGH?
#define PIN_STEP_ON HIGH // is a step made on LOW or HIGH?
#define PIN_ENABLE_ON HIGH // is enable on LOW or HIGH?
struct StepperPins {
  const int dir; // direction
  const int step; // step
  const int enable; // enable
  const int ms1; // microstep 1
  const int ms2; // microstep 2
  const int ms3; // microstep 3
  StepperPins(int dir, int step, int enable, int ms1, int ms2, int ms3) :
    dir(dir), step(step), enable(enable), ms1(ms1), ms2(ms2), ms3(ms3) {};
};

// microstep mode structur (driver chip specific)
#define MS_MODE_AUTO  -1 // code for automatic determination of microstep modes
struct MicrostepMode {
  int mode; // the mode for the ms mode
  bool ms1; // HIGH or LOW for ms1
  bool ms2; // HIGH or LOW for ms2
  bool ms3; // HIGH or LOW for ms3
  float rpm_limit; // the RPM limit for this microstepping mode (filled by PumpController later)
  MicrostepMode() {};
  MicrostepMode(int mode, bool ms1, bool ms2, bool ms3) :
    mode(mode), ms1(ms1), ms2(ms2), ms3(ms3), rpm_limit(-1) {}
};

// pump settings (microcontroller and pump stepper specific)
struct PumpSettings {
public:
  int steps; // numer of steps / rotation
  double gearing; // the gearing (if any, otherwise should be 1)
  float max_speed; // maximum # of steps/s the processor can reliably support (i.e. how often can the PumpController::update() be called?)

  PumpSettings(int steps, double gearing, float max_speed) :
    steps(steps), gearing(gearing), max_speed(max_speed) {};

};

// pump state
#define DIR_CW           1
#define DIR_CC          -1
#define STATUS_ON        1
#define STATUS_OFF       2
#define STATUS_HOLD      3
#define STATUS_ROTATE    4
#define STATUS_TRIGGER   5 // TODO: implement signal triggering mode
#define STEP_FLOW_UNDEF -1
#define STATE_VERSION    2 // change whenver PumpState structure changes
#define STATE_ADDRESS    0 // EEPROM storage location
struct PumpState {
  const int version = STATE_VERSION;
  int direction; // DIR_CW or DIR_CC
  int ms_index; // MS_MODE_AUTO or index of the selected microstep mode
  int ms_mode; // stores the actual ms_mode that is active (just for convenience)
  int status; // STATUS_ON, OFF, HOLD
  float rpm; // speed in rotations / minute (actual speed in steps/s depends on microstepping mode)
  double step_flow; // mass/step calibration
  bool locked; // whether settings are locked

  PumpState() {};
  PumpState(int direction, int ms_index, int status, float rpm, double step_flow, bool locked) :
    direction(direction), ms_index(ms_index), status(status), rpm(rpm), step_flow(step_flow), locked(locked) {};
};

// command from spark cloud
struct PumpCommand {
  char buffer[63]; //(spark.functions are limited to 63 char long call)
  char type[20];
  char variable[25];
  char value[20];
  char units[20];
  char msg[63];
  PumpCommand() {};
  PumpCommand(String& command_string) {
    command_string.toCharArray(buffer, sizeof(buffer));
  }
};

// actual pump controller class
class PumpController {

  private:

    AccelStepper stepper; // stepper
    const StepperPins pins; // pins
    const int ms_modes_n; // number of microstepping modes
    MicrostepMode* ms_modes; // microstepping modes
    const PumpSettings settings; // settings

    // internal functions
    void extractCommandParam(char* param);
    void assignCommandMessage();
    void updateStatus();
    void updateStatus(int status);
    float calculateSpeed();
    int findAutoMicrostepIndex(float rpm);
    int activateMicrostepping(int ms_index, float rpm);
    bool setSpeedWithSteppingLimit(int ms_index, float rpm); // sets state.speed and returns true if request set, false if had to set to limit

    void savePumpState(); // returns TRUE if pump state was updated, FALSE if it didn't need updating
    bool loadPumpState(); // returns TRUE if pump state loaded successfully, FALSE if it didn't match the required structure and default was loaded instead

  public:

    // state
    PumpState state;

    // command
    PumpCommand command;

    // callback
    typedef void (*PumpCommandCallbackType) (const PumpController&);

    // constructors
    PumpController (StepperPins pins, MicrostepMode* ms_modes, int ms_modes_n,
      PumpSettings settings, PumpState state, PumpCommandCallbackType callback) :
    pins(pins), ms_modes_n(ms_modes_n), settings(settings), state(state), command_callback(callback) {
      Particle.function(CMD_ROOT, &PumpController::parseCommand, this);
      // allocate microstep modes and calculate rpm limits for each
      this->ms_modes = new MicrostepMode[ms_modes_n];
      for (int i = 0; i < ms_modes_n; i++) {
        this->ms_modes[i] = ms_modes[i];
        this->ms_modes[i].rpm_limit = settings.max_speed * 60.0 / (settings.steps * settings.gearing * ms_modes[i].mode);
      }
    }

    // destructor (deallocate ms_modes)
    virtual ~PumpController(){
       delete[] ms_modes;
    }

    // methods
    void init(); // to be run during setup()
    void init(bool reset); // whether to reset pump state back to the default
    void update(); // to be run during loop()

    bool setMicrosteppingMode(int ms_mode); // set microstepping by mode, return false if can't find requested mode
    bool setSpeedRpm(float rpm); // return false if had to limit speed, true if taking speed directly
    void setDirection(int direction); // set direction

    void lock(); // lock pump
    void unlock(); // unlock pump

    void start(); // start the pump
    void stop(); // stop the pump
    void hold(); // hold position
    long rotate(double number); // returns the number of steps the motor will take

    int parseCommand (String command); // parse a cloud command

  // pump command callback
  private: PumpCommandCallbackType command_callback;

};
