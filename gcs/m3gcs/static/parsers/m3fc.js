var M3FC = function(gcs){
    var CAN_ID_M3FC = 1
    var CAN_MSG_ID_M3FC_STATUS = CAN_ID_M3FC | msg_id(0);
    var CAN_MSG_ID_M3FC_SET_CFG_PROFILE = CAN_ID_M3FC | msg_id(1);
    var CAN_MSG_ID_M3FC_SET_CFG_PYROS = CAN_ID_M3FC | msg_id(2);
    var CAN_MSG_ID_M3FC_LOAD_CFG = CAN_ID_M3FC | msg_id(3);
    var CAN_MSG_ID_M3FC_SAVE_CFG = CAN_ID_M3FC | msg_id(4);
    var CAN_MSG_ID_M3FC_MOCK_ENABLE = CAN_ID_M3FC | msg_id(5);
    var CAN_MSG_ID_M3FC_MOCK_ACCEL = CAN_ID_M3FC | msg_id(6);
    var CAN_MSG_ID_M3FC_MOCK_BARO = CAN_ID_M3FC | msg_id(7);
    var CAN_MSG_ID_M3FC_ARM = CAN_ID_M3FC | msg_id(8);
    var CAN_MSG_ID_M3FC_FIRE = CAN_ID_M3FC | msg_id(9);
    var CAN_MSG_ID_M3FC_MISSION_STATE = CAN_ID_M3FC | msg_id(32);
    var CAN_MSG_ID_M3FC_ACCEL = CAN_ID_M3FC | msg_id(48);
    var CAN_MSG_ID_M3FC_BARO = CAN_ID_M3FC | msg_id(49);
    var CAN_MSG_ID_M3FC_SE_T_H = CAN_ID_M3FC | msg_id(50);
    var CAN_MSG_ID_M3FC_SE_V_A = CAN_ID_M3FC | msg_id(51);
    var CAN_MSG_ID_M3FC_SE_VAR_H = CAN_ID_M3FC | msg_id(52);
    var CAN_MSG_ID_M3FC_SE_VAR_V_A = CAN_ID_M3FC | msg_id(53);
    var CAN_MSG_ID_M3FC_CFG_PROFILE = CAN_ID_M3FC | msg_id(54);
    var CAN_MSG_ID_M3FC_CFG_PYROS = CAN_ID_M3FC | msg_id(55);
    var CAN_MSG_ID_M3FC_CFG_ACCEL_CAL_X = CAN_ID_M3FC | msg_id(56);
    var CAN_MSG_ID_M3FC_CFG_ACCEL_CAL_Y = CAN_ID_M3FC | msg_id(57);
    var CAN_MSG_ID_M3FC_CFG_ACCEL_CAL_Z = CAN_ID_M3FC | msg_id(58);
    var CAN_MSG_ID_M3FC_CFG_RADIO_FREQ = CAN_ID_M3FC | msg_id(59);
    var CAN_MSG_ID_M3FC_CFG_CRC = CAN_ID_M3FC | msg_id(60);

    var components = {
        1: "Mission Control",
        2: "State Estimation",
        3: "Configuration",
        4: "Beeper",
        5: "LEDs",
        6: "Accelerometer",
        7: "Barometer",
        8: "Flash",
        9: "Pyros",
        10: "Mock",
        11: "PSU",
    };

    var component_errors = {
        0: "No Error",
        1: "Flash CRC", 2: "Flash Write",
        3: "Config Read", 8: "Config Check Profile", 9: "Config Check Pyros",
        10: "Accel Bad ID", 11: "Accel Self Test", 12: "Accel Timeout",
        13: "Accel Axis", 14: "SE Pressure", 15: "Pyro Arm", 4: "Pyro Continuity",
        5: "Pyro Supply", 16: "Mock Enabled", 17: "CAN Bad Command",
        18: "Config Check Accel Cal", 19: "Config Check Radio Freq",
        20: "Config Check CRC", 21: "Battleshort",
    };

    // State
    this.status = {overall: 0, components: {}};
    for(var i in components){
        var c = components[i];
        this.status.components[c] = {state: 0, reason: "Unknown"};
    }

    this.config = {
        profile: {
            position: "",
            accel_axis: "",
            burnout_timeout: 0,
            apogee_timeout: 0,
            main_altitude: 0,
            main_timeout: 0,
            land_timeout: 0,
        },
        pyros: [],
        accel: {},
        radio_freq: 0,
        crc: "",
    };
    for(var i=0; i<4; i++){
        this.config.pyros[i] = {usage: "UNSET", type: "UNSET"};
    }
    for(var i=0; i<3; i++){
        var axes = ["x", "y", "z"];
        this.config.accel[axes[i]] = {scale: 0, offset: 0};
    }

    this.accel = {x: 0, y: 0, z: 0};

    this.temperature = 0;
    this.pressure = 0;

    this.armed = false; // TODO: what if we don't hear the ARM message?

    this.met = 0.0;
    this.state = "INVAL";

    this.se = {
        dt: 0,
        h: 0,
        v: 0,
        a: 0,
        sdev_h: 0,
        sdev_v: 0,
        sdev_a: 0,
    };

    var _this = this;

    // Packet parsers
    gcs.registerPacket(CAN_MSG_ID_M3FC_STATUS, function(data){
        var [overall, comp, comp_state, comp_error] = gcs.struct.Unpack("BBBB", data);
        var c = components[comp];
        var c_status = _this.status.components[c];
        c_status.state = comp_state;
        c_status.reason = component_errors[comp_error];

        _this.status.overall = ["OK", "INIT", "ERROR"][overall];
    });

    var handleCfgProfile = function(data){
        var [position, accel_axis, ignition_acc, burnout_timeout,
            apogee_timeout, main_altitude, main_timeout, land_timeout] = data;
        var p = _this.config.profile;
        p.position = ["INVAL", "dart", "core"][position];
        p.accel_axis = ["INVAL", "X", "-X", "Y", "-Y", "Z", "-Z"][accel_axis];
        p.burnout_timeout = burnout_timeout / 10.0;
        p.apogee_timeout = apogee_timeout;
        p.main_altitude = main_altitude * 10;
        p.main_timeout = main_timeout;
        p.land_timeout = land_timeout * 10;
    };
    gcs.registerPacket(CAN_MSG_ID_M3FC_SET_CFG_PROFILE, handleCfgProfile);
    gcs.registerPacket(CAN_MSG_ID_M3FC_CFG_PROFILE, handleCfgProfile);

    var handleCfgPyros = function(data){
        var usages = {0: "None", 1: "Drogue", 2: "Main", 3: "Dart Separation"};
        var types = {0: "None", 1: "E-match", 2: "Talon", 3: "Metron"};
        for(var i=0;  i<4; i++){
            var p = _this.config.pyros[i];
            p.usage = usages[data[2*i]];
            p.type = types[data[2*i+1]];
        }
    };
    gcs.registerPacket(CAN_MSG_ID_M3FC_SET_CFG_PYROS, handleCfgPyros);
    gcs.registerPacket(CAN_MSG_ID_M3FC_CFG_PYROS, handleCfgPyros);

    gcs.registerPacket(CAN_MSG_ID_M3FC_LOAD_CFG, function(data){ });
    gcs.registerPacket(CAN_MSG_ID_M3FC_SAVE_CFG, function(data){ });
    gcs.registerPacket(CAN_MSG_ID_M3FC_MOCK_ENABLE, function(data){ });

    var handleAccel = function(data){
        // 6 bytes, 3 int16_ts for 3 accelerations
        // 3.9 MSB per milli-g
        var factor = 3.9 / 1000.0 * 9.80665;
        var [a1, a2, a3] = gcs.struct.Unpack("<hhh", data);
        _this.accel.x = a1*factor;
        _this.accel.y = a2*factor;
        _this.accel.z = a3*factor;
    };
    gcs.registerPacket(CAN_MSG_ID_M3FC_MOCK_ACCEL, handleAccel);
    gcs.registerPacket(CAN_MSG_ID_M3FC_ACCEL, handleAccel);

    var handleBaro = function(data){
        // 8 bytes: 4 bytes of temperature in centidegrees celcius,
        // 4 bytes of pressure in Pascals
        var [temperature, pressure] = gcs.struct.Unpack("<ii", data);
        _this.temperature = temperature / 100.0;
        _this.pressure = pressure;
    };
    gcs.registerPacket(CAN_MSG_ID_M3FC_MOCK_BARO, handleBaro);
    gcs.registerPacket(CAN_MSG_ID_M3FC_BARO, handleBaro);

    gcs.registerPacket(CAN_MSG_ID_M3FC_ARM, function(data){
        _this.armed = true;
    });

    gcs.registerPacket(CAN_MSG_ID_M3FC_FIRE, function(data){ });

    gcs.registerPacket(CAN_MSG_ID_M3FC_MISSION_STATE, function(data){
        // 5 bytes total. 4 bytes met, 1 byte can_state
        var [met, can_state] = gcs.struct.Unpack("<IB", data);
        var states = ["Init", "Pad", "Ignition", "Powered Ascent", "Burnout",
            "Free Ascent", "Apogee", "Drogue Descent",
            "Release Main", "Main Descent", "Land", "Landed"];
        _this.met = met / 1000.0;
        _this.state = states[can_state];
    });

    gcs.registerPacket(CAN_MSG_ID_M3FC_SE_T_H, function(data){
        // 8 bytes, 2 float32s
        var [dt, h] = gcs.struct.Unpack("<ff", data);
        _this.se.dt = dt;
        _this.se.h = h;
    });

    gcs.registerPacket(CAN_MSG_ID_M3FC_SE_V_A, function(data){
        var [v, a] = gcs.struct.Unpack("<ff", data);
        _this.se.v = v;
        _this.se.a = a;
    });

    gcs.registerPacket(CAN_MSG_ID_M3FC_SE_VAR_H, function(data){
        var [var_h] = gcs.struct.Unpack("<f", data);
        _this.se.sdev_h = Math.sqrt(var_h);
    });

    gcs.registerPacket(CAN_MSG_ID_M3FC_SE_VAR_V_A, function(data){
        var [var_v, var_a] = gcs.struct.Unpack("<ff", data);
        _this.se.sdev_v = Math.sqrt(var_v);
        _this.se.sdev_a = Math.sqrt(var_a);
    });

    var handleCfgAccel = function(idx, data){
        // scale is in g/LSB
        // offset is in LSB
        var [scale, offset] = gcs.struct.Unpack("<ff", data);
        var axis = ["x", "y", "z"][idx];
        var acc = _this.config.accel[axis];
        acc.scale = scale;
        acc.offset = offset;
    };
    gcs.registerPacket(CAN_MSG_ID_M3FC_CFG_ACCEL_CAL_X, function(data){
        handleCfgAccel(0, data);
    });
    gcs.registerPacket(CAN_MSG_ID_M3FC_CFG_ACCEL_CAL_Y, function(data){
        handleCfgAccel(1, data);
    });
    gcs.registerPacket(CAN_MSG_ID_M3FC_CFG_ACCEL_CAL_Z, function(data){
        handleCfgAccel(2, data);
    });

    gcs.registerPacket(CAN_MSG_ID_M3FC_CFG_RADIO_FREQ, function(data){
        var [freq] = gcs.struct.Unpack("<I", data);
        _this.config.radio_freq = freq / 1e6;
    });

    gcs.registerPacket(CAN_MSG_ID_M3FC_CFG_CRC, function(data){
        var [crc] = gcs.struct.Unpack("<I", data);
        crc = (crc) >>> 0; // convert internally to unsigned
        _this.config.crc = crc.toString(16);
    });
};
