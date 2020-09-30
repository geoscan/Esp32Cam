#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import curses
import math
import os
import signal
import time
import threading
import re
import sys

import file_parser
import flag_parser
import proto


def isCorrectChar(code):
    return code >= 32 and code <= 126


class WatchEntry:
    def __init__(self, component, field, interval=1.0):
        self.component = component
        self.field = field
        # Update interval measured in seconds
        self.interval = interval
        # Next update time
        self.next = time.time()


# class KeyMapper:
#     def __init__(self, keys={}, funs=[]):
#         self.keys = keys
#         self.funs = funs
#
#     def handle(self, value):
#         if value in self.keys:
#             self.keys[value]()
#         else:
#             for fun in self.funs:
#                 fun(value)


class GenericPage:
    REFRESH_PERIOD = 0.2

    def __init__(self, parent):
        self.parent = parent
        self.cursor = None
        self.timer = None
        self.idle = True

    def enter(self):
        self.timer = threading.Timer(GenericPage.REFRESH_PERIOD, self.timerCallback)
        self.timer.start()

    def leave(self):
        if self.timer is not None:
            self.timer.cancel()
            self.timer = None

    def drawCursor(self):
        if self.cursor is not None:
            self.parent.screen.move(self.cursor[1], self.cursor[0])

    def drawProgressBar(self, top, bottom):
        width = bottom[0] - top[0]
        progress, completed, total = self.parent.hub.messenger.getProgress()

        if progress < 1.0:
            progress = int(progress * (width - 1))
            progressText = ' {:d}/{:d}'.format(completed, total)
            progressText = progressText[:width]

            if progress > 0:
                self.parent.screen.addstr(top[1], top[0], progressText[:progress],
                        curses.color_pair(Window.Color.HOVER))
            if progress < len(progressText):
                self.parent.screen.addstr(top[1], top[0] + progress, progressText[progress:],
                        curses.color_pair(Window.Color.DEFAULT))
            elif progress > len(progressText):
                self.parent.screen.addstr(top[1], top[0] + len(progressText), ' ' * (progress - len(progressText)),
                        curses.color_pair(Window.Color.HOVER))

    def onUpdateEvent(self):
        progress, _, _ = self.parent.hub.messenger.getProgress()
        if progress < 1.0:
            self.idle = False
            self.parent.sem.release()
        elif not self.idle:
            self.idle = True
            self.parent.sem.release()

    def timerCallback(self):
        self.onUpdateEvent()
        self.timer = threading.Timer(GenericPage.REFRESH_PERIOD, self.timerCallback)
        self.timer.start()


class ComponentPage(GenericPage):
    MARGIN_TOP, MARGIN_BOTTOM = 2, 1
    MARGIN_Y = MARGIN_TOP + MARGIN_BOTTOM

    def __init__(self, parent):
        GenericPage.__init__(self, parent)

        self.name = 'Components'
        self.pane = 1
        self.component = 0
        self.field = 0

        self.errorMessage = None
        self.fieldValueError = False
        self.editModeEnabled = False
        self.editModeValue = ''

        self.watchIndex = 0
        self.watchList = []

    def isFieldsInWatchList(self, component, fields):
        for entry in self.watchList:
            if entry.component == component and entry.field in fields:
                return True
        return False

    def enterEditMode(self):
        self.fieldValueError = False
        self.editModeEnabled = True
        self.editModeValue = ''
        self.errorMessage = None
        curses.curs_set(1)

    def exitEditMode(self):
        self.editModeEnabled = False
        self.cursor = None
        curses.curs_set(0)

    def resetEditMode(self):
        if self.editModeEnabled:
            self.editModeEnabled = False
            self.cursor = None
            curses.curs_set(0)
        self.fieldValueError = False
        self.editModeValue = ''
        self.errorMessage = None

    def prepareFieldValue(self, field):
        if proto.Protocol.isUnsignedType(field.type) and field.value is not None:
            return ' Value  {:d} / 0x{:08X}'.format(field.value, field.value)
        else:
            return ' Value  {}'.format(field.value)

    def prepareEditBox(self):
        return ' Value  {}'.format(self.editModeValue)

    @staticmethod
    def prepareComponentDescription(component):
        lines = []
        lines.append(' Type   {:d}'.format(component.type))
        lines.append(' HW     {:d}.{:d}'.format(*component.hwVersion))
        lines.append(' SW     {:d}.{:d}.{:d}'.format(*component.swVersion))
        lines.append(' Hash   {:08X}'.format(component.hash))
        return lines

    @staticmethod
    def prepareComponentFlags(component, field):
        lines = []
        if field.value is not None:
            for parser in flag_parser.parsers:
                if parser.match(field):
                    extendedValueInfo = parser.parse(field.value)
                    if len(extendedValueInfo) > 0:
                        for line in parser.parse(field.value):
                            lines.append('    ' + line)
                        lines.append('')
                    break
        return lines

    @staticmethod
    def prepareFieldDescription(component, field):
        lines = []
        lines.extend(ComponentPage.prepareComponentFlags(component, field))
        lines.append(' Type   {:s}'.format(proto.Protocol.typeToString(field.type)))
        lines.append(' Size   {:d}'.format(field.size))
        lines.append(' Scale  {:d}'.format(field.scale))
        lines.append(' Unit   {:s}'.format(field.unit))
        lines.append(' Min    {}'.format(field.min))
        lines.append(' Max    {}'.format(field.max))
        lines.append(' Flags  {:s}'.format(proto.Field.flagsToString(field.flags)))
        return lines

    def drawComponentPane(self, top, bottom):
        self.parent.screen.addstr(top[1], top[0], 'Components',
                curses.color_pair(Window.Color.DEFAULT) | curses.A_BOLD)
        width, height = bottom[0] - top[0], bottom[1] - top[1]

        self.parent.hub.mutex.acquire()
        count = len(self.parent.hub.components)

        if self.component >= ComponentPage.MARGIN_TOP and height - ComponentPage.MARGIN_Y < count:
            startIndex = min(self.component - ComponentPage.MARGIN_TOP, count - height + ComponentPage.MARGIN_Y)
            section = (startIndex, startIndex + min(height - ComponentPage.MARGIN_Y, count - startIndex))
        else:
            section = (0, min(height - ComponentPage.MARGIN_Y, count))

        for i in list(self.parent.hub.components)[section[0]:section[1]]:
            rowSelected = self.pane > 0 and i == self.component
            component = self.parent.hub.components[i]
            componentStatus = component.getStatus()
            componentEntryText = ' {:<3d} {:s}'.format(component.address, component.name)
            componentEntryText = componentEntryText[:width]

            if componentStatus is not None:
                if componentStatus[0] == proto.Component.MODE_OPERATIONAL\
                        and componentStatus[1] == proto.Component.HEALTH_OK:
                    color = Window.Color.ready(rowSelected)
                elif componentStatus[1] >= proto.Component.HEALTH_ERROR:
                    color = Window.Color.error(rowSelected)
                else:
                    color = Window.Color.warning(rowSelected)
            else:
                color = Window.Color.default(rowSelected)

            self.parent.screen.attron(curses.color_pair(color))
            self.parent.screen.addstr(top[1] + ComponentPage.MARGIN_TOP + i - section[0],
                    top[0], componentEntryText)
            self.parent.screen.addstr(top[1] + ComponentPage.MARGIN_TOP + i - section[0],
                    top[0] + len(componentEntryText), ' ' * (width - len(componentEntryText)))
            self.parent.screen.attroff(curses.color_pair(color))

        self.parent.hub.mutex.release()

    def drawFieldPane(self, top, bottom):
        self.parent.screen.addstr(top[1], top[0], 'Fields', curses.color_pair(Window.Color.DEFAULT) | curses.A_BOLD)
        width, height = bottom[0] - top[0], bottom[1] - top[1]

        self.parent.hub.mutex.acquire()
        if self.component in self.parent.hub.components:
            component = self.parent.hub.components[self.component]
            count = len(component.fields)

            if self.field >= ComponentPage.MARGIN_TOP and height - ComponentPage.MARGIN_Y < count:
                startIndex = min(self.field - ComponentPage.MARGIN_TOP, count - height + ComponentPage.MARGIN_Y)
                section = (startIndex, startIndex + min(height - ComponentPage.MARGIN_Y, count - startIndex))
            else:
                section = (0, min(height - ComponentPage.MARGIN_Y, count))

            for i in range(section[0], section[1]):
                field = component.fields[i]
                fieldValueText = '{} '.format(field.value)
                fieldEntryText = ' {:<3d} {:s}'.format(i, field.name)
                fieldEntryText += ' ' * (max(1, width - len(fieldEntryText) - len(fieldValueText))) + fieldValueText
                fieldEntryText = fieldEntryText[:width]
                if self.pane in (2, 3) and i == self.field:
                    self.parent.screen.attron(curses.color_pair(Window.Color.HOVER))
                    self.parent.screen.addstr(top[1] + ComponentPage.MARGIN_TOP + i - section[0],
                            top[0], fieldEntryText)
                    self.parent.screen.addstr(top[1] + ComponentPage.MARGIN_TOP + i - section[0],
                            top[0] + len(fieldEntryText), ' ' * (width - len(fieldEntryText)))
                    self.parent.screen.attroff(curses.color_pair(Window.Color.HOVER))
                else:
                    self.parent.screen.addstr(top[1] + ComponentPage.MARGIN_TOP + i - section[0],
                            top[0], fieldEntryText, curses.color_pair(Window.Color.DEFAULT))
        self.parent.hub.mutex.release()

    def drawPropertiesPane(self, top, bottom):
        self.parent.screen.addstr(top[1], top[0], 'Properties',
                curses.color_pair(Window.Color.DEFAULT) | curses.A_BOLD)

        width, height = bottom[0] - top[0], bottom[1] - top[1]
        xPos, yPos = top[0], ComponentPage.MARGIN_TOP

        hub = self.parent.hub
        hub.mutex.acquire()

        if self.component in hub.components and self.field < len(hub.components[self.component].fields):
            component = self.parent.hub.components[self.component]

            if self.pane == 1:
                # Print details about component
                for line in ComponentPage.prepareComponentDescription(component):
                    if yPos < bottom[1] - ComponentPage.MARGIN_BOTTOM:
                        self.parent.screen.addstr(yPos, xPos, line)
                        yPos += 1

                # Print flags if available
                if component.flagField is not None and component.flagField.value is not None:
                    self.parent.screen.addstr(yPos + 1, xPos, ' Status:')
                    yPos += 2
                    for line in ComponentPage.prepareComponentFlags(component, component.flagField):
                        if yPos < bottom[1] - ComponentPage.MARGIN_BOTTOM:
                            self.parent.screen.addstr(yPos, xPos, line)
                            yPos += 1
            elif self.pane == 2 or self.pane == 3:
                field = component.fields[self.field]
                if not self.editModeEnabled:
                    # Print field value
                    line = self.prepareFieldValue(field)

                    if self.pane == 3:
                        color = Window.Color.ERROR if self.fieldValueError else Window.Color.HOVER

                        if yPos < bottom[1] - ComponentPage.MARGIN_BOTTOM:
                            self.parent.screen.attron(curses.color_pair(color))
                            self.parent.screen.addstr(yPos, xPos, line)
                            self.parent.screen.addstr(yPos, xPos + len(line), ' ' * (width - len(line)))
                            self.parent.screen.attroff(curses.color_pair(color))
                            yPos += 1
                    else:
                        if yPos < bottom[1] - ComponentPage.MARGIN_BOTTOM:
                            self.parent.screen.addstr(yPos, xPos, line)
                            yPos += 1
                else:
                    line = self.prepareEditBox()

                    # Print edit box
                    if yPos < bottom[1] - ComponentPage.MARGIN_BOTTOM:
                        # Preserve cursor position
                        self.cursor = (xPos + len(line), yPos)

                        self.parent.screen.attron(curses.color_pair(Window.Color.HOVER))
                        self.parent.screen.addstr(yPos, xPos, line)
                        self.parent.screen.addstr(yPos, xPos + len(line), ' ' * (width - len(line)))
                        self.parent.screen.attroff(curses.color_pair(Window.Color.HOVER))
                        yPos += 1

                # Print details about field
                for line in ComponentPage.prepareFieldDescription(component, field):
                    if yPos < bottom[1] - ComponentPage.MARGIN_BOTTOM:
                        self.parent.screen.addstr(yPos, xPos, line)
                        yPos += 1

        hub.mutex.release() #TODO Reduce scope

    def drawWatchList(self, top, bottom):
        self.parent.screen.addstr(top[1], top[0], 'Watch list',
                curses.color_pair(Window.Color.DEFAULT) | curses.A_BOLD)

        width, height = bottom[0] - top[0], bottom[1] - top[1]
        xPos, yPos = top[0], ComponentPage.MARGIN_TOP

        hub = self.parent.hub
        hub.mutex.acquire()

        #TODO Draw partially

        i = 0
        for entry in self.watchList:
            if entry.component in hub.components and entry.field < len(hub.components[entry.component].fields):
                component = self.parent.hub.components[entry.component]
                field = component.fields[entry.field]

                fieldValueText = '{} '.format(field.value)
                fieldEntryText = ' {:s}:{:s}'.format(component.name, field.name)
                fieldEntryText += ' ' * (max(1, width - len(fieldEntryText) - len(fieldValueText))) + fieldValueText
                fieldEntryText = fieldEntryText[:width]

                self.parent.screen.addstr(top[1] + ComponentPage.MARGIN_TOP + i,# - section[0],
                        top[0], fieldEntryText, curses.color_pair(Window.Color.DEFAULT))
                i += 1

        hub.mutex.release() #TODO Reduce scope

    def drawStatusBar(self, top, bottom):
        width = bottom[0] - top[0]

        if self.errorMessage is not None:
            errorText = ' ' * FilePage.MARGIN_LEFT + self.errorMessage
            self.parent.screen.attron(curses.color_pair(Window.Color.HOVER_ERROR))
            self.parent.screen.addstr(top[1], top[0], errorText)
            self.parent.screen.addstr(top[1], top[0] + len(errorText), ' ' * (width - len(errorText) - 1))
            self.parent.screen.attroff(curses.color_pair(Window.Color.HOVER_ERROR))
        else:
            self.drawProgressBar(top, bottom)

    def enter(self):
        GenericPage.enter(self)
        self.parent.hub.onFieldsChanged = self.onFieldsChanged

    def leave(self):
        self.parent.hub.onFieldsChanged = None
        GenericPage.leave(self)

    def draw(self):
        height, width = self.parent.screen.getmaxyx()
        self.drawComponentPane((0, 0), (int(width / 3), int(2 * height / 3)))
        self.drawFieldPane((int(width / 3), 0), (int(2 * width / 3), int(2 * height / 3)))
        self.drawPropertiesPane((int(2 * width / 3), 0), (width, int(2 * height / 3)))
        self.drawWatchList((0, int(2 * height / 3)), (width, height - 1))
        self.drawStatusBar((0, height - 1), (width, height))
        self.drawCursor()

    def handleUserInput(self, key):
        if self.pane < 0:
            if key == curses.KEY_DOWN:
                self.watchIndex = max(min(self.watchIndex + 1, len(self.watchList) - 1), 0)
            elif key == curses.KEY_UP:
                self.watchIndex = max(min(self.watchIndex - 1, len(self.watchList) - 1), 0)
            elif key == ord('d'):
                if self.watchIndex > 0 and self.watchIndex < len(self.watchList) - 1:
                    self.watchList = self.watchList[:self.watchIndex] + self.watchList[self.watchIndex + 1:]
                elif self.watchIndex == 0:
                    self.watchList = self.watchList[1:]
                elif self.watchIndex == len(self.watchList) - 1:
                    self.watchList = self.watchList[:-1]
                self.watchIndex = max(min(self.watchIndex, len(self.watchList) - 1), 0)
            elif key == ord('\t'):
                self.pane = -self.pane
            else:
                self.parent.handleUserInput(key)
        elif self.pane == 1:
            # Component list
            self.parent.hub.mutex.acquire()
            count = len(self.parent.hub.components)
            self.parent.hub.mutex.release()

            if key == ord('r'):
                self.parent.hub.messenger.resetProgress()
                self.parent.hub.mutex.acquire()
                for component in self.parent.hub.components.values():
                    component.readFields(lambda: self.onFieldsLoaded(component.address))
                self.parent.hub.mutex.release()
            elif key == curses.KEY_DOWN:
                self.component = max(min(self.component + 1, count - 1), 0)
            elif key == curses.KEY_UP:
                self.component = max(min(self.component - 1, count - 1), 0)
            elif key == curses.KEY_RIGHT:
                self.field = 0
                self.pane += 1
            elif key == ord('\t'):
                self.pane = -self.pane
            else:
                self.parent.handleUserInput(key)
        elif self.pane == 2:
            # Field list
            self.parent.hub.mutex.acquire()
            if self.component in self.parent.hub.components:
                component = self.parent.hub.components[self.component]
                count = len(component.fields)
                field = component.fields[self.field]
            else:
                component = None
            self.parent.hub.mutex.release()

            if component is not None:
                if self.editModeEnabled:
                    if isCorrectChar(key):
                        self.editModeValue += chr(key)
                    elif key == curses.KEY_BACKSPACE:
                        if len(self.editModeValue) > 0:
                            self.editModeValue = self.editModeValue[:-1]
                else:
                    if key == ord('r'):
                        self.parent.hub.messenger.resetProgress()
                        component.readFields(lambda: self.onFieldsLoaded(component.address))
                    elif key == ord('w'):
                        # TODO Check existence
                        self.watchList.append(WatchEntry(self.component, self.field))

                if key == curses.KEY_DOWN:
                    self.field = max(min(self.field + 1, count - 1), 0)
                    self.resetEditMode()
                elif key == curses.KEY_UP:
                    self.field = max(min(self.field - 1, count - 1), 0)
                    self.resetEditMode()
                elif key == curses.KEY_LEFT:
                    self.pane -= 1
                    self.resetEditMode()
                elif key == ord('\n'):
                    if not self.editModeEnabled:
                        self.enterEditMode()
                    else:
                        self.exitEditMode()
                        value = proto.Protocol.stringToValue(field.type, field.size, self.editModeValue)
                        if value is not None:
                            field.write(value, self.onFieldUpdated)
                        else:
                            self.fieldValueError = True
                            self.errorMessage = 'Incorrect value'
                elif key == ord('\t'):
                    self.pane = -self.pane
                elif not self.editModeEnabled:
                    self.parent.handleUserInput(key)
            else:
                if key == curses.KEY_LEFT:
                    self.pane -= 1
                elif key == ord('\t'):
                    self.pane = -self.pane
                else:
                    self.parent.handleUserInput(key)
        else:
            raise Exception()

    def onFieldsLoaded(self, component):
        # Callback for reload event
        if self.component == component:
            self.parent.sem.release()

    def onFieldsChanged(self, component, fields):
        # Callback for automatic updates
        if self.component == component or self.isFieldsInWatchList(component, fields):
            self.parent.sem.release()

    def onFieldUpdated(self, field, result, value):
        if result.value == proto.Result.SUCCESS:
            self.parent.sem.release()
        else:
            self.errorMessage = str(result)

    def onUpdateEvent(self):
        GenericPage.onUpdateEvent(self)
        self.parent.sem.release()


class GlobalInfoPage(GenericPage):
    MARGIN_TOP, MARGIN_BOTTOM = 2, 1
    MARGIN_Y = MARGIN_TOP + MARGIN_BOTTOM
    PARAM_IDLE, PARAM_CHANGED, PARAM_SELECTED = range(0, 3)

    def __init__(self, parent):
        GenericPage.__init__(self, parent)

        self.name = 'Information'
        self.pane = 0
        self.cmd = 0
        self.param = 0
        self.paramCountReq = False
        self.pendingParams = []

        self.filterModeEnabled = False
        self.loadModeEnabled = False
        self.saveModeEnabled = False
        self.filterValue = ''
        self.fieldValueError = False
        self.editModeEnabled = False
        self.editModeValue = ''
        self.errorMessage = None

    def enter(self):
        GenericPage.enter(self)
        self.errorMessage = None

    def enterEditMode(self):
        self.fieldValueError = False
        self.editModeEnabled = True
        self.editModeValue = ''
        curses.curs_set(1)

    def exitEditMode(self):
        self.editModeEnabled = False
        self.cursor = None
        curses.curs_set(0)

    def resetEditMode(self):
        if self.editModeEnabled:
            self.loadModeEnabled = False
            self.saveModeEnabled = False
            self.filterModeEnabled = False
            self.editModeEnabled = False
            self.cursor = None
            curses.curs_set(0)
        self.fieldValueError = False
        self.editModeValue = ''

    def drawCommandPane(self, top, bottom):
        self.parent.screen.addstr(top[1], top[0], 'Commands', curses.color_pair(Window.Color.DEFAULT) | curses.A_BOLD)
        width, height = bottom[0] - top[0], bottom[1] - top[1]

        count = len(proto.Protocol.SYSTEM_COMMANDS)

        if self.cmd >= GlobalInfoPage.MARGIN_TOP and height - GlobalInfoPage.MARGIN_Y < count:
            startIndex = min(self.cmd - GlobalInfoPage.MARGIN_TOP, count - height + GlobalInfoPage.MARGIN_Y)
            section = (startIndex, startIndex + min(height - GlobalInfoPage.MARGIN_Y, count - startIndex))
        else:
            section = (0, min(height - GlobalInfoPage.MARGIN_Y, count))

        for i in range(section[0], section[1]):
            color = Window.Color.default(self.pane == 1 and i == self.cmd)
            key = list(proto.Protocol.SYSTEM_COMMANDS)[i]
            commandEntryText = ' {:<2d}  {:s}'.format(key, proto.Protocol.SYSTEM_COMMANDS[key])
            commandEntryText = commandEntryText[:width]

            self.parent.screen.attron(curses.color_pair(color))
            self.parent.screen.addstr(top[1] + GlobalInfoPage.MARGIN_TOP + i - section[0],
                    top[0], commandEntryText)
            self.parent.screen.addstr(top[1] + GlobalInfoPage.MARGIN_TOP + i - section[0],
                    top[0] + len(commandEntryText), ' ' * (width - len(commandEntryText)))
            self.parent.screen.attroff(curses.color_pair(color))

    def filterParamList(self, params, words):
        result = []
        for key in params:
            failed = False
            for word in [x for x in words if len(x) > 0]:
                failed = re.search(word, params[key][0], re.I) is None
                if failed:
                    break
            if not failed:
                result.append((key, params[key][0], params[key][1], None, GlobalInfoPage.PARAM_IDLE))
        return result

    def injectPendingParams(self, params):
        for entry in self.pendingParams:
            for i in range(0, len(params)):
                p = params[i]
                if p[1] == entry[0] and not math.isclose(a=p[2], b=entry[1], rel_tol=1e-5):
                    params[i] = (p[0], p[1], p[2], entry[1],
                            GlobalInfoPage.PARAM_SELECTED if entry[2] else GlobalInfoPage.PARAM_CHANGED)
                    break

    def getFilteredParamList(self):
        params = self.parent.hub.parameters
        if self.filterModeEnabled:
            params = self.filterParamList(params, self.editModeValue.split(' '))
        elif self.filterValue != '':
            params = self.filterParamList(params, self.filterValue.split(' '))
        else:
            params = [(key, params[key][0], params[key][1], None, GlobalInfoPage.PARAM_IDLE) for key in params]
        self.injectPendingParams(params)
        params.sort(key=lambda x: x[0])
        params.sort(key=lambda x: x[4] == GlobalInfoPage.PARAM_IDLE)
        return params

    def markCurrentParam(self):
        paramName = self.getFilteredParamList()[self.param][1]
        for i in range(0, len(self.pendingParams)):
            p = self.pendingParams[i]
            if p[0] == paramName:
                self.pendingParams[i] = (p[0], p[1], not p[2])
                break

    def flushMarkedParams(self):
        selCount = 0
        self.parent.hub.messenger.resetProgress()
        for i in range(0, len(self.pendingParams)):
            entry = self.pendingParams[i]
            if entry[2]:
                self.parent.hub.setParam(value=entry[1], name=entry[0],
                        callback=lambda number, value, name=entry[0]: self.onParamFlushed(number, value, name))
                selCount += 1
        if selCount == 0:
            self.pendingParams = []

    def drawParameterPane(self, top, bottom):
        self.parent.screen.addstr(top[1], top[0], 'Parameters',
                curses.color_pair(Window.Color.DEFAULT) | curses.A_BOLD)
        width, height = bottom[0] - top[0], bottom[1] - top[1]

        params = self.getFilteredParamList()
        count = len(params)
        currentParam = min(self.param, count - 1)
        if currentParam != self.param:
            self.param = currentParam

        if currentParam >= height // 2 and height - GlobalInfoPage.MARGIN_Y < count:
            startIndex = min(currentParam - height // 2, count - height + GlobalInfoPage.MARGIN_Y)
            section = (startIndex, startIndex + min(height - GlobalInfoPage.MARGIN_Y, count - startIndex))
        else:
            section = (0, min(height - GlobalInfoPage.MARGIN_Y, count))

        if count == 0:
            helpText = [
                    ' Commands:',
                    '   R to read from device',
                    '   L to load file',
                    '   S to save file',
                    '   / to filter',
                    '   F to flush',
                    '   I to invert selection',
                    '   Space to select']
            color = Window.Color.default(self.pane == 0)

            self.parent.screen.attron(curses.color_pair(color))
            self.parent.screen.addstr(top[1] + GlobalInfoPage.MARGIN_TOP - section[0], top[0], helpText[0])
            self.parent.screen.addstr(top[1] + GlobalInfoPage.MARGIN_TOP - section[0], top[0] + len(helpText[0]),
                    ' ' * (width - len(helpText[0])))
            self.parent.screen.attroff(curses.color_pair(color))

            self.parent.screen.attron(curses.color_pair(Window.Color.default(False)))
            for i in range(1, len(helpText)):
                self.parent.screen.addstr(top[1] + GlobalInfoPage.MARGIN_TOP - section[0] + i, top[0], helpText[i])
            self.parent.screen.attroff(curses.color_pair(Window.Color.default(False)))

        for i in range(section[0], section[1]):
            key, paramName, paramValue, newParamValue, mode = params[i]

            if self.editModeEnabled and not (self.loadModeEnabled or self.saveModeEnabled or self.filterModeEnabled)\
                    and i == currentParam:
                paramValueText = '{} '.format(self.editModeValue)
                paramEntryText = ' {:<3d} {:s}'.format(key, paramName)
                paramEntryText += ' ' * (max(1, width - len(paramEntryText) - len(paramValueText))) + paramValueText
                self.cursor = (top[0] + len(paramEntryText) - 2, top[1] + GlobalInfoPage.MARGIN_TOP + i - section[0])
            else:
                if newParamValue is None:
                    paramValueText = '{:g} '.format(paramValue)
                else:
                    paramValueText = '{:g} -> {:g} '.format(paramValue, newParamValue)
                paramEntryText = ' {:<3d} {:s}'.format(key, paramName)
                paramEntryText += ' ' * (max(1, width - len(paramEntryText) - len(paramValueText))) + paramValueText
            paramEntryText = paramEntryText[:width]

            if self.pane == 0 and i == currentParam:
                if self.fieldValueError:
                    color = Window.Color.ERROR
                elif mode == GlobalInfoPage.PARAM_SELECTED:
                    color = Window.Color.HOVER_READY
                elif mode == GlobalInfoPage.PARAM_CHANGED:
                    color = Window.Color.HOVER_WARNING
                else:
                    color = Window.Color.HOVER

                self.parent.screen.attron(curses.color_pair(color))
                self.parent.screen.addstr(top[1] + GlobalInfoPage.MARGIN_TOP + i - section[0],
                        top[0], paramEntryText)
                self.parent.screen.addstr(top[1] + GlobalInfoPage.MARGIN_TOP + i - section[0],
                        top[0] + len(paramEntryText), ' ' * (width - len(paramEntryText)))
                self.parent.screen.attroff(curses.color_pair(color))
            else:
                if mode == GlobalInfoPage.PARAM_SELECTED:
                    color = Window.Color.READY
                elif mode == GlobalInfoPage.PARAM_CHANGED:
                    color = Window.Color.WARNING
                else:
                    color = Window.Color.DEFAULT

                self.parent.screen.addstr(top[1] + GlobalInfoPage.MARGIN_TOP + i - section[0],
                        top[0], paramEntryText, curses.color_pair(color))

    def drawStatusBar(self, top, bottom):
        width = bottom[0] - top[0]

        if (self.filterModeEnabled or self.loadModeEnabled or self.saveModeEnabled) and self.editModeEnabled:
            editModeText = ' ' * FilePage.MARGIN_LEFT + self.editModeValue
            self.parent.screen.attron(curses.color_pair(Window.Color.HOVER))
            self.parent.screen.addstr(top[1], top[0], editModeText)
            self.parent.screen.addstr(top[1], top[0] + len(editModeText), ' ' * (width - len(editModeText) - 1))
            self.parent.screen.attroff(curses.color_pair(Window.Color.HOVER))
            self.cursor = (top[0] + len(editModeText), top[1])
        elif self.errorMessage is not None:
            errorText = ' ' * FilePage.MARGIN_LEFT + self.errorMessage[1]
            self.parent.screen.attron(curses.color_pair(self.errorMessage[0]))
            self.parent.screen.addstr(top[1], top[0], errorText)
            self.parent.screen.addstr(top[1], top[0] + len(errorText), ' ' * (width - len(errorText) - 1))
            self.parent.screen.attroff(curses.color_pair(self.errorMessage[0]))
        else:
            self.drawProgressBar(top, bottom)

    def draw(self):
        height, width = self.parent.screen.getmaxyx()
        self.drawParameterPane((0, 0), (int(width / 2), height - 1))
        self.drawCommandPane((int(width / 2), 0), (width, height - 1))
        # self.drawPropertiesPane((2 * width / 3, 0), (width, height - 1))
        self.drawStatusBar((0, height - 1), (width, height))
        self.drawCursor()

    @staticmethod
    def unwrapUserInput(s):
        if len(s) >= 2 and s[0] in ('\'', '"') and s[-1] in ('\'', '"'):
            return s[1:-1]
        else:
            return s

    def handleUserInput(self, key):
        if self.pane == 0:
            count = len(self.parent.hub.parameters)

            if self.editModeEnabled:
                if isCorrectChar(key):
                    self.editModeValue += chr(key)
                elif key == curses.KEY_BACKSPACE:
                    if len(self.editModeValue) > 0:
                        self.editModeValue = self.editModeValue[:-1]
            else:
                if key == ord('r'):
                    self.resetEditMode()
                    self.errorMessage = None
                    self.paramCountReq = True
                    self.param = 0
                    self.parent.hub.clearParamList()
                    self.parent.hub.getParamCount(self.onParamCountReceived)
                elif key == ord('f'):
                    self.flushMarkedParams()
                elif key == ord('l'):
                    self.enterEditMode()
                    self.errorMessage = None
                    self.loadModeEnabled = True
                elif key == ord('s'):
                    self.enterEditMode()
                    self.errorMessage = None
                    self.saveModeEnabled = True
                elif key == ord(' '):
                    self.markCurrentParam()
                elif key == ord('/'):
                    self.enterEditMode()
                    self.errorMessage = None
                    self.filterModeEnabled = True

            if key == curses.KEY_DOWN:
                self.resetEditMode()
                self.param = max(min(self.param + 1, count - 1), 0)
            elif key == curses.KEY_UP:
                self.resetEditMode()
                self.param = max(min(self.param - 1, count - 1), 0)
            elif key == curses.KEY_RIGHT:
                self.resetEditMode()
                self.pane += 1
            elif key == curses.KEY_NPAGE:
                self.resetEditMode()
                self.param = max(min(self.param + 24, count - 1), 0)
            elif key == curses.KEY_PPAGE:
                self.resetEditMode()
                self.param = max(min(self.param - 24, count - 1), 0)
            elif key == ord('\n'):
                if not self.editModeEnabled:
                    self.errorMessage = None
                    self.enterEditMode()
                else:
                    self.exitEditMode()

                    if self.filterModeEnabled:
                        self.filterValue = self.editModeValue
                        self.filterModeEnabled = False
                        self.param = 0
                    elif self.loadModeEnabled:
                        self.loadModeEnabled = False
                        self.editModeValue = GlobalInfoPage.unwrapUserInput(self.editModeValue.strip())
                        data = self.loadFromFile(self.editModeValue)
                        if data is not None:
                            self.pendingParams = self.parent.hub.paramFromInf(data)
                            for i in range(0, len(self.pendingParams)):
                                self.pendingParams[i] = (*self.pendingParams[i], False)
                        else:
                            self.errorMessage = (Window.Color.HOVER_ERROR, 'Incorrect file')
                    elif self.saveModeEnabled:
                        self.saveModeEnabled = False
                        self.editModeValue = GlobalInfoPage.unwrapUserInput(self.editModeValue.strip())
                        data = self.parent.hub.paramToInf()
                        if not self.saveToFile(self.editModeValue, data):
                            self.errorMessage = (Window.Color.HOVER_ERROR, 'Incorrect file')
                    else:
                        try:
                            value = float(self.editModeValue)
                            # paramIndex = self.getFilteredParamList()[self.param][0]
                            # self.parent.hub.setParam(value=value, number=paramIndex, callback=self.onParamChanged)
                            # TODO
                            paramName = self.getFilteredParamList()[self.param][1]
                            self.parent.hub.setParam(value=value, name=paramName, callback=self.onParamChanged)
                        except:
                            self.fieldValueError = True

            elif not self.editModeEnabled:
                self.parent.handleUserInput(key)
        elif self.pane == 1:
            if key == curses.KEY_DOWN:
                self.cmd = max(min(self.cmd + 1, len(proto.Protocol.SYSTEM_COMMANDS) - 1), 0)
            elif key == curses.KEY_UP:
                self.cmd = max(min(self.cmd - 1, len(proto.Protocol.SYSTEM_COMMANDS) - 1), 0)
            elif key == curses.KEY_LEFT:
                self.pane -= 1
            elif key == ord('\n'):
                self.errorMessage = None
                self.parent.hub.messenger.resetProgress()
                self.parent.hub.sendCommand(list(proto.Protocol.SYSTEM_COMMANDS)[self.cmd],
                        callback=self.onCommandCompleted)
            else:
                self.parent.handleUserInput(key)
        else:
            raise Exception()

    def loadFromFile(self, path):
        try:
            return open(path, 'rb').read().decode()
        except:
            return None

    def saveToFile(self, path, data):
        try:
            open(path, 'wb').write(data.encode())
            return True
        except:
            return False

    def onParamCountReceived(self, total):
        if self.paramCountReq:
            self.paramCountReq = False
            self.parent.hub.messenger.resetProgress()
            for i in range(0, total):
                self.parent.hub.getParam(i, self.onParamReceived)

    def onParamReceived(self, number, value):
        if value is not None:
            self.parent.sem.release()

    def onParamFlushed(self, number, value, name):
        selCount = 0
        for i in range(0, len(self.pendingParams)):
            p = self.pendingParams[i]
            if p[0] == name:
                self.pendingParams[i] = (p[0], p[1], False)
            if self.pendingParams[i][2]:
                selCount += 1
        if selCount == 0:
            self.pendingParams = []
        self.parent.sem.release()

    def onParamChanged(self, number, value):
        self.parent.sem.release()

    def onCommandCompleted(self, code, result):
        if result.value == proto.Result.SUCCESS:
            self.errorMessage = (Window.Color.HOVER_READY, str(result))
        elif result.value == proto.Result.COMMAND_QUEUED:
            self.errorMessage = (Window.Color.HOVER_WARNING, str(result))
        else:
            self.errorMessage = (Window.Color.HOVER_ERROR, str(result))
        self.parent.sem.release()


class SystemInfoPage(GenericPage):
    MARGIN_TOP, MARGIN_BOTTOM = 2, 1
    MARGIN_LEFT, MARGIN_RIGHT = 1, 1
    MARGIN_Y = MARGIN_TOP + MARGIN_BOTTOM

    def __init__(self, parent):
        GenericPage.__init__(self, parent)

        self.name = 'Statistics'

    def drawRatePane(self, top, bottom):
        self.parent.screen.addstr(top[1], top[0], 'Speed', curses.color_pair(Window.Color.DEFAULT) | curses.A_BOLD)
        width, height = bottom[0] - top[0], bottom[1] - top[1]

        rates = self.parent.hub.getMessageRates()
        maxRate = max([max(x) for x in rates]) if len(rates) > 0 else 0

        for i in range(0, min(len(rates), height - SystemInfoPage.MARGIN_Y)):
            entry = rates[i]
            parts = (float(entry[0]) / maxRate, float(entry[1]) / maxRate) if maxRate > 0 else (0, 0)
            text = '{:d} / {:d} B/s'.format(entry[0], entry[1])[:width]

            self.parent.screen.addstr(top[1] + i + SystemInfoPage.MARGIN_TOP, top[0] + SystemInfoPage.MARGIN_LEFT, text)

    def drawStatPane(self, top, bottom):
        self.parent.screen.addstr(top[1], top[0], 'Statistics', curses.color_pair(Window.Color.DEFAULT) | curses.A_BOLD)
        width, height = bottom[0] - top[0], bottom[1] - top[1]

        stats = list(self.parent.hub.getMessageStats().items())
        stats.sort(key=lambda x: x[0])

        maxWidth = 1
        for entry in stats:
            entryWidth = int(math.ceil(math.log10(entry[1]))) if entry[1] > 0 else 1
            if entryWidth > maxWidth:
                maxWidth = entryWidth

        textFormat = '{{:<{:d}d}}  0x{{:02X}} ({{:s}})'.format(maxWidth)
        for i in range(0, min(len(stats), height - SystemInfoPage.MARGIN_Y)):
            text = textFormat.format(stats[i][1], stats[i][0], str(proto.Message(stats[i][0])))[:width]
            self.parent.screen.addstr(top[1] + i + SystemInfoPage.MARGIN_TOP, top[0] + SystemInfoPage.MARGIN_LEFT, text)

    def drawCounterPane(self, top, bottom):
        self.parent.screen.addstr(top[1], top[0], 'Counters', curses.color_pair(Window.Color.DEFAULT) | curses.A_BOLD)
        width, height = bottom[0] - top[0], bottom[1] - top[1]

        counters = self.parent.hub.getMessageCounters()
        latency = self.parent.hub.getMessageLatency()

        self.parent.screen.addstr(top[1] + SystemInfoPage.MARGIN_TOP + 0, top[0], ' Received  {:d}'.format(counters[0]))
        self.parent.screen.addstr(top[1] + SystemInfoPage.MARGIN_TOP + 1, top[0], ' Sent      {:d}'.format(counters[1]))
        self.parent.screen.addstr(top[1] + SystemInfoPage.MARGIN_TOP + 2, top[0], ' Errors    {:d}'.format(counters[2]))
        self.parent.screen.addstr(top[1] + SystemInfoPage.MARGIN_TOP + 3, top[0], ' Latency   {:.5f}'.format(latency))

    def draw(self):
        height, width = self.parent.screen.getmaxyx()
        self.drawRatePane((0, 0), (int(width / 4), height - 1))
        self.drawStatPane((int(width / 4), 0), (int(3 * width / 4), height - 1))
        self.drawCounterPane((int(3 * width / 4), 0), (width, height - 1))
        self.drawProgressBar((0, height - 1), (width, height))

    def handleUserInput(self, key):
        self.parent.handleUserInput(key)

    def onUpdateEvent(self):
        self.parent.sem.release()


class FilePage(GenericPage):
    MARGIN_TOP, MARGIN_BOTTOM = 2, 1
    MARGIN_LEFT, MARGIN_RIGHT = 1, 1
    MARGIN_Y = MARGIN_TOP + MARGIN_BOTTOM
    MARGIN_X = MARGIN_LEFT + MARGIN_RIGHT

    CMD_IDLE, CMD_SAVE, CMD_LOAD, CMD_APPEND = range(0, 4)
    MODE_INFO, MODE_HEX, MODE_STR = range(0, 3)

    def __init__(self, parent):
        GenericPage.__init__(self, parent)

        self.name = 'File tree'
        self.pane = 0
        self.entry = 0
        self.line = 0
        self.cmd = FilePage.CMD_IDLE
        self.mode = FilePage.MODE_INFO

        self.parser = None

        self.errorMessage = None
        self.editModeEnabled = False
        self.editModeValue = ''

    def enterEditMode(self):
        self.errorMessage = None
        self.editModeEnabled = True
        curses.curs_set(1)

    def exitEditMode(self):
        self.editModeEnabled = False
        self.cursor = None
        curses.curs_set(0)

    def resetEditMode(self):
        if self.editModeEnabled:
            self.editModeEnabled = False
            self.cursor = None
            curses.curs_set(0)
        self.errorMessage = None

    def makeParser(self, stream):
        for parser in file_parser.parsers:
            if parser.match(stream):
                return parser(stream)
        raise Exception()

    def proposeSettings(self, stream):
        for parser in file_parser.parsers:
            if parser.match(stream):
                return parser.propose(stream)
        raise Exception()

    def getFileTree(self):
        entries = []
        self.parent.hub.mutex.acquire()
        for component in self.parent.hub.components.values():
            for stream in component.files.values():
                entries.append(stream)
        self.parent.hub.mutex.release()
        return entries

    def drawTreePane(self, top, bottom):
        self.parent.screen.addstr(top[1], top[0], 'File tree',
                curses.color_pair(Window.Color.DEFAULT) | curses.A_BOLD)
        width, height = bottom[0] - top[0], bottom[1] - top[1]

        tree = self.getFileTree()
        previous = None
        yPos = 0

        # FIXME
        if self.entry >= ComponentPage.MARGIN_TOP and height - ComponentPage.MARGIN_Y < len(tree):
            startIndex = min(self.entry - ComponentPage.MARGIN_TOP, len(tree) - height + ComponentPage.MARGIN_Y)
            section = (startIndex, startIndex + min(height - ComponentPage.MARGIN_Y, len(tree) - startIndex))
        else:
            section = (0, min(height - ComponentPage.MARGIN_Y, len(tree)))

        if len(tree) == 0:
            helpText = ' Files not found'
            color = Window.Color.default(self.pane == 0)

            self.parent.screen.attron(curses.color_pair(color))
            self.parent.screen.addstr(top[1] + GlobalInfoPage.MARGIN_TOP - section[0], top[0], helpText)
            self.parent.screen.addstr(top[1] + GlobalInfoPage.MARGIN_TOP - section[0], top[0] + len(helpText),
                    ' ' * (width - len(helpText)))
            self.parent.screen.attroff(curses.color_pair(color))

        for i in range(section[0], section[1]):
            stream = tree[i]
            component = stream.component

            rowSelected = self.pane == 0 and self.entry == i
            color = Window.Color.default(rowSelected)

            if component != previous:
                componentEntryText = ' {:<3d} {:s}'.format(component.address, component.name)
                componentEntryText = componentEntryText[:width]
                self.parent.screen.addstr(top[1] + ComponentPage.MARGIN_TOP + yPos, top[0], componentEntryText)
                previous = component
                yPos += 1

            streamEntryText = '    File {:d} ({:s})'.format(stream.index, proto.File.flagsToString(stream.status))
            streamEntryText = streamEntryText[:width]

            self.parent.screen.attron(curses.color_pair(color))
            self.parent.screen.addstr(top[1] + ComponentPage.MARGIN_TOP + yPos, top[0],
                    streamEntryText)
            self.parent.screen.addstr(top[1] + ComponentPage.MARGIN_TOP + yPos, top[0] + len(streamEntryText),
                    ' ' * (width - len(streamEntryText)))
            self.parent.screen.attroff(curses.color_pair(color))
            yPos += 1

    def drawDataPane(self, top, bottom):
        self.parent.screen.addstr(top[1], top[0], 'Data',
                curses.color_pair(Window.Color.DEFAULT) | curses.A_BOLD)
        width, height = bottom[0] - top[0], bottom[1] - top[1]
        maxRows = height - FilePage.MARGIN_Y

        tree = self.getFileTree()
        if self.entry < len(tree):
            stream = tree[self.entry]

            if stream.data is not None and len(stream.data) == 0:
                self.parent.screen.addstr(top[1] + FilePage.MARGIN_TOP, top[0] + FilePage.MARGIN_LEFT, 'Empty file')
            elif stream.data is not None and len(stream.data) > 0:
                if self.parser is None:
                    self.parser = self.makeParser(tree[self.entry])

                windowSize = (width - FilePage.MARGIN_X, height - FilePage.MARGIN_Y)
                if self.mode == FilePage.MODE_HEX:
                    cache = self.parser.raw(windowSize)
                elif self.mode == FilePage.MODE_STR:
                    cache = self.parser.native(windowSize)
                else:
                    cache = self.parser.info(windowSize)

                # TODO Rewrite using pages
                if self.line >= FilePage.MARGIN_TOP and height - FilePage.MARGIN_Y < len(cache):
                    startIndex = min(self.line - FilePage.MARGIN_TOP, len(cache) - height + FilePage.MARGIN_Y)
                    section = (startIndex, startIndex + min(height - FilePage.MARGIN_Y, len(cache) - startIndex))
                else:
                    section = (0, min(height - FilePage.MARGIN_Y, len(cache)))

                for i in range(section[0], section[1]):
                    self.parent.screen.addstr(top[1] + FilePage.MARGIN_TOP + i - section[0],
                            top[0] + FilePage.MARGIN_LEFT, cache[i])

    def drawStatusBar(self, top, bottom):
        width = bottom[0] - top[0]

        if self.errorMessage is not None:
            errorText = ' ' * FilePage.MARGIN_LEFT + self.errorMessage
            self.parent.screen.attron(curses.color_pair(Window.Color.HOVER_ERROR))
            self.parent.screen.addstr(top[1], top[0], errorText)
            self.parent.screen.addstr(top[1], top[0] + len(errorText), ' ' * (width - len(errorText) - 1))
            self.parent.screen.attroff(curses.color_pair(Window.Color.HOVER_ERROR))
        elif self.editModeEnabled:
            editModeText = ' ' * FilePage.MARGIN_LEFT + self.editModeValue
            self.parent.screen.attron(curses.color_pair(Window.Color.HOVER))
            self.parent.screen.addstr(top[1], top[0], editModeText)
            self.parent.screen.addstr(top[1], top[0] + len(editModeText), ' ' * (width - len(editModeText) - 1))
            self.parent.screen.attroff(curses.color_pair(Window.Color.HOVER))
            self.cursor = (top[0] + len(editModeText), top[1])
        else:
            self.drawProgressBar(top, bottom)

    def draw(self):
        height, width = self.parent.screen.getmaxyx()
        self.drawTreePane((0, 0), (int(width / 3), height - 1))
        self.drawDataPane((int(width / 3), 0), (width, height - 1))
        self.drawStatusBar((0, height - 1), (width, height))
        self.drawCursor()

    @staticmethod
    def unwrapUserInput(s):
        if len(s) >= 2 and s[0] in ('\'', '"') and s[-1] in ('\'', '"'):
            return s[1:-1]
        else:
            return s

    def handleUserInput(self, key):
        if self.pane == 0:
            count = len(self.getFileTree())

            if self.editModeEnabled:
                if isCorrectChar(key):
                    self.editModeValue += chr(key)
                elif key == curses.KEY_BACKSPACE:
                    if len(self.editModeValue) > 0:
                        self.editModeValue = self.editModeValue[:-1]
            else:
                if key == ord('i'):
                    if self.mode != FilePage.MODE_INFO:
                        self.mode = FilePage.MODE_INFO
                        if self.parser is not None:
                            self.parser.reset()
                elif key == ord('n'):
                    if self.mode != FilePage.MODE_STR:
                        self.mode = FilePage.MODE_STR
                        if self.parser is not None:
                            self.parser.reset()
                elif key == ord('r'):
                    tree = self.getFileTree()
                    if self.entry < len(tree):
                        self.parent.hub.messenger.resetProgress()
                        self.resetEditMode()
                        self.parser = None
                        stream = tree[self.entry]
                        settings = self.proposeSettings(stream)
                        stream.read(chunkSize=settings['chunk'], burstSize=settings['burst'],
                                verify=settings['checksum'], callback=self.onFileTransferFinished)
                elif key == ord('a'):
                    self.cmd = FilePage.CMD_APPEND
                    self.enterEditMode()
                elif key == ord('l'):
                    self.cmd = FilePage.CMD_LOAD
                    self.enterEditMode()
                elif key == ord('s'):
                    self.cmd = FilePage.CMD_SAVE
                    self.enterEditMode()
                elif key == ord('x'):
                    if self.mode != FilePage.MODE_HEX:
                        self.mode = FilePage.MODE_HEX
                        if self.parser is not None:
                            self.parser.reset()

            if key == curses.KEY_DOWN:
                self.entry = max(min(self.entry + 1, count - 1), 0)
                self.resetEditMode()
                self.parser = None
            elif key == curses.KEY_UP:
                self.entry = max(min(self.entry - 1, count - 1), 0)
                self.resetEditMode()
                self.parser = None
            elif key == curses.KEY_NPAGE:
                if self.parser is not None and self.parser.cache is not None:
                    self.line = max(min(self.line + 1, len(self.parser.cache) - 1), 0)
                    self.resetEditMode()
            elif key == curses.KEY_PPAGE:
                if self.parser is not None and self.parser.cache is not None:
                    self.line = max(min(self.line - 1, len(self.parser.cache) - 1), 0)
                    self.resetEditMode()
            elif key == ord('\n'):
                if self.editModeEnabled:
                    self.exitEditMode()

                    if len(self.editModeValue) > 0:
                        if self.cmd == FilePage.CMD_SAVE:
                            self.editModeValue = FilePage.unwrapUserInput(self.editModeValue.strip())
                            if not self.saveToFile(self.entry, self.editModeValue):
                                self.errorMessage = 'Incorrect file'
                        elif self.cmd == FilePage.CMD_APPEND:
                            data = self.loadFromFile(self.editModeValue)
                            if data is not None:
                                self.parent.hub.messenger.resetProgress()
                                self.parser = None
                                tree = self.getFileTree()
                                if self.entry < len(tree):
                                    stream = tree[self.entry]
                                    settings = self.proposeSettings(stream)
                                    stream.write(data=data, chunkSize=settings['chunk'], burstSize=settings['burst'],
                                            append=True, verify=settings['checksum'],
                                            callback=self.onFileTransferFinished)
                            else:
                                self.errorMessage = 'Incorrect file'
                        elif self.cmd == FilePage.CMD_LOAD:
                            self.editModeValue = FilePage.unwrapUserInput(self.editModeValue.strip())
                            data = self.loadFromFile(self.editModeValue)
                            if data is not None:
                                self.parent.hub.messenger.resetProgress()
                                self.parser = None
                                tree = self.getFileTree()
                                if self.entry < len(tree):
                                    stream = tree[self.entry]
                                    settings = self.proposeSettings(stream)
                                    stream.write(data=data, chunkSize=settings['chunk'], burstSize=settings['burst'],
                                            verify=settings['checksum'], callback=self.onFileTransferFinished)
                            else:
                                self.errorMessage = 'Incorrect file'
                        else:
                            raise Exception()
                    else:
                        self.fieldValueError = False

                    self.cmd = FilePage.CMD_IDLE
            elif not self.editModeEnabled:
                self.parent.handleUserInput(key)
        else:
            raise Exception()

    def onFileTransferFinished(self, stream, result):
        if result.value == proto.Result.SUCCESS:
            self.parser = self.makeParser(stream)
            self.parent.sem.release()
        else:
            self.errorMessage = str(result)

    def onUpdateEvent(self):
        self.parent.sem.release()

    def loadFromFile(self, path):
        try:
            return open(path, 'rb').read()
        except:
            return None

    def saveToFile(self, entry, path):
        tree = self.getFileTree()
        if entry < len(tree):
            try:
                open(path, 'wb').write(tree[entry].data)
                return True
            except:
                return False
        else:
            return False


class Window:
    class Color:
        DEFAULT       = 1
        READY         = 2
        WARNING       = 3
        ERROR         = 4
        HOVER         = 5
        HOVER_READY   = 6
        HOVER_WARNING = 7
        HOVER_ERROR   = 8

        @staticmethod
        def default(hover):
            return Window.Color.HOVER if hover else Window.Color.DEFAULT

        @staticmethod
        def ready(hover):
            return Window.Color.HOVER_READY if hover else Window.Color.READY

        @staticmethod
        def warning(hover):
            return Window.Color.HOVER_WARNING if hover else Window.Color.WARNING

        @staticmethod
        def error(hover):
            return Window.Color.HOVER_ERROR if hover else Window.Color.ERROR


    def __init__(self, hub, screen):
        self.hub = hub
        self.screen = screen
        self.sem = threading.Semaphore(0)
        self.terminate = False

        self.page = 0
        self.pages = [
                ComponentPage(self),
                GlobalInfoPage(self),
                FilePage(self),
                SystemInfoPage(self)]
        self.pages[0].enter()

        # Clear and refresh the screen for a blank canvas
        curses.curs_set(0)
        curses.halfdelay(1)
        self.screen.clear()
        self.screen.refresh()

        # Start colors in curses
        curses.start_color()
        curses.init_pair(Window.Color.DEFAULT, curses.COLOR_WHITE, curses.COLOR_BLACK)
        curses.init_pair(Window.Color.READY, curses.COLOR_GREEN, curses.COLOR_BLACK)
        curses.init_pair(Window.Color.WARNING, curses.COLOR_YELLOW, curses.COLOR_BLACK)
        curses.init_pair(Window.Color.ERROR, curses.COLOR_RED, curses.COLOR_BLACK)
        curses.init_pair(Window.Color.HOVER, curses.COLOR_BLACK, curses.COLOR_CYAN)
        curses.init_pair(Window.Color.HOVER_READY, curses.COLOR_BLACK, curses.COLOR_GREEN)
        curses.init_pair(Window.Color.HOVER_WARNING, curses.COLOR_BLACK, curses.COLOR_YELLOW)
        curses.init_pair(Window.Color.HOVER_ERROR, curses.COLOR_BLACK, curses.COLOR_RED)

    def handleUserInput(self, key):
        nextPage = None

        if key in (ord('1'), ord('2'), ord('3'), ord('4')):
            nextPage = key - ord('1')
            if nextPage != self.page:
                self.pages[self.page].leave()
                self.page = nextPage
                self.pages[self.page].enter()
        elif key == ord('q'):
            self.pages[self.page].leave()
            self.terminate = True

    def draw(self):
        key = 0

        # Loop where key is the last character pressed
        while not self.terminate:
            if key == -1 and not self.sem.acquire(blocking=False):
                key = self.screen.getch()
                continue

            self.pages[self.page].handleUserInput(key)
            self.screen.clear()
            self.pages[self.page].draw()

            # Refresh the screen
            self.screen.refresh()
            # Wait for next input
            key = self.screen.getch()


args = argparse.ArgumentParser()
args.add_argument('--serial', dest='serial', help='serial port name', default='')
args.add_argument('--baudrate', dest='baudrate', help='serial port baud rate', type=int, default=57600)
args.add_argument('--address', dest='address', help='server address', default='')
args.add_argument('--modem', dest='modem', help='modem socket', default='1:2')
args.add_argument('--cache', dest='cache', help='component cache directory', default='cache')
args.add_argument('-d', dest='debug', help='enable debug messages', default=False, action='store_true')
args.add_argument('-v', dest='verbose', help='enable verbose mode', default=False, action='store_true')
options = args.parse_args()

proto.debugEnabled, proto.verboseEnabled = options.debug, options.verbose

if options.serial != '' and options.address != '':
    print('Only one connection type can be specified')
    raise Exception()
elif options.serial != '':
    stream = proto.SerialStream(options.serial, options.baudrate)
elif options.address != '':
    try:
        parts = options.address.split(':')
        if len(parts) == 1:
            address, port = parts[0], 5500
        else:
            address, port = parts[0], int(parts[1])
        parts = options.modem.split(':')
        modemAddress, modemPort = int(parts[0]), int(parts[1])

        stream = proto.NetworkStream(address, port, modemAddress, modemPort)
    except:
        print('Incorrect arguments')
        exit()
else:
    print('No connection type specified')
    raise Exception()

messenger = proto.Messenger(stream, options.cache)
messenger.connect()
curses.wrapper(lambda screen: Window(messenger.hub, screen).draw())
messenger.stop()
