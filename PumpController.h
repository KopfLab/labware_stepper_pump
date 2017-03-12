/*
 * PumpController.h
 * Created on: February 17, 2017
 * Author: Sebastian Kopf <seb.kopf@gmail.com>
 * Pump controller
 TODO implement set acceleration
separate the mass/s aspects from the rest? probably not at the pump class level
 */

#pragma once
#include <AccelStepper.h>
#include "PumpSettings.h"
#include "PumpState.h"
#include "PumpCommands.h"

// actual pump controller class
class PumpController {

  private:

    AccelStepper stepper; // stepper
    const StepperPins pins; // pins
    const int ms_modes_n; // number of microstepping modes
    MicrostepMode* ms_modes; // microstepping modes
    const PumpSettings settings; // settings

    // internal functions
    void extractCommandParam(char* param);
    void assignCommandMessage();
    void updateStatus();
    void updateStatus(int status);
    float calculateSpeed();
    int findAutoMicrostepIndex(float rpm);
    int activateMicrostepping(int ms_index, float rpm);
    bool setSpeedWithSteppingLimit(int ms_index, float rpm); // sets state.speed and returns true if request set, false if had to set to limit

    void savePumpState(); // returns TRUE if pump state was updated, FALSE if it didn't need updating
    bool loadPumpState(); // returns TRUE if pump state loaded successfully, FALSE if it didn't match the required structure and default was loaded instead

  public:

    // state
    PumpState state;

    // command
    PumpCommand command;

    // callback
    typedef void (*PumpCommandCallbackType) (const PumpController&);

    // constructors
    PumpController (StepperPins pins, MicrostepMode* ms_modes, int ms_modes_n,
      PumpSettings settings, PumpState state, PumpCommandCallbackType callback) :
    pins(pins), ms_modes_n(ms_modes_n), settings(settings), state(state), command_callback(callback) {
      Particle.function(CMD_ROOT, &PumpController::parseCommand, this);
      // allocate microstep modes and calculate rpm limits for each
      this->ms_modes = new MicrostepMode[ms_modes_n];
      for (int i = 0; i < ms_modes_n; i++) {
        this->ms_modes[i] = ms_modes[i];
        this->ms_modes[i].rpm_limit = settings.max_speed * 60.0 / (settings.steps * settings.gearing * ms_modes[i].mode);
      }
    }

    // destructor (deallocate ms_modes)
    virtual ~PumpController(){
       delete[] ms_modes;
    }

    // methods
    void init(); // to be run during setup()
    void init(bool reset); // whether to reset pump state back to the default
    void update(); // to be run during loop()

    bool setMicrosteppingMode(int ms_mode); // set microstepping by mode, return false if can't find requested mode
    bool setSpeedRpm(float rpm); // return false if had to limit speed, true if taking speed directly
    void setDirection(int direction); // set direction

    void lock(); // lock pump
    void unlock(); // unlock pump

    void start(); // start the pump
    void stop(); // stop the pump
    void hold(); // hold position
    long rotate(double number); // returns the number of steps the motor will take

    int parseCommand (String command); // parse a cloud command

  // pump command callback
  private: PumpCommandCallbackType command_callback;

};
