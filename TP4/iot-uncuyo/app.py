from flask import Flask, render_template, jsonify, request
import serial
import threading
import time
import logging

app = Flask(__name__)

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# Configuración del puerto serial
SERIAL_PORT = '/dev/ttyACM0' 
BAUD_RATE = 9600

# --- Variables Globales para el Estado ---
people_count = 0
is_alarm_active = False
person_detected_momentary = False # Para el indicador visual momentáneo
last_valid_count = 0 # Guardar la última cuenta válida por si falla la lectura

# --- Variables de Control ---
serial_connection = None
serial_thread = None
stop_serial_thread = threading.Event() # Para detener el hilo limpiamente
reset_detection_timer = None # Timer para resetear person_detected_momentary

# --- Función para manejar el timer de detección momentánea ---
def reset_momentary_detection():
    global person_detected_momentary
    person_detected_momentary = False

# --- Función para leer del puerto serial ---
def read_from_serial():
    global people_count, is_alarm_active, person_detected_momentary
    global serial_connection, stop_serial_thread, reset_detection_timer, last_valid_count

    while not stop_serial_thread.is_set():
        if serial_connection and serial_connection.is_open:
            try:
                if serial_connection.in_waiting > 0:
                    line = serial_connection.readline().decode('utf-8', errors='ignore').strip()
                    logging.debug(f"Serial RAW: {line}")

                    if not line:
                        continue

                    # --- Parseo de Mensajes del Arduino ---
                    if line.startswith("peopleCount:"):
                        try:
                            count_str = line.split(":")[1]
                            current_count = int(count_str)
                            if current_count != people_count:
                                logging.info(f"Contador actualizado por Arduino: {current_count}")
                                people_count = current_count
                                last_valid_count = people_count # Actualizar última válida
                        except (IndexError, ValueError) as e:
                            logging.warning(f"Error parseando peopleCount '{line}': {e}")

                    elif line == "PERSON_CONFIRMED_EVENT":
                        logging.info("Evento: Persona confirmada detectada.")
                        person_detected_momentary = True
                        # Cancelar timer anterior si existe y empezar uno nuevo
                        if reset_detection_timer:
                            reset_detection_timer.cancel()
                        # Resetear el flag después de 1 segundo
                        reset_detection_timer = threading.Timer(1.0, reset_momentary_detection)
                        reset_detection_timer.start()

                    elif line == "ALARM_ACTIVE":
                        if not is_alarm_active:
                            logging.info("Estado: Alarma ACTIVADA por Arduino.")
                            is_alarm_active = True

                    elif line == "ALARM_OFF":
                        if is_alarm_active:
                            logging.info("Estado: Alarma DESACTIVADA por Arduino.")
                            is_alarm_active = False
                    
                    elif line.startswith("INFO:") or line.startswith("WARN:") or line.startswith("ALERTA:") or line.startswith("EVENTO:"):
                         logging.debug(f"Arduino Info: {line}") 


            except serial.SerialException as e:
                logging.error(f"Error de Serial en el hilo: {e}")
                serial_connection.close()
                serial_connection = None
                break # Salir del bucle si hay error grave
            except Exception as e:
                 logging.error(f"Error inesperado en hilo serial: {e}")
                 # Continuar si es posible, o decidir terminar
                 time.sleep(1) # Pausa antes de reintentar

        else:
            # Si la conexión no está abierta o no existe, esperar y reintentar conexión
            logging.warning("Conexión serial perdida o no iniciada. Intentando reconectar...")
            stop_serial_thread.wait(5) # Esperar 5 segundos antes de reintentar
            setup_serial_connection() # Intentar reestablecer la conexión

    logging.info("Hilo de lectura serial detenido.")

# --- Función para configurar la conexión serial ---
def setup_serial_connection():
    global serial_connection
    try:
        if serial_connection and serial_connection.is_open:
             serial_connection.close() # Cerrar si ya estaba abierta
        logging.info(f"Intentando conectar a {SERIAL_PORT} a {BAUD_RATE} baudios...")
        serial_connection = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2) # Esperar a que Arduino se reinicie/establezca conexión
        serial_connection.flushInput() # Limpiar buffer de entrada al inicio
        logging.info("Conectado a Arduino exitosamente.")
        return True
    except serial.SerialException as e:
        logging.error(f"No se pudo conectar a Arduino en {SERIAL_PORT}: {e}")
        serial_connection = None
        return False
    except Exception as e:
        logging.error(f"Error inesperado al configurar conexión serial: {e}")
        serial_connection = None
        return False


# --- Rutas Flask ---
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/get_data')
def get_data():
    global people_count, is_alarm_active, person_detected_momentary, last_valid_count

    current_count_to_send = people_count if serial_connection and serial_connection.is_open else last_valid_count

    data = {
        'count': current_count_to_send,
        'alarm': is_alarm_active,
        'detected': person_detected_momentary
    }


    return jsonify(data)

@app.route('/reset', methods=['POST'])
def reset_counter_request():
    global serial_connection
    logging.info("Recibida solicitud de RESET.")
    if serial_connection and serial_connection.is_open:
        try:
            serial_connection.write(b'RESET\n')
            logging.info("Comando RESET enviado a Arduino.")
            return jsonify({'status': 'success', 'message': 'Comando RESET enviado.'})
        except serial.SerialException as e:
             logging.error(f"Error al enviar comando RESET a Arduino: {e}")
             return jsonify({'status': 'error', 'message': 'Error de comunicación serial al enviar RESET.'}), 500
        except Exception as e:
             logging.error(f"Error inesperado al enviar comando RESET: {e}")
             return jsonify({'status': 'error', 'message': 'Error interno al enviar RESET.'}), 500
    else:
        logging.warning("Intento de RESET fallido: No hay conexión serial con Arduino.")
        return jsonify({'status': 'error', 'message': 'No se pudo enviar RESET: Sin conexión serial.'}), 503 # Service Unavailable


# --- Inicio y Parada ---
def start_serial_thread():
    global serial_thread
    if setup_serial_connection(): # Intentar conectar primero
        stop_serial_thread.clear() # Asegurar que el flag de parada está desactivado
        serial_thread = threading.Thread(target=read_from_serial, daemon=True)
        serial_thread.start()
        logging.info("Hilo de lectura serial iniciado.")
    else:
         logging.error("No se pudo iniciar el hilo serial porque la conexión falló.")

def stop_flask_app():
    global reset_detection_timer
    logging.info("Deteniendo aplicación Flask...")
    stop_serial_thread.set() # Señal para detener el hilo serial
    if reset_detection_timer:
        reset_detection_timer.cancel() # Cancelar timer si está activo
    if serial_connection and serial_connection.is_open:
        serial_connection.close() # Cerrar puerto serial
    if serial_thread:
        serial_thread.join(timeout=2) # Esperar un poco a que el hilo termine
        if serial_thread.is_alive():
             logging.warning("El hilo serial no terminó limpiamente.")
    logging.info("Aplicación detenida.")

if __name__ == '__main__':
    start_serial_thread() # Iniciar la conexión y el hilo al arrancar Flask
    try:
        app.run(host='0.0.0.0', port=5000, debug=False, use_reloader=False)
    finally:
        stop_flask_app() 