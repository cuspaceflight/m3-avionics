import React, { Component } from 'react';

function padLeft(num, wid, char, base=10) {
  return num.toString(base).padStart(wid, char);
}

export class Timer extends Component {
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

export class Label extends Component {
  render() {
    var extraclasses = this.props.className || "";
    return (
      <span className={"label-text " + extraclasses}>
      	{this.props.text}
      </span>
    );
  }
}

export class StatusLabel extends Component {
  render() {
    const statusmap = {0: "green", 1: "orange", 2: "red"};
    return (
      <div className={"status-label"} style={{"backgroundColor": statusmap[this.props.state]}}>
        {this.props.text}
      </div>
    );
  }
}

export class StatusLabels extends Component {
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

export class Subsystem extends Component {
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

export class Row extends Component {
  render() {
    var height = +this.props.height || 1;
    return (
      <div className="row" style={{"height": (50*height)+"px"}}>
        {this.props.children}
      </div>
    );
  }
}

export class Data extends Component {
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

export class Button extends Component {
  render() {
    return (
      <div className="button-container col-lg-2">
        <button className="btn btn-default command-button" onClick={this.props.onClick}>
          {this.props.text}
        </button>
      </div>
    );
  }
}

export class DropdownButton extends Component {
  render() {
    var optionslist = [];
    for (var idx in this.props.options) {
      var o = this.props.options[idx];
      optionslist.push(
        <li className="dropdown-item" onClick={o.onClick}>
          {o.text}
        </li>
      );
    }
    return (
      <div className="dropdown-container col-lg-2">
        <div className="dropdown">
          <button type="button" className="btn btn-default command-button dropdown-toggle" data-toggle="dropdown">{this.props.text}</button>
          <div className="dropdown-menu">
            {optionslist}
          </div>
        </div>
      </div>
    );
  }
}

