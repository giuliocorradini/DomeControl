import sys
from PySide6 import QtCore, QtWidgets
from serial import Serial
from typing import Callable
import paho.mqtt.client as mqtt

class MovementButton(QtWidgets.QPushButton):
    def __init__(self, direction: str, command: str, sendcb: Callable, stopcb: Callable):
        super().__init__(direction)

        self.direction = direction
        self.command = command

        self.sendcb = sendcb

        self.pressed.connect(self.sendCommand)
        self.released.connect(stopcb)

    @QtCore.Slot()
    def sendCommand(self):
        self.sendcb(self.command)

class MainWindow(QtWidgets.QWidget):
    def __init__(self, client: mqtt.Client):
        super().__init__()

        self.cw_button = MovementButton("Clockwise", "MoveCw", self.sendcmd, self.sendstop)
        self.ccw_button = MovementButton("Counterclockwise", "MoveCcw", self.sendcmd, self.sendstop)
        self.stop_button = QtWidgets.QPushButton(text = "Security stop")
        
        self.stop_button.clicked.connect(self.sendstop)

        self.layout = QtWidgets.QVBoxLayout(self)
        self.layout.addWidget(self.cw_button)
        self.layout.addWidget(self.ccw_button)
        self.layout.addWidget(self.stop_button)

        self.client = client

    @QtCore.Slot()
    def sendstop(self):
        self.client.publish("T1/cupola/cmd", "Stop")

    def sendcmd(self, command: str):
        self.client.publish("T1/cupola/cmd", command)


if __name__ == "__main__":
    app = QtWidgets.QApplication([])

    client = mqtt.Client()
    client.connect("localhost")

    window = MainWindow(client)

    window.resize(200, 500)
    window.show()

    sys.exit(app.exec())