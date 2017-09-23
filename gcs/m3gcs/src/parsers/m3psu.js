import { msg_id, bool, versionParser, DataPoint } from './utils.js';

const CAN_ID_M3PSU = 2;
const CAN_MSG_ID_M3PSU_STATUS = CAN_ID_M3PSU | msg_id(0);
const CAN_MSG_ID_M3PSU_TOGGLE_PYROS = CAN_ID_M3PSU | msg_id(16);
const CAN_MSG_ID_M3PSU_TOGGLE_CHANNEL = CAN_ID_M3PSU | msg_id(17);
const CAN_MSG_ID_M3PSU_TOGGLE_CHARGER = CAN_ID_M3PSU | msg_id(18);
const CAN_MSG_ID_M3PSU_PYRO_STATUS = CAN_ID_M3PSU | msg_id(48);
const CAN_MSG_ID_M3PSU_CHANNEL_STATUS_12 = CAN_ID_M3PSU | msg_id(49);
const CAN_MSG_ID_M3PSU_CHANNEL_STATUS_34 = CAN_ID_M3PSU | msg_id(50);
const CAN_MSG_ID_M3PSU_CHANNEL_STATUS_56 = CAN_ID_M3PSU | msg_id(51);
const CAN_MSG_ID_M3PSU_CHANNEL_STATUS_78 = CAN_ID_M3PSU | msg_id(52);
const CAN_MSG_ID_M3PSU_CHANNEL_STATUS_910 = CAN_ID_M3PSU | msg_id(53);
const CAN_MSG_ID_M3PSU_CHANNEL_STATUS_1112= CAN_ID_M3PSU | msg_id(54);
const CAN_MSG_ID_M3PSU_CHARGER_STATUS = CAN_ID_M3PSU | msg_id(55);
const CAN_MSG_ID_M3PSU_BATT_VOLTAGES = CAN_ID_M3PSU | msg_id(56);
const CAN_MSG_ID_M3PSU_CAPACITY = CAN_ID_M3PSU | msg_id(57);
const CAN_MSG_ID_M3PSU_AWAKE_TIME = CAN_ID_M3PSU | msg_id(58);
const CAN_MSG_ID_M3PSU_VERSION = CAN_ID_M3PSU | msg_id(63);

class M3PSU {
    constructor(gcs) {
        const components = {
            0: "DCDC1",
            1: "DCDC2",
            2: "DCDC3",
            3: "DCDC4",
            4: "DCDC5",
            5: "DCDC6",
            6: "Charger",
            7: "Pyro Monitor",
        };

        const dcdc_errors = {
            0: "No Error",
            1: "Init",
            2: "Ch1 Alert",
            3: "Ch2 Alert",
            4: "Ch1 Alert",
            5: "Ch2 Alert",
            6: "Comms",
        };

        const charger_errors = {
            0: "No Error",
            1: "Init",
            2: "Read",
        };

        const pyro_mon_errors = {
            0: "No Error",
            1: "Init",
            2: "Comms",
        };

        // State
        var status = {overall: 0, components: {}};
        for(var i in components){
            var c = components[i];
            status.components[c] = {state: 0, reason: "Unknown"};
        }
        this.status = new DataPoint(status);

        console.log(this.status);

        var channels = [];
        for(var i=0; i<12; i++){
            channels[i] = {v:0, i:0, p:0};
        }
        this.channels = new DataPoint(channels);

        this.charger = new DataPoint({
            enabled: false,
            charging: false,
            inhibit: false,
            battleshort: false,
            acfet: false,
            voltage_mode: "low",
            temperature: 0,
            current: 0,
        });

        this.runtime = new DataPoint(0);
        this.percent = new DataPoint(0);

        this.cells = new DataPoint([0, 0]);
        this.batt_voltage = new DataPoint(0);

        this.pyro = new DataPoint({
            v:0, i:0, p:0,
            enabled: false,
        });

        this.awake_time = new DataPoint(0);
        this.power_mode = new DataPoint("low");

        this.version = new DataPoint("UNKNOWN");

        var _this = this;

        //TODO handle registering commands?
        gcs.registerPacket(CAN_MSG_ID_M3PSU_TOGGLE_CHANNEL, function(data){} );

        gcs.registerPacket(CAN_MSG_ID_M3PSU_TOGGLE_PYROS, function(data){} );

        gcs.registerPacket(CAN_MSG_ID_M3PSU_TOGGLE_CHARGER, function(data){} );

        // Packet parsers
        gcs.registerPacket(CAN_MSG_ID_M3PSU_STATUS, function(data){
            var [overall, comp, comp_state, comp_error] = gcs.struct.Unpack("BBBB", data);
            if(comp >= 0 && comp <= 5){
                var reason = dcdc_errors[comp_error];
            }else if(comp == 6){
                var reason = charger_errors[comp_error];
            }else if(comp == 7){
                var reason = pyro_mon_errors[comp_error];
            }
            var stateupdate = {};
            stateupdate[components[comp]] = {state: comp_state, reason: reason};
            _this.status.set({components: stateupdate});

            _this.status.get().overall.set(["OK", "INIT", "ERROR"][overall]);
        });

        gcs.registerPacket(CAN_MSG_ID_M3PSU_PYRO_STATUS, function(data){
            var [v, i, p, state] = gcs.struct.Unpack("<HHHb", data);
            _this.pyro.set({
                "v": v * 0.001,
                "i": i * 0.001,
                "p": p * 0.001,
                "enabled": bool(state & 1),
            });
        });

        var handleChannels = function(c, data){
            var c1 = _this.channels.get()[c];
            var c2 = _this.channels.get()[c+1];

            c1.set({
                "v": data[0] * 0.03,
                "i": data[1] * 0.0003,
                "p": data[2] * 0.002,
            });

            c2.set({
                "v": data[4] * 0.03,
                "i": data[5] * 0.0003,
                "p": data[6] * 0.002,
            });
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

            _this.charger.set({
                current: current,
                enabled: bool(state & 1),
                charging: bool(state & 2),
                inhibit: bool(state & 4),
                battleshort: bool(state & 32),
                acfet: bool(state & 64),
                voltage_mode: ["Pre-charge", "Low", "Med", "High", "INVAL"][vmode],
                temperature: (tempcK/10) - 273.2,
            });
        });

        gcs.registerPacket(CAN_MSG_ID_M3PSU_BATT_VOLTAGES, function(data){
            var [v1, v2, vb] = gcs.struct.Unpack("<HHH", data);
            _this.cells.set([v1 * 0.01, v2 * 0.01]);
            _this.batt_voltage.set(vb * 0.01);
        });

        gcs.registerPacket(CAN_MSG_ID_M3PSU_CAPACITY, function(data){
            var [mins, percent] = gcs.struct.Unpack("<HB", data);

            if(mins == 65535){ // Charging, so runtime is infinite
                mins = Infinity;
            }

            _this.percent.set(percent);
            _this.runtime.set(mins);
        });

        gcs.registerPacket(CAN_MSG_ID_M3PSU_AWAKE_TIME, function(data){
            var [secs, status] = gcs.struct.Unpack("<HB", data);

            _this.awake_time.set(secs);
            _this.power_mode.set((status & 1) ? "low" : "high");
        });

        gcs.registerPacket(CAN_MSG_ID_M3PSU_VERSION, versionParser(this));
    };
};

export default M3PSU;

