#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import math
import numpy
import serial
import socket
import struct
import sys
import threading
import time

debug = False
opengl = True
try:
    from OpenGL.GL import *
    from OpenGL.GLU import *
    from OpenGL.GLUT import *
    from OpenGL.GL.shaders import *
except:
    opengl = False

def pDebug(text, level = 1):
    if debug:
        print(text)

def getAngle(v1, v2):
    mag1 = math.sqrt(v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2])
    mag2 = math.sqrt(v2[0]*v2[0] + v2[1]*v2[1] + v2[2]*v2[2])
    res = (v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2]) / (mag1 * mag2)
    ac = math.acos(res)
    if v2[0]*v1[1] - v2[1]*v1[0] < 0:
        ac *= -1
    return ac

def normalize(vect):
    val = numpy.linalg.norm(vect)
    if val != 0:
        return vect / val
    else:
        return vect

def getNormal(v1, v2):
    return numpy.matrix([[float(v1[1] * v2[2] - v1[2] * v2[1])],
                         [float(v1[2] * v2[0] - v1[0] * v2[2])],
                         [float(v1[0] * v2[1] - v1[1] * v2[0])]])

def fillRotateMatrix(v, angle):
    cs = math.cos(angle)
    sn = math.sin(angle)
    v = [float(v[0]), float(v[1]), float(v[2])]
    return numpy.matrix([
            [     cs + v[0]*v[0]*(1 - cs), v[0]*v[1]*(1 - cs) - v[2]*sn, v[0]*v[2]*(1 - cs) + v[1]*sn, 0.],\
            [v[1]*v[0]*(1 - cs) + v[2]*sn,      cs + v[1]*v[1]*(1 - cs), v[1]*v[2]*(1 - cs) - v[0]*sn, 0.],\
            [v[2]*v[0]*(1 - cs) - v[1]*sn, v[2]*v[1]*(1 - cs) + v[0]*sn,      cs + v[2]*v[2]*(1 - cs), 0.],\
            [                          0.,                           0.,                           0., 1.]])

class SerialHandler:
    DEVICE_TYPES = {\
            1: [("enabled", 1), ("automatic", 1), ("take", 1), ("distance", 2)],\
            2: [("enabled", 1), ("automatic", 1), ("roll", 2), ("pitch", 2), ("latitude", 4), ("longitude", 4)],\
            3: [("enabled", 1)],\
            4: [],\
            5: [("enabled", 1)],\
            6: [("enabled", 1)],\
            7: [("enabled", 1), ("delay", 2)],\
            8: [("verbose", 1), ("magEnabled", 1), ("magDeclination", 2), ("altitudeOffset", 4),\
                    ("magX", 2), ("magY", 2), ("magZ", 2),\
                    ("accelX", 2), ("accelY", 2), ("accelZ", 2),\
                    ("pressure", 4), ("diffPressure", 4),\
                    ("gyroX", 2), ("gyroY", 2), ("gyroZ", 2),\
                    ("accelSampleX", 2), ("accelSampleY", 2), ("accelSampleZ", 2)],\
            9: [("mode", 1), ("interval", 2), ("altitude", 4), ("speed", 2), ("heading", 2), ("roll", 2)],\
            10: [("enabled", 1), ("mode", 1),\
                    ("roll", 2), ("pitch", 2), ("yaw", 2),\
                    ("rollRate", 2), ("pitchRate", 2), ("yawRate", 2),\
                    ("latitude", 4), ("longitude", 4), ("altitude", 4),\
                    ("realRoll", 2), ("realPitch", 2), ("realYaw", 2)],\
            11: [("temperature", 2), ("humidity", 2), ("windAzimuth", 2), ("windSpeed", 2)],\
            12: [("enabled", 1), ("zoom", 2), ("horizAngle", 2), ("vertAngle", 2)],\
            13: [],\
            14: [("type", 1), ("status", 1), ("pdop", 2), ("latitude", 4), ("longitude", 4), ("altitude", 4),\
                    ("velNorth", 2), ("velEast", 2), ("velDown", 2), ("bytesReceived", 4),\
                    ("satTotal", 1), ("satGps", 1), ("satGlonass", 1), ("satGalileo", 1), ("satBeidou", 1)],\
            15: [],\
            16: [("flags", 4), ("time", 8), ("uptime", 4), ("roll", 2), ("pitch", 2), ("yaw", 2),\
                    ("altitude", 4), ("airspeed", 2), ("distance", 4), ("gnss", 1), ("mode", 1),\
                    ("aileron", 2), ("elevator", 2), ("rudder", 2), ("throttle", 2)]
    }

    def __init__(self, render, ports):
        if ports["ip"] is not None:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((ports["ip"], 5500))
            self.socket.sendall("1:6\n")
            self.net = True
        elif ports["serial"] is not None:
            self.socket = serial.Serial()
            self.socket.port     = ports["serial"]
            self.socket.baudrate = 57600
            self.socket.parity   = "N"
            self.socket.rtscts   = False
            self.socket.xonxoff  = False
            self.socket.timeout  = 1
            try:
                self.socket.open()
            except serial.SerialException, e:
                print("Could not open serial port %s: %s" % (self.socket.portstr, e))
                exit()
            self.net = False
        else:
            print("No connection types specified")
            exit()

        self.render = render
        self.parser = PacketParser()
        self.dump = open("dump.txt", "ab")
        self.dump.write(time.strftime("%c\r\n"))

    def send(self, data):
        if self.net:
            self.socket.sendall(data)
        else:
            self.socket.write(data)

    def thread(self):
        while True:
            if not self.render.run:
                break
            
            if self.net:
                data = self.socket.recv(64)
            else:
                try:
                    data = self.socket.read()
                except:
                    print("Serial port error")
                    exit()

            while len(data) > 0:
                count = self.parser.process(data)
                data = data[count:]
                
                if self.parser.state == PacketParser.STATE_DONE:
                    if len(self.parser.packet.buffer) == 0:
                        self.parser.reset()
                        continue

                    arg = ord(self.parser.packet.buffer[0])

                    if self.parser.packet.id == 0xF5 and arg == 0x00: #Magnetometer sample
                        self.render.lastDebugTime = time.time()
                        color = (0.5, 1.0, 0.5)
                        x, y, z = SerialHandler.arrayToFloat(self.parser.packet.buffer[1:], 3, 2)
                        self.pushPoint((x, y, z), color)

                    elif self.parser.packet.id == 0xF5 and arg == 0x01: #Magnetometer offset
                        x, y, z, mean = SerialHandler.arrayToFloat(self.parser.packet.buffer[1:], 4, 2)
                        print("Magnetometer mean: %.4f, offset: [%.4f, %.4f, %.4f]" % (mean, x, y, z))

                    elif self.parser.packet.id == 0xF5 and arg == 0x02: #Magnetometer deviation
                        dev, req = SerialHandler.arrayToFloat(self.parser.packet.buffer[1:], 2, 2, 10000.0, False)
                        print("Magnetometer deviation: %.4f, max deviation: %.4f" % (dev, req))

                    elif self.parser.packet.id == 0xF5 and arg == 0x03: #Accelerometer sample
                        self.render.lastDebugTime = time.time()
                        color = (1.0, 0.5, 0.5)
                        x, y, z = SerialHandler.arrayToFloat(self.parser.packet.buffer[1:], 3, 2)
                        self.pushPoint((x, y, z), color)

                    elif self.parser.packet.id == 0xF5 and arg == 0x04: #Accelerometer offset
                        x, y, z = SerialHandler.arrayToFloat(self.parser.packet.buffer[1:], 3, 2)
                        print("Accelerometer offset: [%.4f, %.4f, %.4f]" % (x, y, z))

                    elif self.parser.packet.id == 0xF5 and arg == 0x06: #Accelerometer average
                        x, y, z = SerialHandler.arrayToFloat(self.parser.packet.buffer[1:], 3, 2)
                        print("Accelerometer average: [%.4f, %.4f, %.4f]" % (x, y, z))

                    elif self.parser.packet.id == 0xF5 and arg == 0x07: #Gyroscope offset
                        x, y, z = SerialHandler.arrayToFloat(self.parser.packet.buffer[1:], 3, 2)
                        print("Gyroscope offset: [%.4f, %.4f, %.4f]" % (x, y, z))

                    elif self.parser.packet.id == 0xF5 and arg == 0x08: #Gyroscope sample
                        ax  = struct.unpack("<f", self.parser.packet.buffer[1:5])[0]
                        ay  = struct.unpack("<f", self.parser.packet.buffer[5:9])[0]
                        az  = struct.unpack("<f", self.parser.packet.buffer[9:13])[0]
                        gx  = struct.unpack("<f", self.parser.packet.buffer[13:17])[0]
                        gy  = struct.unpack("<f", self.parser.packet.buffer[17:21])[0]
                        gz  = struct.unpack("<f", self.parser.packet.buffer[21:25])[0]
                        dgx = struct.unpack("<f", self.parser.packet.buffer[25:29])[0]
                        dgy = struct.unpack("<f", self.parser.packet.buffer[29:33])[0]
                        dgz = struct.unpack("<f", self.parser.packet.buffer[33:37])[0]
                        t   = struct.unpack("<f", self.parser.packet.buffer[37:41])[0]
                        print("[% 8.3f % 8.3f % 8.3f] [% 8.3f % 8.3f % 8.3f] [% 8.3f] [% 8.3f % 8.3f % 8.3f]"\
                                % (ax, ay, az, gx, gy, gz, t, dgx, dgy, dgz))

                    elif self.parser.packet.id in (0xF1, 0xF2): #Memory debug
                        print("Message: %s" % "".join(self.parser.packet.buffer))

                    elif self.parser.packet.id == 0xF4: #Memory copying
                        print("Internal data copy finished")

                    elif self.parser.packet.id == 0x34: #State
                        yaw, roll, pitch = SerialHandler.arrayToFloat(self.parser.packet.buffer[10:], 3, 2, 10.0)
                        self.render.ap = (yaw, roll, -pitch)

                    elif self.parser.packet.id == 0x37: #System command response
                        print("Command %d, result: %d" % (ord(self.parser.packet.buffer[0]), ord(self.parser.packet.buffer[1])))

                    elif self.parser.packet.id == 0x38: #Component message
                        values = {}
                        componentId = ord(self.parser.packet.buffer[0])
                        componentType = ord(self.parser.packet.buffer[1])
                        if componentType in SerialHandler.DEVICE_TYPES.keys():
                            description = SerialHandler.DEVICE_TYPES[componentType]
                            offset = 2
#                             print("[%.3f] Message from a component %u, type %u" % (time.time(), componentId,\
#                                     componentType))
                            while offset < len(self.parser.packet.buffer):
                                field = ord(self.parser.packet.buffer[offset])
                                if field >= len(description):
                                    break
                                value = SerialHandler.arrayToFloat(self.parser.packet.buffer[offset + 1:], 1,\
                                        description[field][1], 1.0, True)[0]
#                                 print("  Received field %s, value %f" % (description[field][0], value));
                                offset += 1 + description[field][1]
                                values[description[field][0]] = value
                        else:
                            print("Message from an unknown component %u, type %u" % (componentId, componentType))

                        if len(set(values.keys()).intersection(["accelX", "accelY", "accelZ"])) == 3:
                            color = (1.0, 0.5, 0.5)
                            x, y, z = values["accelX"] / 1000.0, values["accelY"] / 1000.0, values["accelZ"] / 1000.0
                            self.render.lastDebugTime = time.time()
                            self.pushPoint((x, y, z), color)
                        elif len(set(values.keys()).intersection(["magX", "magY", "magZ"])) == 3:
                            color = (0.5, 1.0, 0.5)
                            x, y, z = values["magX"] / 1000.0, values["magY"] / 1000.0, values["magZ"] / 1000.0
                            self.render.lastDebugTime = time.time()
                            self.pushPoint((x, y, z), color)
                        elif len(set(values.keys()).intersection(["roll", "pitch", "yaw"])) == 3:
                            r, p, y = values["roll"] / 100.0, values["pitch"] / 100.0, values["yaw"] / 100.0
                            self.render.ap = (y, -r, p)
                        elif len(set(values.keys()).intersection(["accelSampleX", "accelSampleY",
                                "accelSampleZ"])) == 3:
                            x = values["accelSampleX"] / 1000.0
                            y = values["accelSampleY"] / 1000.0
                            z = values["accelSampleZ"] / 1000.0
                            print("Accelerometer average: [%.4f, %.4f, %.4f]" % (x, y, z))

                    self.parser.reset()
                elif self.parser.state == PacketParser.STATE_ERROR:
                    self.parser.reset()
        self.socket.close()

    @staticmethod
    def arrayToFloat(array, count, width, div=10000.0, signed=True):
        if len(array) < width:
            return [None] * count
        mask, sign = (1 << (8 * width)) - 1, 1 << (8 * width - 1)
        raws = []
        for i in range(0, count):
            value = 0
            for j in range(0, width):
                value = value | (ord(array[i * width + j]) << (8 * j) & (0xFF << (8 * j)))
            raws.append(value)
        values = map(lambda v: -(((~v) & mask) + 1) if v & sign else v, raws) if signed else raws
        return map(lambda v: float(v) / div, values)

    def pushPoint(self, position, color):
        index = self.render.varrayIndex

        self.render.varray[index * 3 + 0] = position[0]
        self.render.varray[index * 3 + 1] = position[1]
        self.render.varray[index * 3 + 2] = position[2]

        self.render.carray[index * 4 + 0] = color[0]
        self.render.carray[index * 4 + 1] = color[1]
        self.render.carray[index * 4 + 2] = color[2]
        self.render.carray[index * 4 + 3] = 1.0

        self.render.varrayIndex += 1
        if self.render.varraySize < len(self.render.varray) / 3:
            self.render.varraySize += 1
        if self.render.varrayIndex >= len(self.render.varray) / 3:
            self.render.varrayIndex = 0


class PacketParser:
    STATE_SYNC_A, STATE_SYNC_B, STATE_ID, STATE_LENGTH, STATE_PAYLOAD, STATE_CHECKSUM_A, STATE_CHECKSUM_B, \
            STATE_DONE, STATE_ERROR = range(0, 9)

    class Packet:
        def __init__(self):
            self.buffer = []
            self.id = 0

    def __init__(self):
        self.reset()
        self.counters = {"received": 0, "errors": 0}

    def reset(self):
        self.state = PacketParser.STATE_SYNC_A
        self.packet = PacketParser.Packet()
        self.length = 0
        self.position = 0
        self.checksum = (0, 0)

    def process(self, data):
        for i in range(0, len(data)):
            if self.state == PacketParser.STATE_DONE or self.state == PacketParser.STATE_ERROR:
                return i
            value = data[i]

            if self.state == PacketParser.STATE_SYNC_A:
                if value == 'p':
                    self.state = PacketParser.STATE_SYNC_B
            elif self.state == PacketParser.STATE_SYNC_B:
                if value == 'l':
                    self.checksum = (0, 0)
                    self.position = 0
                    self.state = PacketParser.STATE_ID
                else:
                    self.state = PacketParser.STATE_SYNC_A
            elif self.state == PacketParser.STATE_ID:
                self.checksum = PacketParser.getChecksum(self.checksum, [ord(value)])
                self.packet.buffer = "" #Reset packet buffer
                self.packet.id = ord(value)
                self.state = PacketParser.STATE_LENGTH;
            elif self.state == PacketParser.STATE_LENGTH:
                self.checksum = PacketParser.getChecksum(self.checksum, [ord(value)])
                self.length = ord(value)
                self.state = PacketParser.STATE_PAYLOAD if self.length > 0 else PacketParser.STATE_CHECKSUM_A
            elif self.state == PacketParser.STATE_PAYLOAD:
                self.checksum = PacketParser.getChecksum(self.checksum, [ord(value)])
                self.packet.buffer += str(value)
                self.length -= 1
                if self.length == 0:
                    self.state = PacketParser.STATE_CHECKSUM_A
            elif self.state == PacketParser.STATE_CHECKSUM_A:
                if self.checksum[0] == ord(value):
                    self.state = PacketParser.STATE_CHECKSUM_B
                else:
                    self.state = PacketParser.STATE_ERROR
                    self.counters["errors"] += 1
            elif self.state == PacketParser.STATE_CHECKSUM_B:
                if self.checksum[1] == ord(value):
                    self.state = PacketParser.STATE_DONE
                    self.counters["received"] += 1
                else:
                    self.state = PacketParser.STATE_ERROR
                    self.counters["errors"] += 1
        return len(data)

    @staticmethod
    def create(identifier, payload):
        buf = "pl"
        crc = PacketParser.getChecksum((0, 0), [identifier])
        buf += chr(identifier)
        crc = PacketParser.getChecksum(crc, [len(payload)])
        buf += chr(len(payload))

        for value in payload:
            crc = PacketParser.getChecksum(crc, [ord(value)])
            buf += value

        buf += chr(crc[0])
        buf += chr(crc[1])
        return buf

    @staticmethod
    def getChecksum(previous, data):
        for value in data:
            b1 = (previous[0] + value) & 0xFF
            b2 = (previous[1] + b1) & 0xFF
        return (b1, b2)


class Render:
    def __init__(self, ports):
        self.ap = (0.0, 0.0, 0.0)
        self.lastDebugTime = 0.0

        self.run = True
        self.serial = SerialHandler(self, ports)
        self.packeter = threading.Thread(target=self.serial.thread)
        self.addressPhoto, self.addressVideo = 4, 0
        self.zoom = 1.0
        self.packeter.start()

        self.camera = numpy.matrix([[0.], [3.], [3.], [1.]])
        self.pov    = numpy.matrix([[0.], [0.], [0.], [1.]])
        self.lighta = numpy.matrix([[20.], [20.], [20.], [1.]])
        self.lightb = numpy.matrix([[-20.], [-20.], [-20.], [1.]])
        self.axis   = numpy.matrix([[0.], [0.], [1.], [1.]])
        self.updated = True
        self.rotateCamera = False
        self.moveCamera = False
        self.mousePos = [0., 0.]
        self.drawList = None
        self.width = 640
        self.height = 480
        self.cntr = time.time()
        self.fps = 0
        self.data = []
        self.varray = numpy.zeros(16384 * 3, dtype=numpy.float32)
        self.carray = numpy.zeros(16384 * 4, dtype=numpy.float32)
        self.varraySize, self.varrayIndex = 0, 0
        glutInit(sys.argv)
        glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH)
        glutInitWindowSize(self.width, self.height)
        glutInitWindowPosition(0, 0)
        glutCreateWindow("Sensor calibrator")
        glutDisplayFunc(self.drawScene)
        glutIdleFunc(self.drawScene)
        glutReshapeFunc(self.resize)
        glutKeyboardFunc(self.keyHandler)
        glutMotionFunc(self.mouseMove)
        glutMouseFunc(self.mouseButton)
        self.initGraphics()
        self.initScene()
        glutMainLoop()

    def initGraphics(self):
        glClearColor(0.0, 0.0, 0.0, 0.0)
        glClearDepth(1.0)
        glDepthFunc(GL_LESS)
        glEnable(GL_DEPTH_TEST)
        glEnable(GL_LIGHTING)
        glEnable(GL_LIGHT1)
        #Setup global lighting
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, [0.2, 0.2, 0.2, 1.])
        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR)
        #Setup light 0
        glEnable(GL_LIGHT0)
        glLightfv(GL_LIGHT0, GL_POSITION, self.lighta)
        glLightfv(GL_LIGHT0, GL_AMBIENT,  [0.0, 0.0, 0.0, 1.])
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  [1.0, 1.0, 1.0, 1.])
        glLightfv(GL_LIGHT0, GL_SPECULAR, [0.5, 0.5, 0.5, 1.])
        glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0)
        glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0)
        glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.00005)
        #Setup light 1
        glEnable(GL_LIGHT1)
        glLightfv(GL_LIGHT1, GL_POSITION, self.lightb)
        glLightfv(GL_LIGHT1, GL_AMBIENT,  [0.0, 0.0, 0.0, 1.])
        glLightfv(GL_LIGHT1, GL_DIFFUSE,  [0.5, 0.5, 0.5, 1.])
        glLightfv(GL_LIGHT1, GL_SPECULAR, [0.3, 0.3, 0.3, 1.])
        glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0)
        glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.0)
        glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.00005)
        #glEnable(GL_COLOR_MATERIAL)
        #Blending using shader
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluPerspective(45.0, float(self.width)/float(self.height), 0.1, 1000.0)
        glMatrixMode(GL_MODELVIEW)

    def initScene(self):
        pass

    def drawAxis(self):
        glEnableClientState(GL_VERTEX_ARRAY)
        glEnableClientState(GL_COLOR_ARRAY)
        glEnable(GL_COLOR_MATERIAL)

        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.)
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, [0., 0., 0., 1.])
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, [0., 0., 0., 1.])
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE)
        varray = numpy.array([0.0, 0.0, 0.0,
                              3.0, 0.0, 0.0,
                              0.0, 0.0, 0.0,
                              0.0, 3.0, 0.0,
                              0.0, 0.0, 0.0,
                              0.0, 0.0, 2.0], dtype=numpy.float32)
        carray = numpy.array([1.0, 0.0, 0.0, 1.0,
                              1.0, 0.0, 0.0, 1.0,
                              0.0, 1.0, 0.0, 1.0,
                              0.0, 1.0, 0.0, 1.0,
                              0.0, 0.0, 1.0, 1.0,
                              0.0, 0.0, 1.0, 1.0], dtype=numpy.float32)
        glBindBuffer(GL_ARRAY_BUFFER, 0)
        glColorPointer(4, GL_FLOAT, 0, carray)
        glVertexPointer(3, GL_FLOAT, 0, varray)
        glDrawArrays(GL_LINES, 0, len(varray) / 3)

        glDisable(GL_COLOR_MATERIAL)
        glDisableClientState(GL_COLOR_ARRAY)
        glDisableClientState(GL_VERTEX_ARRAY)

    def drawVect(self):
        if time.time() - self.lastDebugTime < 2.0:
            if self.varraySize < 2:
                return

            glEnableClientState(GL_VERTEX_ARRAY)
            glEnableClientState(GL_COLOR_ARRAY)
            glEnable(GL_COLOR_MATERIAL)

            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.)
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, [0., 0., 0., 1.])
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, [0., 0., 0., 1.])
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE)
            glBindBuffer(GL_ARRAY_BUFFER, 0)
            glColorPointer(4, GL_FLOAT, 0, self.carray)
            glVertexPointer(3, GL_FLOAT, 0, self.varray)

            if self.varraySize < len(self.varray) / 3: 
                glDrawArrays(GL_LINE_STRIP, 0, self.varraySize)
            else:
                glDrawArrays(GL_LINE_STRIP, 0, self.varrayIndex)
                glDrawArrays(GL_LINE_STRIP, self.varrayIndex, self.varraySize - self.varrayIndex)

            glDisable(GL_COLOR_MATERIAL)
            glDisableClientState(GL_COLOR_ARRAY)
            glDisableClientState(GL_VERTEX_ARRAY)
        else:
            glEnableClientState(GL_VERTEX_ARRAY)
            glEnableClientState(GL_COLOR_ARRAY)
            glEnable(GL_COLOR_MATERIAL)
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.)
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, [0., 0., 0., 1.])
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, [0., 0., 0., 1.])
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE)

            (yaw, pitch, roll) = self.ap
            glRotatef(-yaw, 0.0, 0.0, 1.0);
            glRotatef(-pitch, 1.0, 0.0, 0.0);
            glRotatef(-roll, 0.0, 1.0, 0.0);

            varray = numpy.array([ 1.0,  0.0, 0.0,
                                  -0.8,  0.5, 0.0,
                                  -0.8, -0.5, 0.0], dtype=numpy.float32)
            carray = numpy.array([1.0, 0.2, 0.2, 1.0,
                                  0.2, 1.0, 0.2, 1.0,
                                  0.2, 0.2, 1.0, 1.0], dtype=numpy.float32)

            glBindBuffer(GL_ARRAY_BUFFER, 0)
            glColorPointer(4, GL_FLOAT, 0, carray)
            glVertexPointer(3, GL_FLOAT, 0, varray)
            glDrawArrays(GL_TRIANGLES, 0, len(varray) / 3)
            glDisable(GL_COLOR_MATERIAL)
            glDisableClientState(GL_COLOR_ARRAY)
            glDisableClientState(GL_VERTEX_ARRAY)

    def drawScene(self):
        self.updated = True
        if self.updated:
            self.updated = False
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
            glLoadIdentity()
            gluLookAt(float(self.camera[0]), float(self.camera[1]), float(self.camera[2]), 
                      float(self.pov[0]), float(self.pov[1]), float(self.pov[2]), 
                      float(self.axis[0]), float(self.axis[1]), float(self.axis[2]))
            glLightfv(GL_LIGHT0, GL_POSITION, self.lighta)
            glLightfv(GL_LIGHT1, GL_POSITION, self.lightb)
            glUseProgram(0)
            self.drawAxis()

            self.drawVect();

            glutSwapBuffers()
            self.fps += 1
            if time.time() - self.cntr >= 1.:
                glutSetWindowTitle("Calibrator: %d FPS" % (self.fps / (time.time() - self.cntr)))
                self.cntr = time.time()
                self.fps = 0
        else:
            time.sleep(.001)

    def resize(self, width, height):
        if height == 0:
            height = 1
        self.width = width
        self.height = height
        glViewport(0, 0, self.width, self.height)
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluPerspective(45.0, float(self.width)/float(self.height), 0.1, 1000.0)
        glMatrixMode(GL_MODELVIEW)
        self.updated = True

    def mouseButton(self, bNumber, bAction, xPos, yPos):
        if bNumber == GLUT_LEFT_BUTTON:
            if bAction == GLUT_DOWN:
                self.rotateCamera = True
                self.mousePos = [xPos, yPos]
            else:
                self.rotateCamera = False
        elif bNumber == GLUT_MIDDLE_BUTTON:
            if bAction == GLUT_DOWN:
                self.moveCamera = True
                self.mousePos = [xPos, yPos]
            else:
                self.moveCamera = False
        elif bNumber == 3 and bAction == GLUT_DOWN:
            zm = 0.9
            scaleMatrix = numpy.matrix([[zm, 0., 0., 0.],
                                        [0., zm, 0., 0.],
                                        [0., 0., zm, 0.],
                                        [0., 0., 0., 1.]])
            self.camera -= self.pov
            self.camera = scaleMatrix * self.camera
            self.camera += self.pov
        elif bNumber == 4 and bAction == GLUT_DOWN:
            zm = 1.1
            scaleMatrix = numpy.matrix([[zm, 0., 0., 0.],
                                        [0., zm, 0., 0.],
                                        [0., 0., zm, 0.],
                                        [0., 0., 0., 1.]])
            self.camera -= self.pov
            self.camera = scaleMatrix * self.camera
            self.camera += self.pov
        self.updated = True
    def mouseMove(self, xPos, yPos):
        if self.rotateCamera:
            self.camera -= self.pov
            zrot = (self.mousePos[0] - xPos) / 100.
            nrot = (yPos - self.mousePos[1]) / 100.
            if zrot != 0.:
                rotMatrixA = numpy.matrix([[math.cos(zrot), -math.sin(zrot), 0., 0.],
                                           [math.sin(zrot),  math.cos(zrot), 0., 0.],
                                           [            0.,              0., 1., 0.],
                                           [            0.,              0., 0., 1.]])
                self.camera = rotMatrixA * self.camera
            if nrot != 0.:
                normal = normalize(getNormal(self.camera, self.axis))
                angle = getAngle(self.camera, self.axis)
                if (nrot > 0 and nrot > angle) or (nrot < 0 and -nrot > math.pi - angle):
                    self.axis = -self.axis
                rotMatrixB = fillRotateMatrix(normal, nrot)
                self.camera = rotMatrixB * self.camera
            self.camera += self.pov
            self.mousePos = [xPos, yPos]
        elif self.moveCamera:
            tlVector = numpy.matrix([[(xPos - self.mousePos[0]) / 50.], [(self.mousePos[1] - yPos) / 50.], [0.], [0.]])
            self.camera -= self.pov
            normal = normalize(getNormal([0., 0., 1.], self.camera))
            angle = getAngle(self.camera, [0., 0., 1.])
            ah = getAngle(normal, [1., 0., 0.])
            rotZ = numpy.matrix([[math.cos(ah), -math.sin(ah), 0., 0.],
                                 [math.sin(ah),  math.cos(ah), 0., 0.],
                                 [          0.,            0., 1., 0.],
                                 [          0.,            0., 0., 1.]])
            self.camera += self.pov
            rotCNormal = fillRotateMatrix(normal, angle)
            tlVector = rotZ * tlVector
            tlVector = rotCNormal * tlVector
            self.camera = self.camera - tlVector
            self.pov = self.pov - tlVector
            self.mousePos = [xPos, yPos]
        self.updated = True
    def keyHandler(self, key, xPos, yPos):
        if key in ('\x1b', 'q', 'Q'):
            self.run = False
            exit()
        elif key in ('r', 'R'):
            self.camera = numpy.matrix([[0.], [2.], [2.], [1.]])
            self.pov    = numpy.matrix([[0.], [0.], [0.], [1.]])
        elif key in ('c', 'C'):
            self.varraySize, self.varrayIndex = 0, 0
        elif key in ('['):
            print("Magnetometer calibration started")
            data = PacketParser.create(0x36, "\x04")
            self.serial.send(data)
            self.varraySize, self.varrayIndex = 0, 0
        elif key in (']'):
            print("Magnetometer calibration stopped")
            data = PacketParser.create(0x36, "\x05")
            self.serial.send(data)
        elif key in ('-'):
            print("Magnetometer check started")
            data = PacketParser.create(0x36, "\x0B")
            self.serial.send(data)
            self.varraySize, self.varrayIndex = 0, 0
        elif key in ('='):
            print("Magnetometer check stopped")
            data = PacketParser.create(0x36, "\x0C")
            self.serial.send(data)
        elif key in ('f', 'F'):
            print("Request memory information")
            data = PacketParser.create(0xF1, "")
            self.serial.send(data)
        elif key in (','):
            print("Accelerometer calibration started")
            data = PacketParser.create(0x36, "\x06")
            self.serial.send(data)
            self.varraySize, self.varrayIndex = 0, 0
        elif key in ('.'):
            print("Accelerometer calibration stopped")
            data = PacketParser.create(0x36, "\x07")
            self.serial.send(data)
        elif key in ('m', 'M'):
            print("Accelerometer averaging started")
            data = PacketParser.create(0x36, "\x08")
            self.serial.send(data)
        elif key in ('p', 'P'):
            print("Pressure calibration started")
            data = PacketParser.create(0x36, "\x01")
            self.serial.send(data)
        elif key in ('g', 'G'):
            print("Gyroscope calibration started")
            data = PacketParser.create(0x36, "\x03")
            self.serial.send(data)
        elif key in ('y', 'Y'):
            print("Gyroscope calibration stopped")
            data = PacketParser.create(0x36, "\x0E")
            self.serial.send(data)
        elif key in ('a', 'A'):
            print("Align orientation")
            data = PacketParser.create(0x36, "\x0A")
            self.serial.send(data)
        elif key in ('s', 'S'):
            print("Reset orientation alignment")
            data = PacketParser.create(0x36, "\x09")
            self.serial.send(data)
        elif key in ('e', 'E'):
            print("Camera enable")
            data = PacketParser.create(0x2F, "%c\x00\x00\x01" % chr(self.addressPhoto))
            self.serial.send(data)
        elif key in ('d', 'D'):
            print("Camera disable")
            data = PacketParser.create(0x2F, "%c\x00\x00\x00" % chr(self.addressPhoto))
            self.serial.send(data)
        elif key in ('t', 'T'):
            print("Camera take photo")
            data = PacketParser.create(0x2F, "%c\x02\x00\x01" % chr(self.addressPhoto))
            self.serial.send(data)
        elif key in ('v', 'V'):
            print("Video enable")
            data = PacketParser.create(0x2F, "%c\x00\x00\x01" % chr(self.addressVideo))
            self.serial.send(data)
        elif key in ('w', 'W'):
            print("Video disable")
            data = PacketParser.create(0x2F, "%c\x00\x00\x00" % chr(self.addressVideo))
            self.serial.send(data)
        elif key in ('1'):
            if self.zoom > 1.0:
                self.zoom /= 1.5
            print("Zoom out to %f" % self.zoom)
            zl, zh = int(self.zoom) & 0xFF, (int(self.zoom) >> 8) & 0xFF
            data = PacketParser.create(0x2F, "%c\x01\x07%c%c" % (chr(self.addressVideo), chr(zl), chr(zh)))
            self.serial.send(data)
        elif key in ('2'):
            if self.zoom < 65535.0 / 1.5:
                self.zoom *= 1.5
            print("Zoom in to %f" % self.zoom)
            zl, zh = int(self.zoom) & 0xFF, (int(self.zoom) >> 8) & 0xFF
            data = PacketParser.create(0x2F, "%c\x01\x07%c%c" % (chr(self.addressVideo), chr(zl), chr(zh)))
            self.serial.send(data)
        elif key in ('b', 'B'):
            print("Backup data from internal memory")
            data = PacketParser.create(0xF4, "")
            self.serial.send(data)
        elif key in ('i', 'I'):
            print("Debug info enabled")
            data = PacketParser.create(0xF3, "\x05")
            self.serial.send(data)
        elif key in ('o', 'O'):
            print("Debug info disabled")
            data = PacketParser.create(0xF3, "\x00")
            self.serial.send(data)
        elif key in ('x', 'X'):
            print("Request stack information")
            data = PacketParser.create(0xF2, "")
            self.serial.send(data)
        elif key in ('3'):
            print("HOME")
            data = PacketParser.create(0x0B, "\x0B")
            self.serial.send(data)
        elif key in ('4'):
            print("Iridium")
            data = PacketParser.create(0x2F, "\x06\x01\x00\x01")
            self.serial.send(data)
        elif key in ('9'):
            print("Gyro check started")
            data = PacketParser.create(0x36, "\x0F")
            self.serial.send(data)
        elif key in ('0'):
            print("Gyro check stopped")
            data = PacketParser.create(0x36, "\x10")
            self.serial.send(data)
        elif key in ('h', 'H'):
            print("q\texit")
            print("r\treset view")
            print("c\tclear samples")
            print("[\tstart magnetometer calibration")
            print("]\tstop magnetometer calibration")
            print(",\tstart accelerometer calibration")
            print(".\tstop accelerometer calibration")
            print("m\tmake accelerometer sample")
            print("p\tstart pressure calibration")
            print("g\tstart gyroscope compensation calibration")
            print("y\tstop gyroscope compensation calibration")
            print("n\tclear gyroscope calibration")
            print("-\tstart magnetometer check")
            print("=\tstop magnetometer check")
            print("f\trequest memory information")
            print("a\talign orientation")
            print("s\treset orientation alignment")
            print("e\tenable photo camera")
            print("d\tdisable photo camera")
            print("t\tmake photo")
            print("v\tenabe video camera")
            print("w\tdisable video camera")
            print("b\tcopy data from internal memory")
            print("i\tenable debug info")
            print("o\tdisable debug info")
            print("x\trequest stack info")
            print("1\tzoom out")
            print("2\tzoom in")
            print("h\tshow help message")
        elif key in (' '):
            print self.serial.parser.counters
        self.updated = True


args = argparse.ArgumentParser()
args.add_argument("--serial", dest="serial", help="Serial port name", default="")
args.add_argument("--address", dest="ip", help="Server address", default="")
options = args.parse_args()

ports = {}
ports["ip"] = options.ip if options.ip != "" else None
ports["serial"] = options.serial if options.serial != "" else None

render = Render(ports)
