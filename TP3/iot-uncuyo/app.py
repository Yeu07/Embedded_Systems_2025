from flask import Flask, render_template, jsonify
import serial
import threading
app = Flask(__name__)

# Configurar la conexión con el Arduino (ajustar el puerto según sea necesario)
arduino = serial.Serial('/dev/ttyACM0', 9600, timeout=1)  # Cambia 'COM3' por el puerto correcto en tu sistema

ldr_value = 0
lectura_activada = False
alarma_activa = False

def read_arduino():
    global ldr_value, alarma_activa
    while True:
        if lectura_activada:
            try:
                arduino.write(b'READ\n')  # Comando para solicitar datos al Arduino
                data = arduino.readline().decode().strip()
                if data.isdigit():
                    ldr_value = int(data)
                    alarma_activa = ldr_value > 800
            except Exception as e:
                print("Error leyendo datos:", e)

# Hilo para leer datos del Arduino continuamente
threading.Thread(target=read_arduino, daemon=True).start()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/ldr')
def get_ldr():
    return jsonify({"ldr": ldr_value, "alarma": alarma_activa, "lectura": lectura_activada})

@app.route('/toggle_lectura', methods=['POST'])
def toggle_lectura():
    global lectura_activada
    lectura_activada = not lectura_activada
    return jsonify({"lectura": lectura_activada})

if __name__ == '__main__':
    app.run(debug=True)
