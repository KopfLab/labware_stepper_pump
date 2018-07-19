#pragma once
#include "device/SerialDeviceState.h"

// pump state
#define DIR_CW           1
#define DIR_CC          -1
#define STATUS_ON        1
#define STATUS_OFF       2
#define STATUS_HOLD      3
#define STATUS_MANUAL    4
#define STATUS_ROTATE    5
#define STATUS_TRIGGER   6 // TODO: implement signal triggering mode
#define STEP_FLOW_UNDEF -1
#define STATE_ADDRESS    0 // EEPROM storage location

/**** textual translations of state values ****/

struct StepperState : public DeviceState {
  int direction; // DIR_CW or DIR_CC
  bool ms_auto; // whether microstepping is in automatic mode or not
  int ms_index; // the index of the currently active microstep mode
  int ms_mode; // stores the actual ms_mode that is active (just for convenience)
  int status; // STATUS_ON, OFF, HOLD
  float rpm; // speed in rotations / minute (actual speed in steps/s depends on microstepping mode)

  StepperState() {};
  // construct StepperState in autostepping mode
  StepperState(bool locked, bool state_logging, bool data_logging, uint data_logging_period, uint8_t data_logging_type, int direction, int status, float rpm) :
    DeviceState(locked, state_logging, data_logging, data_logging_period, data_logging_type), direction(direction), ms_auto(true), ms_index(-1), status(status), rpm(rpm) {};
  // construct StepperState with specific ms mode
  StepperState(bool locked, bool state_logging, bool data_logging, uint data_logging_period, uint8_t data_logging_type, int direction, int status, float rpm, int ms_index) :
    DeviceState(locked, state_logging, data_logging, data_logging_period, data_logging_type), direction(direction), ms_auto(false), ms_index(ms_index), status(status), rpm(rpm){};
};

// state info (note: long label may not be necessary)
struct StepperStateInfo {
   int value;
   char *short_label;
   char *long_label;
};

const StepperStateInfo STATUS_INFO[] = {
   {STATUS_ON, "on", "running"},
   {STATUS_OFF, "off", "off"},
   {STATUS_HOLD, "hold", "holding position"},
   {STATUS_MANUAL, "man", "manual mode"},
   {STATUS_ROTATE, "rot", "executing number of rotations"},
   {STATUS_TRIGGER, "trig", "triggered by external signal"}
};

const StepperStateInfo DIR_INFO[] = {
   {DIR_CW, "cw", "clockwise"},
   {DIR_CC, "cc", "counter-clockwise"}
};

// pump status info
static void getStepperStateStatusInfo(int status, char* target, int size, char* pattern, bool include_key = true)  {
  int i = 0;
  for (; i < sizeof(STATUS_INFO) / sizeof(StepperStateInfo); i++) {
    if (STATUS_INFO[i].value == status) break;
  }
  getStateStringText("status", STATUS_INFO[i].short_label, target, size, pattern, include_key);
}

static void getStepperStateStatusInfo(int status, char* target, int size, bool value_only = false) {
  if (value_only) getStepperStateStatusInfo(status, target, size, PATTERN_V_SIMPLE, false);
  else getStepperStateStatusInfo(status, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// pump direction info
static void getStepperStateDirectionInfo(int direction, char* target, int size, char* pattern, bool include_key = true)  {
  int i = 0;
  for (; i < sizeof(DIR_INFO) / sizeof(StepperStateInfo); i++) {
    if (DIR_INFO[i].value == direction) break;
  }
  getStateStringText("dir", DIR_INFO[i].short_label, target, size, pattern, include_key);
}

static void getStepperStateDirectionInfo(int direction, char* target, int size, bool value_only = false) {
  if (value_only) getStepperStateDirectionInfo(direction, target, size, PATTERN_V_SIMPLE, false);
  else getStepperStateDirectionInfo(direction, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// speed
static void getStepperStateSpeedInfo(float rpm, char* target, int size, char* pattern, bool include_key = true) {
  int decimals = find_signif_decimals(rpm, 3, true, 3); // 3 significant digits by default, max 3 after decimals
  getStateDoubleText("speed", rpm, "rpm", target, size, pattern, decimals, include_key);
}

static void getStepperStateSpeedInfo(float rpm, char* target, int size, bool value_only = false) {
  if (value_only) getStepperStateSpeedInfo(rpm, target, size, PATTERN_VU_SIMPLE, false);
  else getStepperStateSpeedInfo(rpm, target, size, PATTERN_KVU_JSON_QUOTED, true);
}

// microstepping info
static void getStepperStateMSInfo(bool ms_auto, int ms_mode, char* target, int size, char* pattern, bool include_key = true) {
  char ms_text[10];
  (ms_auto) ?
    snprintf(ms_text, sizeof(ms_text), "%dA", ms_mode) :
    snprintf(ms_text, sizeof(ms_text), "%d", ms_mode);
  getStateStringText("ms", ms_text, target, size, pattern, include_key);
}

static void getStepperStateMSInfo(bool ms_auto, int ms_mode, char* target, int size, bool value_only = false) {
  if (value_only) getStepperStateMSInfo(ms_auto, ms_mode, target, size, PATTERN_V_SIMPLE, false);
  else getStepperStateMSInfo(ms_auto, ms_mode, target, size, PATTERN_KV_JSON_QUOTED, true);
}
