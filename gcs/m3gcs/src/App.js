import React, { Component } from 'react';
import 'bootstrap/dist/css/bootstrap.css';
import './App.css';

import GCS from './m3gcs.js';

function padLeft(num, wid, char, base=10) {
  return num.toString(base).padStart(wid, char);
}

class Timer extends Component {
  render() {
    var seconds = this.props.seconds % 60;
    var minutes = Math.floor((this.props.seconds % 3600) / 60);
    var hours = Math.floor(this.props.seconds / 3600);
    function pad(x) { return padLeft(x, 2, '0'); }

    var hourselem = [];
    if (hours > 0) {
      hourselem = hours + ":";
    }
    return (
      <span className="timer-container">
        {hourselem}
        {pad(minutes)}
        {":"}
        {pad(seconds)}
      </span>
    );
  }
}

class Label extends Component {
  render() {
    var extraclasses = this.props.className || "";
    return (
      <span className={"label-text " + extraclasses}>
      	{this.props.text}
      </span>
    );
  }
}

class StatusLabel extends Component {
  render() {
    const statusmap = {0: "green", 1: "orange", 2: "red"};
    return (
      <div className={"status-label"} style={{"backgroundColor": statusmap[this.props.state]}}>
        {this.props.text}
      </div>
    );
  }
}

class StatusLabels extends Component {
  render() {
    //TODO reasons
    var labels = [];
    var components = this.props.status.components.get();
    for(var cname in components){
      var c = components[cname].get();
      labels.push(<StatusLabel key={cname} state={c.state.get()} text={cname} />);
    }
    return (
      <div className="status-container">
        {labels}
      </div>
    );
  }
}

class Subsystem extends Component {
  render() {
    var label = this.props.label || "";
    var version = this.props.version || "";
    return (
      <div className="card">
        <div className="card-header">
          {label}
        </div>
        <div className="card-block">
          <div className="subsystem-container container-fluid">
          	{this.props.children}
          </div>
        </div>
        <div className="card-footer">
          {version}
        </div>
      </div>
    );
  }
}

class Row extends Component {
  render() {
    var height = +this.props.height || 1;
    return (
      <div className="row" style={{"height": (50*height)+"px"}}>
        {this.props.children}
      </div>
    );
  }
}

class Data extends Component {
  render() {
    var width = +this.props.width || 4;
    var extraclasses = this.props.className || "";
    var backcolor = this.props.color || "";
    var lastupdated = this.props.updateTime || 0;
    var updatetext = ((new Date() - lastupdated)/1000).toFixed(1) + "s";
    if (lastupdated === 0){
      backcolor = "gray";
      updatetext = "-";
    }
    return (
      <div className={"data-container col-md-" + width + " " + extraclasses} style={{"backgroundColor": backcolor}}>
        <div className="data-lastupdate">
          {updatetext}
        </div>
        <div className="data-title">
          {this.props.label}
        </div>
        <div className="data-body">
          {this.props.children}
        </div>
      </div>
    );
  }
}

class ChargerStatus extends Component {
  render() {
    var data = this.props.data.get();
    var enabled = data.enabled.get();
    var charging = data.charging.get();
    var inhibit = data.inhibit.get();
    var battleshort = data.battleshort.get();
    var acfet = data.acfet.get();
    var voltage_mode = data.voltage_mode.get();
    var temperature = data.temperature.get().toFixed(1);
    var current = data.current.get();

    var state_label = <StatusLabel state={inhibit ? 1 : 0} text={charging ? "C" : (inhibit ? "I" : "D")} />;
    var bs_label = <StatusLabel state={battleshort ? 0 : 2} text={battleshort ? "WAR" : "PEACE"} />;
    var current_label = <StatusLabel state={current < 0 ? 0 : 1} text={current + "mA"} />;
    var temp_label = <StatusLabel state={temperature > 60 ? 2 : (temperature < 40 ? 0 : 1)} text={temperature + "°C"} />;
    if (enabled) {
      return (
        <Data width={8} label="Charger" updateTime={this.props.data.updateTime()}>
          {state_label}
          {bs_label}
          {current_label}
          {temp_label}
        </Data>
      );
    } else {
      return (
        <Data width={8} label="Charger" updateTime={this.props.data.updateTime()}>
          <Label text="Disabled" />
        </Data>
      );
    }
  }
}

class M3PSU extends Component {
  render() {
    var data = this.props.state;

    var channels = [];
    const cnames = ["CAN 5V", "CAMERAS", "IMU 5V", "RADIO 5V", "IMU 3V", "RADIO 3V", "FC", "PYRO", "DL", "BASE", "SPARE1", "SPARE2"];
    for(var idx=0; idx<12; idx++){
      var chan = data.channels.get()[idx].get();
      var v = chan.v.get();
      var i = chan.i.get();
      var p = chan.p.get();
      var labeltext = v.toFixed(1) + "V " + Math.floor(1000 * i) + "mA " + p.toFixed(1) + "W";
      channels.push(
        <Data key={idx} label={cnames[idx]} color={v < 3.2 ? "red" : "green"} updateTime={data.channels.get()[idx].updateTime()}>
          <Label className="smalltext" text={labeltext} />
        </Data>
      );
    }
    var pyro = data.pyro.get();
    var pyro_v = pyro.v.get();
    var pyro_i = pyro.i.get();
    var pyro_p = pyro.p.get();
    var pyro_enabled = pyro.enabled.get();
    const statusmap = {"OK": "green", "INIT": "orange", "ERROR": "red"};
    return (
      <Subsystem label="M3PSU" version={data.version.get()}>
        <Row>
          <Data width={8} label="Status" color={statusmap[data.status.get().overall.get()]} updateTime={data.status.updateTime()}>
            <StatusLabels status={data.status.get()} />
          </Data>
          <Data label="Runtime" updateTime={data.runtime.updateTime()}>
            <Timer seconds={data.runtime.get()*60} />
          </Data>
        </Row>
        <Row>
          <ChargerStatus data={data.charger} />
          <Data label="Awake Time" updateTime={data.awake_time.updateTime()}>
            <Timer seconds={data.awake_time.get()} />
          </Data>
        </Row>
        <Row>
          <Data label="Cell 1" updateTime={data.cells.get()[0].updateTime()}>
            <Label text={data.cells.get()[0].get().toFixed(2) + "V"} />
          </Data>
          <Data label="Cell 2" updateTime={data.cells.get()[1].updateTime()}>
            <Label text={data.cells.get()[1].get().toFixed(2) + "V"} />
          </Data>
          <Data label="Batt" updateTime={data.batt_voltage.updateTime()}>
            <Label text={data.batt_voltage.get().toFixed(2) + "V"} />
          </Data>
        </Row>
        <Row>
          <Data label="Pyro" color={!pyro_enabled ? "orange" : (pyro_v < 6 ? "red" : "green")} updateTime={data.pyro.updateTime()}>
            <Label className="smalltext" text={pyro_v.toFixed(1) + "V " + pyro_i.toFixed(1) + "A " + pyro_p.toFixed(1) + "W"} />
          </Data>
          <Data label="Power Mode" color={data.power_mode.get()==="high" ? "green" : "orange"} updateTime={data.power_mode.updateTime()}>
            <Label text={data.power_mode.get()==="high" ? "NORMAL" : "LOW POWER"} />
          </Data>
          <Data label="Percent" updateTime={data.percent.updateTime()}>
            <Label text={data.percent.get() + "%"} />
          </Data>
        </Row>
        <Row>
          {channels.slice(0,3)}
        </Row>
        <Row>
          {channels.slice(3,6)}
        </Row>
        <Row>
          {channels.slice(6,9)}
        </Row>
        <Row>
          {channels.slice(9,12)}
        </Row>
      </Subsystem>
    );
  }
}

class App extends Component {
  constructor(props){
    super(props);
    const _this = this;

    this.gcs = new GCS();

    this.state = {
      "m3psu": this.gcs.m3psu,
    };

    this.timer = setInterval(() => {
      _this.setState({
        "m3psu": _this.gcs.m3psu,
      });
    }, 100);
  }

  render() {
    return (
      <div className="App">
        <div className="container-fluid">
          <div className="row">
            <div className="col-lg-4">
              <M3PSU state={this.state.m3psu} />
            </div>
          </div>
        </div>
      </div>
    );
  }
}

export default App;
