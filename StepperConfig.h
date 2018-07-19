#pragma once

// board
struct StepperBoard {
  const int dir; // direction
  const int step; // step
  const int enable; // enable
  const int ms1; // microstep 1
  const int ms2; // microstep 2
  const int ms3; // microstep 3
  const float max_speed; // maximum # of steps/s the processor can reliably support (i.e. how often can the StepperController::update() be called?)
  StepperBoard(int dir, int step, int enable, int ms1, int ms2, int ms3, float max_speed) :
    dir(dir), step(step), enable(enable), ms1(ms1), ms2(ms2), ms3(ms3), max_speed(max_speed) {};
};

// microstep mode structure (driver chip specific)
struct MicrostepMode {
  int mode; // the mode for the ms mode
  bool ms1; // HIGH or LOW for ms1
  bool ms2; // HIGH or LOW for ms2
  bool ms3; // HIGH or LOW for ms3
  float rpm_limit; // the RPM limit for this microstepping mode (filled by StepperController later)
  MicrostepMode() {};
  MicrostepMode(int mode, bool ms1, bool ms2, bool ms3) :
    mode(mode), ms1(ms1), ms2(ms2), ms3(ms3), rpm_limit(-1) {}
};


// driver
struct StepperDriver {
  const bool dir_cw; // is clockwise LOW or HIGH?
  const bool step_on; // is a step made on LOW or HIGH?
  const bool enable_on; // is enable on LOW or HIGH?
  const int ms_modes_n; // number of microstepping modes
  MicrostepMode* ms_modes; // microstepping modes
  StepperDriver(bool dir_cw, bool step_on, bool enable_on, MicrostepMode* ms_modes, int ms_modes_n) :
    dir_cw(dir_cw), step_on(step_on), enable_on(enable_on), ms_modes_n(ms_modes_n) {
      // allocate microstep modes (FIXME: is there a better way to do this with pointers?)
      this->ms_modes = new MicrostepMode[ms_modes_n];
      for (int i = 0; i < ms_modes_n; i++) this->ms_modes[i] = ms_modes[i];
    };

  // calculates rpm limits for all modes
  void calculateRpmLimits(float max_speed, int steps, double gearing) {
    for (int i = 0; i < ms_modes_n; i++)
      this->ms_modes[i].rpm_limit = max_speed * 60.0 / (steps * gearing * ms_modes[i].mode);
  }

  // find microstep index for specified rpm
  int findMicrostepIndexForRpm(float rpm) {
    //find lowest MS mode that can handle the rpm
    int ms_index = 0;
    for (int i = 1; i < ms_modes_n; i++) {
      if (rpm <= ms_modes[i].rpm_limit) ms_index = i;
    }
    return(ms_index);
  }

  // find microstep index for specified mode
  int findMicrostepIndexForMode(int mode) {
    // find index for requested microstepping mode
    int ms_index = -1;
    for (int i = 0; i < ms_modes_n; i++) {
      if (ms_modes[i].mode == mode) {
        ms_index = i;
        break;
      }
    }
    // none found --> -1
    return(ms_index);
  }

  // get mode for index
  int getMode(int index) {
    return(ms_modes[index].mode);
  }

  // get rpm limit for mode index
  float getRpmLimit(int index) {
    return(ms_modes[index].rpm_limit);
  }

  // test whether rpm is too high for the mode index
  bool testRpmLimit(int index, float rpm) {
    return(rpm > getRpmLimit(index));
  }

  // destructor (deallocate ms_modes)
  virtual ~StepperDriver(){
     delete[] ms_modes;
  }

};

// motor
struct StepperMotor {
  const int steps; // numer of steps / rotation
  const double gearing; // the gearing (if any, otherwise should be 1)
  StepperMotor(int steps, double gearing) :
    steps(steps), gearing(gearing) {};

};

/****** pre-configured options **********/

StepperBoard PHOTON_STEPPER_BOARD (
  /* dir */         D2,
  /* step */        D3,
  /* enable */      D7,
  /* ms1 */         D6,
  /* ms2 */         D5,
  /* ms3 */         D4,
  /* max steps/s */ 1000.0
);

// microstep modes of the DRV8825 chip
const int DRV8825_MICROSTEP_MODES_N = 6;
MicrostepMode DRV8825_MICROSTEP_MODES[DRV8825_MICROSTEP_MODES_N] =
  {
    /* n_steps, MS1, MS2, MS3 */
    {1,  LOW,  LOW,  LOW},  // full step
    {2,  HIGH, LOW,  LOW},  // half step
    {4,  LOW,  HIGH, LOW},  // quarter step
    {8,  HIGH, HIGH, LOW},  // eight step
    {16, LOW,  LOW,  HIGH}, // sixteenth step
    {32, HIGH, LOW,  HIGH},  // 1/32 step
  };

StepperDriver DRV8825(
  /* dir cw */      LOW,
  /* step on */     HIGH,
  /* enable on */   HIGH,
  /* modes */       DRV8825_MICROSTEP_MODES,
  /* modes_n */     DRV8825_MICROSTEP_MODES_N
);

// watson marlow pumphead
StepperMotor WM114ST (
  /* steps */      200,  // 200 steps/rotation
  /* gearing */      1
);
