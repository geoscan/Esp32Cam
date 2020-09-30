#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import proto
import time


class Parser:
    HEALTH = {
            0: 'HEALTH_OK',
            1: 'HEALTH_WARNING',
            2: 'HEALTH_ERROR',
            3: 'HEALTH_CRITICAL'
    }

    MODES = {
            0: 'MODE_OPERATIONAL',
            1: 'MODE_INITIALIZATION',
            2: 'MODE_MAINTENANCE',
            3: 'MODE_SOFTWARE_UPDATE',
            7: 'MODE_OFFLINE'
    }

    def __init__(self, flags):
        self.flags = flags

    def bitWiseCheck(self, value):
        lines = []
        for key in self.flags:
            if value & key != 0:
                lines.append(self.flags[key])
        return lines

    def componentState(self, value):
        lines = []

        if value & (1 << 26) != 0:
            lines.append('RESTART')

        mode = (value >> 27) & 7
        if mode in Parser.MODES:
            lines.append(Parser.MODES[mode])

        health = (value >> 30) & 3
        if health in Parser.HEALTH:
            lines.append(Parser.HEALTH[health])

        return lines


class ActuatorsParser(Parser):
    FLAGS = {
            0x000001: 'INTERFACE_ERROR',
            0x000002: 'RPM_ERROR',
            0x000004: 'ENGINE_SYNC_LOSS',
            0x000008: 'ENGINE_MISMATCH',
            0x000010: 'SERVO_MISMATCH',
            0x000020: 'ENGINE_CRITICAL',
            0x000040: 'SERVO_CRITICAL',
            0x000080: 'ENGINE_LOSS',
            0x000100: 'SERVO_LOSS',
            0x000200: 'ACT_TEST_FAILED',
            0x000400: 'ATT_TEST_FAILED'
    }

    def __init__(self):
        super().__init__(ActuatorsParser.FLAGS)

    def match(self, field):
        return field.component.name == 'Actuators' and field.name == 'flags'

    def parse(self, value):
        return self.componentState(value) + self.bitWiseCheck(value)


class CBoardParser(Parser):
    FLAGS = {
            0x000001: 'BOARD_1_SWITCH_OC',
            0x000002: 'BOARD_2_SWITCH_OC',
            0x000004: 'BOARD_3_SWITCH_OC',
            0x000008: 'BOARD_4_SWITCH_OC',
            0x000010: 'BOARD_5_SWITCH_OC',
            0x000020: 'BOARD_6_SWITCH_OC',
            0x000040: 'BOARD_5VCH1_TOO_HIGH',
            0x000080: 'BOARD_5VCH1_TOO_LOW',
            0x000100: 'BOARD_5VCH2_TOO_HIGH',
            0x000200: 'BOARD_5VCH2_TOO_LOW',
            0x000400: 'BOARD_12V_TOO_HIGH',
            0x000800: 'BOARD_12V_TOO_LOW',
            0x001000: 'BOARD_TEMP_TOO_HIGH',
    }

    def __init__(self, flags):
        super().__init__(flags)

    def parse(self, value):
        return self.componentState(value) + self.bitWiseCheck(value)


class CBoard201MParser(CBoardParser):
    FLAGS_201M = {
            0x004000: 'BOARD_7V_SERVO_TOO_LOW',
            0x008000: 'BOARD_7V_SERVO_TOO_HIGH'
    }

    def __init__(self):
        super().__init__({**CBoardParser.FLAGS, **CBoard201MParser.FLAGS_201M})

    def match(self, field):
        return field.component.name.startswith('CBoard201M') and field.name == 'flags'


class CBoard301BaseParser(CBoardParser):
    FLAGS_301_BASE = {
            0x002000: 'BOARD_MAIN_V_TOO_LOW',
            0x004000: 'BOARD_MAIN_V_TOO_HIGH',
            0x008000: 'BOARD_BAT_RSVD_ERROR'
    }

    def __init__(self):
        super().__init__({**CBoardParser.FLAGS, **CBoard301BaseParser.FLAGS_301_BASE})

    def match(self, field):
        return field.component.name.startswith('CBoard301_Base') and field.name == 'flags'


class CBoard301MotorParser(CBoardParser):
    FLAGS_301_MOTOR = {
            0x000001: 'BOARD_MAIN_BAT_UNDERVOLTAGE',
            0x000002: 'BOARD_RSVD_BAT_UNDERVOLTAGE',
            0x000004: 'BOARD_INTERFACE_ERROR',
            0x000008: 'BOARD_MAIN_PWR_ERROR',
            0x000010: 'BOARD_RSVD_BAT_DISABLED'
    }

    def __init__(self):
        super().__init__(CBoard301MotorParser.FLAGS_301_MOTOR)

    def match(self, field):
        return field.component.name.startswith('CBoard301_Motor') and field.name == 'flags'


class CBoard425Parser(CBoardParser):
    FLAGS_425 = {
            0x002000: 'BOARD_42EXTRA_SWITCH',
            0x004000: 'BOARD_42EXTRA_FUSE',
            0x008000: 'BOARD_OLD_BATTERY'
    }

    def __init__(self):
        super().__init__({**CBoardParser.FLAGS, **CBoard425Parser.FLAGS_425})

    def match(self, field):
        return field.component.name.startswith('CBoard425') and field.name == 'flags'


class CometParser(Parser):
    FLAGS = {
            0x000001: 'FAILURE',
            0x000100: 'OVERHEATING',
            0x000200: 'EMI_PRESENT',
            0x000400: 'EMI_OVERFLOW',
            0x200000: 'FAN_ENABLED',
            0x400000: 'FAN_FAILURE'
    }

    def __init__(self):
        super().__init__(CometParser.FLAGS)

    def match(self, field):
        return field.component.name == 'Comet' and field.name == 'flags'

    def parse(self, value):
        return self.bitWiseCheck(value)


class FlightManagerParser(Parser):
    FLAGS = {
            0x000001: 'MISSION_ERROR',
            0x000002: 'LICENSE_ERROR',
            0x000004: 'LICENSE_EXPIRED',
            0x000008: 'LICENSE_INCORRECT',
            0x000010: 'LAUNCHES_SPENT',
            0x000020: 'ZONE_MISMATCH',
            0x000040: 'ZONE_NOT_FOUND',
            0x000080: 'ZONE_EXPIRED',
            0x000100: 'ZONE_SIGN_ERROR',
            0x000200: 'ZONE_VIOLATION',
            0x000400: 'ZONE_CHECK_ONGOING'
    }

    def __init__(self):
        super().__init__(FlightManagerParser.FLAGS)

    def match(self, field):
        return field.component.name == 'FlightManager' and field.name == 'flags'

    def parse(self, value):
        return self.componentState(value) + self.bitWiseCheck(value)


class GnssCapabilitiesParser(Parser):
    FLAGS = {
            0x00000001: 'NAVIGATIONAL',
            0x00000002: 'HIGH_PRECISION',
            0x00000004: 'RTK',
            0x00000008: 'HAS_GPS',
            0x00000010: 'HAS_GLONASS',
            0x00000020: 'HAS_GALILEO',
            0x00000040: 'HAS_BEIDOU',
            0x00000080: 'HAS_SBAS'
    }

    def __init__(self):
        super().__init__(GnssCapabilitiesParser.FLAGS)

    def match(self, field):
        return (field.component.type == 14
                and field.name in ('capabilities', 'features', 'actualCapabilities', 'defaultCapabilities'))

    def parse(self, value):
        return self.bitWiseCheck(value)


class GnssSystemsParser(Parser):
    FLAGS = {
            0x00000001: 'GPS',
            0x00000002: 'GLONASS',
            0x00000004: 'GALILEO',
            0x00000008: 'BEIDOU',
            0x00000010: 'SBAS',
            0x00000100: 'GPS L2',
            0x00000200: 'GLONASS L2',
            0x00000400: 'GALILEO E5',
            0x00000800: 'BEIDOU B2',
            0x00001000: 'QZSS L2'
    }

    def __init__(self):
        super().__init__(GnssSystemsParser.FLAGS)

    def match(self, field):
        return field.component.type == 14 and field.name == 'systems'

    def parse(self, value):
        return self.bitWiseCheck(value)


class LuaStatusParser(Parser):
    STATES = {
            0: 'NOT_INIT',
            1: 'LOAD',
            2: 'INIT',
            3: 'RUN',
            4: 'PAUSE',
            5: 'STOP',
            6: 'ERROR',
            7: 'FATAL_ERROR'
    }

    def __init__(self):
        super().__init__({})

    def match(self, field):
        return field.component.name == 'LuaScript' and field.name == 'status'

    def parse(self, value):
        if value in LuaStatusParser.STATES:
            return [LuaStatusParser.STATES[value]]
        else:
            return []


class ModemParser(Parser):
    FLAGS = {
            0x000004: 'NO_NETWORK',
            0x000008: 'NO_RECEPTION',
            0x000010: 'NETWORKS_COLLISION',
            0x000020: 'IN_OVERVOLTAGE',
            0x000040: 'IN_UNDERVOLTAGE',
            0x000080: 'OVERHEATING',
            0x000100: 'POWER_MISMATCH',
            0x000200: 'HIGH_SWR',
            0x000400: '3V3_OVERVOLTAGE',
            0x000800: '3V3_UNDERVOLTAGE',
            0x001000: '3V0_OVERVOLTAGE',
            0x002000: '3V0_UNDERVOLTAGE'
    }

    def __init__(self):
        super().__init__(ModemParser.FLAGS)

    def match(self, field):
        return field.component.name == 'Modem' and field.name == 'flags'

    def parse(self, value):
        return self.componentState(value) + self.modemMode(value) + self.bitWiseCheck(value)

    def modemMode(self, value):
        MODES = ['MASTER', 'SLAVE', 'REPEATER', 'LISTENER']
        return [MODES[value & 3]]


class ModemFrequencyParser:
    def __init__(self):
        pass

    def match(self, field):
        return field.component.name == 'Modem' and field.name == 'channel'

    def parse(self, value):
        if value is not None:
            return ['{:.2f} MHz'.format(867.75 + 0.5 * value)]
        else:
            return []


class SensorMonitorParser(Parser):
    FLAGS = {
            0x000001: 'ACCEL_FAILURE',
            0x000002: 'GYRO_FAILURE',
            0x000004: 'MAG_FAILURE',
            0x000008: 'MAG_OVERFLOW',
            0x000010: 'MAG_TABLE_MISSING',
            0x000020: 'STAT_BARO_FAILURE',
            0x000040: 'DIFF_BARO_FAILURE',
            0x000080: 'DIFF_BARO_OVERFLOW',
            0x000100: 'BARO_OVERHEATING',
            0x000200: 'BARO_UNDERHEATING',
            0x000400: 'BARO_HEATER_FAILURE',
            0x000800: 'ACCEL_CALIB',
            0x001000: 'GYRO_CALIB',
            0x002000: 'GYRO_CHECK',
            0x004000: 'MAG_CALIB',
            0x008000: 'MAG_CHECK',
            0x010000: 'BARO_CALIB',
            0x020000: 'IMU_OVERHEATING',
            0x040000: 'IMU_UNDERHEATING',
            0x080000: 'IMU_HEATER_FAILURE',
            0x100000: 'UAV_NOT_FIXED',
            0x200000: 'ACCEL_OFFSET_ERROR',
            0x400000: 'GYRO_OFFSET_ERROR',
            0x800000: 'GYRO_CALIB_ERROR'
    }

    def __init__(self):
        super().__init__(SensorMonitorParser.FLAGS)

    def match(self, field):
        return field.component.name == 'SensorMonitor' and field.name == 'flags'

    def parse(self, value):
        return self.componentState(value) + self.bitWiseCheck(value)


class UavMonitorParser1V4(Parser):
    FLAGS = {
            0x00000001: 'AIRSTART',
            0x00000002: 'DIRECT_CONTROL',
            0x00000004: 'ATTITUDE_CONTROL',
            0x00000008: 'GNSS_ERROR',
            0x00000010: 'ACCEL_ERROR',
            0x00000020: 'EEPROM_ERROR',
            0x00000040: 'LINK_ERROR',
            0x00000080: 'PHOTO_ERROR',
            0x00000100: 'GYRO_ERROR',
            0x00000200: 'ALT_ERROR',
            0x00000400: 'DEBUG_BUILD',
            0x00000800: 'PARAM_ERROR',
            0x00001000: 'MISSION_ERROR',
            0x00002000: 'NOT_READY',
            0x00004000: 'STORAGE_ERROR',
            0x00008000: 'SIMULATION',
            0x00010000: 'HEADING_CONTROL',
            0x00020000: 'HPGNSS_ERROR',
            0x00040000: 'ENGINE_ERROR',
            0x00080000: 'MAG_ERROR',
            0x00100000: 'AS_ERROR',
            0x00200000: 'FLASH_ERROR',
            0x00400000: 'RESTART_NEEDED',
            0x00800000: 'NO_GNSS_TIME',
            0x01000000: 'DIRECT_ATT_CTL',
            0x02000000: 'DEVICE_CONFLICT',
            0x04000000: 'RESTRICTED_ZONE',
            0x08000000: 'MAYDAY',
            0x10000000: 'LOGGER_ERROR',
            0x20000000: 'ATTITUDE_ERROR'
    }

    def __init__(self):
        super().__init__(UavMonitorParser1V4.FLAGS)

    def match(self, field):
        return (field.component.name == 'UavMonitor' and field.name == 'flags'
            and field.component.messenger.hub.version[1] <= 4)

    def parse(self, value):
        return self.bitWiseCheck(value)


class UavMonitorParser1V5(Parser):
    FLAGS = {
            0x000001: 'START_ALLOWED',
            0x000002: 'RESTART_REQUIRED',
            0x000004: 'SIMULATION',
            0x000008: 'DEBUGGING',
            0x000010: 'UNPROTECTED',
            0x000020: 'NO_TIMESYNC',
            0x000040: 'BUS_ERROR',
            0x000080: 'EXT_MEM_ERROR',
            0x000100: 'INT_MEM_ERROR',
            0x000200: 'EEPROM_ERROR',
            0x000400: 'PARAM_ERROR',
            0x000800: 'LOGGER_ERROR',
            0x001000: 'POWER_ERROR',
            0x002000: 'DEVICE_CONFLICT',
            0x004000: 'FS_ERROR',
            0x008000: 'BATTERY_NOT_READY',
            0x010000: 'LOGGER_TIMEOUT'
    }

    def __init__(self):
        super().__init__(UavMonitorParser1V5.FLAGS)

    def match(self, field):
        return (field.component.name == 'UavMonitor' and field.name == 'flags'
            and field.component.messenger.hub.version[1] in (5, 6))

    def parse(self, value):
        return self.componentState(value) + self.bitWiseCheck(value)


class UavMonitorModeParser:
    def __init__(self):
        pass

    def match(self, field):
        return field.component.name == 'UavMonitor' and field.name == 'mode'

    def parse(self, value):
        MODES = [
                'ROOT',
                'DISARMED',
                'IDLE',
                'TEST_ACTUATION',
                'TEST_PARACHUTE',
                'TEST_ENGINE',
                'PARACHUTE',
                'WAIT_FOR_LANDING',
                'LANDED',
                'CATAPULT',
                'PREFLIGHT',
                'ARMED',
                'TAKEOFF',
                'WAIT_FOR_GPS',
                'WIND_MEASURE',
                'MISSION',
                'ASCEND',
                'DESCEND',
                'RTL',
                'UNCONDITIONAL_RTL',
                'MANUAL_HEADING',
                'MANUAL_ROLL',
                'MANUAL_SPEED',
                'LANDING',
                'ON_DEMAND'
        ]
        if value < len(MODES):
            return [MODES[value]]
        else:
            return []


class UavMonitorTimeParser:
    def __init__(self):
        pass

    def match(self, field):
        return field.component.name == 'UavMonitor' and field.name == 'time'

    def parse(self, value):
        UNIX_TO_GPS = 315964800
        gmtime = time.gmtime(int(value / 1000000) + UNIX_TO_GPS)
        return ['{:02d}.{:02d}.{:04d} {:02d}:{:02d}:{:02d}.{:02d}'.format(gmtime.tm_mday, gmtime.tm_mon,
                gmtime.tm_year, gmtime.tm_hour, gmtime.tm_min, gmtime.tm_sec, value % 1000000)]


class UavMonitorEraseCommandParser(Parser):
    FLAGS = {
            0x00000001: 'ALL',
            0x00000002: 'EXTERNAL_MEMORY',
            0x00000004: 'PASSPORTS',
            0x00000008: 'PARAMS_CONST',
            0x00000010: 'PARAMS_VOLATILE',
            0x00000020: 'SENSOR_LOG',
            0x00000040: 'TERRAIN',
            0x00000080: 'FLIGHT_ZONE',
            0x00000100: 'IMU_TABLE',
            0x00000200: 'MAG_TABLE',
            0x00000400: 'FLIGHT_TASK'
    }

    def __init__(self):
        super().__init__(UavMonitorEraseCommandParser.FLAGS)

    def match(self, field):
        return field.component.name == 'UavMonitor' and field.name == 'eraseCommand'

    def parse(self, value):
        helper = [
                'ALL               1',
                'PASSPORTS         4',
                'SECURE PARAMS     8',
                'COMMON PARAMS    16',
                'IMU TABLE       256',
                'MAG TABLE       512',
                'FLIGHT TASK    1024']
        return ([] if value else helper) + self.bitWiseCheck(value)


class UbloxVersionParser:
    def __init__(self):
        pass

    def match(self, field):
        return field.component.name == 'Ublox' and field.name in ('version', 'actualVersion')

    def parse(self, value):
        VERSIONS = [
				'UNKNOWN',
				'UBLX9_HPG',
				'UBLX8_SPG',
				'UBLX8_HPG',
				'UBLX8_ADR',
				'UBLX8_UDR',
				'UBLX8_FTS',
				'UBLX8_TIM',
				'UBLX6_60_UP_TO_73'
        ]
        if value < len(VERSIONS):
            return [VERSIONS[value]]
        else:
            return []


class DfuEraseHelper:
    def __init__(self):
        pass

    def match(self, field):
        return field.component.name.startswith('DFU') and field.name == 'erase'

    def parse(self, value):
        return [
                'FW      0x53F9B66E',
                'SPEC    0x95DBF48B',
                'EEPROM  0x3AA528A1',
                'DFU     0xC5DBECEB']


class DfuRebootHelper:
    def __init__(self):
        pass

    def match(self, field):
        return field.name == 'reboot' and proto.Protocol.isIntType(field.type)

    def parse(self, value):
        return [
                'DFU    0x16CC1379',
                'Reset  0xCE551B1E']


class DefaultFlagParser(Parser):
    def __init__(self):
        super().__init__({})

    def match(self, field):
        return field.name == 'flags'

    def parse(self, value):
        return self.componentState(value)


parsers = [
        ActuatorsParser(),
        CBoard201MParser(),
        CBoard301BaseParser(),
        CBoard301MotorParser(),
        CBoard425Parser(),
        CometParser(),
        DfuEraseHelper(),
        DfuRebootHelper(),
        FlightManagerParser(),
        GnssCapabilitiesParser(),
        GnssSystemsParser(),
        ModemParser(),
        ModemFrequencyParser(),
        LuaStatusParser(),
        SensorMonitorParser(),
        UavMonitorEraseCommandParser(),
        UavMonitorParser1V4(),
        UavMonitorParser1V5(),
        UavMonitorModeParser(),
        UavMonitorTimeParser(),
        UbloxVersionParser(),
        DefaultFlagParser()]
