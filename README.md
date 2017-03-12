# labware_stepper_pump

## wiring diagram

 - TODO

## particle photon setup

 - setup the [Particle Photon](https://store.particle.io/collections/photon) microcontroller (via Wifi and cell phone or via computer and command line interface), instructions in the [Particle Docs](https://docs.particle.io/guide/getting-started/start/photon/)
 - clone this repository to preferred location on your local harddrive
 - easiest use of this repository:
  - install the [Particle Interactive Development Environment](https://www.particle.io/products/development-tools/particle-desktop-ide)
  - load the cloned repository folder in the  Particle IDE
  - adjust the parameters in `pump.ino` that define the used pins and characteristics of the stepper (e.g. steps, etc.)
  - login into your particle account in the Particle IDE
  - select the Photon flash the code (lightning bolt symbol) over WIFI
  - that's it, if wired correctly you should now have full access to your pump's functionality from your local command line anywhere via the particle cloud
 - alternatively, use [build.particle.io](http://build.particle.io)
  - log in to the Particle web interface and create a new project
  - add the `AccelStepper` (and if used the `LiquidCrsytal_I2C`) libraries to your project from the online library tab
  - add the remaining `.h` and `.cpp` files in the repository to your project and upload the `.ino` file's content to the main project file
  - select the target Particle Photon and flash the program

## web commands

To run these web commands, you need to either have the [Particle Cloud command line interface (CLI)](https://github.com/spark/particle-cli) installed, or format the appropriate POST request to the [Particle Cloud API](https://docs.particle.io/reference/api/). Here only the currently implemented CLI calls are listed but they translate directly into the corresponding API requests.

All calls are issued from the terminal and start with `particle call <deviceID>` where `<deviceID>` is the name of the photon you want to issue a command to. If the command was successfully received and executed `0` is returned, if the command was received but could not be interpreted, `-1` is the return value. Other return values can mean executed with warning or provide additional information. You can change all of the following command's exact wording and all the return codes in `PumpCommands.h` if you want them to be different. Make sure to be logged in (`particle login`) to have access to your photons.

  - `particle call pump "start"` to start the pump (at the currently set speed and microstepping)
  - `... pump "stop"` to stop the pump and disengage it (no holding torque applied)
  - `... pump "hold"` to stop the pump but hold the position (maximum holding torque)
  - `... pump "rotate <x>"` to have the pump do `<x>` rotations and then execute a `stop` commands
  - `... pump "ms <x>"` to set the microstepping mode to `<x>` (1= full step, 2 = half step, 4 = quarter step, etc.)
  - `... pump "ms auto"` to set the microstepping mode to automatic in which case the lowest step mode that the current speed allows will be automatically set
  - `... pump "speed <x> rpm"` to set the pump speed to `<x>` rotations per minute (if the pump is currently running, it will change the speed to this and keep running). if microstepping mode is in `auto` it will automatically select the appropriate microstepping mode for the selected speed. If the microstepping mode is fixed and the requested rpm exceeds the maximally possible speed for the selected mode (or if in `auto` mode, the requested rpm exceeds the fastest possible on full step mode), the maximum speed will automatically be set instead and a warning return code will be issued.
  - `... pump "direction cc"` to set the direction to counter clockwise
  - `... pump "direction cw"` to set the direction to clockwise
  - `... pump "direction switch"` to reverse the direction (note that any direction changes stops the pump if it is in `rotate <x>` mode)
  - `... pump "lock"` to lock the pump (i.e. no commands will be accepted until `unlock` is called)
  - `... pump "unlock"` to unlock the pump if it is locked
  - to be continued (more commands in progress)...
