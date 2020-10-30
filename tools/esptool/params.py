import proto
import serial.tools.list_ports as list_ports
import math
import time


def _parse_parameters(filename="par.properties"):
    """
    File is supposed to be entirely valid
    :param filename: name of the file
    """
    updater = []
    try:
        with open(filename, 'r') as f:
            updater = f.readlines()
        updater = [tuple(s.split('=')) for s in updater]
        updater = [s for s in updater if len(s) == 2]
        updater = [(s[0].strip(), float(s[1])) for s in updater]
        updater = [s for s in updater if s[0] != 'BoardPioneerMini_modules_uMux']
        return updater
    except:
        print(f'Could not read parameters from "{filename}"')


def _write_parameters(messenger, updater) -> int:
    parsed = 0

    for p, k in zip(messenger.hub.parameters.values(), messenger.hub.parameters.keys()):
        for u in updater:
            if p[0] == u[0]:
                if math.isclose(a=p[1], b=u[1], rel_tol=1e-5):
                    print('untouched {:s}: value {:f}'.format(p[0], p[1]))
                    parsed += 1
                else:
                    time.sleep(0.01)
                    messenger.hub.setParam(value=u[1], name=u[0])
                    p0, p1 = messenger.hub.getParam(number=k)
                    if p0 != p[0] or not math.isclose(a=p1, b=u[1], rel_tol=1e-5):
                        print('update error {:s}'.format(p[0]))
                    else:
                        print('successfully updated {:s}: old {:f}, new {:f}'.format(p[0], p[1], p1))
                        parsed += 1

    return parsed


PARAMS_CHECK_NOT_NULL = [
    "Offsets_accel_xOffset",
    "Offsets_accel_yOffset",
    "Offsets_accel_zOffset",
    "Offsets_gyro_xOffset",
    "Offsets_gyro_yOffset",
    "Offsets_gyro_zOffset"
]


def _check_parameters_not_null(messenger, params) -> bool:
    print("""
        Checking parameters not null
        """)
    messenger.hub.getParamList()
    for p, k in zip(messenger.hub.parameters.values(), messenger.hub.parameters.keys()):
        for u in params:
            if p[0] == u:
                if not math.isclose(p[1], b=0, rel_tol=1e-5):
                    print(f"SUCCESS: checked {p[0]} = {p[1]}")
                else:
                    print(f"FAILURE: checked {p[0]} = {p[1]}")


def connect_and_prepare() -> bool and str:
    updater = _parse_parameters()
    if len(updater) == 0:
        print("Found no parameters")
        return
    updater.append(('BoardPioneerMini_modules_uMux', 2,))
    # print(updater)

    comports = list_ports.comports()
    comports.reverse()
    devices = [ports.device for ports in comports]
    for dev in devices:
        print(f'Attempting to connect{dev}')
        try:
            messenger = proto.Messenger(proto.SerialStream(dev, 115200), 'cache')
            messenger.connect()
            messenger.hub.getParamList()
        except:
            print("Could not connect to the device")

        # updater = [('BoardPioneerMini_modules_uMux', 2)]
        parsed = _write_parameters(messenger, updater)

        if parsed > 0:
            print("OK")

            _check_parameters_not_null(messenger, PARAMS_CHECK_NOT_NULL)

            print('Resetting controller...')
            time.sleep(3)
            messenger.hub.sendCommand(18)  # Reset
            print("Please wait...")
            time.sleep(10)

            return True, str(dev)
        else:
            print("ERROR")

        try:
            messenger.stop()
        except:
            pass

    return False, ''