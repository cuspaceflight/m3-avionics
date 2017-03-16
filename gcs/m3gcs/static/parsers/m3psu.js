var M3PSU = function(gcs){
    var CAN_ID_M3PSU = 2;
    var CAN_MSG_ID_M3PSU_STATUS = CAN_ID_M3PSU | msg_id(0);
    var CAN_MSG_ID_M3PSU_TOGGLE_PYROS = CAN_ID_M3PSU | msg_id(16);
    var CAN_MSG_ID_M3PSU_TOGGLE_CHANNEL = CAN_ID_M3PSU | msg_id(17);
    var CAN_MSG_ID_M3PSU_TOGGLE_CHARGER = CAN_ID_M3PSU | msg_id(18);
    var CAN_MSG_ID_M3PSU_PYRO_STATUS = CAN_ID_M3PSU | msg_id(48);
    var CAN_MSG_ID_M3PSU_CHANNEL_STATUS_12 = CAN_ID_M3PSU | msg_id(49);
    var CAN_MSG_ID_M3PSU_CHANNEL_STATUS_34 = CAN_ID_M3PSU | msg_id(50);
    var CAN_MSG_ID_M3PSU_CHANNEL_STATUS_56 = CAN_ID_M3PSU | msg_id(51);
    var CAN_MSG_ID_M3PSU_CHANNEL_STATUS_78 = CAN_ID_M3PSU | msg_id(52);
    var CAN_MSG_ID_M3PSU_CHANNEL_STATUS_910 = CAN_ID_M3PSU | msg_id(53);
    var CAN_MSG_ID_M3PSU_CHANNEL_STATUS_1112= CAN_ID_M3PSU | msg_id(54);
    var CAN_MSG_ID_M3PSU_CHARGER_STATUS = CAN_ID_M3PSU | msg_id(55);
    var CAN_MSG_ID_M3PSU_BATT_VOLTAGES = CAN_ID_M3PSU | msg_id(56);
    var CAN_MSG_ID_M3PSU_CAPACITY = CAN_ID_M3PSU | msg_id(57);
    var CAN_MSG_ID_M3PSU_AWAKE_TIME = CAN_ID_M3PSU | msg_id(58);

    var components = {
        0: "DCDC1",
        1: "DCDC2",
        2: "DCDC3",
        3: "DCDC4",
        4: "DCDC5",
        5: "DCDC6",
        6: "Charger",
        7: "Pyro Monitor",
    };

    var dcdc_errors = {
        0: "No Error",
        1: "Init",
        2: "Ch1 Alert",
        3: "Ch2 Alert",
        4: "Comms",
    };

    var charger_errors = {
        0: "No Error",
        1: "Init",
        2: "Read",
    };

    var pyro_mon_errors = {
        0: "No Error",
        1: "Init",
        2: "Comms",
    };

    // State
    this.status = {overall: 0, components: {}};
    for(var i in components){
        var c = components[i];
        this.status.components[c] = {state: 0, reason: "Unknown"};
    }

    this.channels = [];
    for(var i=0; i<12; i++){
        this.channels[i] = {v:0, i:0, p:0};
    }

    this.charger = {
        enabled: false,
        charging: false,
        inhibit: false,
        battleshort: false,
        acfet: false,
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

    this.awake_time = 0;
    this.power_mode = "low";

    var _this = this;

    //TODO handle registering commands?
    gcs.registerPacket(CAN_MSG_ID_M3PSU_TOGGLE_CHANNEL, function(data){} );

    gcs.registerPacket(CAN_MSG_ID_M3PSU_TOGGLE_PYROS, function(data){} );

    gcs.registerPacket(CAN_MSG_ID_M3PSU_TOGGLE_CHARGER, function(data){} );

    // Packet parsers
    gcs.registerPacket(CAN_MSG_ID_M3PSU_STATUS, function(data){
        var [overall, comp, comp_state, comp_error] = gcs.struct.Unpack("BBBB", data);
        var c = components[comp];
        var c_status = _this.status.components[c];
        c_status.state = comp_state;
        if(comp >= 0 && comp <= 5){
            c_status.reason = dcdc_errors[comp_error];
        }else if(comp == 6){
            c_status.reason = charger_errors[comp_error];
        }else if(comp == 7){
            c_status.reason = pyro_mon_errors[comp_error];
        }

        _this.status.overall = ["OK", "INIT", "ERROR"][overall];
    });

    gcs.registerPacket(CAN_MSG_ID_M3PSU_PYRO_STATUS, function(data){
        var [v, i, p, state] = gcs.struct.Unpack("<HHHb", data);
        _this.pyro.v = v * 0.001;
        _this.pyro.i = i * 0.001;;
        _this.pyro.p = p * 0.001;;
        _this.pyro.enabled = bool(state & 1);
    });

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

    gcs.registerPacket(CAN_MSG_ID_M3PSU_CHARGER_STATUS, function(data){
        var [current, state, tempcK]= gcs.struct.Unpack("<hBH", data);

        var vmode = (state >> 3) & 0x3;

        var c = _this.charger;
        c.current = current;
        c.enabled = bool(state & 1);
        c.charging = bool(state & 2);
        c.inhibit = bool(state & 4);
        c.battleshort = bool(state & 32);
        c.acfet = bool(state & 64);
        c.voltage_mode = ["Pre-charge", "Low", "Med", "High", "INVAL"][vmode];
        c.temperature = (tempcK/10) - 273.2;
    });

    gcs.registerPacket(CAN_MSG_ID_M3PSU_BATT_VOLTAGES, function(data){
        var [v1, v2, vb] = gcs.struct.Unpack("<HHH", data);
        _this.cells[0] = v1 * 0.01;
        _this.cells[1] = v2 * 0.01;
        _this.batt_voltage = vb * 0.01;
    });

    gcs.registerPacket(CAN_MSG_ID_M3PSU_CAPACITY, function(data){
        var [mins, percent] = gcs.struct.Unpack("<HB", data);

        if(mins == 65535){ // Charging, so runtime is infinite
            mins = Infinity;
        }

        _this.percent = percent;
        _this.runtime = mins;
    });

    gcs.registerPacket(CAN_MSG_ID_M3PSU_AWAKE_TIME, function(data){
        var [secs, stats] = gcs.struct.Unpack("<HB", data);

        _this.awake_time = secs;
        _this.power_mode = (status & 1) ? "low" : "high";
    });
}
