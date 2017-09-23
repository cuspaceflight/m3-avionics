import { msg_id, versionParser } from './utils.js';

const CAN_ID_M3IMU = 5;
const CAN_MSG_ID_M3IMU_STATUS = CAN_ID_M3IMU | msg_id(0);
const CAN_MSG_ID_M3IMU_VERSION = CAN_ID_M3IMU | msg_id(63);

class M3IMU {
    constructor(gcs) {
        const components = {
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

        // Multipacket handling
        var telemetry_id_table = {};
        var TELEMETRY_SOURCE = function(name, parent_name, id, suffix_length){
            var t = telemetry_id_table;
            t[name + "_mask"] = ((1<<suffix_length)-1);
            t[name] = ((id << t[parent_name + "_suffix_length"]) & t[name + "_mask"]) | t[parent_name];
            t[name + "_suffix_length"] = suffix_length;
        };
        telemetry_id_table["ts_all"] = 0;
        telemetry_id_table["ts_all_mask"] = 0;
        telemetry_id_table["ts_all_suffix_length"] = 0;
        TELEMETRY_SOURCE("ts_m3fc", "ts_all",                                 1,        5);
        TELEMETRY_SOURCE("ts_m3psu", "ts_all",                                2,        5);
        TELEMETRY_SOURCE("ts_m3pyro", "ts_all",                               3,        5);
        TELEMETRY_SOURCE("ts_m3radio", "ts_all",                              4,        5);
        TELEMETRY_SOURCE("ts_m3imu", "ts_all",                                5,        5);
        TELEMETRY_SOURCE("ts_m3dl", "ts_all",                                 6,        5);

        TELEMETRY_SOURCE("ts_component_state", "ts_m3imu",                    0,       11);

        // State Estimation
        TELEMETRY_SOURCE("ts_state_estimation", "ts_m3imu",                   0b001,    8);
        TELEMETRY_SOURCE("ts_state_estimate_data", "ts_state_estimation",     0b000,    8);

        // MS5611
        TELEMETRY_SOURCE("ts_ms5611", "ts_m3imu",                             0b001010,11);
        TELEMETRY_SOURCE("ts_ms5611_data", "ts_ms5611",                       0,       11);

        // MPU9250
        TELEMETRY_SOURCE("ts_mpu9250", "ts_m3imu",                            0b011,    8);
        TELEMETRY_SOURCE("ts_mpu9250_data", "ts_mpu9250",                     0b001,    9);

        // ADIS16405
        TELEMETRY_SOURCE("ts_adis16405", "ts_m3imu",                          0b110,    8);
        TELEMETRY_SOURCE("ts_adis16405_data", "ts_adis16405",                 0b000,    9);

        // UBLOX
        TELEMETRY_SOURCE("ts_ublox", "ts_m3imu",                              0b101,    8);
        TELEMETRY_SOURCE("ts_ublox_nav", "ts_ublox",                          0b000,    8);

        var packet_ids = [];

        var multipackets = [];
        var multipacket_message_definitions = [];

        var multipacket_handlers = {};

        var registerMultipacket = function(basename, size, handler){
            var base_id = telemetry_id_table[basename];
            var mask = telemetry_id_table[basename + "_mask"];
            var suffix_length = telemetry_id_table[basename + "_suffix_length"];
            var size_in_packets = Math.floor((size + 7)/8);
            multipacket_message_definitions.push({
                base_id: base_id,
                size_in_bytes: size,
                size_in_packets: size_in_packets,
                seqno_mask: ~mask,
                suffix_length: suffix_length,
            });
            var packet = {
                buffer: [],
                valid: [],
            };
            // Valid flag for each sub-packet
            for(var i=0; i<size_in_packets; i++){
                packet.valid.push(false);
            }

            // Data buffer
            for(i=0; i<size; i++){
                packet.buffer.push(0);
            }
            multipackets.push(packet);

            // Enumerate all the possible packet ids
            for(i=0; i<size_in_packets; i++){
                var pid = base_id;
                pid |= i << suffix_length; // sequence number

                packet_ids.push(pid);
            }

            // Register handler on complete data
            multipacket_handlers[base_id] = handler;
        };

        // State
        this.status = {overall: 0, components: {}};
        for(var i in components){
            var c = components[i];
            this.status.components[c] = {state: 0, reason: "Unknown"};
        }

        this.mpu9250 = {
            accel: {x: 0, y: 0, z: 0},
            temp: 0,
            gyro: {x: 0, y: 0, z: 0},
            magno: {x: 0, y: 0, z: 0},
        };

        this.state_estimate = {
            timestamp: 0,
            orientation: [0, 0, 0, 0],
            angular_velocity: {x: 0, y: 0, z: 0},
            latitude: 0,
            longitude: 0,
            altitude: 0,
        };

        this.version = "UNKNOWN";

        var _this = this;

        // Packet parsers
        gcs.registerPacket(CAN_MSG_ID_M3IMU_STATUS, function(data){
            var [overall, comp, comp_state, comp_error] = gcs.struct.Unpack("BBBB", data);
            var c = components[comp];
            var c_status = _this.status.components[c];
            c_status.state = comp_state;
            c_status.reason = comp_error;

            _this.status.overall = ["OK", "INIT", "ERROR"][overall];
        });

        var resetMultipacketMessage = function(multipacket){
            // Reset all the valid flags
            for(var i=0; i<multipacket.valid.length; i++){
                multipacket.valid[i] = false;
            }
        }

        var isMultipacketValid = function(def, multipacket){
            // If all sub-packets are valid, then OK
            for(var i=0; i<def.size_in_packets; i++){
                if(!multipacket.valid[i]){
                    return false;
                }
            }
            return true;
        };

        var getMultipacketIndex = function(id){
            for(var i=0; i<multipacket_message_definitions.length; i++){
                var mask = ~(multipacket_message_definitions[i].seqno_mask);
                if((id & mask) === multipacket_message_definitions[i].base_id){
                    return i;
                }
            }
            return -1;
        };

        var handleFullPacket = function(id, data){
            // TODO
            console.log(id, data);
        };

        var handleMultipacket = function(can_id, data){
            var multipacket_index = getMultipacketIndex(can_id);
            if(multipacket_index < 0){
                handleFullPacket(can_id, data);
                return;
            }

            var multipacket = multipackets[multipacket_index];
            var def = multipacket_message_definitions[multipacket_index];
            var seqno = (can_id & def.seqno_mask) >> def.suffix_length;
            var ptr = seqno * 8;

            // Copy this packet's data into the buffer
            for(var i=0; i<data.length; i++){
                multipacket.buffer[ptr+i] = data[i];
            }

            // Mark this sub-packet as valid
            multipacket.valid[seqno] = true;

            // If all sub-packets are valid, handle the full packet
            if(isMultipacketValid(def, multipacket)){
                if(multipacket_handlers[def.base_id]){
                    multipacket_handlers[def.base_id](multipacket.buffer);
                }else{
                    console.log("No multipacket handler found for base_id: " + def.base_id);
                }
                resetMultipacketMessage(multipacket);
            }
        };

        var handleMPU9250 = function(data){
            var [ax, ay, az, temp, gx, gy, gz, mx, my, mz] =
                gcs.struct.Unpack("<hhhhhhhhhh", data);
            var m = _this.mpu9250;
            m.accel.x = ax;
            m.accel.y = ay;
            m.accel.z = az;
            m.temp = temp;
            m.gyro.x = gx;
            m.gyro.y = gy;
            m.gyro.z = gz;
            m.magno.x = mx;
            m.magno.y = my;
            m.magno.z = mz;
        };

        var handleADIS16405 = function(data){
            var [supply, gx, gy, gz, ax, ay, az, mx, my, mz] =
                gcs.struct.Unpack("<hhhhhhhhhh", data);
            var a = _this.adis16405;
            a.supply = supply;
            a.gyro.x = gx;
            a.gyro.y = gy;
            a.gyro.z = gz;
            a.accel.x = ax;
            a.accel.y = ay;
            a.accel.z = az;
            a.magno.x = mx;
            a.magno.y = my;
            a.magno.z = mz;
        };

        var handleStateEstimate = function(data){
            var [data_timestamp, oq1, oq2, oq3, oq4, avx, avy, avz, lat, lon, alt] =
                gcs.struct.Unpack("<Iffffffffff", data);
            var se = _this.state_estimate;
            se.timestamp = data_timestamp;
            se.orientation[0] = oq1;
            se.orientation[1] = oq2;
            se.orientation[2] = oq3;
            se.orientation[3] = oq4;
            se.angular_velocity.x = avx;
            se.angular_velocity.y = avy;
            se.angular_velocity.z = avz;
            se.latitude = lat;
            se.longitude = lon;
            se.altitude = alt;
        };

        var handleUBloxNAV = function(data){
            // TODO
            console.log(data);
        };

        registerMultipacket("ts_mpu9250_data", 20, handleMPU9250);
        registerMultipacket("ts_adis16405_data", 20, handleADIS16405);
        registerMultipacket("ts_state_estimate_data", 44, handleStateEstimate);
        registerMultipacket("ts_ublox_nav", 64, handleUBloxNAV);
        registerMultipacket("ts_state_estimate_data", 44, handleStateEstimate);

        // Register all generated multipacket ids
        for(i=0; i<packet_ids.length; i++){
            (function(id){
                gcs.registerPacket(id, function(data){
                    handleMultipacket(id, data);
                });
            })(packet_ids[i]);
        }

        gcs.registerPacket(CAN_MSG_ID_M3IMU_VERSION, versionParser(this));
    };
};

export default M3IMU;

