
global_state = {}

def update(parent, name, value):
    if parent not in global_state:
        global_state[parent] = {}
    global_state[parent][name] = value
