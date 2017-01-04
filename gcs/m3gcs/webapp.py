from flask import Flask, request, render_template
from flask_socketio import SocketIO
from threading import Thread
from time import sleep
from queue import Empty

from . import command_processor
from .packets import registered_packets, registered_commands

app = Flask(__name__, static_url_path="/static")
socketio = SocketIO(app)

wa_queue = None

@app.route("/")
def index():
    names = set(registered_packets.keys())
    names = names.union(set(registered_commands.keys()))
    return render_template("gcs.html", names=names, commands=registered_commands, packets=registered_packets)

@app.route("/command", methods=["POST"])
def sendcommand():
    command_processor.process(request.form['parent'], request.form['name'], request.form['arg'])
    return ""

def handle_packets():
    while True:
        try:
            frame = wa_queue.get_nowait()
            socketio.emit('packet', {'sid': frame.sid, 'data': frame.data})
        except Empty:
            sleep(0.01)
            pass

def run(queue):
    global wa_queue
    wa_queue = queue

    queue_handler = Thread(target=handle_packets)
    queue_handler.daemon = True
    queue_handler.start()

    socketio.run(app)
