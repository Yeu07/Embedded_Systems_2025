from flask import Flask, render_template, jsonify, request
import serial
import threading
import time

app = Flask(__name__)

# Configuración del puerto serial 
SERIAL_PORT = 'COM3'  # Windows
# SERIAL_PORT = '/dev/ttyACM0'  # Linux/Mac
BAUD_RATE = 9600

# Variable global para el contador
people_count = 0
alarm_active = False
last_detection = None

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)  # Esperar a que se establezca la conexión
except:
    ser = None
    print("No se pudo conectar al Arduino")

def read_from_serial():
    global people_count, alarm_active, last_detection
    while True:
        if ser and ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            if line == "DETECTION_EVENT":
                people_count += 1
                last_detection = time.time()
            elif line == "ALARM_EVENT":
                alarm_active = True
            else:
                print(f"Datos recibidos: {line}")

# Iniciar hilo para leer del serial en segundo plano
if ser:
    serial_thread = threading.Thread(target=read_from_serial, daemon=True)
    serial_thread.start()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/get_data')
def get_data():
    global people_count, alarm_active
    return jsonify({
        'count': people_count,
        'alarm': alarm_active,
        'last_detection': last_detection
    })

@app.route('/reset', methods=['POST'])
def reset_counter():
    global people_count, alarm_active
    people_count = 0
    alarm_active = False
    if ser:
        ser.write(b'RESET\n')
    return jsonify({'status': 'success'})

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')