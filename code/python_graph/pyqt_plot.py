import pyqtgraph as pg
from pyqtgraph.Qt import QtWidgets
import numpy as np
from get_from_serial import get_data,ReadLine

import serial
import numpy as np
    

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

fft_size = 2048//2

x_axis = np.linspace(0,24000,fft_size)
app = QtWidgets.QApplication([])
win = pg.plot()
data = np.random.normal(size=fft_size)
curve = win.plot(x_axis,data)

ser = serial.Serial('COM7', 115200)
rl = ReadLine(ser)

def update():
    try:
        line= rl.readline()
        values = line.decode("utf8")
        if((values[0] == '[') and (values[-2]==']') and (values[-1] == '\n')):
            values=values[1:-2].split(',')
            if('' not in values):
                values = np.asfarray(values,dtype = float)
            else:
                raise ValueError
        else:
            raise ValueError
    except Exception as e:
        print(e)
        values = [0 for i in range(fft_size)]
    
    #value = np.random.normal(size=256)#get_data()
    
    curve.setData(x_axis,values)

timer = pg.QtCore.QTimer()
timer.timeout.connect(update)
timer.start(10)

app.exec()
