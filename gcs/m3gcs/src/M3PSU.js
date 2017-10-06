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
    var m3psu = this.props.m3psu;
    const sendCommand = this.props.sendCommand;

    var channels = [];
    const cnames = ["CAN 5V", "CAMERAS", "IMU 5V", "RADIO 5V", "IMU 3V", "RADIO 3V", "FC", "PYRO", "DL", "BASE", "SPARE1", "SPARE2"];
    for(var idx=0; idx<12; idx++){
      var chan = m3psu.channels.get()[idx].get();
      var v = chan.v.get();
      var i = chan.i.get();
      var p = chan.p.get();
      var labeltext = v.toFixed(1) + "V " + Math.floor(1000 * i) + "mA " + p.toFixed(1) + "W";
      channels.push(
        <Data key={idx} label={cnames[idx]} color={v < 3.2 ? "red" : "green"} updateTime={m3psu.channels.get()[idx].updateTime()}>
          <Label className="smalltext" text={labeltext} />
        </Data>
      );
    }
    var pyro = m3psu.pyro.get();
    var pyro_v = pyro.v.get();
    var pyro_i = pyro.i.get();
    var pyro_p = pyro.p.get();
    var pyro_enabled = pyro.enabled.get();
    const statusmap = {"OK": "green", "INIT": "orange", "ERROR": "red"};

    var buttons = [];
    for(idx=0; idx<12; idx++){
      ((channel) => {
        buttons.push(<DropdownButton text={cnames[channel]} options={[
          {text: "On", onClick: () => { m3psu.channelOn(channel) }},
          {text: "Off", onClick: () => { m3psu.channelOff(channel) }},
        ]} />);
      })(idx);
    }

    return (
      <Subsystem label="M3PSU" version={m3psu.version.get()}>
        <Row>
          <Data width={8} label="Status" color={statusmap[m3psu.status.get().overall.get()]} updateTime={m3psu.status.updateTime()}>
            <StatusLabels status={m3psu.status.get()} />
          </Data>
          <Data label="Runtime" updateTime={m3psu.runtime.updateTime()}>
            <Timer seconds={m3psu.runtime.get()*60} />
          </Data>
        </Row>
        <Row>
          <ChargerStatus data={m3psu.charger} />
          <Data label="Awake Time" updateTime={m3psu.awake_time.updateTime()}>
            <Timer seconds={m3psu.awake_time.get()} />
          </Data>
        </Row>
        <Row>
          <Data label="Cell 1" updateTime={m3psu.cells.get()[0].updateTime()}>
            <Label text={m3psu.cells.get()[0].get().toFixed(2) + "V"} />
          </Data>
          <Data label="Cell 2" updateTime={m3psu.cells.get()[1].updateTime()}>
            <Label text={m3psu.cells.get()[1].get().toFixed(2) + "V"} />
          </Data>
          <Data label="Batt" updateTime={m3psu.batt_voltage.updateTime()}>
            <Label text={m3psu.batt_voltage.get().toFixed(2) + "V"} />
          </Data>
        </Row>
        <Row>
          <Data label="Pyro" color={!pyro_enabled ? "orange" : (pyro_v < 6 ? "red" : "green")} updateTime={m3psu.pyro.updateTime()}>
            <Label className="smalltext" text={pyro_v.toFixed(1) + "V " + pyro_i.toFixed(1) + "A " + pyro_p.toFixed(1) + "W"} />
          </Data>
          <Data label="Power Mode" color={m3psu.power_mode.get()==="high" ? "green" : "orange"} updateTime={m3psu.power_mode.updateTime()}>
            <Label text={m3psu.power_mode.get()==="high" ? "NORMAL" : "LOW POWER"} />
          </Data>
          <Data label="Percent" updateTime={m3psu.percent.updateTime()}>
            <Label text={m3psu.percent.get() + "%"} />
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
          <DropdownButton text="Pyro Supply" options={[
            {text: "On", onClick: () => { m3psu.pyroEnable(true) }},
            {text: "Off", onClick: () => { m3psu.pyroEnable(false) }},
          ]} />
          <DropdownButton text="Charger" options={[
            {text: "On", onClick: () => { m3psu.chargerEnable(true) }},
            {text: "Off", onClick: () => { m3psu.chargerEnable(false) }},
          ]} />
          <DropdownButton text="Lowpower" options={[
            {text: "On", onClick: () => { m3psu.lowpowerEnable(true) }},
            {text: "Off", onClick: () => { m3psu.lowpowerEnable(false) }},
          ]} />
          <DropdownButton text="Battleshort" options={[
            {text: "Peace", onClick: () => { m3psu.battleshortEnable(false) }},
            {text: "WAR", onClick: () => { m3psu.battleshortEnable(true) }},
          ]} />
        </Row>
        <Row>
          {buttons.slice(0,6)}
        </Row>
        <Row>
          {buttons.slice(6,12)}
        </Row>
      </Subsystem>
    );
  }
}

export default M3PSU;

