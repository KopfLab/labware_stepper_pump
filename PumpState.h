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
static void get_pump_state_status_info(int status, char* target_short, int size_short, char* target_long, int size_long)  {
  for (int i = 0; i < sizeof(STATUS_INFO) / sizeof(PumpStateInfo); i++) {
    if (STATUS_INFO[i].value == status) {
      strncpy(target_long, STATUS_INFO[i].long_label, size_long-1);
      strncpy(target_short, STATUS_INFO[i].short_label, size_short-1);
      break;
    }
  }
}

static void get_pump_state_direction_info(int direction, char* target_short, int size_short, char* target_long, int size_long) {
  for (int i = 0; i < sizeof(DIR_INFO) / sizeof(PumpStateInfo); i++) {
    if (DIR_INFO[i].value == direction) {
      strncpy(target_short, DIR_INFO[i].short_label, size_short-1);
      strncpy(target_long, DIR_INFO[i].long_label, size_long-1);
      break;
    }
  }
}

static void get_pump_state_speed_info(float rpm, char* target_short, int size_short, char* target_long, int size_long) {
    snprintf(target_short, size_short, "%2.2f", rpm);
    snprintf(target_long, size_long, "%2.5f", rpm);
}

static void get_pump_state_ms_info(int ms_index, int ms_mode, char* target_short, int size_short, char* target_long, int size_long) {
  if (ms_index == MS_MODE_AUTO) {
    snprintf(target_short, size_short, "%2dA", ms_mode);
    snprintf(target_long, size_long, "%2d (auto)", ms_mode);
  } else {
    snprintf(target_short, size_short, "%2d", ms_mode);
    snprintf(target_long, size_long, "%2d", ms_mode);
  }
}

static void get_pump_stat_locked_info(bool locked, char* target_short, int size_short, char* target_long, int size_long) {
  if (locked) {
    strncpy(target_short, "LOCK", size_short - 1);
    strncpy(target_long, "locked", size_long - 1);
  } else {
    strcpy(target_short, "");
    strncpy(target_long, "ready to receive", size_long - 1);
  }
}
