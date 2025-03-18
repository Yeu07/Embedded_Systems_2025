from flask import Flask, render_template, request, jsonify
import serial

tapp = Flask(__name__)

# Configuración del puerto serie
SERIAL_PORT = '/dev/ttyACM0'  # Ajusta esto según el puerto de tu Arduino
BAUD_RATE = 9600

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
except Exception as e:
    print(f"Error abriendo el puerto serial: {e}")
    ser = None

app = Flask(__name__)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/update_led', methods=['POST'])
def update_led():
    if ser:
        led = request.form['led']
        brillo = request.form['brillo']
        ser.write(f"{led},{brillo}\n".encode())
        return "OK"
    return "Error: No se puede comunicar con Arduino", 500

@app.route('/toggle_pin13', methods=['POST'])
def toggle_pin13():
    estado = request.form['estado']
    if ser:
        ser.write(f"13,{estado}\n".encode())
        return "OK"
    return "Error: No se puede comunicar con Arduino", 500

@app.route('/ldr')
def get_ldr():
    if ser:
        # ser.write("LDR\n".encode())
        ldr_value = ser.readline().decode().strip()
        return {"ldr": ldr_value}
    return {"error": "No se puede leer el sensor"}, 500

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
