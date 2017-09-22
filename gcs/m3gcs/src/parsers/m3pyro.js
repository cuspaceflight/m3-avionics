import { msg_id, bool, versionParser } from './utils.js';

const CAN_ID_M3PYRO = 3;
const CAN_MSG_ID_M3PYRO_STATUS = (CAN_ID_M3PYRO | msg_id(0));
const CAN_MSG_ID_M3PYRO_FIRE_COMMAND = (CAN_ID_M3PYRO | msg_id(1));
const CAN_MSG_ID_M3PYRO_ARM_COMMAND = (CAN_ID_M3PYRO | msg_id(2));
const CAN_MSG_ID_M3PYRO_FIRE_STATUS = (CAN_ID_M3PYRO | msg_id(16));
const CAN_MSG_ID_M3PYRO_ARM_STATUS = (CAN_ID_M3PYRO | msg_id(17));
const CAN_MSG_ID_M3PYRO_CONTINUITY = (CAN_ID_M3PYRO | msg_id(48));
const CAN_MSG_ID_M3PYRO_SUPPLY_STATUS = (CAN_ID_M3PYRO | msg_id(49));
const CAN_MSG_ID_M3PYRO_VERSION = CAN_ID_M3PYRO | msg_id(63);

class M3Pyro {
    constructor(gcs) {
        const components = {
            1: "Continuity",
            2: "Arming",
            3: "Firing",
        };

        const component_errors = {
            0: "No Error",
            1: "ADC Error",
        };

        // State
        this.status = {overall: 0, components: {}};
        for(var i in components){
            var c = components[i];
            this.status.components[c] = {state: 0, reason: "Unknown"};
        }

        this.channels = [];
        for(var i=0; i<4; i++){
            this.channels[i] = {continuity: Infinity, fire_status: "INVAL"};
        }

        this.armed = false;
        this.supply_voltage = 0.0;

        this.version = "UNKNOWN";

        var _this = this;

        // Packet parsers
        gcs.registerPacket(CAN_MSG_ID_M3PYRO_STATUS, function(data){
            var [overall, comp, comp_state, comp_error] = gcs.struct.Unpack("BBBB", data);
            var c = components[comp];
            var c_status = _this.status.components[c];
            c_status.state = comp_state;
            c_status.reason = component_errors[comp_error];

            _this.status.overall = ["OK", "INIT", "ERROR"][overall];
        });

        gcs.registerPacket(CAN_MSG_ID_M3PYRO_FIRE_COMMAND, function(data){ });

        gcs.registerPacket(CAN_MSG_ID_M3PYRO_ARM_COMMAND, function(data){ });

        gcs.registerPacket(CAN_MSG_ID_M3PYRO_FIRE_STATUS, function(data){
            var status_map = {0: "Off", 1: "EMatch", 2: "Talon", 3: "Metron"};
            for(var i=0; i<4; i++){
                _this.channels[i].fire_status = status_map[data[i]];
            }
        });

        gcs.registerPacket(CAN_MSG_ID_M3PYRO_ARM_STATUS, function(data){
            _this.armed = data[0] == 1;
        });

        gcs.registerPacket(CAN_MSG_ID_M3PYRO_CONTINUITY, function(data){
            for(var i=0; i<4; i++){
                var c = data[i] < 255 ? data[i]*2 : Infinity;
                _this.channels[i].continuity = c;
            }
        });

        gcs.registerPacket(CAN_MSG_ID_M3PYRO_SUPPLY_STATUS, function(data){
            _this.supply_voltage = data[0] / 10.0;
        });

        gcs.registerPacket(CAN_MSG_ID_M3PYRO_VERSION, versionParser(this));
    };
};

export default M3Pyro;

