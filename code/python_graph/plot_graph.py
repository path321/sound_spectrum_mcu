"""PySide6-based GUI to complement Sound Analyzer v1.0 board for Audio Power Spectrum analysis.

Programm doesn't take any external arguments and shall be run AFTER the respective board is connected to host PC. Please make sure that no other STM32 USB devices are simultaneously connected

"""

from PySide6.QtWidgets import QApplication, QMainWindow, QPushButton, QGraphicsProxyWidget
from PySide6.QtCore import QTimer
from pyqtgraph import GraphicsLayoutWidget, mkPen
from serial import Serial
from serial.tools.list_ports import comports
from numpy import asarray, linspace
from time import sleep
from sys import exit

DEBUG = False
FFT_N = 1024
MIC_LIMIT_LOW = 50  # Hz
MIC_LIMIT_HIGH = 20.3 * pow(10, 3)  # Hz


class ReadLine:
    def __init__(self, s):
        self.buf = bytearray()
        self.s = s

    def readline(self):
        """read line of bytes ending with '\n'"""
        i = self.buf.find(b"\n")
        if i >= 0:
            r = self.buf[:i + 1]
            self.buf = self.buf[i + 1:]
            return r
        while True:
            i = max(1, min(512, self.s.in_waiting))
            data = self.s.read(i)
            i = data.find(b"\n")
            if i >= 0:
                r = self.buf + data[:i + 1]
                self.buf[0:] = data[i + 1:]
                return r
            else:
                self.buf.extend(data)


def get_data_from_device(com_port, rl, start_freq_index, stop_freq_index):
    """Connect via serial port, read line of data and parse them accordingly"""

    # Connect with device, if required
    if (com_port is None):
        ports = comports()
        for port in sorted(ports):
            if ((port.vid == 0x0483) and (port.pid == 0x5740)
                ):  # STM32 Virtual COM Port
                com_port = port.device

        if (com_port is None):
            print("Device not found")
            sleep(2)
            exit(0)
        else:
            try:
                ser = serial.Serial(com_port, 12000000)
                rl = ReadLine(ser)
            except Exception as e:
                print(e)
                sleep(2)
                exit(0)

    # Get data from com port
    try:
        line = rl.readline()
        values = line.decode("utf8")
        if ((values[0] == '[') and (values[-2] == ']')
                and (values[-1] == '\n')):
            values = values[1:-2].split(',')
            if ('' not in values):
                values = asarray(values, dtype=float)
                values = values[start_freq_index:stop_freq_index + 1]
            else:
                raise ValueError
        else:
            raise ValueError
    except Exception as e:
        if (DEBUG):
            print(e)
            values = [0 for i in range(stop_freq_index + 1 - start_freq_index)]
        values = None

    return values


def compute_x_axis(fft_n):
    """Compute X axis of main Plot, according to MEMS Microphone limitations & FFT number"""
    start_freq_index = 0
    stop_freq_index = 0
    x_axis = linspace(0, 24000, fft_n // 2)

    for i in range(fft_n // 2):
        freq = x_axis[i]
        if ((start_freq_index == 0) and (freq > MIC_LIMIT_LOW)):
            start_freq_index = i
        if ((stop_freq_index == 0) and (freq > MIC_LIMIT_HIGH)):
            stop_freq_index = i - 1

    x_axis = x_axis[start_freq_index:stop_freq_index + 1]
    len_axis = stop_freq_index + 1 - start_freq_index

    return x_axis, start_freq_index, stop_freq_index


class MainWindow(QMainWindow):
    """Main PyQt GUI window configuration"""

    def __init__(self):
        super().__init__()

        # init variables
        self.com_port = None
        self.rl = None
        self.x, self.index_start, self.index_stop = compute_x_axis(FFT_N)
        self.y = [0 for i in range(self.index_stop + 1 - self.index_start)]
        title = "Audio Spectrum"

        # Window
        self.setWindowTitle(title)
        self.win = GraphicsLayoutWidget()
        self.win.setBackground('w')
        self.setCentralWidget(self.win)

        # Plot
        self.plot_graph = self.win.addPlot()
        self.plot_graph.showGrid(x=True, y=True)
        self.pen = mkPen(color=(0, 0, 255))
        self.plot_graph.setLabel("left", "dBSPL")
        self.plot_graph.setLabel("bottom", "Hz")
        self.plot_graph.setYRange(0, 120)
        self.data_line = self.plot_graph.plot(
            x=self.x, y=self.y, pen=self.pen, symbol='o', symbolSize=3)
    

        # Button
        self.btn = QPushButton("Run")
        self.btn.clicked.connect(self.onButtonClicked)
        self.proxy = QGraphicsProxyWidget()
        self.proxy.setWidget(self.btn)
        self.win.addItem(self.proxy)
        self.btn.setCheckable(True)
        self.btn.setChecked(False)

        # Update timer
        self.timer = QTimer()
        self.timer.setInterval(10)
        self.timer.timeout.connect(self.update_plot_data)
        self.timer.start()

    def onButtonClicked(self):
        """Change text of Button depending on its state"""
        if (self.btn.isChecked()):
            self.btn.setText("Stop")
        else:
            self.btn.setText("Run")

    def update_plot_data(self):
        """Read & Plot new data, dependig on Button state"""
        if not self.btn.isChecked():
            self.y = get_data_from_device(
                self.com_port, self.rl, self.index_start, self.index_stop)
            if (self.y is not None):
                self.data_line.setData(x=self.x, y=self.y)


def main():
    app = QApplication([])
    w = MainWindow()
    w.show()
    app.exec()


if __name__ == "__main__":
    main()
