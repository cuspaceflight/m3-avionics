var msg_id = function(x){
    return x << 5;
}

var bool = function(x){
    return !!x;
}

var GCS = function(){
    this.handlers = {};
    this.m3psu = new M3PSU(this);

    this.ws = io.connect('http://' + document.domain + ':' + location.port);

    var _this = this;

    this.ws.on('packet', function(packet){
        _this.handlePacket(packet);
    });

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
