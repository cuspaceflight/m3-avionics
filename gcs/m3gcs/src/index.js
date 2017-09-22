import React from 'react';
import ReactDOM from 'react-dom';
import './index.css';
import App from './App';

import GCS from './m3gcs.js';

var app = <App />;

ReactDOM.render(app, document.getElementById('root'));

var gcs = new GCS();
