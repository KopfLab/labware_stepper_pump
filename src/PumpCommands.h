
// PUMP SPECIFIC COMMANDS - NOT IMPLEMENTED YET FIXME
#define CMD_DISPENSE    "dispense" // pump dispense amount [msg] : dispense a certain amount (requires step-flow calibration)

#define CMD_SET         "calibrate" // pump calibrate variable value [msg]
#define SET_STEP_FLOW   "step-flow" // set flow per step (step-flow)
#define SET_ROT_FLOW    "rotation-flow" // set step-flow (step-flow) from flow per rotation [requires step-angle]
#define SET_FPM         "fpm" // set step-flow (step-flow) from flow per minute [requires step-angle and current speed]

#define SPEED_FPM       "fpm" // pump speed number fpm [msg] : set speed in flow per minute (requires step-flow calibration)

#define CMD_RET_ERR_SET  -4
#define ERROR_SET        "unknown calibrate"
