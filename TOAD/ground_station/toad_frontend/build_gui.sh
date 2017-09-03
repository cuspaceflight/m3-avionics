#!/bin/bash
# Build TOAD gui from QT4 Designer ui files
# Gregory Brooks August 2017

# Create basic toad_frame class
pyuic4 -w ui/toad_frame.ui -o python/toad_frame.py

# Create frames for each toad unit
pyuic4 -w ui/toad_frame_master.ui -o python/toad_frame_master.py
pyuic4 -w ui/toad_frame_1.ui -o python/toad_frame_1.py
pyuic4 -w ui/toad_frame_2.ui -o python/toad_frame_2.py
pyuic4 -w ui/toad_frame_3.ui -o python/toad_frame_3.py
pyuic4 -w ui/toad_frame_4.ui -o python/toad_frame_4.py
pyuic4 -w ui/toad_frame_5.ui -o python/toad_frame_5.py


# Build main window

# executable for testing
pyuic4 -x ui/toad_gui.ui -o python/toad_gui.py
