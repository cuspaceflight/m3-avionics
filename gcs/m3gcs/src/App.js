import React, { Component } from 'react';
import 'bootstrap/dist/css/bootstrap.css';
import './App.css';

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
    for(var component in this.props.status.components){
      var c = this.props.status.components[component];
      labels.push(<StatusLabel key={component} state={c.state} text={component} />);
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
    return (
      <div className={"data-container col-md-" + width + " " + extraclasses} style={{"backgroundColor": backcolor}}>
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
    var data = this.props.data;
    var enabled = !!data.enabled;
    var charging = !!data.charging;
    var inhibit = !!data.inhibit;
    var battleshort = !!data.battleshort;
    var acfet = !!data.acfet;
    var voltage_mode = data.voltage_mode || "UKN";
    var temperature = data.temperature ? data.temperature.toFixed(1) : 0.0;
    var current = data.current || 0;

    var state_label = <StatusLabel state={inhibit ? 1 : 0} text={charging ? "C" : (inhibit ? "I" : "D")} />;
    var bs_label = <StatusLabel state={battleshort ? 0 : 2} text={battleshort ? "WAR" : "PEACE"} />;
    var current_label = <StatusLabel state={current < 0 ? 0 : 1} text={current + "mA"} />;
    var temp_label = <StatusLabel state={temperature > 60 ? 2 : (temperature < 40 ? 0 : 1)} text={temperature + "°C"} />;
    if (enabled) {
      return (
        <Data width={8} label="Charger">
          {state_label}
          {bs_label}
          {current_label}
          {temp_label}
        </Data>
      );
    } else {
      return (
        <Data width={8} label="Charger">
          <Label text="Disabled" />
        </Data>
      );
    }
  }
}

class M3PSU extends Component {
  render() {
    var data = this.props.state;
    var status = data.status || {};
    var c = [];
    for(var i=0; i<12; i++){
      c[i] = {
        v: data.channels[i] ? data.channels[i].v.toFixed(1) : 0.0,
        i: data.channels[i] ? Math.floor(1000 * data.channels[i].i) : 0.0,
        p: data.channels[i] ? data.channels[i].p.toFixed(1) : 0.0,
      };
    }
    var charger = data.charger || {};
    var runtime = data.runtime || 0;
    var percent = data.percent || 0;
    var cells = data.cells || [0.0, 0.0];
    var batt_voltage = data.batt_voltage || 0.0;
    var pyro = {
      v: data.pyro ? data.pyro.v.toFixed(1) : 0.0,
      i: data.pyro ? data.pyro.i.toFixed(1) : 0.0,
      p: data.pyro ? data.pyro.p.toFixed(1) : 0.0,
      enabled: data.pyro ? !!data.pyro.enabled : false,
    };
    var awake_time = data.awake_time || 0;
    var power_mode = data.power_mode || "UKN";

    var channels = [];
    const cnames = ["CAN 5V", "CAMERAS", "IMU 5V", "RADIO 5V", "IMU 3V", "RADIO 3V", "FC", "PYRO", "DL", "BASE", "SPARE1", "SPARE2"];
    for(i=0; i<c.length; i++){
      channels.push(
        <Data key={i} label={cnames[i]} color={c[i].v < 3.2 ? "red" : "green"}>
          <Label className="smalltext" text={c[i].v.toString() + "V " + c[i].i.toString() + "mA " + c[i].p.toString() + "W"} />
        </Data>
      );
    }
    const statusmap = {"OK": "green", "INIT": "orange", "ERROR": "red"};
    return (
      <Subsystem label="M3PSU">
        <Row>
          <Data width={8} label="Status" color={statusmap[status.overall]}>
            <StatusLabels status={status} />
          </Data>
          <Data label="Runtime">
            <Timer seconds={runtime*60} />
          </Data>
        </Row>
        <Row>
          <ChargerStatus data={charger} />
          <Data label="Awake Time">
            <Timer seconds={awake_time} />
          </Data>
        </Row>
        <Row>
          <Data label="Cell 1">
            <Label text={cells[0].toFixed(2) + "V"} />
          </Data>
          <Data label="Cell 2">
            <Label text={cells[1].toFixed(2) + "V"} />
          </Data>
          <Data label="Batt">
            <Label text={batt_voltage.toFixed(2) + "V"} />
          </Data>
        </Row>
        <Row>
          <Data label="Pyro" color={!pyro.enabled ? "orange" : (pyro.v < 6 ? "red" : "green")}>
            <Label className="smalltext" text={pyro.v.toString() + "V " + pyro.i.toString() + "A " + pyro.p.toString() + "W"} />
          </Data>
          <Data color={power_mode==="high" ? "green" : "orange"} label="Power Mode">
            <Label text={power_mode==="high" ? "NORMAL" : "LOW POWER"} />
          </Data>
          <Data label="Percent">
            <Label text={percent + "%"} />
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
  constructor(){
    super();
    this.state = {
      m3psu: JSON.parse('{"status":{"overall":"ERROR","components":{"DCDC1":{"state":0,"reason":"No Error"},"DCDC2":{"state":0,"reason":"No Error"},"DCDC3":{"state":2,"reason":"Ch1 Alert"},"Charger":{"state":2,"reason":"Read"},"Pyro Monitor":{"state":0,"reason":"No Error"}}},"channels":[{"v":0.15,"i":0,"p":0},{"v":5.0,"i":0.333,"p":1.555},{"v":3.27,"i":0.0084,"p":0.028},{"v":3.27,"i":0.0087,"p":0.028},{"v":4.9799999999999995,"i":0,"p":0},{"v":1.2,"i":0,"p":0},{"v":3.27,"i":0.0029999999999999996,"p":0.01},{"v":3.27,"i":0.0162,"p":0.052000000000000005},{"v":0.15,"i":0,"p":0},{"v":0.15,"i":0,"p":0},{"v":3.27,"i":0.017099999999999997,"p":0.056},{"v":4.9799999999999995,"i":0.006599999999999999,"p":0.03}],"charger":{"enabled":true,"charging":false,"inhibit":false,"battleshort":false,"acfet":false,"voltage_mode":"Med","temperature":22.69999999999999,"current":-484},"runtime":521,"percent":97,"cells":[3.64,3.64],"batt_voltage":7.28,"pyro":{"v":7.325,"i":0.002,"p":0.014,"enabled":true},"awake_time":180,"power_mode":"high"}'),
    };
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
