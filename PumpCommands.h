// commands
#define CMD_ROOT        "pump" // command root

// running
#define CMD_START       "start" // pump start [msg] : starts the pump
#define CMD_STOP        "stop" // pump stop [msg] : stops the pump (stops power)
#define CMD_HOLD        "hold" // pump hold [msg] : engage the pump but not running (power on, no flow)
#define CMD_RUN         "run" // pump run minutes [msg] : runs the pump for x minutes
#define CMD_AUTO        "auto" // pump auto [msg] : listens to external trigger on the trigger pin
#define CMD_DISPENSE    "dispense" // pump dispense amount [msg] : dispense a certain amount (requires step-flow calibration)
#define CMD_ROTATE      "rotate" // # pump rotate number [msg] : run for x number of rotations (requires step-angle)

// speed setting
#define CMD_SPEED       "speed" // pump speed number rpm/fpm [msg] : set the pump speed
  #define SPEED_RPM       "rpm" // pump speed number rpm [msg] : set speed in rotations per minute (requires step-angle)
  #define SPEED_FPM       "fpm" // pump speed number fpm [msg] : set speed in flow per minute (requires step-flow calibration)
#define CMD_RAMP        "ramp"  // pump ramp number rpm/fpm minutes [msg] : ramp speed from current to [number] rpm/fpm over [minutes] of time

// microstepping setting
#define CMD_STEP        "ms" // pump ms number/auto [msg] : set the microstepping
  #define CMD_STEP_AUTO       "auto" // signal to put microstepping into automatic mode (i.e. always pick the highest microstepping that the clockspeed supports)

// direction
#define CMD_DIR         "direction" // pump direction cw/cc/switch [msg] : set the direction
  #define CMD_DIR_CW      "cw"
  #define CMD_DIR_CC      "cc"
  #define CMD_DIR_CHANGE  "switch"

// calibrate pump flow (TODO: not all of these are implemented yet)
#define CMD_SET         "calibrate" // pump calibrate variable value [msg]
#define SET_STEP_FLOW   "step-flow" // set flow per step (step-flow)
#define SET_ROT_FLOW    "rotation-flow" // set step-flow (step-flow) from flow per rotation [requires step-angle]
#define SET_FPM         "fpm" // set step-flow (step-flow) from flow per minute [requires step-angle and current speed]

// message types
#define TYPE_ERROR      "error"
#define TYPE_EVENT      "event"
#define TYPE_SET        CMD_SET

// error messages
#define ERROR_CMD       "unknown command"
#define ERROR_SET       "unknown calibrate"
#define ERROR_DIR       "unknown direction"
#define ERROR_MS        "unknown microstepping"
#define ERROR_SPEED     "unknown speed"
#define ERROR_SPEED_MAX "speed exceeds maximum of"
