/******************************************************************************

 @file zb_lib.js

 @brief library of Zigbee Spec constants

 Group: WCS LPC

 ******************************************************************************
 $License: BSD3 2018 $

   Copyright (c) 2018, Texas Instruments Incorporated
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   *  Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

   *  Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   *  Neither the name of Texas Instruments Incorporated nor the names of
      its contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 $Release Name: ZStack Linux Gateway SDK ENG$
 $Release Date: May 07, 2018 $
 *****************************************************************************/

// define all possible binding combinations. 
// Each JavaScript "key" is define to be a "source" device
// Each array value is define to be "destination" devices
var POSSIBLE_BINDING_TABLE = {
    "OnOffSwitch": ["OnOffLight", "DimmableLight", "ColorLight"],
    "DoorLockController": ["DoorLock"]
};

// Status Enumerations
var STATUS = {
    SUCCESS: 0x00, 
    FAILURE: 0x01,
    NOT_AUTHORIZED: 0x7e,
    RESERVED_FIELD_NOT_ZERO: 0x7f,
    MALFORMED_COMMAND: 0x80,
    UNSUP_CLUSTER_COMMAND: 0x81,
    UNSUP_GENERAL_COMMAND: 0x82,
    UNSUP_MANUF_CLUSTER_COMMAND: 0x83,
    UNSUP_MANUF_GENERAL_COMMAND: 0x84,
    INVALID_FIELD: 0x85,
    UNSUPPORTED_ATTRIBUTE: 0x86,
    INVALID_VALUE: 0x87,
    READ_ONLY: 0x88,
    INSUFFICIENT_SPACE: 0x89,
    DUPLICATE_EXISTS: 0x8a,
    NOT_FOUND: 0x8b,
    UNREPORTABLE_ATTRIBUTE: 0x8c,
    INVALID_DATA_TYPE: 0x8d,
    INVALID_SELECTOR: 0x8e,
    WRITE_ONLY: 0x8f,
    INCONNSISTENT_STARTUP_STATE: 0x90,
    DEFINED_OUT_OF_BAND: 0x91,
    INCONSISTENT: 0x92,
    ACTION_DENIED: 0x93,
    TIMEOUT: 0x94,
    ABORT: 0x95,
    INVALID_IMAGE: 0x96,
    WAIT_FOR_DATA: 0x97,
    NO_IMAGE_AVAILABLE: 0x98,
    REQUIRE_MORE_IMAGE: 0x99,
    NOTIFICATION_PENDING: 0x9a,
    HARDWARE_FAILURE: 0xc0,
    SOFTWARE_FAILURE: 0xc1,
    CALIBRATION_ERROR: 0xc2,
    UNSUPPOPRTED_CLUSTER: 0xc3,
    LIMIT_REACHED: 0xc4,
    name: {
        0: "success",
        1: "failure",
        126: "not_authorized",
        127: "reserved_field_not_zero",
        128: "malformed_command",
        129: "unsup_cluster_command",
        130: "unsup_general_command",
        131: "unsup_manuf_general_command",
        132: "unsup_manuf_general_command",
        133: "invalid_field",
        134: "unsupported_attribute",
        135: "invalid_value",
        136: "read_only",
        137: "insufficient_space",
        138: "duplicate_exists",
        139: "not_found",
        140: "unreportable_attribute",
        141: "invalid_data_type",
        142: "invalid_selector",
        143: "write_only",
        144: "inconnsistent_startup_state",
        145: "defined_out_of_band",
        146: "inconsistent",
        147: "action_denied",
        148: "timeout",
        149: "abort",
        150: "invalid_image",
        151: "wait_for_data",
        152: "no_image_available",
        153: "require_more_image",
        154: "notification_pending",
        155: "hardware_failure",
        156: "software_failure",
        157: "calibration_error",
        158: "unsuppoprted_cluster"
    }
};

// Data Types
var DATA_TYPE = {
    NODATA: 0x00,
    DATA8: 0x08,
    DATA16: 0x09,
    DATA24: 0x0a,
    DATA32: 0x0b,
    DATA40: 0x0c,
    DATA48: 0x0d,
    DATA56: 0x0e,
    DATA64: 0x0f,
    BOOL: 0x10,
    MAP8: 0x18,
    MAP16: 0x19,
    MAP24: 0x1a,
    MAP32: 0x1b,
    MAP40: 0x1c,
    MAP48: 0x1d,
    MAP56: 0x1e,
    MAP64: 0x1f,
    UINT8: 0x20,
    UINT16: 0x21,
    UINT24: 0x22,
    UINT32: 0x23,
    UINT40: 0x24,
    UINT48: 0x25,
    UINT56: 0x26,
    UINT64: 0x27,
    IINT8: 0x28,
    INT16: 0x29,
    INT24: 0x2a,
    INT32: 0x2b,
    INT40: 0x2c,
    INT48: 0x2d,
    INT56: 0x2e,
    INT64: 0x2f,
    ENUM8: 0x30,
    ENUM16: 0x31,
    SEMI: 0x38,
    SINGLE: 0x39,
    DOUBLE: 0x3a,
    OCTSTR: 0x41,
    STRING: 0x42,
    OCTSTR16: 0x43,
    STRING16: 0x44,
    ARRAY: 0x48,
    STRUCT: 0x4c,
    SET: 0x50,
    BAG: 0x51,
    ToD: 0xe0,
    DATE: 0xe1,
    UTC: 0xe2,
    CLUSTERID: 0xe8,
    ATTRIBID: 0xe9,
    BACOID: 0xea,
    EUI164: 0xf0,
    KEY128: 0xf1,
    UNK: 0xff
};


// Zigbee Profile ID Table
var ZB_PROFILE_ID = {
    HOME_AUTOMATION: 0x0104,
    BUILDING_AUTOMATION: 0x0105,
    TELECOM_APPLICATIONS: 0x0107,
    HEALTH_CARE: 0x0108,
    SMART_ENERGY: 0x0109,
    RETAIL_SERVICES: 0x010A,
    LIGHT_LINK: 0xC05E
};


// Home Automation - Device ID Table
var HA_DEVICE_ID = {
    ON_OFF_SWITCH: 0x0000,
    LEVEL_CONTROL_SWITCH: 0x0001,
    ON_OFF_OUTPUT: 0x0002,
    LEVEL_CONTROLLABLE_OUTPUT: 0x0003,
    SCENE_SELECTOR: 0x0004,
    CONFIGURATION_TOOL: 0x0005,
    REMOTE_CONTROL: 0x0006,
    COMBINED_INTERFACE: 0x0007,
    RANGE_EXTENDER: 0x0008,
    MAINS_POWER_OUTLET: 0x0009,
    DOOR_LOCK: 0x000A,
    DOOR_LOCK_CONTROLLER: 0x000B,
    SIMPLE_SENSOR: 0x000C,
    CONSUMPTION_AWARENESS_DEVICE: 0x000D,
    HOME_GATEWAY: 0x0050,
    SMART_PLUG: 0x0051,
    WHITE_GOODS: 0x0052,
    METER_INTERFACE: 0x0053,
    ON_OFF_LIGHT: 0x0100,
    DIMMABLE_LIGHT: 0x0101,
    COLOR_DIMMABLE_LIGHT: 0x0102,
    ON_OFF_LIGHT_SWITCH: 0x0103,
    DIMMER_SWITCH: 0x0104,
    COLOR_DIMMER_SWITCH: 0x0105,
    LIGHT_SENSOR: 0x0106,
    OCCUPANCY_SENSOR: 0x0107,
    ON_OFF_BALLAST: 0x0108,
    DIMMABLE_BALLAST: 0x0109,
    ON_OFF_PLUG_IN_UNIT: 0x010a,
    DIMMABLE_PLUG_IN_UNIT: 0x010b,
    COLOR_TEMPERATURE_LIGHT: 0x010c,
    EXTENDED_COLOR_LIGHT: 0x010d,
    LIGHT_LEVEL_SENSOR: 0x010e,
    COLOR_CONTROLLER: 0x8000,
    NON_COLOR_CONTROLLER: 0x0820,
    NON_COLOR_SCENE_CONTROLLER: 0x0830,
    CONTROL_BRIDGE: 0x0840,
    ON_OFF_SENSOR: 0x0850,
    SHADE: 0x0200,
    SHADE_CONTROLLER: 0x0201,
    WINDOW_COVERING_DEVICE: 0x0202,
    WINDOW_COVERING_CONTROLLER: 0x0203,
    HEATING_COOLING_UNIT: 0x0300,
    THERMOSTAT: 0x0301,
    TEMPERATURE_SENSOR: 0x0302,
    PUMP: 0x0303,
    PUMP_CONTROLLER: 0x0304,
    PRESSURE_SENSOR: 0x0305,
    FLOW_SENSOR: 0x0306,
    MINI_SPLIT_AC: 0x0307,
    IAS_CONTROL_AND_INDICATING_EQUIPMENT: 0x0400,
    IAS_ANCILLARY_CONTROL_EQUIPMENT: 0x0401,
    IAS_ZONE: 0x0402,
    IAS_WARNING_DEVICE: 0x0403 
};


// Home Automation - Cluster ID Table
var HA_CLUSTER_ID = {
    BASIC: 0x0000,
    POWER_CONFIGURATION: 0x0001,
    DEVICE_TEMPERATURE_CONFIGURATION: 0x0002,
    IDENTIFY: 0x0003,
    GROUPS: 0x0004,
    SCENES: 0x0005,
    ON_OFF: 0x0006,
    ON_OFF_SWITCH_CONFIGURATION: 0x0007,
    LEVEL_CONTROL: 0x0008,
    ALARMS: 0x0009,
    TIME: 0x000A,
    BINARY_INPUT: 0x000F,
    PARTITION: 0x0016,
    POWER_PROFILE: 0x001A,
    EN50523_APPLIANCE_CONTROL: 0x001B,
    POLL_CONTROL: 0x0020,
    SHADE_CONFIGURATION: 0x0100,
    DOOR_LOCK: 0x0101,
    WINDOW_COVERING: 0x0102,
    PUMP_CONFIGURATION_AND_CONTROL: 0x0200,
    THERMOSTAT: 0x0201,
    FAN_CONTROL: 0x0202,
    THERMOSTAT_USER_INTERFACE_CONFIGURATION: 0x0204,
    COLOR_CONTROL: 0x0300,
    ILLUMINANCE_MEASUREMENT: 0x0400,
    ILLUMINANCE_LEVEL_SENSING: 0x0401,
    TEMPERATURE_MEASUREMENT: 0x0402,
    PRESSURE_MEASUREMENT: 0x0403,
    FLOW_MEASUREMENT: 0x0404,
    RELATIVE_HUMIDITY_MEASUREMENT: 0x0405,
    OCCUPANCY_SENSING: 0x0406,
    IAS_ZONE: 0x0500,
    IAS_ACE: 0x0501,
    IAS_WD: 0x0502,
    METERING: 0x0702,
    EN50523_APPLIANCE_IDENTIFICATION: 0x0B00,
    METER_IDENTIFICATION: 0x0B01,
    EN50523_APPLIANCE_EVENTS_AND_ALERT: 0x0B02,
    APPLIANCE_STATISTICS: 0x0B03,
    ELECTRICITY_MEASUREMENT: 0x0B04,
    DIAGNOSTICS: 0x0B05
};


// Zigbee Light Link - Device ID Table
var ZLL_DEVICE_ID = {
    ON_OFF_LIGHT: 0x0000,
    ON_OFF_PLUG_IN_UNIT: 0x0010,
    DIMMABLE_LIGHT: 0x0100,
    DIMMABLE_PLUG_IN_UNIT: 0x0110,
    COLOR_LIGHT: 0x0200,
    EXTENDED_COLOR_LIGHT: 0x0210,
    COLOR_TEMPERATURE_LIGHT: 0x0220,
    COLOR_CONTROLLER: 0x0800,
    COLOR_SCENE_CONTROLLER: 0x0810,
    NON_COLOR_CONTROLLER: 0x0840,
    NON_COLOR_SCENE_CONTROLLER: 0x0830,
    CONTROL_BRIDGE: 0x0840,
    ON_OFF_SENSOR: 0x850
};


// Smart Energy - Device ID Table
var SE_DEVICE_ID = {
    RANGE_EXTENDER: 0x0008,
    ENERGY_SERVICE_INTERFACE: 0x0500,
    METERING_DEVICE: 0x0501,
    IN_HOME_DISPLAY: 0x0502,
    PROGRAMMABLE_COMMUNICATING_THERMOSTAT: 0x0503,
    LOAD_CONTROL_DEVICE: 0x0504,
    SMART_APPLIANCE: 0x0505,
    PREPAYMENT_TERMINAL: 0x0506,
    PHYSICAL_DEVICE: 0x0507,
    REMOTE_COMMUNICATIONS_DEVICE: 0x0508,
    ERL_INTERFACE: 0x0509
};


// Smart Energy - Cluster ID Table
var SE_CLUSTER_ID = {
    BASIC: 0x0000,
    POWER_CONFIGURATION: 0x0001,
    IDENTIFY: 0x0003,
    ALARMS: 0x0009,
    TIME: 0x000A,
    COMMISSIONING: 0x0015,
    KEY_ESTABLISHMENT: 0x0800,
    KEEP_ALIVE: 0x0025,
    PRICE: 0x0700,
    DEMAND_RESPONSE_AND_LOAD_CONTROL: 0x0701,
    METERING: 0x0702,
    MESSAGING: 0x0703,
    SMART_ENERGY_TUNNELING: 0x0704,
    PREPAYMENT: 0x0705,
    ENERGY_MANAGEMENT: 0x0706,
    CALENDAR: 0x0707,
    DEVICE_MANAGEMENT: 0x0708,
    EVENTS: 0x0709,
    MDU_PAIRING: 0x070A,
    SUB_GHZ: 0x070B
};


// Building Automation - Device ID Table
var BA_DEVICE_ID = {
    ON_OFF_SWITCH: 0x0000,
    LEVEL_CONTROL_SWITCH: 0x0001,
    ON_OFF_OUTPUT: 0x0002,
    LEVEL_CONTROLLABLE_OUTPUT: 0x0003,
    SCENE_SELECTOR: 0x0004,
    CONFIGURATION_TOOL: 0x0005,
    REMOTE_CONTROL: 0x0006,
    COMBINED_INTERFACE: 0x0007,
    RANGE_EXTENDER: 0x0008,
    MAINS_POWER_OUTLET: 0x0009,
    CONSTRUCTED_BACNET_DEVICE: 0x000A,
    BACNET_TUNNELED_DEVICE: 0x000B,
    ON_OFF_LIGHT: 0x0100,
    DIMMABLE_LIGHT: 0x0101,
    COLOR_DIMMABLE_LIGHT: 0x0102,
    ON_OFF_LIGHT_SWITCH: 0x0103,
    DIMMER_SWITCH: 0x0104,
    COLOR_DIMMER_SWITCH: 0x0105,
    LIGHT_SENSOR: 0x0106,
    OCCUPANCY_SENSOR: 0x0107,
    ON_OFF_BALLAST: 0x0108,
    DIMMABLE_BALLAST: 0x0109,
    SHADE: 0x0200,
    SHADE_CONTROLLER: 0x0201,
    THERMOSTAT: 0x0301,
    TEMPERATURE_SENSOR: 0x0302,
    PUMP: 0x0303,
    PUMP_CONTROLLER: 0x0304,
    PRESSURE_SENSOR: 0x0305,
    FLOW_SENSOR: 0x0306,
    HUMIDITY_SENSOR: 0x0307,
    IAS_CONTROL_AND_INDICATING_EQUIPMENT: 0x0400,
    IAS_ANCILLARY_CONTROL_EQUIPMENT: 0x0401,
    IAS_ZONE: 0x0402,
    IAS_WARNING_DEVICE: 0x0403
};


// Telecom Applications - Device ID Table
var TA_DEVICE_ID = {
    ZSIM: 0x0000,
    ZMT: 0x0001,
    CONFIGURATION_TOOL: 0x0005,
    RANGE_EXTENDER: 0x0008,
    ZAP: 0x0100,
    ZIN: 0x0101,
    ZIT: 0x0102,
    POINT_OF_SALE: 0x0200,
    TICKETING_MACHINE: 0x0201,
    PAY_CONTROLLER: 0x0202,
    BILLING_UNIT: 0x0203,
    CHARGING_UNIT: 0x0204,
    ZIGBEE_FLASH_CARD: 0x0300,
    ZIGBEE_PC_SMART_CARD_READER: 0x0301,
    ZIGBEE_HEADSET: 0x0400,
    ZIGBEE_MICROPHONE: 0x0401,
    ZIGBEE_SPEAKER: 0x0402,
    RAN: 0x0500,
    RLN: 0x0501,
    RLG: 0x0502,
    CHATTING_UNIT: 0x0600,
    CHATTING_STATION: 0x0601
};


// Zigbee Telecom Applications - Cluster ID Table
var TA_CLUSTER_ID = {
    BASIC: 0x0000,
    IDENTIFY: 0x0003,
    GROUPS: 0x0004,
    ON_OFF: 0x0006,
    COMMISSIONING: 0x0015,
    PARTITION: 0x0016,
    RSSI_LOCATION: 0x000B,
    ALPHA_SECURE_KEY_ESTABLISHMENT: 0x0017,
    ALPHA_SECURE_ACCESS_CONTROL: 0x0018,
    ISO7816_PROTOCOL_TUNNEL: 0x0615,
    INFORMATION: 0x0900,
    DATA_SHARING: 0x0901,
    GAMING: 0x0902,
    DATA_RATE_CONTROL: 0x0903,
    VOICE_OVER_ZIGBEE: 0x0904,
    CHATTING: 0x0905,
    PAYMENT: 0x0A01,
    BILLING: 0x0A02
};


// Zigbee Retail Services - Device ID Table
var ZRS_DEVICE_ID = {
    HHD: 0x0030,
    ISC: 0x0032,
    ESL: 0x0033,
    CIP: 0x0034,
    CC: 0x0035,
    ZIGBEE_MOBILE_TERMINAL: 0x0021,
    CONFIGURATION_TOOL: 0x0005,
    RANGE_EXTENDER: 0x0008,
    ZAP: 0x0120,
    ZIN: 0x0121,
    ZIT: 0x0122,
    POS: 0x0220,
    RAN: 0x0500,
    RLN: 0x0501,
    RLG: 0x0502
};


// Zigbee Retail Services - Cluster ID Table
var ZRS_CLUSTER_ID = {
    BASIC: 0x0000,
    IDENTIFY: 0x0003,
    GROUPS: 0x0004,
    RSSI_LOCATION: 0x000B,
    COMMISSIONING: 0x0015,
    TEMPERATURE_MEASUREMENT: 0x0402,
    OVER_THE_AIR_UPGRADE_NEW: 0x0019,
    GENERIC_TUNNEL: 0x0600,
    ISO7816_PROTOCOL_TUNNEL: 0x0615,
    INFORMATION: 0x0900,
    PAYMENT: 0x0A01,
    RETAIL_TUNNEL: 0x0617,
    MOBILE_DEVICE_CONFIGURATION_CLUSTER: 0x0022,
    NEIGHBOR_CLEANING_CLUSTER: 0x0023,
    NEAREST_GATEWAY_CLUSTER: 0x0024
};


// Zigbee Health Care - Device ID Table
var HC_DEVICE_ID = {
    DCU: 0x0000,
    GENERIC_MALFUNCTION_HEALTHCARE_DEVICE: 0x0F00,
    PULSE_OXIMETER: 0x1004,
    ECG: 0x1006,
    BLOOD_PRESSURE_MONITOR: 0x1007,
    THERMOMETER: 0x1008,
    WEIGHT_SCALE: 0x100F,
    GLUCOSE_METER: 0x1011,
    INR: 0x1012,
    INSULIN_PUMP: 0x1013,
    PEAK_FLOW_MONITOR: 0x1015,
    CARDIOVASCULAR_FITNESS_AND_ACTIVITY_MONITOR: 0x1029,
    STRENGTH_FITNESS_EQUIPMENT: 0x102A,
    PHYSICAL_ACTIVITY_MONITOR: 0x102B,
    STEP_COUNTER: 0x1068,
    ILAH: 0x1047,
    ADHERENCE_MONITOR: 0x1048,
    FALL_SENSOR: 0x1075,
    PERS_SENSOR: 0x1076,
    SMOKE_SENSOR: 0x1077,
    CO_SENSOR: 0x1078,
    WATER_SENSOR: 0x1079,
    GAS_SENSOR: 0x107A,
    MOTION_SENSOR: 0x107B,
    PROPERTY_EXIT_SENSOR: 0x107C,
    ENURESIS_SENSOR: 0x107D,
    CONTACT_CLOSURE_SENSOR: 0x107E,
    USAGE_SENSOR: 0x107F,
    SWITCH_USAGE_SENSOR: 0x1080,
    DOSAGE_SENSOR: 0x1081,
    TEMPERATURE_SENSOR: 0x1082
};



// Zigbee Health Care - Cluster ID Table
var HC_CLUSTER_ID = {
    BASIC: 0x0000,
    POWER_CONFIGURATION: 0x0001,
    IDENTIFY: 0x0003,
    ALARMS: 0x0009,
    TIME: 0x000A,
    RSSI_LOCATION: 0x000B,
    COMMISSIONING: 0x0015,
    PARTITION: 0x0016,
    ALPHA_SECURE_KEY_ESTABLISHMENT: 0x0017,
    ALPHA_SECURE_ACCESS_CONTROL: 0x0018,
    GENERIC_TUNNEL: 0x0600,
    PROTOCOL_TUNNEL_11073: 0x0614,
    VOICE_OVER_ZIGBEE: 0x0904
};


// Cluster List (CL)
var CL = {
    BASIC: 0,
    POWER_CONFIGURATION: 1,
    DEVICE_TEMPERATURE_CONFIGURATION: 2,
    IDENTIFY: 3,
    GROUPS: 4,
    SCENES: 5,
    ON_OFF: 6,
    ON_OFF_SWITCH_CONFIGURATION: 7,
    LEVEL_CONTROL: 8,
    ALARMS: 9,
    TIME: 10,
    RSSI_LOCATION: 11,
    DIAGNOSTICS: 2821,
    POLL_CONTROL: 32,
    POWER_PROFILE: 26, 
    METER_IDENTIFICATION: 2817,
    ANALOG_INPUT: 12, 
    ANALOG_OUTPUT: 13, 
    ANALOG_VALUE: 14, 
    BINARY_INPUT: 15, 
    BINARY_OUTPUT: 16, 
    BINARY_VALUE: 17, 
    MULTISTATE_INPUT: 18, 
    MULTISTATE_OUTPUT: 19, 
    MULTISTATE_VALUE: 20,
    ILLUMINANCE_MEASUREMENT: 1024,
    ILLUMINANCE_LEVEL_SENSING: 1025, 
    TEMPERATURE_MEASUREMENT: 1026,
    PRESSURE_MEASUREMENT: 1027,
    FLOW_MEASUREMENT: 1028,
    RELATIVE_HUMIDITY_MEASUREMENT: 1029,
    OCCUPANCY_SENSING: 1030,
    ELECTRICAL_MEASUREMENT: 2820,
    PUMP_CONFIGURATION_AND_CONTROL: 512,
    THERMOSTAT: 513, 
    FAN_CONTROL: 514,
    DEHUMIDIFICATION_CONTROL: 515,
    THERMOSTAT_USER_INTERFACE_CONFIGURATION: 516,

    cluster: {
        0: "basic",
        1: "power_configuration",
        2: "device_temperature_configuration",
        3: "identify",
        4: "groups",
        5: "scenes",
        6: "on_off",
        7: "on_off_switch_configuration",
        8: {
            name: "level_control",
                attributeId: {
                    0: "current_level",
                    1: "remaining_time",
                    16: "on_off_transition_time",
                    17: "on_level",
                    18: "on_transition_time",
                    19: "off_transition_time",
                    20: "default_move_rate"
                }
            },
        9: "alarms",
        10: "time",
        11: "rssi_location",
        2821: "diagnostics",
        32: "poll_control",
        26: "power_profile",
        2817: "meter_identification",
        12: "analog_input",
        13: "analog_output",
        14: "analog_value",
        15: "binary_input",
        16: "binary_output",
        17: "binary_value",
        18: "multistate_input",
        19: "multistate_output",
        20: "multistate_value",
        1024: "illuminance_measurement",
        1025: "illuminance_level_sensing", 
        1026: "temperature_measurement",
        1027: "pressure_measurement",
        1028: "flow_measurement",
        1029: "relative_humidity_measurement",
        1030: "occupancy_sensing",
        2820: "electrical_measurement",
        512: "pump_configuration_and_control",
        513: "thermostat", 
        514: "fan_control",
        515: "dehumidification_control",
        516: "thermostat_user_interface_configuration"
    }   
};


var ON_OFF_CLUSTER = {
    OnOff: 0x0000,
    GlobalSceneControl: 0x4000,
    OnTime: 0x4001,
    OffWaitTime: 0x4002
};

var ON_OFF_CLUSTER_OnOff = {
    OFF: 0,
    ON: 1
};

// Status Enumerations
var LEVEL_CONTROL_CLUSTER = {
    CURRENT_LEVEL: 0,
    REMAINING_TIME: 1, 
    ON_OFF_TRANSITION_TIME: 16,
    ON_LEVEL: 17,
    ON_TRANSITION_TIME: 18,
    OFF_TRANSITION_TIME: 19,
    DEFAULT_MOVE_RATE: 20,

    name: {
        0: "current_level",
        1: "remaining_time",
        16: "on_off_transition_time",
        17: "on_level",
        18: "on_transition_time",
        19: "off_transition_time",
        20: "default_move_rate"
    }
};

var TEMPERATURE_MEASUREMENT_CLUSTER = {
    MeasuredValue: 0x0000,
    MinMeasuredValue: 0x0001,
    MaxMeasuredValue: 0x0002,
    Tolerance: 0x0003
};

var THERMOSTAT_CLUSTER = {
    LocalTemperature: 0x0000,
    OutdoorTemperature: 0x0001,
    Occupancy: 0x0002,
    AbsMinHeatSetpointLimit: 0x0003,
    AbsMaxHeatSetpointLimit: 0x0004,
    AbsMinCoolSetpointLimit: 0x0005,
    AbsMaxCoolSetpointLimit: 0x0006,
    PICoolingDemand: 0x0007,
    PIHeatingDemand: 0x0008,
    HVACSystemTypeConfiguration: 0x0009,
    LocalTemperatureCalibration: 0x0010,
    OccupiedCoolingSetpoint: 0x0011,
    OccupiedHeatingSetpoint: 0x0012,
    UnoccupiedCoolingSetpoint: 0x0013,
    UnoccupiedHeatingSetpoint: 0x0014,
    MinHeatSetpointLimit: 0x0015,
    MaxHeatSetpointLimit: 0x0016,
    MinCoolSetpointLimit: 0x0017,
    MaxCoolSetpointLimit: 0x0018,
    MinSetpointDeadBand: 0x0019,
    RemoteSensing: 0x001a,
    ControlSequenceOfOperation: 0x001b,
    SystemMode: 0x001c,
    AlarmMask: 0x001d,
    ThermostatRunningMode: 0x001e
};

var DOOR_LOCK_CLUSTER = {
    LockState: 0x0000,
    LockType: 0x0001,
    ActuatorEnabled: 0x0002,
    DoorState: 0x0003,
    DoorOpenEvents: 0x0004,
    DoorClosedEvents: 0x0005,
    OpenPeriod: 0x006
};

var DOOR_LOCK_CLUSTER_LockState = {
    NOT_FULLY_LOCKED: 0x00,
    LOCKED: 0x01,
    UNLOCKED: 0x02,
    UNDEFINED: 0xFF
};

var DOOR_LOCK_CLUSTER_LockType = {
    DEAD_BOLT: 0x00,
    MAGNETIC: 0x01,
    OTHER: 0x02,
    MORTISE: 0x03,
    RIM: 0x04,
    LATCH_BOLT: 0x05,
    CYLINDRICAL_LOCK: 0x06,
    TUBULAR_LOCK: 0x07,
    INTERCONNECTED_LOCK: 0x08,
    DEAD_LATCH: 0x09,
    DOOR_FURNITURE: 0x0a
};

var DOOR_LOCK_CLUSTER_ActuatorEnabled = {
    DISABLED: 0,
    ENABLED: 1
};

var DOOR_LOCK_CLUSTER_DoorState = {
    OPEN: 0x00,
    CLOSED: 0x01,
    ERROR_JAMMED: 0x02,
    ERROR_FORCED_OPEN: 0x03,
    ERROR_UNSPECIFIED: 0x04,
    UNDEFINED: 0xff
};

var DOOR_LOCK_CLUSTER_LockMode = {
    LOCK_MODE_LOCK: 0,
    LOCK_MODE_UNLOCK: 1
};


module.exports.POSSIBLE_BINDING_TABLE= POSSIBLE_BINDING_TABLE;
module.exports.STATUS = STATUS;
module.exports.DATA_TYPE = DATA_TYPE;
module.exports.ZB_PROFILE_ID = ZB_PROFILE_ID;
module.exports.HA_DEVICE_ID = HA_DEVICE_ID;
module.exports.HA_CLUSTER_ID = HA_CLUSTER_ID;
module.exports.ZLL_DEVICE_ID = ZLL_DEVICE_ID;
module.exports.SE_DEVICE_ID = SE_DEVICE_ID;
module.exports.SE_CLUSTER_ID = SE_CLUSTER_ID;
module.exports.BA_DEVICE_ID = BA_DEVICE_ID;
module.exports.TA_DEVICE_ID = TA_DEVICE_ID;
module.exports.TA_CLUSTER_ID = TA_CLUSTER_ID;
module.exports.ZRS_DEVICE_ID = ZRS_DEVICE_ID;
module.exports.ZRS_CLUSTER_ID = ZRS_CLUSTER_ID;
module.exports.HC_DEVICE_ID = HC_DEVICE_ID;
module.exports.HC_CLUSTER_ID = HC_CLUSTER_ID;
module.exports.CL = CL;
module.exports.ON_OFF_CLUSTER = ON_OFF_CLUSTER;
module.exports.ON_OFF_CLUSTER_OnOff = ON_OFF_CLUSTER_OnOff;
module.exports.LEVEL_CONTROL_CLUSTER = LEVEL_CONTROL_CLUSTER;
module.exports.TEMPERATURE_MEASUREMENT_CLUSTER = TEMPERATURE_MEASUREMENT_CLUSTER;
module.exports.THERMOSTAT_CLUSTER = THERMOSTAT_CLUSTER;
module.exports.DOOR_LOCK_CLUSTER = DOOR_LOCK_CLUSTER;
module.exports.DOOR_LOCK_CLUSTER_LockState = DOOR_LOCK_CLUSTER_LockState;
module.exports.DOOR_LOCK_CLUSTER_LockType = DOOR_LOCK_CLUSTER_LockType;
module.exports.DOOR_LOCK_CLUSTER_ActuatorEnabled = DOOR_LOCK_CLUSTER_ActuatorEnabled;
module.exports.DOOR_LOCK_CLUSTER_DoorState = DOOR_LOCK_CLUSTER_DoorState;
module.exports.DOOR_LOCK_CLUSTER_LockMode = DOOR_LOCK_CLUSTER_LockMode;