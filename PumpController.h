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

// mode constants
#define DIR_CW    1
#define DIR_CC   -1

// microstep modes
struct MicrostepMode {
  const int mode; // the mode for the ms mode
  const bool ms1; // HIGH or LOW for ms1
  const bool ms2; // HIGH or LOW for ms2
  const bool ms3; // HIGH or LOW for ms3
};

// microstep modes for DRV8825
// NOTE: these may vary for different stepper chips
#define MS_MODE_AUTO 0 // the key for automatic determination of microstep modes
#define MS_MODES_N   6 // the number of defined stepping modes
const MicrostepMode ms_modes[MS_MODES_N] =
  {
    {1,  LOW,  LOW,  LOW},  // full step
    {2,  HIGH, LOW,  LOW},  // half step
    {4,  LOW,  HIGH, LOW},  // quarter step
    {8,  HIGH, HIGH, LOW},  // eight step
    {16, LOW,  LOW,  HIGH}, // sixteenth step
    {32, HIGH, LOW,  HIGH},  // 1/32 step
  };

// return codes for webhook
#define CMD_RET_SUCCESS   0
#define CMD_RET_ERROR    -1

// settings
// TODO: expand the additional settings for the pump that need to be
// stored in order to recreate the previous state on power off/on
// NOTE: not all of these need to / should be accessible via constructor
#define STATUS_ON       1
#define STATUS_OFF      2
#define STATUS_HOLD     3
#define STATUS_ROTATE   4
struct PumpSettings {
  double step_angle; // angle/step
  double step_flow; // mass/step
  int direction; // -1 or 1
  int microstep; // MS_AUTO or
  float max_speed; // steps/s
  float speed; // steps/s
  int status; // STATUS_ON, OFF, HOLD
  PumpSettings() {};
  PumpSettings(double step_angle, double step_flow, int direction, int microstep, float max_speed, float speed) :
    step_angle(step_angle), step_flow(step_flow), direction(direction), microstep(microstep), max_speed(max_speed), speed(speed), status(STATUS_OFF) {};
};

// command
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

// class
class PumpController {

  private:

    const StepperPins pins; // pins
    AccelStepper stepper; // stepper

    // internal functions
    void extractCommandParam(char* param);
    void assignCommandMessage();
    void updateStatus();
    void updateStatus(int status);

  public:

    float getRotationSteps();
    float rpmToSpeed(float rpm);
    float speedToRpm(float speed); // convert any speed
    float getRpm(); // convert set speed

    // settings
    PumpSettings settings;

    // command
    PumpCommand command;

    // callback
    typedef void (*PumpCommandCallbackType) (const PumpController&);

    // constructors
    PumpController (StepperPins pins, PumpSettings settings,
      PumpCommandCallbackType callback) :
    pins(pins), settings(settings), command_callback(callback) {
      Particle.function(CMD_ROOT, &PumpController::parseCommand, this);
    }

    void init();
    bool setMicrostepping(int ms);

    int parseCommand (String command);

    void setDirection(int direction);
    float setSpeedRpm(float rpm); // returns speed in step/s

    void start();
    void stop();
    void hold();
    long rotate(double number); // returns the number of steps the motor will take

    void update(); // run in loop

    private: PumpCommandCallbackType command_callback;
};
