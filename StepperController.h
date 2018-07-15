#pragma once
#include "StepperState.h"
#include "StepperConfig.h"
#include "StepperCommands.h"
#include "device/DeviceController.h"
#include <AccelStepper.h>

// stepper controller class
class StepperController : public DeviceController {

  private:

    // internal functions
    void updateStepper(); // update stepper object
    float calculateSpeed(); // calculate speed based on settings
    int findMicrostepIndexForRpm(float rpm); // finds the correct ms index for the requested rpm (takes ms_auto into consideration)
    bool setSpeedWithSteppingLimit(float rpm); // sets state->speed and returns true if request set, false if had to set to limit

    // configuration
    StepperBoard* board;
    StepperDriver* driver;
    StepperMotor* motor;
    AccelStepper stepper;

    // state
    StepperState* state;
    DeviceState* ds = state;

  public:

    // constructors
    StepperController() {};

    StepperController (int reset_pin, DeviceDisplay* lcd, StepperBoard* board, StepperDriver* driver, StepperMotor* motor, StepperState* state) :
      DeviceController(reset_pin, lcd), board(board), driver(driver), motor(motor), state(state) {
        driver->calculateRpmLimits(board->max_speed, motor->steps, motor->gearing);
    };

    // methods
    void init(); // to be run during setup()
    void update(); // to be run during loop()

    float getMaxRpm(); // returns the maximum rpm for the pump

    bool changeStatus(int status);
    bool changeDirection(int direction);
    bool changeSpeedRpm(float rpm); // return false if had to limit speed, true if taking speed directly
    bool changeToAutoMicrosteppingMode(); // set to automatic microstepping mode
    bool changeMicrosteppingMode(int ms_mode); // set microstepping by mode, return false if can't find requested mode

    bool start(); // start the pump
    bool stop(); // stop the pump
    bool hold(); // hold position
    long rotate(float number); // returns the number of steps the motor will take

    void saveDS(); // save device state to EEPROM
    bool restoreDS(); // load device state from EEPROM

    void assembleStateInformation();

    void parseCommand();
    bool parseStatus();
    bool parseDirection();
    bool parseSpeed();
    bool parseMS();

};

/**** SETUP AND LOOP ****/

void StepperController::init() {
  stepper = AccelStepper(AccelStepper::DRIVER, board->step, board->dir);
  stepper.setEnablePin(board->enable);
  stepper.setPinsInverted	(
            driver->step_on != HIGH,
            driver->dir_cw != LOW,
            driver->enable_on != LOW
        );
  stepper.disableOutputs();
  stepper.setMaxSpeed(board->max_speed);

  // microstepping
  pinMode(board->ms1, OUTPUT);
  pinMode(board->ms2, OUTPUT);
  pinMode(board->ms3, OUTPUT);
  #ifdef STEPPER_DEBUG_ON
    Serial.println("INFO: available microstepping modes");
    for (int i = 0; i < driver->ms_modes_n; i++) {
      Serial.printf("   Mode %d: %s steps, max rpm: %.1f\n", i, driver->ms_modes[i].mode, driver->ms_modes[i].rpm_limit);
    }
  #endif

  DeviceController::init();
  updateStepper();
}

// loop function
void StepperController::update() {
  if (state->status == STATUS_ROTATE) {
    // WARNING: FIXME known bug, when power out, saved rotate status will lead to immediate stop of pump
    if (stepper.distanceToGo() == 0) {
      changeStatus(STATUS_OFF); // disengage if reached target location
    } else {
      stepper.runSpeedToPosition();
    }
  } else {
    stepper.runSpeed();
  }
  DeviceController::update();
}

/**** STATE PERSISTENCE ****/

// save device state to EEPROM
void StepperController::saveDS() {
  EEPROM.put(STATE_ADDRESS, *state);
  #ifdef STATE_DEBUG_ON
    Serial.println("INFO: pump state saved in memory (if any updates were necessary)");
  #endif
}

// load device state from EEPROM
bool StepperController::restoreDS(){
  StepperState saved_state;
  EEPROM.get(STATE_ADDRESS, saved_state);
  bool recoverable = saved_state.version == STATE_VERSION;
  if(recoverable) {
    EEPROM.get(STATE_ADDRESS, *state);
    Serial.printf("INFO: successfully restored state from memory (version %d)\n", STATE_VERSION);
  } else {
    Serial.printf("INFO: could not restore state from memory (found version %d), sticking with initial default\n", saved_state.version);
    saveDS();
  }
  return(recoverable);
}

/**** UPDATING STEPPER ****/

void StepperController::updateStepper() {
  // update microstepping
  if (state->ms_index >= 0 && state->ms_index < driver->ms_modes_n) {
    digitalWrite(board->ms1, driver->ms_modes[state->ms_index].ms1);
    digitalWrite(board->ms2, driver->ms_modes[state->ms_index].ms2);
    digitalWrite(board->ms3, driver->ms_modes[state->ms_index].ms3);
  }

  // update speed
  stepper.setSpeed(calculateSpeed());

  // update enabled / disabled
  if (state->status == STATUS_ON || state->status == STATUS_ROTATE) {
    stepper.enableOutputs();
  } else if (state->status == STATUS_HOLD) {
    stepper.setSpeed(0);
    stepper.enableOutputs();
  } else {
    // STATUS_OFF
    stepper.setSpeed(0);
    stepper.disableOutputs();
  }
}

float StepperController::calculateSpeed() {
  float speed = state->rpm/60.0 * motor->steps * motor->gearing * state->ms_mode * state->direction;
  #ifdef STEPPER_DEBUG_ON
    Serial.printf("INFO: calculated speed %.5f\n", speed);
  #endif
  return(speed);
}

/* DEVICE STATE CHANGE FUNCTIONS */

bool StepperController::changeStatus(int status) {

  // only update if necessary
  bool changed = status != state->status;

  #ifdef STEPPER_DEBUG_ON
    if (changed)
      Serial.printf("INFO: status updating to %d\n", status);
    else
      Serial.printf("INFO: status unchanged (%d)\n", status);
  #endif

  if (changed) {
    state->status = status;
    updateStepper();
    saveDS();
  }
  return(changed);
}

bool StepperController::changeDirection(int direction) {

  // only update if necessary
  bool changed = (direction == DIR_CW || direction == DIR_CC) && state->direction != direction;

  #ifdef STEPPER_DEBUG_ON
    if (changed)
      (direction == DIR_CW) ? Serial.println("INFO: changing direction to clockwise") : Serial.println("INFO: changing direction to counter clockwise");
    else
      (direction == DIR_CW) ? Serial.println("INFO: direction unchanged (clockwise)") : Serial.println("INFO: direction unchanged (counter clockwise)");
  #endif

  if (changed) {
    state->direction = direction;
    if (state->status == STATUS_ROTATE) {
      // if rotating to a specific position, changing direction turns the pump off
      #ifdef STEPPER_DEBUG_ON
        Serial.println("INFO: stepper stopped due to change in direction during 'rotate'");
      #endif
      state->status = STATUS_OFF;
    }
    updateStepper();
    saveDS();
  }

  return(changed);
}

bool StepperController::changeSpeedRpm(float rpm) {
  int original_ms_mode = state->ms_mode;
  float original_rpm = state->rpm;
  state->ms_index = findMicrostepIndexForRpm(rpm);
  state->ms_mode = driver->getMode(state->ms_index); // tracked for convenience
  setSpeedWithSteppingLimit(rpm);
  bool changed = state->ms_mode != original_ms_mode | !approximatelyEqual(state->rpm, rpm, 0.0001);

  #ifdef STEPPER_DEBUG_ON
    if (changed)
      Serial.printf("INFO: changing rpm %.3f\n", state->rpm);
    else
      Serial.printf("INFO: rpm staying unchanged ()%.3f)\n", state->rpm);
  #endif

  if (changed) {
    updateStepper();
    saveDS();
  }
  return(changed);
}

bool StepperController::setSpeedWithSteppingLimit(float rpm) {
  if (driver->testRpmLimit(state->ms_index, rpm)) {
    state->rpm = driver->getRpmLimit(state->ms_index);
    Serial.printf("WARNING: stepping mode is not fast enough for the requested rpm: %.3f --> switching to MS mode rpm limit of %.3f\n", rpm, state->rpm);
    return(false);
  } else {
    state->rpm = rpm;
    return(true);
  }
}

bool StepperController::changeToAutoMicrosteppingMode() {

  bool changed = !state->ms_auto;

  #ifdef STEPPER_DEBUG_ON
    if (changed)
      Serial.println("INFO: activating automatic microstepping");
    else
      Serial.println("INFO: automatic microstepping already active");
  #endif

  if (changed) {
    state->ms_auto = true;
    state->ms_index = findMicrostepIndexForRpm(state->rpm);
    state->ms_mode = driver->getMode(state->ms_index); // tracked for convenience
    updateStepper();
    saveDS();
  }
  return(changed);
}

bool StepperController::changeMicrosteppingMode(int ms_mode) {

  // find index for requested microstepping mode
  int ms_index = driver->findMicrostepIndexForMode(ms_mode);

  // no index found for requested mode
  if (ms_index == -1) {
    Serial.printf("WARNING: could not find microstep index for mode %d\n", ms_mode);
    return(false);
  }

  bool changed = state->ms_auto | state->ms_index != ms_index;
  #ifdef STEPPER_DEBUG_ON
    if (changed)
      Serial.printf("INFO: activating microstepping index %d for mode %d\n", ms_index, ms_mode);
    else
      Serial.printf("INFO: microstepping mode already active (%d)\n", state->ms_mode);
  #endif

  if (changed) {
    // update with new microstepping mode
    state->ms_auto = false; // deactivate auto microstepping
    state->ms_index = ms_index; // set the found index
    state->ms_mode = driver->getMode(ms_index); // tracked for convenience
    setSpeedWithSteppingLimit(state->rpm); // update speed (if necessary)
    updateStepper();
    saveDS();
  }
  return(changed);
}

int StepperController::findMicrostepIndexForRpm(float rpm) {
  if (state->ms_auto) {
    // automatic mode --> find lowest MS mode that can handle these rpm (otherwise go to full step -> ms_index = 0)
    return(driver->findMicrostepIndexForRpm(rpm));
  } else {
    return(state->ms_index);
  }
}

// start, stop, hold
bool StepperController::start() { changeStatus(STATUS_ON); }
bool StepperController::stop() { changeStatus(STATUS_OFF); }
bool StepperController::hold() { changeStatus(STATUS_HOLD); }

// number of rotations
long StepperController::rotate(float number) {
  long steps = state->direction * number * motor->steps * motor->gearing * state->ms_mode;
  stepper.setCurrentPosition(0);
  stepper.moveTo(steps);
  changeStatus(STATUS_ROTATE);
  return(steps);
}

/****** STATE INFORMATION *******/

void StepperController::assembleStateInformation() {
  DeviceController::assembleStateInformation();
  char pair[60];
  getStepperStateStatusInfo(state->status, pair, sizeof(pair)); addToStateInformation(pair);
  getStepperStateDirectionInfo(state->direction, pair, sizeof(pair)); addToStateInformation(pair);
  getStepperStateSpeedInfo(state->rpm, pair, sizeof(pair)); addToStateInformation(pair);
  getStepperStateMSInfo(state->ms_auto, state->ms_mode, pair, sizeof(pair)); addToStateInformation(pair);
}

/****** WEB COMMAND PROCESSING *******/

bool StepperController::parseStatus() {
  if (command.parseVariable(CMD_START)) {
    // start
    command.success(start());
  } else if (command.parseVariable(CMD_STOP)) {
    // stop
    command.success(stop());
  } else if (command.parseVariable(CMD_HOLD)) {
    // hold
    command.success(hold());
  } else if (command.parseVariable(CMD_RUN)) {
    // run - not yet implemented FIXME
  } else if (command.parseVariable(CMD_AUTO)) {
    // auto - not yet implemented FIXME
  } else if (command.parseVariable(CMD_ROTATE)) {
    // rotate
    command.extractValue();
    char* end;
    float number = strtof (command.value, &end);
    int converted = end - command.value;
    if (converted > 0) {
      // valid number
      rotate(number);
      // rotate always counts as new command b/c rotation starts from scratch
      command.success(true);
    } else {
      // no number, invalid value
      command.errorValue();
    }
  }

  // set command data if type defined
  if (command.isTypeDefined()) {
    getStepperStateStatusInfo(state->status, command.data, sizeof(command.data));
  }

  return(command.isTypeDefined());
}

bool StepperController::parseDirection() {

  if (command.parseVariable(CMD_DIR)) {
    // direction
    command.extractValue();
    if (command.parseValue(CMD_DIR_CW)) {
      // clockwise
      command.success(changeDirection(DIR_CW));
    } else if (command.parseValue(CMD_DIR_CC)) {
      // counter clockwise
      command.success(changeDirection(DIR_CC));
    } else if (command.parseValue(CMD_DIR_SWITCH)) {
      // switch
      command.success(changeDirection(-state->direction));
    } else {
      // invalid
      command.errorValue();
    }
  }

  // set command data if type defined
  if (command.isTypeDefined()) {
    getStepperStateDirectionInfo(state->direction, command.data, sizeof(command.data));
  }

  return(command.isTypeDefined());
}

bool StepperController::parseSpeed() {

  if (command.parseVariable(CMD_SPEED)) {
    // speed
    command.extractValue();
    command.extractUnits();

    if (command.parseUnits(SPEED_RPM)) {
      // speed rpm
      char* end;
      float number = strtof (command.value, &end);
      int converted = end - command.value;
      if (converted > 0) {
        // valid number
        command.success(changeSpeedRpm(number));
        if(definitelyLessThan(state->rpm, number, 0.0001)) {
          // could not set to rpm, hit the max --> set warning
          command.warning(CMD_RET_WARN_MAX_RPM, CMD_RET_WARN_MAX_RPM_TEXT);
        }
      } else {
        // no number, invalid value
        command.errorValue();
      }
    //} else if (command.parseUnits(SPEED_FPM)) {
      // speed fpm
      // TODO
    } else {
      command.errorUnits();
    }
  }

  // set command data if type defined
  if (command.isTypeDefined()) {
    getStepperStateSpeedInfo(state->rpm, command.data, sizeof(command.data));
  }

  return(command.isTypeDefined());
}

bool StepperController::parseMS() {

  if (command.parseVariable(CMD_STEP)) {
    // microstepping
    command.extractValue();
    if (command.parseValue(CMD_STEP_AUTO)) {
      command.success(changeToAutoMicrosteppingMode());
    } else {
      int ms_mode = atoi(command.value);
      command.success(changeMicrosteppingMode(ms_mode));
    }
  }

  // set command data if type defined
  if (command.isTypeDefined()) {
    getStepperStateMSInfo(state->ms_auto, state->ms_mode, command.data, sizeof(command.data));
  }

  return(command.isTypeDefined());
}

void StepperController::parseCommand() {

  DeviceController::parseCommand();

  if (command.isTypeDefined()) {
    // command processed successfully by parent function
  } else if (parseStatus()) {
    // check for status commands
  } else if (parseDirection()) {
    // check for direction commands
  } else if (parseSpeed()) {
    // check for speed commands
  //} else if (parseRamp()) {
    // check for ramp commands - TODO
  } else if (parseMS()) {
    // check for microstepping commands
  }

}
