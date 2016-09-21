from flask import Flask, jsonify, request, render_template

from . import command_processor
from .packets import registered_packets, registered_commands

app = Flask(__name__, static_url_path="/static")

wa_state = None

@app.route("/")
def index():
    names = set(registered_packets.keys())
    names = names.union(set(registered_commands.keys()))
    return render_template("gcs.html", names=names, commands=registered_commands, packets=registered_packets)

@app.route("/state")
def getstate():
    state = {}
    lasttimes = {}
    for k in wa_state.keys():
        s = wa_state.get(k)
        state[k] = s['data']
        lasttimes[k] = s['time']
    return jsonify({"state":state, "lasttimes":lasttimes})


@app.route("/command", methods=["POST"])
def sendcommand():
    command_processor.process(request.form['parent'], request.form['name'], request.form['arg'])
    return ""


def run(state):
    global wa_state
    wa_state = state
    app.run()
