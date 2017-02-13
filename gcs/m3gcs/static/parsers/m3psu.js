var M3PSU = function(gcs){
    var CAN_ID_M3PSU = 2;
    var CAN_MSG_ID_M3PSU_BATT_VOLTAGES = CAN_ID_M3PSU | msg_id(56);
    var CAN_MSG_ID_M3PSU_TOGGLE_PYROS = CAN_ID_M3PSU | msg_id(16);
    var CAN_MSG_ID_M3PSU_CHANNEL_STATUS_12 = CAN_ID_M3PSU | msg_id(49);
    var CAN_MSG_ID_M3PSU_CHANNEL_STATUS_34 = CAN_ID_M3PSU | msg_id(50);
    var CAN_MSG_ID_M3PSU_CHANNEL_STATUS_56 = CAN_ID_M3PSU | msg_id(51);
    var CAN_MSG_ID_M3PSU_CHANNEL_STATUS_78 = CAN_ID_M3PSU | msg_id(52);
    var CAN_MSG_ID_M3PSU_CHANNEL_STATUS_910 = CAN_ID_M3PSU | msg_id(53);
    var CAN_MSG_ID_M3PSU_CHANNEL_STATUS_1112= CAN_ID_M3PSU | msg_id(54);
    var CAN_MSG_ID_M3PSU_TOGGLE_CHANNEL = CAN_ID_M3PSU | msg_id(17);
    var CAN_MSG_ID_M3PSU_CAPACITY = CAN_ID_M3PSU | msg_id(57);
    var CAN_MSG_ID_M3PSU_PYRO_STATUS = CAN_ID_M3PSU | msg_id(48);
    var CAN_MSG_ID_M3PSU_CHARGER_STATUS = CAN_ID_M3PSU | msg_id(55);
    var CAN_MSG_ID_M3PSU_TOGGLE_CHARGER = CAN_ID_M3PSU | msg_id(18);

    this.channels = [];
    for(var i=0; i<12; i++){
        this.channels[i] = {v:0, i:0, p:0};
    }

    this.charger = {
        enabled: false,
        charging: false,
        inhibit: false,
        voltage_mode: "low",
        temperature: 0,
        current: 0,
    };
    
    this.runtime = 0;
    this.percent = 0;

    this.cells = [0, 0];
    this.batt_voltage = 0;

    this.pyro = {
        v:0, i:0, p:0,
        enabled: false,
    };

    var _this = this;

    gcs.registerPacket(CAN_MSG_ID_M3PSU_BATT_VOLTAGES, function(data){
        _this.cells[0] = ((data[1] << 8) | data[0]) * 0.01;
        _this.cells[1] = ((data[3] << 8) | data[2]) * 0.01;
        _this.batt_voltage = ((data[5] << 8) | data[4]) * 0.01;
    });

    gcs.registerPacket(CAN_MSG_ID_M3PSU_TOGGLE_PYROS, function(data){} );

    var handleChannels = function(c, data){
        var c1 = _this.channels[c];
        var c2 = _this.channels[c+1];

        c1.v = data[0] * 0.03;
        c1.i = data[1] * 0.0003;
        c1.p = data[2] * 0.002;

        c2.v = data[4] * 0.03;
        c2.i = data[5] * 0.0003;
        c2.p = data[6] * 0.002;
    };
    gcs.registerPacket(CAN_MSG_ID_M3PSU_CHANNEL_STATUS_12, function(data){
        handleChannels(0, data);
    });
    gcs.registerPacket(CAN_MSG_ID_M3PSU_CHANNEL_STATUS_34, function(data){
        handleChannels(2, data);
    });
    gcs.registerPacket(CAN_MSG_ID_M3PSU_CHANNEL_STATUS_56, function(data){
        handleChannels(4, data);
    });
    gcs.registerPacket(CAN_MSG_ID_M3PSU_CHANNEL_STATUS_78, function(data){
        handleChannels(6, data);
    });
    gcs.registerPacket(CAN_MSG_ID_M3PSU_CHANNEL_STATUS_910, function(data){
        handleChannels(8, data);
    });
    gcs.registerPacket(CAN_MSG_ID_M3PSU_CHANNEL_STATUS_1112, function(data){
        handleChannels(10, data);
    });

    gcs.registerPacket(CAN_MSG_ID_M3PSU_TOGGLE_CHANNEL, function(data){} );

    gcs.registerPacket(CAN_MSG_ID_M3PSU_PYRO_STATUS, function(data){
        var v = ((data[1] << 8) | data[0]) / 1000.0;
        var i = ((data[3] << 8) | data[2]) / 1000000.0;
        var p = ((data[5] << 8) | data[4]) / 100000.0;
        var state = data[6];

        _this.pyro.v = v;
        _this.pyro.i = i;
        _this.pyro.p = p;
        _this.pyro.enabled = bool(state & 1);
    });

    gcs.registerPacket(CAN_MSG_ID_M3PSU_CHARGER_STATUS, function(data){
        var current = (data[1] << 8) | data[0];
        if(current > 32767){ // Convert to unsigned
            current -= 65536;
        }
        var state = data[2];
        var tempcK = (data[4] << 8) | data[3];
        
        var vmode = (state >> 3) & 0x3;

        var c = _this.charger;
        c.current = current;
        c.enabled = bool(state & 1);
        c.charging = bool(state & 2);
        c.inhibit = bool(state & 4);
        c.voltage_mode = ["Pre-charge", "Low", "Med", "High", "INVAL"][vmode];
        c.temperature = (tempcK/10) - 273.2;
    });
    
    gcs.registerPacket(CAN_MSG_ID_M3PSU_CAPACITY, function(data){
        var mins = ((data[1] << 8) | data[0]);
        var percent = data[2];
        
        if(mins == 65535){ // Charging, so runtime is infinite
            mins = Infinity;
        }
        
        _this.percent = percent;
        _this.runtime = mins;
    });

    gcs.registerPacket(CAN_MSG_ID_M3PSU_TOGGLE_CHARGER, function(data){} );
}
