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

#define MANUAL_DEBOUNCE 50              // the debounce time for analog input in ms
#define MANUAL_STATUS_UPDATE_DELAY 2000 // how long to wait for status update after manual adjustments [ms]


// actual pump controller class
class PumpController {

  private:

    AccelStepper stepper; // stepper
    const StepperPins pins; // pins
    const int ms_modes_n; // number of microstepping modes
    MicrostepMode* ms_modes; // microstepping modes
    const PumpSettings settings; // settings
    bool delay_status_update = false;
    long update_status_time = 0; // set specific time for delayed status updates
    long last_manual_read = 0; // last manual speed read time
    float last_manual_rpm = -1; // last manual speed read

    // internal functions
    void extractCommandParam(char* param);
    void assignCommandMessage();
    void updateStatus();
    void updateStatus(int status);
    float calculateSpeed();
    int findMicrostepIndexForRpm(float rpm); // finds the correct ms index for the requested rpm (takes ms_auto into consideration)
    bool setSpeedWithSteppingLimit(float rpm); // sets state.speed and returns true if request set, false if had to set to limit

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

    void setStatusUpdateDelay(long delay); // set a status update delay [in ms], call before making any status updates
    void setAutoMicrosteppingMode(); // set to automatic icrostepping mode
    bool setMicrosteppingMode(int ms_mode); // set microstepping by mode, return false if can't find requested mode
    bool setSpeedRpm(float rpm); // return false if had to limit speed, true if taking speed directly
    float getMaxRpm(); // returns the maximum rpm for the pump
    void setDirection(int direction); // set direction

    void lock(); // lock pump
    void unlock(); // unlock pump

    void manual(); // turn into manual mode
    void start(); // start the pump
    void stop(); // stop the pump
    void hold(); // hold position
    long rotate(double number); // returns the number of steps the motor will take

    int parseCommand (String command); // parse a cloud command

  // pump command callback
  private: PumpCommandCallbackType command_callback;

};
