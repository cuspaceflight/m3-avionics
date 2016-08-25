from multiprocessing import Manager

state_manager = Manager()
global_state = state_manager.dict()

def update(parent, name, value):
    if parent not in global_state:
        global_state[parent] = {}
    global_state[parent][name] = value
