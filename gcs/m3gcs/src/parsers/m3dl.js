import { msg_id, versionParser } from './utils.js';

const CAN_ID_M3DL = 6;
const CAN_MSG_ID_M3DL_STATUS = CAN_ID_M3DL | msg_id(0);
const CAN_MSG_ID_M3DL_FREE_SPACE = CAN_ID_M3DL | msg_id(32);
const CAN_MSG_ID_M3DL_RATE = CAN_ID_M3DL | msg_id(33);
const CAN_MSG_ID_M3DL_TEMP_1_2 = CAN_ID_M3DL | msg_id(48);
const CAN_MSG_ID_M3DL_TEMP_3_4 = CAN_ID_M3DL | msg_id(49);
const CAN_MSG_ID_M3DL_TEMP_5_6 = CAN_ID_M3DL | msg_id(50);
const CAN_MSG_ID_M3DL_TEMP_7_8 = CAN_ID_M3DL | msg_id(51);
const CAN_MSG_ID_M3DL_TEMP_9 = CAN_ID_M3DL | msg_id(52);
const CAN_MSG_ID_M3DL_PRESSURE = CAN_ID_M3DL | msg_id(53);
const CAN_MSG_ID_M3DL_VERSION = CAN_ID_M3DL | msg_id(63);

class M3DL {
    constructor(gcs) {
        const components = {
            1: "Temperature",
            2: "SD Card",
            3: "Pressure"
        };

        const component_errors = {
            0: "No Error",
            1: "LTC2983 TX Overflow", 2: "LTC2983 Setup",
            3: "SD Card Connection", 4: "SD Card Mounting",
            5: "SD Card File Open", 6: "SD Card Inc File Open",
            7: "SD Card Write", 8: "Logging Cache Flush", 9: "SD Card FULL",
            16: "T1 Invalid", 17: "T2 Invalid", 18: "T3 Invalid",
            19: "T4 Invalid", 20: "T5 Invalid", 21: "T6 Invalid",
            22: "T4 Invalid", 23: "T5 Invalid", 24: "T9 Invalid",
            25: "CRC Failure", 32: "Pressure Timeout"
        };

        // State
        this.status = {overall: 0, components: {}};
        for(var i in components){
            var c = components[i];
            this.status.components[c] = {state: 0, reason: "Unknown"};
        }

        this.free_space_GB = 0.0;

        this.packet_rate = 0;

        this.pressures = [0,0,0,0];

        this.temperatures = [0,0,0,0,0,0,0,0];

        this.version = "UNKNOWN";

        var _this = this;

        // Packet parsers
        gcs.registerPacket(CAN_MSG_ID_M3DL_STATUS, function(data){
            var [overall, comp, comp_state, comp_error] = gcs.struct.Unpack("BBBB", data);
            var c = components[comp];
            var c_status = _this.status.components[c];
            c_status.state = comp_state;
            c_status.reason = component_errors[comp_error];

            _this.status.overall = ["OK", "INIT", "ERROR"][overall];
        });

        gcs.registerPacket(CAN_MSG_ID_M3DL_FREE_SPACE, function(data){
            var [free_clusters] = gcs.struct.Unpack("<I", data);
            var free_space = ((free_clusters*16) / (1024*1024));
            _this.free_space_GB = free_space;
        });

        gcs.registerPacket(CAN_MSG_ID_M3DL_RATE, function(data){
            var [pkt_rate] = gcs.struct.Unpack("<I", data);
            _this.packet_rate = pkt_rate;
        });

        gcs.registerPacket(CAN_MSG_ID_M3DL_PRESSURE, function(data){
            var [pressure1, pressure2, pressure3, pressure4] = gcs.struct.Unpack("<HHHH", data);
            _this.pressures[0] = pressure1;
            _this.pressures[1] = pressure2;
            _this.pressures[2] = pressure3;
            _this.pressures[3] = pressure4;
        });

        var handleTemp = function(idx, data){
            var t1 = (data[1] << 16) | (data[2] << 8) | (data[3]);
            var t2 = (data[5] << 16) | (data[6] << 8) | (data[7]);
            if (t1 >= (1<<23)) {
                t1 -= (1<<24);
            }
            if (t2 >= (1<<23)){
                t2 -= (1<<24);
            }
            _this.temperatures[(idx*2)] = t1 / 1024.0;
            _this.temperatures[(idx*2)+1] = t2 / 1024.0;
        };
        gcs.registerPacket(CAN_MSG_ID_M3DL_TEMP_1_2, function(data){
            handleTemp(0, data);
        });
        gcs.registerPacket(CAN_MSG_ID_M3DL_TEMP_3_4, function(data){
            handleTemp(1, data);
        });
        gcs.registerPacket(CAN_MSG_ID_M3DL_TEMP_5_6, function(data){
            handleTemp(2, data);
        });
        gcs.registerPacket(CAN_MSG_ID_M3DL_TEMP_7_8, function(data){
            handleTemp(3, data);
        });

        gcs.registerPacket(CAN_MSG_ID_M3DL_VERSION, versionParser(this));
    };
};

export default M3DL;

