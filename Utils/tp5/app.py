from flask import Flask, jsonify, render_template
import serial
import threading
import time
from datetime import datetime, timedelta, timezone
import os

app = Flask(__name__)
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


@app.route('/data')
def get_data():
    # Devuelve los últimos 30 datos como JSON
    return jsonify(data[-30:])

@app.route('/')
def index():
    return render_template('index.html')

if __name__ == '__main__':
    load_data()
    app.run(debug=True, host='0.0.0.0', port=5000)
