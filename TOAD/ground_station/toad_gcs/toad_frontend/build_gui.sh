#!/bin/bash
# Build TOAD gui from QT4 Designer ui files
# Gregory Brooks August 2017

parent_path=$( cd "$(dirname "${BASH_SOURCE[0]}")"  ; pwd -P )
cd "$parent_path"

# Create basic toad_frame class
pyuic4 -w ui/toad_frame.ui -o toad_frame.py

# Create frames for each toad unit
pyuic4 -w ui/toad_frame_master.ui -o toad_frame_master.py
pyuic4 -w ui/toad_frame_1.ui -o toad_frame_1.py
pyuic4 -w ui/toad_frame_2.ui -o toad_frame_2.py
pyuic4 -w ui/toad_frame_3.ui -o toad_frame_3.py
pyuic4 -w ui/toad_frame_4.ui -o toad_frame_4.py
pyuic4 -w ui/toad_frame_5.ui -o toad_frame_5.py
pyuic4 -w ui/toad_frame_6.ui -o toad_frame_6.py

# Build main window


pyuic4 ui/toad_gui.ui -o toad_gui.py
