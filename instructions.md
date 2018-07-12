# Adjusting driver current limit

 - it is crucial to adjust the current limit properly to always be < the rated current/phase of the stepper motor (if too high then can damage the motor and driver, if too low won't get full torque and get the annoying scretching). The full step mode will only use 70% of this max current the driver allows but all microstepping modes will energize up to the set current limit.
 - there is a good standard tutorial on how to adjust this available here (the youtube video is extremely helpful): https://www.pololu.com/product/2133
 - in theory for the DRV8825, the reference voltage measured between logic ground and the trim pot (Vref) x 2 = resulting current limit (so e.g. for limiting current to 1A, ref in theory should be set to 0.5V)
 - in practice, however, this can vary quite a bit (maybe quality of resistors on DRV8825 board? or maybe the ref. voltage supplied by the particle photons), and it is highly advisable to actually check the current per phase once the trim pot is set (and then adjust as needed)
 - this is done most easily by adjusting the trim pot (power to the board but not running the stepper!) to the theoretical 1/2 of the max rated current/phase of the stepper, then putting the multimeter in series with one of the coils of the stepper and starting the stepper at a very low speed so you can easily watch it step the current through this coil up and down and figure out the maximum current it is running at
 - for example, for a stepper online stepper (Nema 17 planetary geared stepper) with 1.8 step angle and 5.18 gear ratio, max current per phase of 1.68A the following Vrefs resulted in the following max currents in full step and microstepping modes:
  - 406mV --> 370mA in full step,  620mA in microstepping
  - 564mV --> 540mA in full step,  860mA in microstepping
  - 696mV --> 660mA in full step, 1070mA in microstepping
  - 893mV --> 860mA in full step, 1390mA in microstepping
  - This is actually an almost perfect linearity of 1.6 instead of 2
 - NOTE: the trim pot voltage is most easily adjusted with a voltmeter attached to the screwdriver (easer with flat head than with cross)

# For Watson Marlow stepper motor pump head

  - adjust to <= 1200mA in microstepping mode (Vref ~ 700mV)

  
