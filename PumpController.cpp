#include "PumpController.h"

/**** SETUP AND LOOP ****/

void PumpController::init() { init(false); }
void PumpController::init(bool reset) {
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
  Serial.println("INFO: available microstepping modes");
  for (int i = 0; i < ms_modes_n; i++) {
    Serial.println("   Mode " + String(i) + ": " + String(ms_modes[i].mode) +
      " steps, max rpm: " + String(ms_modes[i].rpm_limit));
  }

  // initialize / restore pump state
  if (!reset){
    loadPumpState();
  } else {
    Serial.println("INFO: resetting pump state back to default values");
  }
  activateMicrostepping(state.ms_index, state.rpm);
  updateStatus();
}

// loop function
void PumpController::update() {
  if (state.status == STATUS_ROTATE) {
    // WARNING: FIXME known bug, when power out, saved rotate status will lead to immediate stop of pump
    if (stepper.distanceToGo() == 0) {
      updateStatus(STATUS_OFF); // disengage if reached target location
      strcpy(command.type, TYPE_EVENT);
      strcpy(command.variable, "rot end"); // rotation finished
      if (command_callback) command_callback(*this);
    } else {
      stepper.runSpeedToPosition();
    }
  } else {
    stepper.runSpeed();
  }
}

/**** LOCK AND UNLOCK ****/

void PumpController::lock() {
  state.locked = true;
  Serial.println("INFO: locking pump");
  savePumpState();
}

void PumpController::unlock() {
  state.locked = false;
  Serial.println("INFO: unlocking pump");
  savePumpState();
}

/**** STATE STORAGE AND RELOAD ****/
void PumpController::savePumpState() {
  EEPROM.put(STATE_ADDRESS, state);
  Serial.println("INFO: pump state saved in memory (if any updates were necessary)");
}

bool PumpController::loadPumpState(){
  PumpState saved_state;
  EEPROM.get(STATE_ADDRESS, saved_state);
  if(saved_state.version == STATE_VERSION) {
    EEPROM.get(STATE_ADDRESS, state);
    Serial.println("INFO: successfully restored pump state from memory (version " + String(STATE_VERSION) + ")");
    return(true);
  } else {
    // EEPROM saved state did not have the corret version
    Serial.println("INFO: could not restore pump state from memory, sticking with initial default");
    return(false);
  }
}

/**** UPDATING PUMP STATUS ****/

void PumpController::updateStatus() {
  Serial.println("INFO: updating status");
  savePumpState();
  if (state.status == STATUS_ON) {
    stepper.setSpeed(calculateSpeed());
    stepper.enableOutputs();
  } else if (state.status == STATUS_HOLD) {
    stepper.setSpeed(0);
    stepper.enableOutputs();
  } else if (state.status == STATUS_OFF) {
    stepper.setSpeed(0);
    stepper.disableOutputs();
  } else if (state.status == STATUS_ROTATE) {
    stepper.setSpeed(calculateSpeed());
    stepper.enableOutputs();
  }
  //TODO: should throw error
}

void PumpController::updateStatus(int status) {
  state.status = status;
  updateStatus();
}

float PumpController::calculateSpeed() {
  float speed = state.rpm/60.0 * settings.steps * settings.gearing * state.ms_mode * state.direction;
  Serial.println("INFO: calculated speed " + String(speed));
  return(speed);
}

/**** CHANGING PUMP STATE ****/

bool PumpController::setMicrosteppingMode(int ms_mode) {
  // determine microstepping
  int ms_index = -1;
  if (ms_mode == MS_MODE_AUTO) {
    Serial.println("INFO: switching to automatic microstepping");
    state.ms_index = MS_MODE_AUTO;
    ms_index = MS_MODE_AUTO;
  } else {
    for (int i = 0; i < ms_modes_n; i++) {
      if (ms_modes[i].mode == ms_mode) {
        state.ms_index = i;
        ms_index = i;
        break;
      }
    }
    // no valid index found
    if (ms_index == -1) return(false);
  }

  // set new microstepping and adjust speed with limit
  int active_ms_index = activateMicrostepping(ms_index, state.rpm);
  setSpeedWithSteppingLimit(active_ms_index, state.rpm);
  updateStatus();
  return(true);
}

int PumpController::findAutoMicrostepIndex(float rpm) {
  // find lowest MS mode that can handle these rpm (otherwise go to full step -> ms_index = 0)
  int ms_index = 0;
  for (int i = 1; i < ms_modes_n; i++) {
    if (rpm <= ms_modes[i].rpm_limit) ms_index = i;
  }
  return(ms_index);
}

int PumpController::activateMicrostepping(int ms_index, float rpm) {

  if (ms_index == MS_MODE_AUTO) {
    // check for appropriate step mode
    ms_index = findAutoMicrostepIndex(rpm);
  }

  // activate microstepping
  if (ms_index >= 0 && ms_index < ms_modes_n) {
    Serial.println("INFO: activating microstepping " + String(ms_modes[ms_index].mode));
    digitalWrite(pins.ms1, ms_modes[ms_index].ms1);
    digitalWrite(pins.ms2, ms_modes[ms_index].ms2);
    digitalWrite(pins.ms3, ms_modes[ms_index].ms3);
    state.ms_mode = ms_modes[ms_index].mode;
  }

  return(ms_index);
}

bool PumpController::setSpeedRpm(float rpm) {
  int active_ms_index = activateMicrostepping(state.ms_index, rpm);
  bool limited = setSpeedWithSteppingLimit(active_ms_index, rpm);
  updateStatus();
  return(limited);
}

bool PumpController::setSpeedWithSteppingLimit(int ms_index, float rpm) {
  if (rpm > ms_modes[ms_index].rpm_limit) {
    Serial.println("WARNING: stepping mode is not fast enough for the requested rpm: " + String(rpm));
    Serial.println("  --> switching to MS mode rpm limit of " + String(ms_modes[ms_index].rpm_limit));
    state.rpm = ms_modes[ms_index].rpm_limit;
    return(false);
  } else {
    state.rpm = rpm;
    return(true);
  }
}

void PumpController::setDirection(int direction) {
  // only update if necesary
  if ( (direction == DIR_CW || direction == DIR_CC) && state.direction != direction) {
    state.direction = direction;
    // if rotating to a specific position, changing direction turns the pump off
    if (state.status == STATUS_ROTATE) {
      state.status = STATUS_OFF;
    }
    updateStatus();
  }
}

void PumpController::start() { updateStatus(STATUS_ON); }
void PumpController::stop() { updateStatus(STATUS_OFF); }
void PumpController::hold() { updateStatus(STATUS_HOLD); }

// number of rotations
long PumpController::rotate(double number) {
  long steps = state.direction * number * settings.steps * settings.gearing * state.ms_mode;
  stepper.setCurrentPosition(0);
  stepper.moveTo(steps);
  updateStatus(STATUS_ROTATE);
  return(steps);
}

/****** WEB COMMAND PROCESSING *******/

// using char array pointers instead of String to make sure we don't get memory leaks here
void PumpController::extractCommandParam(char* param) {
  int space = strcspn(command.buffer, " ");
  strncpy (param, command.buffer, space);
  param[space] = 0;
  if (space == strlen(command.buffer)) {
    command.buffer[0] = 0;
  } else {
    for(int i = space+1; i <= strlen(command.buffer); i+=1) {
      command.buffer[i-space-1] = command.buffer[i];
    }
  }
}

void PumpController::assignCommandMessage() {
  // takes the remainder of the command buffer and assigns it to the message
  strncpy(command.msg, command.buffer, sizeof(command.msg));
}

int PumpController::parseCommand(String command_string) {
  command = PumpCommand(command_string);
  extractCommandParam(command.variable);
  strcpy(command.type, TYPE_EVENT); // default
  strcpy(command.value, ""); // reset
  strcpy(command.units, ""); // reset
  strcpy(command.msg, ""); // reset

  int ret_val = CMD_RET_SUCCESS;

  // locking
  if (strcmp(command.variable, CMD_UNLOCK) == 0) {
    assignCommandMessage();
    unlock();
  } else if (strcmp(command.variable, CMD_LOCK) == 0) {
    assignCommandMessage();
    lock();
  } else if (state.locked) {
    // pump is locked and command is not lock/unlock
    strcpy(command.variable, ERROR_LOCKED);
    ret_val = CMD_RET_ERROR;
  } else {
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
        ret_val = CMD_RET_ERROR;
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
        int new_dir = -state.direction;
        if (new_dir == DIR_CC) {
          strcpy(command.value, "-1"); // TODO: get this from dir_CC
        } else if (new_dir == DIR_CW) {
          strcpy(command.value, "1"); // TODO: get this from dir_CC
        }
        setDirection(new_dir);
      } else {
        strcpy(command.variable, ERROR_DIR);
        strcpy(command.units, ""); // reset
        ret_val = CMD_RET_ERROR;
      }
    } else if (strcmp(command.variable, CMD_STEP) == 0) {
      // microstepping
      extractCommandParam(command.value);
      assignCommandMessage();
      if (strcmp(command.value, CMD_STEP_AUTO) == 0) {
        setMicrosteppingMode(MS_MODE_AUTO);
      } else {
        int ms_mode = atoi(command.value);
        if (!setMicrosteppingMode(ms_mode)) {
            // invalid microstepping passed in
            strcpy(command.variable, ERROR_MS);
            strcpy(command.value, ""); // reset
            ret_val = CMD_RET_ERROR;
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
          sprintf(command.value, "%.2f", state.rpm);
          ret_val = CMD_RET_WARNING;
        }

      } else if (strcmp(command.units, SPEED_FPM) == 0) {
        // speed fpm
        // TODO:
      } else {
        // invalid speed
        strcpy(command.variable, ERROR_SPEED);
        strcpy(command.value, ""); // reset
        strcpy(command.units, ""); // reset
        ret_val = CMD_RET_ERROR;
      }
    } else {
      // no command found
      strcpy(command.variable, ERROR_CMD);
      ret_val = CMD_RET_ERROR;
    }
  }

  // error handling
  if (ret_val == CMD_RET_ERROR) {
    strcpy(command.type, TYPE_ERROR);
    command_string.toCharArray(command.msg, sizeof(command.msg));
  }

  // command reporting callback
  if (command_callback) {
    command_callback(*this);
  }

  return(ret_val);
}
