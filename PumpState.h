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

// state info
#define INFO_SHORT 1
#define INFO_LONG  2
struct PumpStateInfo {
   int value;
   char *short_label;
   char *long_label;
};

const PumpStateInfo STATUS_INFO[] = {
   {STATUS_ON, "on", "running"},
   {STATUS_OFF, "off", "off"},
   {STATUS_HOLD, "hold", "holding position"},
   {STATUS_ROTATE, "rot", "executing number of rotations"},
   {STATUS_TRIGGER, "trig", "triggered by external signal"}
};

const PumpStateInfo DIR_INFO[] = {
   {DIR_CW, "cw", "clockwise"},
   {DIR_CC, "cc", "counter-clockwise"}
};

// NOTE: size is passed as safety precaution to not overallocate the target
// sizeof(target) would not work because it's a pointer (always size 4)
static void get_pump_state_status_info(int status, char* target, int size, int type)  {
  for (int i = 0; i < sizeof(STATUS_INFO) / sizeof(PumpStateInfo); i++) {
    if (STATUS_INFO[i].value == status) {
      if (type == INFO_LONG)
        strncpy(target, STATUS_INFO[i].long_label, size-1);
      else if (type == INFO_SHORT)
        strncpy(target, STATUS_INFO[i].short_label, size-1);
      break;
    }
  }
}

static void get_pump_state_direction_info(int direction, char* target, int size, int type) {
  for (int i = 0; i < sizeof(DIR_INFO) / sizeof(PumpStateInfo); i++) {
    if (DIR_INFO[i].value == direction) {
      if (type == INFO_LONG)
        strncpy(target, DIR_INFO[i].long_label, size-1);
      else if (type == INFO_SHORT)
        strncpy(target, DIR_INFO[i].short_label, size-1);
      break;
    }
  }
}

static void get_pump_state_speed_info(float rpm, char* target, int size) {
  snprintf(target, size, "%2.2f rpm", rpm);
}

static void get_pump_state_ms_info(int ms_index, int ms_mode, char* target, int size, int type) {
  if (type == INFO_SHORT && ms_index == MS_MODE_AUTO) {
    snprintf(target, size, "%2dA", ms_mode);
  } else if (type == INFO_SHORT) {
    snprintf(target, size, "%2d", ms_mode);
  } else if (type == INFO_LONG && ms_index == MS_MODE_AUTO) {
    snprintf(target, size, "%2d (auto)", ms_mode);
  } else if (type == INFO_LONG) {
    snprintf(target, size, "%2d", ms_mode);
  }
}

static void get_pump_stat_locked_info(bool locked, char* target, int size, int type) {
  if (type == INFO_SHORT && locked) {
    strncpy(target, "LOCK", size - 1);
  } else if (type == INFO_SHORT) {
    strcpy(target, "");
  } else if (type == INFO_LONG && locked) {
    strncpy(target, "locked", size - 1);
  } else if (type == INFO_LONG) {
    strncpy(target, "ready to receive", size - 1);
  }
}
