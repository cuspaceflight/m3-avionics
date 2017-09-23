import React, { Component } from 'react';

import {Timer, Label, StatusLabel, StatusLabels, Subsystem, Row, Data, Button, DropdownButton} from './components.js'

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
        <Row>
          <Button text="Test" onClick={() => { alert("hello world!") }} />
          <DropdownButton text="CH 1" options={[
            {text: "on", onClick: () => { alert("ch1 on") }},
            {text: "off", onClick: () => { alert("ch1 off") }},
          ]} />
        </Row>
      </Subsystem>
    );
  }
}

export default M3PSU;

