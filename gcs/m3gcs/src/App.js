import React, { Component } from 'react';

import 'bootstrap/dist/js/bootstrap.js';

import 'bootstrap/dist/css/bootstrap.css';
import './App.css';

import GCS from './m3gcs.js';

import M3PSU from './M3PSU.js';

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
