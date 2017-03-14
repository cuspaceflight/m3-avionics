var msg_id = function(x){
    return x << 5;
}

var bool = function(x){
    return !!x;
}

var GCS = function(){
    this.handlers = {};
    this.m3psu = new M3PSU(this);

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
