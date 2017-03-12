// pin constants and wiring structure
#define PIN_DIR_CW LOW // is clockwise on LOW or HIGH?
#define PIN_STEP_ON HIGH // is a step made on LOW or HIGH?
#define PIN_ENABLE_ON HIGH // is enable on LOW or HIGH?
struct StepperPins {
  const int dir; // direction
  const int step; // step
  const int enable; // enable
  const int ms1; // microstep 1
  const int ms2; // microstep 2
  const int ms3; // microstep 3
  StepperPins(int dir, int step, int enable, int ms1, int ms2, int ms3) :
    dir(dir), step(step), enable(enable), ms1(ms1), ms2(ms2), ms3(ms3) {};
};

// microstep mode structur (driver chip specific)
#define MS_MODE_AUTO  -1 // code for automatic determination of microstep modes
struct MicrostepMode {
  int mode; // the mode for the ms mode
  bool ms1; // HIGH or LOW for ms1
  bool ms2; // HIGH or LOW for ms2
  bool ms3; // HIGH or LOW for ms3
  float rpm_limit; // the RPM limit for this microstepping mode (filled by PumpController later)
  MicrostepMode() {};
  MicrostepMode(int mode, bool ms1, bool ms2, bool ms3) :
    mode(mode), ms1(ms1), ms2(ms2), ms3(ms3), rpm_limit(-1) {}
};

// pump settings (microcontroller and pump stepper specific)
struct PumpSettings {
public:
  int steps; // numer of steps / rotation
  double gearing; // the gearing (if any, otherwise should be 1)
  float max_speed; // maximum # of steps/s the processor can reliably support (i.e. how often can the PumpController::update() be called?)

  PumpSettings(int steps, double gearing, float max_speed) :
    steps(steps), gearing(gearing), max_speed(max_speed) {};

};
