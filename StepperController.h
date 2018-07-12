#pragma once
#include "StepperState->h"
#include "StepperSettings.h"
#include "PumpCommands.h"
#include "device/DeviceController.h"
#include <AccelStepper.h>


// FIXME: REMOVE manual!
#define MANUAL_DEBOUNCE 50              // the debounce time for analog input in ms
#define MANUAL_STATUS_UPDATE_DELAY 2000 // how long to wait for status update after manual adjustments [ms]


// actual pump controller class
// FIXME:
class StepperController : public DeviceController {

  private:

    AccelStepper stepper; // stepper
    const StepperPins pins; // pins
    const int ms_modes_n; // number of microstepping modes
    MicrostepMode* ms_modes; // microstepping modes
    const StepperSettings settings; // settings
    bool delay_status_update = false;
    long update_status_time = 0; // set specific time for delayed status updates
    long last_manual_read = 0; // last manual speed read time
    float last_manual_rpm = -1; // last manual speed read

    // internal functions
    void extractCommandParam(char* param);
    void assignCommandMessage();
    void updateStepper();
    bool changeStatus(int status);
    float calculateSpeed();
    int findMicrostepIndexForRpm(float rpm); // finds the correct ms index for the requested rpm (takes ms_auto into consideration)
    bool setSpeedWithSteppingLimit(float rpm); // sets state->speed and returns true if request set, false if had to set to limit

    // state
    StepperState* state;
    DeviceState* ds = state;

  public:

    // command
    PumpCommand command;

    // callback
    typedef void (*PumpCommandCallbackType) (const StepperController&);

    // constructors
    StepperController (StepperPins pins, MicrostepMode* ms_modes, int ms_modes_n,
      StepperSettings settings, StepperState state, PumpCommandCallbackType callback) :
    pins(pins), ms_modes_n(ms_modes_n), settings(settings), state(state), command_callback(callback) {
      Particle.function(CMD_ROOT, &StepperController::parseCommand, this);
      // allocate microstep modes and calculate rpm limits for each
      this->ms_modes = new MicrostepMode[ms_modes_n];
      for (int i = 0; i < ms_modes_n; i++) {
        this->ms_modes[i] = ms_modes[i];
        this->ms_modes[i].rpm_limit = settings.max_speed * 60.0 / (settings.steps * settings.gearing * ms_modes[i].mode);
      }
    }

    // destructor (deallocate ms_modes)
    virtual ~StepperController(){
       delete[] ms_modes;
    }

    // methods
    void init(); // to be run during setup()
    void init(bool reset); // whether to reset pump state back to the default
    void update(); // to be run during loop()

    void setStatusUpdateDelay(long delay); // set a status update delay [in ms], call before making any status updates
    void setAutoMicrosteppingMode(); // set to automatic icrostepping mode
    bool setMicrosteppingMode(int ms_mode); // set microstepping by mode, return false if can't find requested mode
    bool setSpeedRpm(float rpm); // return false if had to limit speed, true if taking speed directly
    float getMaxRpm(); // returns the maximum rpm for the pump
    void setDirection(int direction); // set direction

    void lock(); // lock pump
    void unlock(); // unlock pump

    void manual(); // turn into manual mode
    void start(); // start the pump
    void stop(); // stop the pump
    void hold(); // hold position
    long rotate(double number); // returns the number of steps the motor will take

    int parseCommand (String command); // parse a cloud command

    void saveDS(); // save device state to EEPROM
    bool restoreDS(); // load device state from EEPROM

  // pump command callback
  private: PumpCommandCallbackType command_callback;

};

/**** SETUP AND LOOP ****/

// FIXME: done
void StepperController::init() {
  stepper = AccelStepper(AccelStepper::DRIVER, pins.step, pins.dir);
  stepper.setEnablePin(pins.enable);
  stepper.setPinsInverted	(
            PIN_STEP_ON != HIGH,
            PIN_DIR_CW != LOW,
            PIN_ENABLE_ON != LOW
        );
  stepper.disableOutputs();
  stepper.setMaxSpeed(settings.max_speed);

  // microstepping
  pinMode(pins.ms1, OUTPUT);
  pinMode(pins.ms2, OUTPUT);
  pinMode(pins.ms3, OUTPUT);
  #ifdef PUMP_DEBUG_ON
    Serial.println("INFO: available microstepping modes");
    for (int i = 0; i < ms_modes_n; i++) {
      Serial.printf("   Mode %d: %s steps, max rpm: %.1f\n", i, ms_modes[i].mode, ms_modes[i].rpm_limit);
    }
  #endif

  SerialDeviceController::init();
  updateStepper();
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
  bool recoverable = saved_state->version == STATE_VERSION;
  if(recoverable) {
    EEPROM.get(STATE_ADDRESS, *state);
    Serial.printf("INFO: successfully restored state from memory (version %d)\n", STATE_VERSION);
  } else {
    Serial.printf("INFO: could not restore state from memory (found version %d), sticking with initial default\n", saved_state->version);
    saveDS();
  }
  return(recoverable);
}

/**** UPDATING PUMP STATUS ****/

bool DeviceController::changeStateLogging (bool on) {
  bool changed = on != getDS()->state_logging;

  if (changed) {
    getDS()->state_logging = on;
    override_state_log = true; // always log this event no matter what
  }

  #ifdef STATE_DEBUG_ON
    if (changed)
      on ? Serial.println("INFO: state logging turned on") : Serial.println("INFO: state logging turned off");
    else
      on ? Serial.println("INFO: state logging already on") : Serial.println("INFO: state logging already off");
  #endif

  if (changed) saveDS();

  return(changed);
}

void StepperController::updateStepper() {

    // update microstepping
    if (state->ms_index >= 0 && state->ms_index < ms_modes_n) {
      digitalWrite(pins.ms1, ms_modes[state->ms_index].ms1);
      digitalWrite(pins.ms2, ms_modes[state->ms_index].ms2);
      digitalWrite(pins.ms3, ms_modes[state->ms_index].ms3);
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
}



float StepperController::calculateSpeed() {
  float speed = state->rpm/60.0 * settings.steps * settings.gearing * state->ms_mode * state->direction;
  Serial.println("INFO: calculated speed " + String(speed));
  return(speed);
}


/* DEVICE STATE CHANGE FUNCTIONS */


// FIXME: done
bool StepperController::changeStatus(int status) {

  // update status value
  bool changed = status != state->status;
  if (changed) state->status = status;

  #ifdef PUMP_DEBUG_ON
    if (changed)
      Serial.printf("INFO: status updated to %d\n", status);
    else
      Serial.printf("INFO: status unchanged (%d)\n", status);
  #endif

  updateStepper();

  if (changed) saveDS();
  return(changed);
}

// FIXME:
void StepperController::setAutoMicrosteppingMode() {
  Serial.println("INFO: activating automatic microstepping");
  state->ms_auto = true;
  state->ms_index = findMicrostepIndexForRpm(state->rpm);
  state->ms_mode = ms_modes[state->ms_index].mode; // tracked for convenience
  updateStepper();
}

// FIXME:
bool StepperController::setMicrosteppingMode(int ms_mode) {

  // find index for requested microstepping mode
  int ms_index = -1;
  for (int i = 0; i < ms_modes_n; i++) {
    if (ms_modes[i].mode == ms_mode) {
      ms_index = i;
      break;
    }
  }

  // no index found for requested mode
  if (ms_index == -1) return(false);

  Serial.println("INFO: activating microstepping index " +  String(ms_index) + " for mode " + String(ms_modes[ms_index].mode));
  // update with new microstepping mode
  state->ms_auto = false; // deactivate auto microstepping
  state->ms_index = ms_index; // set the found index
  state->ms_mode = ms_modes[state->ms_index].mode; // tracked for convenience
  setSpeedWithSteppingLimit(state->rpm); // update speed (if necessary)
  updateStepper();
  return(true);
}

// FIXME:
int StepperController::findMicrostepIndexForRpm(float rpm) {
  if (state->ms_auto) {
    // automatic mode --> find lowest MS mode that can handle these rpm (otherwise go to full step -> ms_index = 0)
    int ms_index = 0;
    for (int i = 1; i < ms_modes_n; i++) {
      if (rpm <= ms_modes[i].rpm_limit) ms_index = i;
    }
    return(ms_index);
  } else {
    return(state->ms_index);
  }
}

// FIXME:
bool StepperController::setSpeedRpm(float rpm) {
  state->ms_index = findMicrostepIndexForRpm(rpm);
  state->ms_mode = ms_modes[state->ms_index].mode; // tracked for convenience
  bool limited = setSpeedWithSteppingLimit(rpm);
  updateStepper();
  return(limited);
}

// FIXME:
bool StepperController::setSpeedWithSteppingLimit(float rpm) {
  if (rpm > ms_modes[state->ms_index].rpm_limit) {
    Serial.println("WARNING: stepping mode is not fast enough for the requested rpm: " + String(rpm));
    Serial.println("  --> switching to MS mode rpm limit of " + String(ms_modes[state->ms_index].rpm_limit));
    state->rpm = ms_modes[state->ms_index].rpm_limit;
    return(false);
  } else {
    state->rpm = rpm;
    return(true);
  }
}

// FIXME:
float StepperController::getMaxRpm() {
  // assume full step mode is index 0 and has the highest rpm_limit
  return(ms_modes[0].rpm_limit);
}

// FIXME:
void StepperController::setDirection(int direction) {
  // only update if necesary
  if ( (direction == DIR_CW || direction == DIR_CC) && state->direction != direction) {
    state->direction = direction;
    // if rotating to a specific position, changing direction turns the pump off
    if (state->status == STATUS_ROTATE) {
      state->status = STATUS_OFF;
    }
    updateStepper();
  }
}

// FIXME: done
void StepperController::start() { changeStatus(STATUS_ON); }
void StepperController::stop() { changeStatus(STATUS_OFF); }
void StepperController::hold() { changeStatus(STATUS_HOLD); }

// number of rotations
// FIXME:
long StepperController::rotate(double number) {
  long steps = state->direction * number * settings.steps * settings.gearing * state->ms_mode;
  stepper.setCurrentPosition(0);
  stepper.moveTo(steps);
  updateStepper(STATUS_ROTATE);
  return(steps);
}

/****** WEB COMMAND PROCESSING *******/

// FIXME:
void StepperController::parseCommand() {

  SerialDeviceController::parseCommand();

  if (command.isTypeDefined()) {
    // command processed successfully by parent function
  } else if (parsePage()) {
    // parse page command
  }

  // more additional, device specific commands

  // regular commands
  if (strcmp(command.variable, CMD_START) == 0) {
    // start
    assignCommandMessage();
    start();
  } else if (strcmp(command.variable, CMD_STOP) == 0) {
    // stop
    assignCommandMessage();
    stop();
  } else if (strcmp(command.variable, CMD_HOLD) == 0) {
    // hold
    assignCommandMessage();
    hold();
  } else if (strcmp(command.variable, CMD_MANUAL) == 0) {
    // hold
    assignCommandMessage();
    manual();
  } else if (strcmp(command.variable, CMD_ROTATE) == 0) {
    // rotate
    extractCommandParam(command.value);
    assignCommandMessage();
    rotate(atof(command.value));
  } else if (strcmp(command.variable, CMD_SET) == 0) {
    // calibrate
    strcpy(command.type, TYPE_SET);
    extractCommandParam(command.variable);
    extractCommandParam(command.value);
    assignCommandMessage();
    // TODO: implement these commands properly
    if (strcmp(command.variable, SET_STEP_FLOW) == 0) {
      // step flow
      strcpy(command.units, "volume/step");
    } else {
      strcpy(command.variable, ERROR_SET);
      strcpy(command.value, ""); // reset
      ret_val = CMD_RET_ERR_SET;
    }
  } else if (strcmp(command.variable, CMD_DIR) == 0) {
    // direction
    char direction[10];
    extractCommandParam(direction);
    strcpy(command.units, "cw/cc");
    assignCommandMessage();
    if (strcmp(direction, CMD_DIR_CW) == 0) {
      strcpy(command.value, "1"); // TODO: get this from dir_CW
      setDirection(DIR_CW);
    } else if (strcmp(direction, CMD_DIR_CC) == 0) {
      strcpy(command.value, "-1"); // TODO: get this from dir_CC
      setDirection(DIR_CC);
    } else if (strcmp(direction, CMD_DIR_CHANGE) == 0) {
      int new_dir = -state->direction;
      if (new_dir == DIR_CC) {
        strcpy(command.value, "-1"); // TODO: get this from dir_CC
      } else if (new_dir == DIR_CW) {
        strcpy(command.value, "1"); // TODO: get this from dir_CC
      }
      setDirection(new_dir);
    } else {
      strcpy(command.variable, ERROR_DIR);
      strcpy(command.units, ""); // reset
      ret_val = CMD_RET_ERR_DIR;
    }
  } else if (strcmp(command.variable, CMD_STEP) == 0) {
    // microstepping
    extractCommandParam(command.value);
    assignCommandMessage();
    if (strcmp(command.value, CMD_STEP_AUTO) == 0) {
      setAutoMicrosteppingMode();
    } else {
      int ms_mode = atoi(command.value);
      if (!setMicrosteppingMode(ms_mode)) {
          // invalid microstepping passed in
          strcpy(command.variable, ERROR_MS);
          strcpy(command.value, ""); // reset
          ret_val = CMD_RET_ERR_MS;
      }
    }
  } else if (strcmp(command.variable, CMD_SPEED) == 0) {
    // speed
    extractCommandParam(command.value);
    extractCommandParam(command.units);
    assignCommandMessage();
    if (strcmp(command.units, SPEED_RPM) == 0) {
      // speed rpm
      float rpm = atof(command.value);

      if (!setSpeedRpm(rpm)) {
        strcpy(command.variable, WARN_SPEED_MAX);
        sprintf(command.value, "%.2f", state->rpm);
        ret_val = CMD_RET_WARN_MAX_RPM;
      }

    } else if (strcmp(command.units, SPEED_FPM) == 0) {
      // speed fpm
      // TODO:
    } else {
      // invalid speed
      strcpy(command.variable, ERROR_SPEED);
      strcpy(command.value, ""); // reset
      strcpy(command.units, ""); // reset
      ret_val = CMD_RET_ERR;
    }
  } else {
    // no command found
    strcpy(command.variable, ERROR_CMD);
    ret_val = CMD_RET_ERR_CMD;
  }

}
