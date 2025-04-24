from flask import Flask, jsonify, render_template
import serial
import threading
import time
from datetime import datetime, timedelta, timezone
import os

app = Flask(__name__)

SERIAL_PORT = '/dev/ttyACM1'
BAUD_RATE = 9600
LOG_FILE = 'log.txt'

data = []

# Cargar datos desde log.txt si existe
def load_data():
    if not os.path.exists(LOG_FILE):
        return

    with open(LOG_FILE, 'r') as f:
        for line in f:
            try:
                timestamp_str, value_str = line.strip().split(',')
                data.append((timestamp_str, int(value_str)))
            except ValueError:
                continue  # Línea mal formada, la ignoramos

# Guardar nueva línea en el archivo
def append_to_log(timestamp, value):
    with open(LOG_FILE, 'a') as f:
        f.write(f'{timestamp},{value}\n')

# Lectura del puerto serie
def read_serial():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print("Conectado al puerto serie.")
    except serial.SerialException:
        print("Error: No se pudo conectar al puerto serie.")
        return

    while True:
        try:
            line = ser.readline().decode().strip()
            if line.isdigit():
                timestamp = datetime.now(timezone.utc)
                value = int(line)
                iso_time = timestamp.isoformat()
                data.append((iso_time, value))
                append_to_log(iso_time, value)
                print(f"{iso_time} -> {value}")
        except Exception as e:
            print(f"Error leyendo del puerto serie: {e}")
        time.sleep(0.1)

@app.route('/data')
def get_data():
    # Devuelve los últimos 30 datos como JSON
    return jsonify(data[-30:])

@app.route('/')
def index():
    return render_template('index.html')

if __name__ == '__main__':
    load_data()
    threading.Thread(target=read_serial, daemon=True).start()
    app.run(debug=True, host='0.0.0.0', port=5000)
