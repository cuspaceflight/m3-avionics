"""Log data to hard drive.

Attributes:
    #filenames? and path to log files

Todo:
    Everything
"""
import os

script_dir = os.path.dirname(__file__)


#def log_serial():

#def log_state():

def run(usb_pipe, trilat_pipe, gui_exit, log_dir):
    trilat_txt_filepath = os.path.abspath(os.path.join(script_dir,log_dir,"trilat_txt.txt"))
    trilat_json_filepath = os.path.abspath(os.path.join(script_dir,log_dir,"trilat_json.txt"))
    with open(trilat_txt_filepath, 'a+') as trilat_f_txt, open(trilat_json_filepath, 'a+') as trilat_f_json:
        while not gui_exit.is_set():
            ## Main loop
            ## TODO: receive from usb pipe
            if trilat_pipe.poll(0.01):
                new_packet_trilat = trilat_pipe.recv()
                new_packet_trilat.print_to_file(trilat_f_txt)
                new_packet_trilat.print_to_js(trilat_f_json)
    trilat_f_txt.closed
    trilat_f_json.closed
