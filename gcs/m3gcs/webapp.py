from flask import Flask, jsonify, request, render_template

from . import state, command
from .packets import registered_packets, registered_commands

app = Flask(__name__, static_url_path="")


@app.route("/")
def index():
    names = set(registered_packets.values())
    names += set(registered_commands.values())
    return render_template("gcs.html")


@app.route("/state")
def getstate():
    return jsonify(state.global_state)


@app.route("/command")
def sendcommand():
    command.process(request.getjson())
