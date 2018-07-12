#include "StepperController.h"


// loop function
void StepperController::update() {
  // check if it's time for a delayed status update
  if (delay_status_update && millis() > update_status_time) {
    Serial.println("INFO: finishing status update delay");
    delay_status_update = false;
    updateStepper();
  }

  // process STATUS dependent pump udpate
  if (state->status == STATUS_ROTATE) {
    // WARNING: FIXME known bug, when power out, saved rotate status will lead to immediate stop of pump
    if (stepper.distanceToGo() == 0) {
      updateStepper(STATUS_OFF); // disengage if reached target location
      strcpy(command.type, TYPE_EVENT);
      strcpy(command.variable, "rot end"); // rotation finished
      if (command_callback) command_callback(*this);
    } else {
      stepper.runSpeedToPosition();
    }
  } else {
    // check if need to adjust anything if in MANUAL mode
    // FIXME: optimize the distruption to the stepper either by multi threading the LCD or by overwriting only the necessary characters always (I2C is the slowest part)
    if (state->status == STATUS_MANUAL) {
      float manual_rpm = round(analogRead(pins.manual)/4095.0 * getMaxRpm()); // particle ADC is 12-bit, i.e. 0-4095 range
      if (manual_rpm != last_manual_rpm) {
        // start debounce
        last_manual_rpm = manual_rpm;
        last_manual_read = millis();
      } else if (manual_rpm == last_manual_rpm && millis() - last_manual_read > MANUAL_DEBOUNCE && manual_rpm != state->rpm) {
        // stable signal for the duration of the debounce interval, different from active rpm
        Serial.println("INFO: new manual rpm: " + String(manual_rpm));
        setStatusUpdateDelay(MANUAL_STATUS_UPDATE_DELAY);
        setSpeedRpm(manual_rpm);
        strcpy(command.type, TYPE_EVENT);
        strcpy(command.variable, "new rpm");
        if (command_callback) command_callback(*this); // command reporting callback
        updateStepper();
      }
    }
    stepper.runSpeed();
  }
}

/**** LOCK AND UNLOCK ****/

void StepperController::lock() {
  state->locked = true;
  Serial.println("INFO: locking pump");
  saveStepperState();
}

void StepperController::unlock() {
  state->locked = false;
  Serial.println("INFO: unlocking pump");
  saveStepperState();
}

/**** STATE STORAGE AND RELOAD ****/
void StepperController::saveStepperState() {
  EEPROM.put(STATE_ADDRESS, state);
  Serial.println("INFO: pump state saved in memory (if any updates were necessary)");
}

bool StepperController::loadStepperState(){
  StepperState saved_state;
  EEPROM.get(STATE_ADDRESS, saved_state);
  if(saved_state->version == STATE_VERSION) {
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

void StepperController::setStatusUpdateDelay(long delay) {
  Serial.println("INFO: activating/resetting status update delay of " + String(delay) + "ms");
  delay_status_update = true;
  update_status_time = millis() + delay;
}

void StepperController::updateStepper() {
  if (delay_status_update) {
    Serial.println("INFO: updating status is still delayed");
  } else {

    // save pump state to EEPROM
    Serial.println("INFO: updating status");
    saveStepperState();

    // update microstepping
    if (state->ms_index >= 0 && state->ms_index < ms_modes_n) {
      digitalWrite(pins.ms1, ms_modes[state->ms_index].ms1);
      digitalWrite(pins.ms2, ms_modes[state->ms_index].ms2);
      digitalWrite(pins.ms3, ms_modes[state->ms_index].ms3);
    }

    // update speed
    stepper.setSpeed(calculateSpeed());

    // update enabled / disabled
    if (state->status == STATUS_ON || state->status == STATUS_ROTATE || (state->status == STATUS_MANUAL && state->rpm > 0)) {
      stepper.enableOutputs();
    } else if (state->status == STATUS_HOLD) {
      stepper.setSpeed(0);
      stepper.enableOutputs();
    } else {
      // STATUS_OFF and STATUS_MANUAL but rpm == 0
      stepper.setSpeed(0);
      stepper.disableOutputs();
    }
  }
}

void StepperController::updateStepper(int status) {
  state->status = status;
  updateStepper();
}

float StepperController::calculateSpeed() {
  float speed = state->rpm/60.0 * settings.steps * settings.gearing * state->ms_mode * state->direction;
  Serial.println("INFO: calculated speed " + String(speed));
  return(speed);
}

/**** CHANGING PUMP STATE ****/



/****** WEB COMMAND PROCESSING *******/

// using char array pointers instead of String to make sure we don't get memory leaks here
void StepperController::extractCommandParam(char* param) {
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

void StepperController::assignCommandMessage() {
  // takes the remainder of the command buffer and assigns it to the message
  strncpy(command.msg, command.buffer, sizeof(command.msg));
}

int StepperController::parseCommand(String command_string) {
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
  } else if (state->locked) {
    // pump is locked and command is not lock/unlock
    strcpy(command.variable, ERROR_LOCKED);
    ret_val = CMD_RET_ERR_LOCKED;
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

  // highlight if it's an error
  if (ret_val < 0) {
    strcpy(command.type, TYPE_ERROR);
    command_string.toCharArray(command.msg, sizeof(command.msg));
  }

  // command reporting callback
  if (command_callback) {
    command_callback(*this);
  }

  return(ret_val);
}
