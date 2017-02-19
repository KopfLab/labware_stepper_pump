#include "PumpController.h"

void PumpController::init() {
  stepper = AccelStepper(AccelStepper::DRIVER, pins.step, pins.dir);
  stepper.setEnablePin(pins.enable);
  stepper.setPinsInverted	(
            PIN_STEP_ON != HIGH,
            PIN_DIR_CW != LOW,
            PIN_ENABLE_ON != LOW
        );
  stepper.disableOutputs();
  stepper.setMaxSpeed(settings.max_speed);
  stepper.setSpeed(settings.direction * settings.speed);

  // microstepping
  pinMode(pins.ms1, OUTPUT);
  pinMode(pins.ms2, OUTPUT);
  pinMode(pins.ms3, OUTPUT);
  setMicrostepping(settings.microstep);
}

bool PumpController::setMicrostepping(int ms) {

  if (ms == MS_MODE_AUTO) {
    Serial.println("INFO: switching to automatic microstepping mode");
    settings.microstep = MS_MODE_AUTO;
    return(true);
  } else {
    for (int i = 0; i < MS_MODES_N; i++) {
      Serial.println("checking " + String(ms_modes[i].mode));
      if (ms_modes[i].mode == ms) {
        Serial.println("found " + String(ms_modes[i].mode));
        settings.microstep = ms;
        digitalWrite(pins.ms1, ms_modes[i].ms1);
        digitalWrite(pins.ms2, ms_modes[i].ms2);
        digitalWrite(pins.ms3, ms_modes[i].ms3);
        return(true);
      }
    }
  }
  return(false);
}

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
      int new_dir = -settings.direction;
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
      setMicrostepping(MS_MODE_AUTO);
    } else {
      int ms = atoi(command.value);
      if (!setMicrostepping(ms)) {
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
      if (rpmToSpeed(rpm) <= settings.max_speed) {
        setSpeedRpm(rpm);
      } else {
        strcpy(command.variable, ERROR_SPEED_MAX);
        sprintf(command.value, "%.2f", speedToRpm(settings.max_speed));
        ret_val = CMD_RET_ERROR;
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

void PumpController::updateStatus() {
  if (settings.status == STATUS_ON) {
    stepper.setSpeed(settings.direction * settings.speed);
    stepper.enableOutputs();
  } else if (settings.status == STATUS_HOLD) {
    stepper.setSpeed(0);
    stepper.enableOutputs();
  } else if (settings.status == STATUS_OFF) {
    stepper.setSpeed(0);
    stepper.disableOutputs();
  } else if (settings.status == STATUS_ROTATE) {
    stepper.setSpeed(settings.direction * settings.speed);
    stepper.enableOutputs();
  }
  //TODO: should throw error
}


void PumpController::setDirection(int direction) {
  // only update if necesary
  if ( (direction == DIR_CW || direction == DIR_CC) && settings.direction != direction) {
    settings.direction = direction;
    // if rotating to a specific position, changing direction turns the pump off
    if (settings.status == STATUS_ROTATE) {
      settings.status = STATUS_OFF;
    }
    updateStatus();
  }
}

float PumpController::getRotationSteps() { return( 360/settings.step_angle * settings.microstep ); }
float PumpController::rpmToSpeed(float rpm) { return(rpm/60 * getRotationSteps()); }
float PumpController::speedToRpm(float speed) { return(speed/getRotationSteps() * 60); }
float PumpController::getRpm() { return speedToRpm(settings.speed); }

float PumpController::setSpeedRpm(float rpm) {
  float speed = rpmToSpeed(rpm);
  if (speed > settings.max_speed) {
    speed = settings.max_speed;
  } else if (speed < 0) {
    speed = 0;
  }
  settings.speed = speed;
  updateStatus();
  return(speed);
}

void PumpController::updateStatus(int status) {
  settings.status = status;
  updateStatus();
}

void PumpController::start() { updateStatus(STATUS_ON); }
void PumpController::stop() { updateStatus(STATUS_OFF); }
void PumpController::hold() { updateStatus(STATUS_HOLD); }

// number of rotations
long PumpController::rotate(double number) {
  long steps = settings.direction * number * getRotationSteps();
  stepper.setCurrentPosition(0);
  stepper.moveTo(steps);
  updateStatus(STATUS_ROTATE);
  return(steps);
}

// loop function
void PumpController::update() {
  if (settings.status == STATUS_ROTATE) {
    if (stepper.distanceToGo() == 0) {
      updateStatus(STATUS_OFF); // disengage if reached target location
    } else {
      stepper.runSpeedToPosition();
    }
  } else {
    stepper.runSpeed();
  }
}
