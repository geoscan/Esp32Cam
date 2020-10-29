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
        return updater
    except:
        print(f'Could not read parameters from "{filename}"')


def connect_and_prepare() -> bool and str:
    updater = _parse_parameters()
    if len(updater) == 0:
        print("Found no parameters")
        return


    devices = [ports.device for ports in list_ports.comports()]
    for dev in devices:
        print(f'Attempting to connect{dev}')
        try:
            messenger = proto.Messenger(proto.SerialStream(dev, 115200), 'cache')
            messenger.connect()
            messenger.hub.getParamList()
        except:
            print("Could not connect to the device")

        # updater = [('BoardPioneerMini_modules_uMux', 2)]
        parsed = 0

        for p, k in zip(messenger.hub.parameters.values(), messenger.hub.parameters.keys()):
            for u in updater:
                if p[0] == u[0]:
                    if math.isclose(a=p[1], b=u[1], rel_tol=1e-5):
                        print('{:s} untouched: value {:f}'.format(p[0], p[1]))
                        parsed += 1
                    else:
                        messenger.hub.setParam(value=u[1], name=u[0])
                        p0, p1 = messenger.hub.getParam(number=k)
                        if p0 != p[0] or not math.isclose(a=p1, b=u[1], rel_tol=1e-5):
                            print('{:s} update error'.format(p[0]))
                        else:
                            print('{:s} updated: old {:f}, new {:f}'.format(p[0], p[1], p1))
                            parsed += 1

        print('Resetting controller...')
        time.sleep(3)
        messenger.hub.sendCommand(18)  # Reset
        print("Please wait...")
        time.sleep(10)

        if parsed > 0:
            print("OK")
            return True, dev
        else:
            print("ERROR")

        try:
            messenger.stop()
        except:
            pass

    return False, ''