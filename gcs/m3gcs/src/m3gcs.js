import M3PSU from './parsers/m3psu.js';
import M3DL from './parsers/m3dl.js';
import M3FC from './parsers/m3fc.js';
import M3Pyro from './parsers/m3pyro.js';
import M3Radio from './parsers/m3radio.js';
import M3Ground from './parsers/m3ground.js';
import M3IMU from './parsers/m3imu.js';
import JSPack from './jspack.js';

class GCS {
    constructor() {
        const _this = this;

        this.handlers = {};
        this.m3psu = new M3PSU(this);
        this.m3dl = new M3DL(this);
        this.m3fc = new M3FC(this);
        this.m3pyro = new M3Pyro(this);
        this.m3radio = new M3Radio(this);
        this.m3ground = new M3Ground(this);
        this.m3imu = new M3IMU(this);

        // e.g. gcs.struct.Unpack( .. )
        this.struct = new JSPack();

        this.ws = new WebSocket("ws://" + window.location.hostname + ":5000" + "/ws")
        this.ws.onmessage = function(event){
            var packet = JSON.parse(event.data);
            _this.handlePacket(packet);
        };
    };

    registerPacket(canID, handler){
        this.handlers[canID] = handler;
    };

    handlePacket(packet){
        var handler = this.handlers[packet.sid];
        if(handler){
            handler(packet.data);
        }else{
            var id = packet.sid & 0x1f;
            var mid = packet.sid >> 5;
            console.log("No Handler found for SID " + packet.sid + " (id: " + id + ", msg: " + mid + ")");
        }
    };
}

export default GCS;
