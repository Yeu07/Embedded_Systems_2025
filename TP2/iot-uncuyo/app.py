from flask import Flask, render_template, jsonify, request
import serial
import threading
import time

app = Flask(__name__)

# Configurar la conexión con el Arduino (ajustar el puerto según sea necesario)
arduino = serial.Serial('/dev/ttyACM1', 9600, timeout=1)  # Cambia 'COM3' por el puerto correcto en tu sistema

ldr_value = 0
lectura_activada = False
alarma_activa = False

# Variable para almacenar el hilo de lectura
read_thread = None

def read_arduino():
    global ldr_value, lectura_activada, alarma_activa
    while True:
        try:
            data = arduino.readline().decode().strip()
            print("Data:", data)
            if data != "" and data != "--":
                print("> LDR value:", data)
                ldr_value = int(data)
                lectura_activada = True
                alarma_activa = ldr_value > 800 or alarma_activa
            elif data == "--":
                ldr_value = "--"  # Resetear el valor cuando la lectura está desactivada
                lectura_activada = False
                alarma_activa = False
            time.sleep(3)
        except Exception as e:
            print("Error leyendo datos:", e)

# Iniciar el hilo solo si no ha sido iniciado
if read_thread is None:
    read_thread = threading.Thread(target=read_arduino, daemon=True)
    read_thread.start()

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
    command = "ON" if lectura_activada else "OFF"
    try:
        arduino.write((command + "\n").encode())
    except Exception as e:
        print("Error enviando comando:", e)
    return jsonify({"lectura": lectura_activada})


if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)