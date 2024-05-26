import serial
import time
import tkinter as tk
import numpy as np
from datetime import datetime
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg as FigureCanvas

# Konfiguracja
port = "/dev/ttyACM0"
NUM_RESULTS = 60

ser: serial.Serial
try:
    ser = serial.Serial(
        port=port,
        baudrate=115200,
        bytesize=8,
        parity="N",
        stopbits=1,
        timeout=2,
        rtscts=False,
        dsrdtr=False,
    )
except:
    print("Nie udalo sie polaczyc z plytka!")
    exit()

def setup():
    value = ser.read(1)
    print("Zestrajanie: odczytano", value)
    while value != b"\n":
        value = ser.read(1)
        print("Zestrajanie: odczytano", value)

def readTemperature():
    line = ser.readline().decode('utf-8').strip()
    print("Odczytano:", line)
    try:
        temperature = float(line.split(" ")[1])
        return temperature
    except (IndexError, ValueError):
        print("Błąd konwersji temperatury:", line)
        return None


setup()

temperatures = []

root = tk.Tk()
root.wm_title("Wykresy")

def updateGraph(data):
    num_readings = len(temperatures)
    axis_x = np.arange(0, num_readings, 1)
    data.clear()
    data.set_xlabel("Nr pomiaru")
    data.set_ylabel("Temperatury")
    data.plot(axis_x, temperatures)

def createTemperatureGraph():
    fig = Figure(figsize=(10, 4), dpi=80)
    data = fig.add_subplot()
    return fig, data

def renderFigure(fig):
    canvas = FigureCanvas(fig, master=root)
    canvas.draw()
    canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=1)
    return canvas

fig, figData = createTemperatureGraph()
figCanvas = renderFigure(fig)

while True:
    temperature = readTemperature()
    if temperature is not None:
        temperatures.append(temperature)
        if len(temperatures) > NUM_RESULTS:
            temperatures.pop(0)
        print(temperatures)
        updateGraph(figData)
        figCanvas.draw()

    root.update_idletasks()
    root.update()
    time.sleep(1)  # Odczekaj 1 sekundę przed kolejnym pomiarem

