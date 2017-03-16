var msg_id = function(x){
    return x << 5;
}

var bool = function(x){
    return !!x;
}

var GCS = function(){
    this.handlers = {};
    this.m3psu = new M3PSU(this);
    this.m3dl = new M3DL(this);
    this.m3fc = new M3FC(this);
    this.m3pyro = new M3Pyro(this);
    this.m3radio = new M3Radio(this);
    this.m3ground = new M3Ground(this);

    // e.g. gcs.struct.Unpack( .. )
    this.struct = new JSPack();

    var _this = this;

    this.ws = new WebSocket("ws://" + window.location.host + "/ws")
    this.ws.onmessage = function(event){
        var packet = JSON.parse(event.data);
        _this.handlePacket(packet);
    };

    this.render = setInterval(function(){
        $("#display-m3psu").html('<pre>' + JSON.stringify(_this.m3psu, null, '\t') + '</pre>');
        $("#display-m3dl").html('<pre>' + JSON.stringify(_this.m3dl, null, '\t') + '</pre>');
        $("#display-m3fc").html('<pre>' + JSON.stringify(_this.m3fc, null, '\t') + '</pre>');
        $("#display-m3pyro").html('<pre>' + JSON.stringify(_this.m3pyro, null, '\t') + '</pre>');
        $("#display-m3radio").html('<pre>' + JSON.stringify(_this.m3radio, null, '\t') + '</pre>');
        $("#display-m3ground").html('<pre>' + JSON.stringify(_this.m3ground, null, '\t') + '</pre>');
    }, 100);
}
GCS.prototype.registerPacket = function(canID, handler){
    this.handlers[canID] = handler;
};
GCS.prototype.handlePacket = function(packet){
    var handler = this.handlers[packet.sid];
    if(handler){
        handler(packet.data);
    }else{
        console.log("No Handler found for SID: " + packet.sid);
    }
};
