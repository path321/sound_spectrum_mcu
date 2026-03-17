#!/usr/bin/python3
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



def get_data():
    ser = serial.Serial('COM7', 115200)
    rl = ReadLine(ser)
    fft_size = 2048
    while(1):
        try:
                line = rl.readline()
                values = line.decode("utf8").split(",")
                values = np.asfarray(values,dtype = float)
                if((len(values) == fft_size) and ('' not in values)):
                    return values
                else:
                    continue
        except e as Exception:
            print(e)
            values = [0 for i in range(fft_size)]
    
