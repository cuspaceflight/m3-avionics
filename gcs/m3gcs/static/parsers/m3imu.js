var M3IMU = function(gcs){
    var CAN_ID_M3IMU = 5;
    var CAN_MSG_ID_M3IMU_STATUS = CAN_ID_M3IMU | msg_id(0);

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
        10: "Mock"
    };

    var components = {
        0: "messaging",
        1: "telemetry_allocator",
        2: "adis16405",
        3: "mpu9250",
        4: "ms5611",
        5: "can_telemetry",
        6: "world_mag_model",
        7: "ublox",
        8: "sd_card",
        9: "file_telemetry_output",
        10: "state_board_config",
    };

    // State
    this.status = {overall: 0, components: {}};
    for(var i in components){
        var c = components[i];
        this.status.components[c] = {state: 0, reason: "Unknown"};
    }

    var _this = this;

    // Packet parsers
    gcs.registerPacket(CAN_MSG_ID_M3IMU_STATUS, function(data){
        var [overall, comp, comp_state, comp_error] = gcs.struct.Unpack("BBBB", data);
        var c = components[comp];
        var c_status = _this.status.components[c];
        c_status.state = comp_state;
        c_status.reason = component_errors[comp_error];

        _this.status.overall = ["OK", "INIT", "ERROR"][overall];
    });
};
