registered_packets = {}
registered_commands = {}


def register_packet(parent, msg_id, name):
    def wrap(f):
        if parent not in registered_packets:
            registered_packets[parent] = {}
        registered_packets[parent][msg_id] = (name, f)
        return f
    return wrap


def register_command(parent, command_name, options):
    def wrap(f):
        if parent not in registered_commands:
            registered_commands[parent] = {}
        registered_commands[parent][command_name] = (f, options)
        return f
    return wrap
