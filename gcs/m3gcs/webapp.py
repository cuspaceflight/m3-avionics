from flask import Flask, jsonify, request, render_template

from . import state, command_processor
from .packets import registered_packets, registered_commands

app = Flask(__name__, static_url_path="/static")


@app.route("/")
def index():
    names = set(registered_packets.keys())
    names = names.union(set(registered_commands.keys()))
    return render_template("gcs.html", names=names, commands=registered_commands, packets=registered_packets)


@app.route("/state")
def getstate():
    return jsonify(state.global_state)


@app.route("/command", methods=["POST"])
def sendcommand():
    command_processor.process(request.form['parent'], request.form['name'], request.form['arg'])
    return ""
