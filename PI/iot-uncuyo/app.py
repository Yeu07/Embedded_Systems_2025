from flask import Flask, request, jsonify, render_template
import serial
import threading
import time  # <--- nuevo

app = Flask(__name__)

arduino = serial.Serial(port='/dev/ttyACM0', baudrate=9600, timeout=1)
time.sleep(2)  # <--- espera a que Arduino esté listo

serial_lock = threading.Lock()
serial_log = []

def read_from_serial():
    while True:
        try:
            line = arduino.readline().decode().strip() # Elimina 'errors='ignore'
            if line:
                with serial_lock:
                    serial_log.append(line)
                    if len(serial_log) > 50:
                        serial_log.pop(0)
                    # Intenta parsear la línea si tiene el formato esperado
                    if line.startswith("STATUS,"):
                        parts = line.split(',')
                        if len(parts) == 4:
                            status, led, level = parts[1], parts[2], parts[3]
                            print(f"Recibido - Status: {status}, LED: {led}, Level: {level}") # Para debugging
        except UnicodeDecodeError as e:
            print(f"Error de decodificación: {e}, Datos recibidos: {arduino.readline()}")
        except Exception as e:
            print(f"Otro error al leer del serial: {e}")
            continue

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/send', methods=['POST'])
def send_command():
    data = request.get_json()
    command = data.get('command', '')
    if command:
        with serial_lock:
            arduino.write((command + '\n').encode())
        return jsonify({'status': 'sent', 'command': command})
    return jsonify({'status': 'error', 'message': 'No command received'}), 400

@app.route('/status', methods=['GET'])
def get_status():
    with serial_lock:
        return jsonify({'log': serial_log[-10:]})

if __name__ == '__main__':
    threading.Thread(target=read_from_serial, daemon=True).start()
    app.run(debug=True, host='0.0.0.0', port=5000)
