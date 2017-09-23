import { msg_id, bool, versionParser } from './utils.js';

const CAN_ID_M3RADIO = 4;
const CAN_MSG_ID_M3RADIO_STATUS = CAN_ID_M3RADIO | msg_id(0);
const CAN_MSG_ID_M3RADIO_GPS_LATLNG = CAN_ID_M3RADIO | msg_id(48);
const CAN_MSG_ID_M3RADIO_GPS_ALT = CAN_ID_M3RADIO | msg_id(49);
const CAN_MSG_ID_M3RADIO_GPS_TIME = CAN_ID_M3RADIO | msg_id(50);
const CAN_MSG_ID_M3RADIO_GPS_STATUS = CAN_ID_M3RADIO | msg_id(51);
const CAN_MSG_ID_M3RADIO_PACKET_COUNT = CAN_ID_M3RADIO | msg_id(53);
const CAN_MSG_ID_M3RADIO_PACKET_STATS = CAN_ID_M3RADIO | msg_id(54);
const CAN_MSG_ID_M3RADIO_PACKET_PING = CAN_ID_M3RADIO | msg_id(55);
const CAN_MSG_ID_M3RADIO_VERSION = CAN_ID_M3RADIO | msg_id(63);

class M3Radio {
    constructor(gcs) {
        const components = {
            1: "uBlox",
            2: "Si4460",
            3: "GPS Antenna",
            4: "Packet Processor",
        };

        const component_errors = {
            0: "No Error",
            1: "uBlox Checksum", 2: "uBlox Timeout", 3: "uBlox UART",
            4: "uBlox Config", 5: "uBlox Decode", 6: "uBlox Flight Mode",
            7: "uBlox NAK",
            8: "Si4460 Config"
        };

        // State
        this.status = {overall: 0, components: {}};
        for(var i in components){
            var c = components[i];
            this.status.components[c] = {state: 0, reason: "Unknown"};
        }

        this.gps = {
            lat: 0.0,
            lon: 0.0,
            alt: 0.0,
            alt_msl: 0.0,
            date: null,
            date_valid: false,
            fix_type: "INVAL",
            flags: 0,
            num_satellites: 0,
        };

        this.stats = {
            txcount: 0,
            rxcount: 0,
            rssi: 0,
            freq_offset: 0,
            bit_errors: 0,
            ldpc_iters: 0,
        };

        this.version = "UNKNOWN";

        var _this = this;

        // Packet parsers
        gcs.registerPacket(CAN_MSG_ID_M3RADIO_STATUS, function(data){
            var [overall, comp, comp_state, comp_error] = gcs.struct.Unpack("BBBB", data);
            var c = components[comp];
            var c_status = _this.status.components[c];
            c_status.state = comp_state;
            c_status.reason = component_errors[comp_error];

            _this.status.overall = ["OK", "INIT", "ERROR"][overall];
        });

        gcs.registerPacket(CAN_MSG_ID_M3RADIO_GPS_LATLNG, function(data){
            var [lat, lon] = gcs.struct.Unpack("<ii", data);
            _this.gps.lat = lat / 1e7;
            _this.gps.lon = lon / 1e7;
        });

        gcs.registerPacket(CAN_MSG_ID_M3RADIO_GPS_ALT, function(data){
            var [height, h_msl] = gcs.struct.Unpack("<ii", data);
            _this.gps.alt = height / 1000;
            _this.gps.alt_msl = h_msl / 1000;
        });

        gcs.registerPacket(CAN_MSG_ID_M3RADIO_GPS_TIME, function(data){
            var [year, month, day, hour, minute, second, valid] =
                gcs.struct.Unpack("<HBBBBBB", data);
            _this.gps.date = new Date(year, month, day, hour, minute, second);
            _this.gps.date_valid = bool((valid & 0b111) === 0b111);
        });

        gcs.registerPacket(CAN_MSG_ID_M3RADIO_GPS_STATUS, function(data){
            var [fix_type, flags, num_sv] = gcs.struct.Unpack("BBB", data);
            var fix_types = {0: "No fix", 2: "2d fix", 3: "3d fix"};
            _this.gps.fix_type = fix_types[fix_type];
            _this.gps.flags = flags;
            _this.gps.num_satellites = num_sv;
        });

        gcs.registerPacket(CAN_MSG_ID_M3RADIO_PACKET_COUNT, function(data){
            var [txcount, rxcount] = gcs.struct.Unpack("<II", data);
            _this.stats.txcount = txcount;
            _this.stats.rxcount = rxcount;
        });

        gcs.registerPacket(CAN_MSG_ID_M3RADIO_PACKET_STATS, function(data){
            var [rssi, freqoff, biterrs, iters] = gcs.struct.Unpack("<hhHH", data);
            _this.stats.rssi = rssi;
            _this.stats.freq_offset = freqoff;
            _this.stats.bit_errors = biterrs;
            _this.stats.ldpc_iters = iters;
        });

        gcs.registerPacket(CAN_MSG_ID_M3RADIO_PACKET_PING, function(data){ });

        gcs.registerPacket(CAN_MSG_ID_M3RADIO_VERSION, versionParser(this));
    };
};

export default M3Radio;

