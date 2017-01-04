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
    var CAN_MSG_ID_M3PSU_INTEXT_STATUS = CAN_ID_M3PSU | msg_id(57);
    var CAN_MSG_ID_M3PSU_PYRO_STATUS = CAN_ID_M3PSU | msg_id(48);
    var CAN_MSG_ID_M3PSU_CHARGER_STATUS = CAN_ID_M3PSU | msg_id(55);
    var CAN_MSG_ID_M3PSU_TOGGLE_CHARGER = CAN_ID_M3PSU | msg_id(18);
    var CAN_MSG_ID_M3PSU_TOGGLE_BALANCE = CAN_ID_M3PSU | msg_id(19);
    var CAN_MSG_ID_M3PSU_TOGGLE_INTEXT = CAN_ID_M3PSU | msg_id(20);

    this.channels = [];
    for(var i=0; i<12; i++){
        this.channels[i] = {v:0, i:0, p:0};
    }

    this.charger = {
        should_balance: false,
        should_charge: false,
        bleed_batt1: false,
        bleed_batt2: false,
        is_charging: false,
        overcurrent: false,
        current: 0,
    };

    this.cells = [0, 0];

    this.pyro = {
        v:0, i:0, p:0,
        enabled: false,
    };

    this.int_enabled = false;
    this.ext_enabled = false;

    var _this = this;

    gcs.registerPacket(CAN_MSG_ID_M3PSU_BATT_VOLTAGES, function(data){
        _this.cells[0] = data[0] * 0.02;
        _this.cells[1] = data[1] * 0.02;

        var state = data[2];

        var c = _this.charger;
        c.should_balance = bool(state & 4);
        c.bleed_batt1 = bool(state & 2);
        c.bleed_batt2 = bool(state & 1);
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
        var current = ((data[1] << 8) | data[0]) / 1000.0;
        var state = data[2];

        var c = _this.charger;
        c.current = current;
        c.should_charge = bool(state & 1);
        c.is_charging = bool(state & 2);
        c.overcurrent = bool(state & 4);
    });

    gcs.registerPacket(CAN_MSG_ID_M3PSU_TOGGLE_CHARGER, function(data){} );

    gcs.registerPacket(CAN_MSG_ID_M3PSU_TOGGLE_BALANCE, function(data){} );

    gcs.registerPacket(CAN_MSG_ID_M3PSU_TOGGLE_INTEXT, function(data){} );

    gcs.registerPacket(CAN_MSG_ID_M3PSU_INTEXT_STATUS, function(data){
        var state = data[0];

        _this.int_enabled = bool(state & 2);
        _this.ext_enabled = bool(state & 1);
    });
}
