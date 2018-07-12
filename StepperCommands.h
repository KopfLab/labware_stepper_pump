// return codes
#define CMD_RET_SUCCESS        0 // succes = 0

#define CMD_RET_ERR           -1 // errors < 0
#define CMD_RET_ERR_CMD       -2 // error unknown command
#define ERROR_CMD             "unknown command"
#define CMD_RET_ERR_LOCKED    -3 // error locked
#define ERROR_LOCKED          "locked"
#define CMD_RET_ERR_DIR       -5
#define ERROR_DIR             "unknown direction"
#define CMD_RET_ERR_MS        -6
#define ERROR_MS              "unknown microstepping"
#define CMD_RET_ERR_SPEED     -7
#define ERROR_SPEED           "unknown speed"

#define CMD_RET_WARN          1 // warnings > 0
#define CMD_RET_WARN_MAX_RPM  2 // warning max speed exceeded
#define WARN_SPEED_MAX        "> max rpm"

// running
#define CMD_START       "start" // pump start [msg] : starts the pump
#define CMD_STOP        "stop" // pump stop [msg] : stops the pump (stops power)
#define CMD_HOLD        "hold" // pump hold [msg] : engage the pump but not running (power on, no flow)
#define CMD_MANUAL      "manual" // pump manual [msg] : switches the pump to manual (i.e. analog signal) control
#define CMD_RUN         "run" // pump run minutes [msg] : runs the pump for x minutes
#define CMD_AUTO        "auto" // pump auto [msg] : listens to external trigger on the trigger pin
#define CMD_ROTATE      "rotate" // # pump rotate number [msg] : run for x number of rotations

// speed setting
#define CMD_SPEED       "speed" // pump speed number rpm/fpm [msg] : set the pump speed
  #define SPEED_RPM       "rpm" // pump speed number rpm [msg] : set speed in rotations per minute (requires step-angle)
#define CMD_RAMP        "ramp"  // pump ramp number rpm/fpm minutes [msg] : ramp speed from current to [number] rpm/fpm over [minutes] of time

// microstepping setting
#define CMD_STEP        "ms" // pump ms number/auto [msg] : set the microstepping
  #define CMD_STEP_AUTO       "auto" // signal to put microstepping into automatic mode (i.e. always pick the highest microstepping that the clockspeed supports)

// direction
#define CMD_DIR         "direction" // pump direction cw/cc/switch [msg] : set the direction
  #define CMD_DIR_CW      "cw"
  #define CMD_DIR_CC      "cc"
  #define CMD_DIR_CHANGE  "switch"

// message types
#define TYPE_ERROR      "error"
#define TYPE_EVENT      "event"
#define TYPE_SET        CMD_SET

// command from spark cloud
struct PumpCommand {
  char buffer[63]; //(spark.functions are limited to 63 char long call)
  char type[20];
  char variable[25];
  char value[20];
  char units[20];
  char msg[63];
  PumpCommand() {};
  PumpCommand(String& command_string) {
    command_string.toCharArray(buffer, sizeof(buffer));
  }
};
