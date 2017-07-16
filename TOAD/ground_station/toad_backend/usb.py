"""
Handle incoming serial data (logging and parsing).

Attributes:

Todo:
    - Everything
    - Log serial data
"""
import serial
from multiprocessing import Pipe

class Toad_Packet:
"""Decoded data packet from TOAD network.

Attributes:

"""
    def __init__(self):


def run(parsed_data_out):
    """
    Args:

    Returns:

    Raises:
    """
    while True:
        #serial read
        #parse data into an object to send to the state estimation process
        #log raw serial data to file
