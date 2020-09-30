#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import parser
import os
import select
import serial
import signal
import socket
import sys
import threading
import time

args = argparse.ArgumentParser()
args.add_argument("--bridge-address", dest="bridgeAddress", help="IP address of the transmitter", default="127.0.0.1")
args.add_argument("--source-address", dest="sourceAddress", help="Source IP address", default="127.0.0.1")
args.add_argument("--bridge-port", dest="bridgePort", help="UDP port to listen", type=int, default=5000)
args.add_argument("--source-port", dest="sourcePort", help="Source UDP port", type=int, default=5002)
args.add_argument("--serial", dest="serial", help="Serial port name", default="/dev/ttyS0")
options = args.parse_args()

ser = serial.Serial()
ser.port     = options.serial
ser.baudrate = 115200
ser.parity   = "N"
ser.rtscts   = False
ser.xonxoff  = False
ser.timeout  = 1

try:
  ser.open()
except serial.SerialException, e:
  print("Could not open serial port %s: %s\n" % (ser.portstr, e))
  exit()

inSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
inSocket.bind((options.bridgeAddress, options.bridgePort))
inSocket.setblocking(0)
outSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

termRequest = False
apActive, fgActive = False, False

def reader(networkSocket, serialPort):
  global fgActive, termRequest

  while not termRequest:
    ready = select.select([networkSocket], [], [], 1.0)
    if not ready[0]:
      fgActive = False
      continue
    else:
      fgActive = True
    data = networkSocket.recv(1024)
    try:
      serialPort.write(data)
    except:
      print("Serial port error")
      termRequest = True
      break

def writer(networkSocket, serialPort):
  global apActive, termRequest

  buf = ""
  while not termRequest:
    try:
      data = serialPort.read()
    except:
      print("Serial port error")
      termRequest = True
      break

    if len(data):
      buf += data
      bufReady = False
      for c in buf:
        if c == "\n":
          bufReady = True
      if not bufReady:
        continue
      apActive = True
      try:
        networkSocket.sendto(buf, (options.sourceAddress, options.sourcePort))
      except:
        pass
      buf = ""
    else:
      apActive = False

def handler(signum, frame):
  global termRequest

  print("Terminate requested")
  termRequest = True

signal.signal(signal.SIGINT, handler)

inThread = threading.Thread(target=reader, args=(inSocket, ser))
outThread = threading.Thread(target=writer, args=(outSocket, ser))

inThread.start()
outThread.start()

while not termRequest:
  print("\rAutopilot %s, FlightGear %s" %\
      tuple(map(lambda flag: "attached" if flag else "detached", [apActive, fgActive]))),
  sys.stdout.flush()
  time.sleep(0.5)

inThread.join()
outThread.join()

print("Bridge terminated")
