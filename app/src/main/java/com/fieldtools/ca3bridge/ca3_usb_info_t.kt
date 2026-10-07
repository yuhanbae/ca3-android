package com.fieldtools.ca3bridge

data class ca3_usb_info_t(
    var vid: Int = 0,
    var pid: Int = 0,
    var interface_number: Int = 0,
    var ep_in: Int = 0,
    var ep_out: Int = 0,
    var ep_in_max_packet: Int = 0,
    var ep_out_max_packet: Int = 0,
    var ep_in_type: Int = 0,
    var ep_out_type: Int = 0
)