from flask import Flask, request, jsonify, render_template
import serial
import ntplib
import time

app = Flask(__name__)

# Configuración del puerto serial
SERIAL_PORT = '/dev/ttyACM1'  # Ajustar según el sistema
BAUD_RATE = 9600
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

def get_ntp_time():
    try:
        client = ntplib.NTPClient()
        response = client.request('pool.ntp.org')
        return int(response.tx_time)
    except Exception as e:
        print(f"Error obteniendo la hora NTP: {e}")
        return int(time.time())
    
@app.route("/")
def index():
    return render_template("index.html")  # Asegúrate de que "index.html" está en la carpeta "templates"

@app.route('/update_time', methods=['POST'])
def update_time():
    unix_time = get_ntp_time()
    ser.write(f"{unix_time}\n".encode())
    return jsonify({"unix_time": unix_time})

@app.route('/retrieve_events', methods=['GET'])
def retrieve_events():
    ser.write(b"retrieve\n")
    time.sleep(1)
    events = []
    while ser.in_waiting:
        line = ser.readline().decode().strip()
        events.append(line)
    return jsonify({"events": events})

@app.route('/clear_eeprom', methods=['POST'])
def clear_eeprom():
    ser.write(b"clear\n")
    return jsonify({"message": "EEPROM cleared"})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
