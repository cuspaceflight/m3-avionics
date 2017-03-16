var M3Ground = function(gcs){
    var CAN_ID_GROUND = 7;
    var CAN_MSG_ID_GROUND_PACKET_COUNT = CAN_ID_GROUND | msg_id(53);
    var CAN_MSG_ID_GROUND_PACKET_STATS = CAN_ID_GROUND | msg_id(54);

    // State
    this.stats = {
        txcount: 0,
        rxcount: 0,
        rssi: 0,
        freq_offset: 0,
        bit_errors: 0,
        ldpc_iters: 0,
    };

    var _this = this;

    // Packet parsers
    gcs.registerPacket(CAN_MSG_ID_GROUND_PACKET_COUNT, function(data){
        var [txcount, rxcount] = gcs.struct.Unpack("<II", data);
        _this.stats.txcount = txcount;
        _this.stats.rxcount = rxcount;
    });

    gcs.registerPacket(CAN_MSG_ID_GROUND_PACKET_STATS, function(data){
        var [rssi, freqoff, biterrs, iters] = gcs.struct.Unpack("<hhHH", data);
        _this.stats.rssi = rssi;
        _this.stats.freq_offset = freqoff;
        _this.stats.bit_errors = biterrs;
        _this.stats.ldpc_iters = iters;
    });
};