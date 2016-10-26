# M3FC Testing

## Usage

```bash
$ make
$ python3 m3fc_mission.py
^Z
$ kill -9 %1
```

## Current Status
* We can build `libm3fc_mission.so`, a dynamic library which contains all the
  code in `m3fc_mission.c` plus mock functions for everything it has to call.
* We can link against this inside Python, using `m3fc_mission.py`, and call
  the `m3fc_mission_init` function, starting the mission state machine code.
* As an example, we can bind a Python variable to the
  `m3fc_state_estimation_trust_baro` global, so we can always read if the
  barometer is being trusted
* We can write to a function pointer in the mock library such that Python
  functions can be called by the mock functions. This is probably the
  best way to get most data (e.g. accelerations, CAN messages) in/out of
  the C code

## To Do
* Implement most of the mock functions:
    * maintain some idea of what the current "time" is (probably via a global) which we increment during `chThdSleepMilliseconds` and return from `chVTGetSystemTimeX`. Python can additionally read this new global so it can always see what the current system time is.
    * hook `m3fc_state_estimation_get_state` into Python so we can provide the 
      current state to the C code
    * hook `m3fc_config` into Python so we can give the C code the flight 
      config
    * record `m3status` calls so we can see when the m3fc status changes
* Python side:
    * Each call to one of the Python callbacks is an opportunity to see what
    state the system is in, so keep a record of the current state of everything 
    against the time for later analysis
    * Provide some way to feed in the height/vel/accel for a given time
    * Right now the mission main loop will run forever, so the Python script 
      can't be quit (use ctrl-z to background and then `kill -9 %1` to 
      terminate). Could improve this by:
      1. Easy option: modify `while(true)` in `m3fc_mission.c` so it depends on 
         a global that is true in the firmware but Python can set to false at 
         some point, causing the loop to end.
         This works but requires modifying the main firmware, and introduces 
         the scary possibility that this variable gets changed during flight 
         and quits the main loop.
      2. In Python, run the mission code in a separate process using the
         multiprocessing module, so we can then kill it from the main thread.
