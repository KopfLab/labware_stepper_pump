// status
#define CMD_START       "start" // device start [msg] : starts the stepper
#define CMD_STOP        "stop" // device stop [msg] : stops the stepper (stops power)
#define CMD_HOLD        "hold" // device hold [msg] : engage the stepper but not running (power on, no flow)
#define CMD_RUN         "run" // device run minutes [msg] : runs the stepper for x minutes
#define CMD_AUTO        "auto" // device auto [msg] : listens to external trigger on the trigger pin
#define CMD_ROTATE      "rotate" // device rotate number [msg] : run for x number of rotations

// direction
#define CMD_DIR         "direction" // device direction cw/cc/switch [msg] : set the direction
  #define CMD_DIR_CW      "cw"
  #define CMD_DIR_CC      "cc"
  #define CMD_DIR_SWITCH  "switch"

// speed
#define CMD_SPEED       "speed" // device speed number rpm/fpm [msg] : set the stepper speed
  #define SPEED_RPM       "rpm" // device speed number rpm [msg] : set speed in rotations per minute (requires step-angle)
#define CMD_RAMP        "ramp"  // device ramp number rpm/fpm minutes [msg] : ramp speed from current to [number] rpm/fpm over [minutes] of time

// microstepping
#define CMD_STEP        "ms" // device ms number/auto [msg] : set the microstepping
  #define CMD_STEP_AUTO   "auto" // signal to put microstepping into automatic mode (i.e. always pick the highest microstepping that the clockspeed supports)

// warnings
#define CMD_RET_WARN_MAX_RPM      101
#define CMD_RET_WARN_MAX_RPM_TEXT "exceeds max rpm"
