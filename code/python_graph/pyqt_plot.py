import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets, QtCore
import numpy as np
import serial
import numpy as np
import serial.tools.list_ports
from time import sleep

DEBUG = False


class ReadLine:
    def __init__(self, s):
        self.buf = bytearray()
        self.s = s
        
    def readline(self):
        i = self.buf.find(b"\n")
        if i >= 0:
            r = self.buf[:i+1]
            self.buf = self.buf[i+1:]
            return r
        while True:
            i = max(1, min(4096, self.s.in_waiting))
            data = self.s.read(i)
            i = data.find(b"\n")
            if i >= 0:
                r = self.buf + data[:i+1]
                self.buf[0:] = data[i+1:]
                return r
            else:
                self.buf.extend(data)

                
com_port = None
ports = serial.tools.list_ports.comports()
for port in sorted(ports):
    if((port.vid == 0x0483)and(port.pid == 0x5740)):
        com_port = port.device

if(com_port == None):
    print("Device not found")
    sleep(2)
    exit(0)
else:   
    ser = serial.Serial('COM6', 12000000)
    rl = ReadLine(ser)


global take_measurements
take_measurements = True

def onButtonClicked():
    global take_measurements
    take_measurements = not take_measurements


def update():
    if(take_measurements):
        try:
            line= rl.readline()
            values = line.decode("utf8")
            if((values[0] == '[') and (values[-2]==']') and (values[-1] == '\n')):
                values=values[1:-2].split(',')
                if('' not in values):
                    values = np.asfarray(values,dtype = float)
                    values = values[start_freq_index:stop_freq_index+1]
                    curve.setData(x= x_axis,y=values)
                else:
                    raise ValueError
            else:
                raise ValueError
        except Exception as e:
            if(DEBUG):
                print(e)
                values = [0 for i in range(len_axis)]

        


fft_size = 2048//2



##Set X axis
x_axis = np.linspace(0,24000,fft_size)

start_freq = 50 #Hz
start_freq_index = 0
stop_freq = 20.3 *pow(10,3) #Hz
stop_freq_index = 0

for i in range(len(x_axis)):
    freq = x_axis[i]
    if((start_freq_index == 0) and (freq > start_freq)):
        start_freq_index = i
    if((stop_freq_index == 0) and (freq > stop_freq)):
        stop_freq_index = i-1

x_axis = x_axis[start_freq_index:stop_freq_index+1]
len_axis = stop_freq_index+1 - start_freq_index



app = QtWidgets.QApplication([])
win = pg.GraphicsLayoutWidget()
plot = win.addPlot()
win.setBackground("w")
pen = pg.mkPen(color=(0, 0, 255))
plot.showGrid(x=True)


btn = QtWidgets.QPushButton("Run")
btn.clicked.connect(onButtonClicked)
proxy = QtWidgets.QGraphicsProxyWidget()
proxy.setWidget(btn)
win.addItem(proxy)
win.show()

values = [0 for i in range(len_axis)]

title = "Audio Spectrum"

win.setWindowTitle(title)
win.setGeometry(100, 100, 600, 500)

curve = plot.plot(x = x_axis,y = values, pen=pen)

    
if __name__ == '__main__':

    timer = pg.QtCore.QTimer()
    timer.timeout.connect(update)
    timer.start(30)

    import sys
    if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
        app.exec()
