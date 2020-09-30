#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import binascii
import struct
import time

import proto


class DefaultParser:
    def __init__(self, stream):
        self.stream = stream
        self.reset()

    @staticmethod
    def match(stream):
        return True

    @staticmethod
    def propose(stream=None):
        return {'chunk': 48, 'burst': 4, 'checksum': False}

    def reset(self):
        self.cache = None
        self.size = (0, 0)

    def info(self, size):
        if self.cache is None or self.size != size:
            data = self.stream.data
            checksum = binascii.crc32(data) & 0xFFFFFFFF
            self.size = size
            self.cache = [
                    'Size      {:d}'.format(len(data))[:size[0]],
                    'Checksum  {:08X}'.format(checksum)[:size[0]]]
        return self.cache

    def native(self, size):
        if self.cache is None or self.size != size:
            self.size = size
            self.cache = ['Viewer unavailable'[:size[0]]]
        return self.cache

    def raw(self, size):
        if self.cache is None or self.size != size:
            cache = []
            maxColumns = int(size[0] / 3) + (1 if size[0] % 3 == 2 else 0)

            if maxColumns >= 16:
                data = self.stream.data
                position = 0
                while position < len(data):
                    chunk = min(maxColumns, len(data) - position)
                    cache.append(' '.join(['{:02X}'.format(x) for x in data[position:position + chunk]]))
                    position += chunk

            self.size = size
            self.cache = cache
        return self.cache


class FlightManagerPointParser1V5(DefaultParser):
    def __init__(self, stream):
        DefaultParser.__init__(self, stream)

    @staticmethod
    def match(stream):
        if stream.component.name == 'FlightManager' and stream.index == 0:
            return stream.component.messenger.hub.version[1] <= 5
        else:
            return False

    @staticmethod
    def propose(stream=None):
        settings = DefaultParser.propose(stream)
        settings.update({'chunk': 48, 'checksum': True})
        return settings

    def native(self, size):
        POINT_TYPES = {
                0: 'HOME',
                1: 'LINE_START',
                2: 'LINE_END',
                3: 'POINT',
                4: 'LANDING',
                5: 'PARACHUTE',
                6: 'ROUTE',
                7: 'WAIT',
                8: 'WIND',
                9: 'KNOT',
                10: 'CIRCLE',
                11: 'WAVE',
                12: 'AUTO_LANDING'}

        if self.cache is None or self.size != size:
            self.size = size
            self.cache = []

            data = self.stream.data
            for i in range(0, int(len(data) / 16)):
                e = data[i * 16:(i + 1) * 16]

                lat = float(struct.unpack('<i', e[0:4])[0]) / 1e7
                lon = float(struct.unpack('<i', e[4:8])[0]) / 1e7
                alt = float(struct.unpack('<i', e[8:12])[0]) / 1e3
                flags, cmd = struct.unpack('<HH', e[12:16])[0:2]
                pointType = flags & 0x000F
                pointDuration = (flags & 0x0FF0) >> 4

                serializedPoint = '{:3d} [ {:+12.7f} {:+11.7f} {:10.3f} ] '.format(i, lat, lon, alt)
                serializedPoint += 'cmd {:5d} '.format(cmd)
                if pointDuration == 0xFF:
                    serializedPoint += 'dur infinite '
                else:
                    serializedPoint += 'dur {:6d} s '.format(int(pointDuration * (60 / 16)))
                serializedPoint += 'flags '
                serializedPoint += 'i' if flags & (1 << 15) != 0 else '-' # Undocumented
                serializedPoint += 'c' if flags & (1 << 14) != 0 else '-'
                serializedPoint += 's' if flags & (1 << 13) != 0 else '-'
                serializedPoint += 'l' if flags & (1 << 12) != 0 else '-'
                serializedPoint += ' type ' + POINT_TYPES[pointType] if pointType in POINT_TYPES else 'UNKNOWN'
                self.cache.append(serializedPoint[:size[0]])
        return self.cache


class FlightManagerPointParser1V6(DefaultParser):
    def __init__(self, stream):
        DefaultParser.__init__(self, stream)

    @staticmethod
    def match(stream):
        if stream.component.name == 'FlightManager' and stream.index == 0:
            return stream.component.messenger.hub.version[1] == 6
        else:
            return False

    @staticmethod
    def propose(stream=None):
        settings = DefaultParser.propose(stream)
        settings.update({'chunk': 51, 'checksum': False})
        return settings

    def native(self, size):
        POINT_TYPES = {
                0: 'HOME',
                1: 'LINE_START',
                2: 'LINE_END',
                3: 'POINT',
                4: 'LANDING',
                5: 'PARACHUTE',
                6: 'ROUTE',
                7: 'WAIT',
                8: 'WIND',
                9: 'KNOT',
                12: 'AUTO_LANDING'}

        def bswap16(value):
            return struct.unpack('>H', struct.pack('<H', value))[0]

        if self.cache is None or self.size != size:
            self.size = size
            self.cache = []

            data = self.stream.data
            for i in range(0, int(len(data) / 17)):
                e = data[i * 17:(i + 1) * 17]

                lat = float(struct.unpack('<i', e[0:4])[0]) / 1e7
                lon = float(struct.unpack('<i', e[4:8])[0]) / 1e7

                alt = e[8] | (e[9] << 8) | ((e[10] & 0xF8) << 13)
                if alt >= 1 << 20:
                    alt = -((~alt & 0xFFFFF) + 1)
                alt = float(alt) / 1e2

                yaw = ((e[11] & 0xFC) >> 2) | ((e[10] & 0x07) << 6)
                yaw = bswap16(yaw << 7)
                yaw = (yaw & 0xFF) | ((yaw & 0x8000) >> 7)
                if yaw <= 360:
                    if yaw > 180:
                        yaw = 360 - yaw
                else:
                    yaw = 511 # Reserved value
                yaw = float(yaw)

                flags = ((e[12] & 0xC0) >> 6) | ((e[11] & 0x03) << 2)
                pointType = (e[12] & 0x3C) >> 2
                rad = float(e[14] | ((e[15] & 0xF0) << 4))

                duration = ((e[12] & 0x03) << 8) | e[13]
                duration = bswap16(duration << 6)
                duration = (duration & 0xFF) | ((duration & 0xC000) >> 6)

                cmd = ((e[15] & 0x0F) << 8) | e[16]
                cmd = bswap16(cmd << 4)
                cmd = (cmd & 0xFF) | ((cmd & 0xF000) >> 4)

                serializedPoint = '{:3d} [ {:+11.7f} {:+12.7f} {:9.2f} ] '.format(i, lat, lon, alt)
                serializedPoint += 'cmd {:4d} '.format(cmd)
                if duration == 1023:
                    serializedPoint += 'dur  inf s '
                else:
                    serializedPoint += 'dur {:4d} s '.format(int(duration * (60 / 16)))
                serializedPoint += 'yaw {:+4d} '.format(int(yaw))
                serializedPoint += 'rad {:4d} '.format(int(rad))
                serializedPoint += 'flags '
                serializedPoint += 'l' if flags & 1 != 0 else '-'
                serializedPoint += 's' if flags & 2 != 0 else '-'
                serializedPoint += 'c' if flags & 4 != 0 else '-'
                serializedPoint += 'g' if flags & 8 != 0 else '-'
                serializedPoint += ' type ' + POINT_TYPES[pointType] if pointType in POINT_TYPES else 'UNKNOWN'
                self.cache.append(serializedPoint[:size[0]])
        return self.cache


class FlightManagerIndexParser(DefaultParser):
    def __init__(self, stream):
        DefaultParser.__init__(self, stream)

    @staticmethod
    def match(stream):
        return stream.component.name == 'FlightManager' and stream.index == 1

    @staticmethod
    def propose(stream=None):
        settings = DefaultParser.propose(stream)
        settings.update({'chunk': 48, 'checksum': True})
        return settings

    def native(self, size):
        if self.cache is None or self.size != size:
            self.size = size
            self.cache = []

            data = self.stream.data
            for i in range(0, int(len(data) / 4)):
                beg, end = struct.unpack('<HH', data[i * 4:(i + 1) * 4])[0:2]
                self.cache.append('{:d}..{:d}'.format(beg, end)[:size[0]])
        return self.cache


class FlightManagerCommandParser(DefaultParser):
    def __init__(self, stream):
        DefaultParser.__init__(self, stream)

    @staticmethod
    def match(stream):
        return stream.component.name == 'FlightManager' and stream.index == 2

    @staticmethod
    def propose(stream=None):
        settings = DefaultParser.propose(stream)
        settings.update({'chunk': 48, 'checksum': True})
        return settings

    def native(self, size):
        if self.cache is None or self.size != size:
            self.size = size
            self.cache = []

            data = self.stream.data
            for i in range(0, int(len(data) / 12)):
                prefix = '{:3d} '.format(i)
                code = data[i * 12]
                if code == 0:
                    self.cache.append(prefix + 'End'[:size[0]])
                elif code == 1:
                    component, field = data[i * 12 + 1], data[i * 12 + 2]
                    value = proto.Protocol.unpack(data[i * 12 + 3], data[i * 12 + 4:(i + 1) * 12], 1)
                    if len(value) == 1:
                        value = value[0]
                    self.cache.append(prefix + 'Write {} to {:d}:{:d}'.format(value, component, field)[:size[0]])
                elif code == 2:
                    delay = struct.unpack('<I', data[i * 12 + 1:i * 12 + 5])[0]
                    self.cache.append(prefix + 'Delay {:.3f} s'.format(delay / 1000.0)[:size[0]])
                elif code == 3:
                    component, field = data[i * 12 + 1], data[i * 12 + 2]
                    value = proto.Protocol.unpack(data[i * 12 + 3], data[i * 12 + 4:(i + 1) * 12], 1)
                    if len(value) == 1:
                        value = value[0]
                    self.cache.append(prefix + 'RMW {} to {:d}:{:d}'.format(value, component, field)[:size[0]])
                elif code == 4:
                    attribute = struct.unpack('<H', data[i * 12 + 1:i * 12 + 3])[0]
                    value = proto.Protocol.unpack(data[i * 12 + 3], data[i * 12 + 4:(i + 1) * 12], 1)
                    if len(value) == 1:
                        value = value[0]
                    self.cache.append(prefix + 'Att {:s} ({:d}) to {}'.format(
                        FlightManagerCommandParser.getAttributeName(attribute), attribute, value))
        return self.cache

    @staticmethod
    def getAttributeName(attribute):
        if attribute == 0:
            return 'YAW'
        elif attribute == 1:
            return 'SPEED'
        elif attribute == 2:
            return 'RADIUS'
        elif attribute == 3:
            return 'TIMEOUT'
        else:
            return 'UNKNOWN'


class GnssReceiverSnrParser(DefaultParser):
    def __init__(self, stream):
        DefaultParser.__init__(self, stream)

    @staticmethod
    def match(stream):
        return stream.component.name in ['Mediatek', 'Topcon', 'Ublox'] and stream.index == 1

    def native(self, size):
        GNSS_SYSTEMS = {0: 'GPS', 1: 'SBAS', 2: 'GALILEO', 3: 'BEIDOU', 4: 'IMES', 5: 'QZSS', 6: 'GLONASS'}
        SNR_RANGE = (0.0, 60.0)

        if self.cache is None or self.size != size:
            cache = []

            if size[0] > 40:
                data = self.stream.data

                satellites = []
                for i in range(0, int(len(data) / 6)):
                    strength = struct.unpack('B', data[i * 6 + 0:i * 6 + 1])[0]
                    elevation = struct.unpack('b', data[i * 6 + 1:i * 6 + 2])[0]
                    azimuth = struct.unpack('<h', data[i * 6 + 2:i * 6 + 4])[0]
                    number = struct.unpack('<H', data[i * 6 + 4:i * 6 + 6])[0]
                    sysId, satId = ((number >> 8) & 0xFF), (number & 0xFF)
                    satellites.append((sysId, satId, strength, elevation, azimuth))

                for entry in satellites:
                    if entry[0] in GNSS_SYSTEMS:
                        level = int(float(entry[2]) * ((size[0] - 25) / (SNR_RANGE[1] - SNR_RANGE[0])))
                        output = ''
                        output += GNSS_SYSTEMS[entry[0]] + ' ' * (8 - len(GNSS_SYSTEMS[entry[0]]))
                        output += '{:3d} {:+4d} {:+4d} {:2d} '.format(entry[1], entry[3], entry[4], entry[2])
                        output += '#' * level + ' ' * int(size[0] - 25 - level) + ']'
                        cache.append(output)

            self.size = size
            self.cache = cache
        return self.cache


class SensorMonitorDeclinationParser(DefaultParser):
    def __init__(self, stream):
        DefaultParser.__init__(self, stream)

    @staticmethod
    def match(stream):
        return stream.component.name == 'SensorMonitor' and stream.index == 0


class SensorMonitorImuTableParser(DefaultParser):
    def __init__(self, stream):
        DefaultParser.__init__(self, stream)

    @staticmethod
    def match(stream):
        return stream.component.name == 'SensorMonitor' and stream.index == 1

    def native(self, size):
        if self.cache is None or self.size != size:
            cache = []

            if size[0] >= 20:
                data = self.stream.data
                for i in range(0, int(len(data) / 6)):
                    values = struct.unpack('<hhh', data[i * 6:(i + 1) * 6])
                    cache.append('{:6d} {:6d} {:6d}'.format(*values)[:size[0]])

            self.size = size
            self.cache = cache
        return self.cache


class SensorMonitorLogParser(DefaultParser):
    def __init__(self, stream):
        DefaultParser.__init__(self, stream)

    @staticmethod
    def match(stream):
        return stream.component.name == 'SensorMonitor' and stream.index == 2

    @staticmethod
    def propose(stream):
        # Check minor version
        if stream.component.swVersion[1] == 5:
            return {'chunk': 240, 'burst': 5, 'checksum': False}
        else:
            return DefaultParser.propose()

    def native(self, size):
        if self.cache is None or self.size != size:
            self.size = size
            self.cache = ['Please use GCS or logger.py'[:size[0]]]
        return self.cache


class UavMonitorSessionParser(DefaultParser):
    def __init__(self, stream):
        DefaultParser.__init__(self, stream)

    @staticmethod
    def match(stream):
        return stream.component.name == 'UavMonitor' and stream.index == 1

    @staticmethod
    def propose(stream):
        return {'chunk': 240, 'burst': 2, 'checksum': True}

    def native(self, size):
        def timestr(value):
            gmtime = time.gmtime(value)
            return '{:02d}.{:02d}.{:04d} {:02d}:{:02d}:{:02d}'.format(gmtime.tm_mday, gmtime.tm_mon,
                    gmtime.tm_year, gmtime.tm_hour, gmtime.tm_min, gmtime.tm_sec)

        if self.cache is None or self.size != size:
            cache = []
            data = self.stream.data

            if len(data) >= 52:
                sessionCount = int((len(data) - 52) / 12)
                timestamp = struct.unpack('<I', data[16:20])[0]
                cache.append('Time  {:s}'.format(timestr(timestamp)))
                cache.append('UUID  {:s}'.format(binascii.hexlify(data[0:16]).decode())[:size[0]])
                cache.append('Sign  {:s}'.format(binascii.hexlify(data[-32:]).decode())[:size[0]])
                cache.append('')
                cache.append('Sessions:')

                if sessionCount > 0:
                    for i in range(0, sessionCount):
                        user, start, end = struct.unpack('<III', data[20 + i * 12:20 + (i + 1) * 12])
                        cache.append('    {:s} - {:s}, ID {:d}'.format(timestr(start), timestr(end), user)[:size[0]])
                else:
                    cache.append('    None')
            self.size = size
            self.cache = cache
        return self.cache


class DfuParser(DefaultParser):
    def __init__(self, stream):
        DefaultParser.__init__(self, stream)

    @staticmethod
    def match(stream):
        return stream.component.name.startswith('DFU')

    @staticmethod
    def propose(stream):
        return {'chunk': 48, 'burst': 1, 'checksum': True}


class PassportStorageParser(DefaultParser):
    def __init__(self, stream):
        DefaultParser.__init__(self, stream)

    @staticmethod
    def match(stream):
        return stream.component.name == 'PassportStorage' and stream.index == 0

    @staticmethod
    def propose(stream):
        return {'chunk': 37, 'burst': 4, 'checksum': True}

    @staticmethod
    def parse(data):
        fields = {}
        fields['device'] = data[0]
        fields['number'] = struct.unpack('<H', data[1:3])[0]
        position = 3

        while position < len(data):
            typeId = data[position]
            position += 1
            if typeId == 0x00: # Time
                if position + 8 > len(data):
                    print('Error in passport structure')
                    break
                else:
                    fields['time'] = struct.unpack('<Q', data[position:position + 8])[0]
                    position += 8
            elif typeId == 0x01: # Position
                if position + 12 > len(data):
                    print('Error in passport structure')
                    break
                else:
                    lat = struct.unpack('<i', data[position + 0:position + 4])[0]
                    lon = struct.unpack('<i', data[position + 4:position + 8])[0]
                    alt = struct.unpack('<i', data[position + 8:position + 12])[0]
                    fields['position'] = (float(lat) / 1e7, float(lon) / 1e7, float(alt) / 1e3)
                    position += 12
            elif typeId == 0x02: # Barometric altitude
                if position + 4 > len(data):
                    print('Error in passport structure')
                    break
                else:
                    alt = struct.unpack('<i', data[position:position + 4])[0]
                    fields['altitude'] = float(alt) / 1e3
                    position += 4
            elif typeId == 0x03: # Orientation
                if position + 6 > len(data):
                    print('Error in passport structure')
                    break
                else:
                    roll = struct.unpack('<h', data[position + 0:position + 2])[0]
                    pitch = struct.unpack('<h', data[position + 2:position + 4])[0]
                    yaw = struct.unpack('<h', data[position + 4:position + 6])[0]
                    fields['orientation'] = (float(roll) / 1e2, float(pitch) / 1e2, float(yaw) / 1e2)
                    position += 6
            elif typeId == 0x04: # Illumination
                if position + 4 > len(data):
                    print('Error in passport structure')
                    break
                else:
                    value = struct.unpack('<i', data[position + 0:position + 4])[0]
                    if 'illumination' not in fields.keys():
                        fields['illumination'] = []
                    fields['illumination'].append(float(value))
                    position += 4
        return fields


    def native(self, size):
        if self.cache is None or self.size != size:
            cache = []
            data = self.stream.data

            for i in range(0, int(len(data) / 37)):
                fields = PassportStorageParser.parse(data[i * 37:(i + 1) * 37])
                cache.append(str(fields)[:size[0]])

            self.size = size
            self.cache = cache
        return self.cache


parsers = [
        FlightManagerPointParser1V5,
        FlightManagerPointParser1V6,
        FlightManagerIndexParser,
        FlightManagerCommandParser,
        GnssReceiverSnrParser,
        SensorMonitorDeclinationParser,
        SensorMonitorImuTableParser,
        SensorMonitorLogParser,
        UavMonitorSessionParser,
        DfuParser,
        PassportStorageParser,
        DefaultParser]
