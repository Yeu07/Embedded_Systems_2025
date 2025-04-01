from flask import Flask, render_template, request, jsonify
import serial
import ntplib
from time import ctime

app = Flask(__name__)
ser = serial.Serial('/dev/ttyACM1', 9600, timeout=1)  # Ajusta el puerto según tu Arduino

def get_ntp_time():
    """Obtiene la hora NTP y la envía al Arduino."""
    try:
        client = ntplib.NTPClient()
        response = client.request('pool.ntp.org')
        unix_time = int(response.tx_time)
        ser.write(f"{unix_time}\n".encode())
        return unix_time
    except Exception as e:
        print(f"Error al obtener la hora NTP: {e}")
        return None

def get_serial_time_from_arduino():
    """Obtiene la hora desde el monitor serial del Arduino."""
    line_bytes = ser.readline()
    try:
        line = line_bytes.decode('utf-8').strip()
        if line.startswith("Hora:"):
            print(f"Recibido: {line}")
            _, time_str = line.split("Hora:")
            return time_str.strip()
    except UnicodeDecodeError:
        print(f"Error de decodificación: {line_bytes}")
    return None

def get_events_from_arduino():
    """Solicita y recibe los eventos de la EEPROM del Arduino."""
    ser.write(b"GET_EVENTS\n")
    events = []
    while True:
        line_bytes = ser.readline()
        if not line_bytes:
            break
        try:
            line = line_bytes.decode('utf-8').strip()
            if line.startswith("Pin:"):
                parts = line.split(", Tiempo: ")
                if len(parts) == 2:
                    pin = parts[0].split(": ")[1]
                    timestamp = int(parts[1])
                    events.append({"pin": pin, "timestamp": timestamp, "readable_time": ctime(timestamp)})
        except UnicodeDecodeError:
            print(f"Error de decodificación: {line_bytes}")
    return events

def clear_eeprom_on_arduino():
    """Solicita al Arduino que borre la EEPROM."""
    ser.write(b"CLEAR_EEPROM\n")
    return "EEPROM borrada"

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/update_time', methods=['POST'])
def update_time():
    unix_time = get_ntp_time()
    if unix_time is not None:
        return jsonify({'message': f'Hora actualizada a {ctime(unix_time)}'})
    else:
        return jsonify({'error': 'No se pudo actualizar la hora'}), 500

@app.route('/get_events', methods=['GET'])
def get_events():
    events = get_events_from_arduino()
    return jsonify(events)

@app.route('/clear_eeprom', methods=['POST'])
def clear_eeprom():
    message = clear_eeprom_on_arduino()
    return jsonify({'message': message})

@app.route('/get_serial_time', methods=['GET'])
def get_serial_time():
    time_str = get_serial_time_from_arduino()
    if time_str:
        return jsonify({'time': time_str})
    else:
        return jsonify({'error': 'No se pudo obtener la hora'}), 500

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
