#include <EEPROM.h> // Asegúrate de incluir la librería
#include <Arduino.h>

const int TRIG_PIN = 5;
const int ECHO_PIN = 4;
const int LED_PINS[] = {6, 7, 8, 9, 10, 11, 12, 13}; // 8 LEDs
const int NUM_LEDS = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

const unsigned long MIN_PRESENCE_TIME_MS = 250;     // 0.25 segundos mínimo delante del sensor
const unsigned long ALARM_STOPPED_THRESHOLD_MS = 2000; // 2 segundos detenido para activar alarma
const unsigned long ALARM_BLINK_INTERVAL_MS = 500;  // Intervalo parpadeo alarma
const int SENSOR_MIN_DISTANCE_CM = 5;               // Distancia mínima detectable confiable
const int SENSOR_MAX_DISTANCE_CM = 200;             // Distancia máxima detectable confiable (ajusta a tu sensor HC-SR04)
const int DETECTION_THRESHOLD_CM = 50;              // Umbral para considerar presencia (objeto más cerca que esto)
const int EXIT_HYSTERESIS_CM = 6;                  // Margen para confirmar salida (evita rebotes) - Debe ser > DETECTION_THRESHOLD_CM
const int MOVEMENT_THRESHOLD_CM = 5;                // Cambio mínimo de distancia para considerar movimiento
const int MAX_READING_CHANGE_CM = 75;               // Cambio máximo respecto a la última lectura válida para filtrar picos

const int FILTER_READINGS_COUNT = 5;                // Número de lecturas para el promedio móvil
int sensorReadings[FILTER_READINGS_COUNT];
int currentReadingIndex = 0;
long readingsSum = 0;
int lastStableFilteredDistance = SENSOR_MAX_DISTANCE_CM; // Empezar asumiendo que no hay nada

// --- EEPROM ---
const int EEPROM_ADDR_COUNT = 0;                    // Dirección para guardar el contador (ocupa 4 bytes para unsigned long)
const int EEPROM_ADDR_MAGIC = sizeof(unsigned long); 
const uint16_t EEPROM_MAGIC_VALUE = 0xABCD;       

// --- Variables de Estado ---
unsigned long peopleCount = 0;
bool isPersonPresent = false;           // Estado: ¿Hay alguien detectado actualmente?
bool isPersonConfirmed = false;         // Estado: ¿La persona ha estado el tiempo mínimo?
unsigned long timeEnteredZone = 0;      // Timestamp: ¿Cuándo entró en la zona?
unsigned long timeLastMovement = 0;     // Timestamp: ¿Cuándo fue el último movimiento detectado?
bool isAlarmActive = false;
unsigned long timeAlarmLastToggle = 0;

// --- Declaraciones de Funciones ---
int readAndFilterDistance();
void updateAlarmState();
void setAllLeds(bool state);
void loadCountFromEEPROM();
void saveCountToEEPROMIfNeeded(bool forceSave = false);
void handleSerialCommands();

// Variable global para rastrear si el contador cambió y necesita guardarse
bool countChanged = false;

void setup() {
  Serial.begin(9600);

  Serial.println("Iniciando Contador de Personas v1.1...");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW); 
  }

  // Inicializar filtro
  for (int i = 0; i < FILTER_READINGS_COUNT; i++) {
    sensorReadings[i] = SENSOR_MAX_DISTANCE_CM; 
  }
  readingsSum = (long)SENSOR_MAX_DISTANCE_CM * FILTER_READINGS_COUNT;
  lastStableFilteredDistance = SENSOR_MAX_DISTANCE_CM;

  loadCountFromEEPROM(); 
  Serial.print("Contador inicial cargado: ");
  Serial.println(peopleCount);
  // Enviar estado inicial a Flask
  Serial.print("peopleCount:");
  Serial.println(peopleCount);
  Serial.println(isAlarmActive ? "ALARM_ACTIVE" : "ALARM_OFF");


  Serial.println("Probando LEDs...");
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], HIGH);
    delay(50);
  }
    for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], LOW);
    delay(50);
  }
  Serial.println("Sistema listo.");
}

void loop() {
  int currentDistance = readAndFilterDistance();
  unsigned long currentTime = millis();


  if (currentDistance < DETECTION_THRESHOLD_CM) {
    // --- DENTRO de la zona de detección ---
    if (!isPersonPresent) {
      // -> Transición: Nadie -> Alguien Entra
      isPersonPresent = true;
      isPersonConfirmed = false; 
      timeEnteredZone = currentTime;
      timeLastMovement = currentTime; 
      Serial.println("INFO: Objeto detectado en zona.");
      // No contar ni activar alarma aún, esperar confirmación
    } else {
      // -> Estado: Alguien ya está presente
      // Verificar si se movió recientemente
      // Usamos la distancia filtrada actual vs la última estable *antes* de actualizarla
      if (abs(currentDistance - lastStableFilteredDistance) > MOVEMENT_THRESHOLD_CM) {
        timeLastMovement = currentTime; // Actualizar tiempo del último movimiento
        if (isAlarmActive) {
           Serial.println("INFO: Movimiento detectado, desactivando alarma.");
           isAlarmActive = false; // Resetear alarma si se mueve
           setAllLeds(LOW);       // Apagar LEDs inmediatamente
           Serial.println("ALARM_OFF"); // Notificar a Flask
        }
      }

      // Verificar si ha estado el tiempo mínimo para confirmarlo como persona
      if (!isPersonConfirmed && (currentTime - timeEnteredZone >= MIN_PRESENCE_TIME_MS)) {
        isPersonConfirmed = true;
        Serial.println("INFO: Presencia confirmada (tiempo mínimo cumplido).");
        Serial.println("PERSON_CONFIRMED_EVENT"); // Notificar a Flask para indicador visual
      }

      // Verificar si está detenido demasiado tiempo (solo si ya está confirmado)
      if (isPersonConfirmed && !isAlarmActive && (currentTime - timeLastMovement >= ALARM_STOPPED_THRESHOLD_MS)) {
         Serial.println("ALERTA: Persona detenida demasiado tiempo!");
         isAlarmActive = true; // Activar alarma
         timeAlarmLastToggle = currentTime; // Iniciar parpadeo inmediatamente
      }
    }
    // Actualizar la última distancia estable *después* de las comprobaciones de movimiento
    lastStableFilteredDistance = currentDistance;

  } else if (currentDistance >= DETECTION_THRESHOLD_CM + EXIT_HYSTERESIS_CM) {
    // --- FUERA de la zona de detección ---
    if (isPersonPresent) {
      // -> Transición: Alguien Sale
      Serial.println("INFO: Objeto saliendo de la zona.");
      if (isPersonConfirmed) {
        // La persona estuvo el tiempo mínimo, ¡CONTAR!
        peopleCount++;
        countChanged = true; // Marcar para guardar en EEPROM
        Serial.print("EVENTO: Persona contada. Total: ");
        Serial.println(peopleCount);
        // Enviar el nuevo contador a Flask
        Serial.print("peopleCount:");
        Serial.println(peopleCount);
        Serial.println("DETECTION_EVENT"); 
      } else {
         Serial.println("INFO: Objeto salió antes de tiempo mínimo, no contado (movimiento rápido?).");
      }

      // Resetear estados al salir de la zona
      isPersonPresent = false;
      isPersonConfirmed = false;
      lastStableFilteredDistance = SENSOR_MAX_DISTANCE_CM;

      if (isAlarmActive) {
          Serial.println("INFO: Alarma desactivada al salir persona.");
          isAlarmActive = false;
          setAllLeds(LOW); // Apagar LEDs
          Serial.println("ALARM_OFF"); // Notificar a Flask
      }
    }
    // Si no había nadie presente (isPersonPresent ya era false), no hacemos nada.
  }

  // --- Tareas Periódicas ---
  updateAlarmState();         // Manejar parpadeo de LEDs si la alarma está activa y notificar
  handleSerialCommands();     // Procesar comandos entrantes (Reset, Status)
  saveCountToEEPROMIfNeeded(); // Guardar contador en EEPROM si cambió (con lógica anti-desgaste)

  delay(50); // Pequeña pausa para estabilidad y reducir carga CPU
}


// readAndFilterDistance: Lee el sensor y aplica filtros
int readAndFilterDistance() {
  long durationMicros;
  int rawDistanceCm;

  // Trigger
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Lectura con Timeout 
  // Timeout un poco mayor que el tiempo de ida y vuelta para la distancia máxima
  long timeoutMicros = (long)(SENSOR_MAX_DISTANCE_CM + 20) * 59; // Añadir margen y usar ~59 us/cm
  durationMicros = pulseIn(ECHO_PIN, HIGH, timeoutMicros);

  if (durationMicros == 0) {
    // Timeout o eco perdido. Puede ser que no haya objeto o esté fuera de rango.
    // Devolver un valor grande es lo más seguro para la lógica de "salida".
    return SENSOR_MAX_DISTANCE_CM + 10; // Valor claramente fuera de rango de detección
  }

  rawDistanceCm = durationMicros / 58.2; // Cálculo de distancia (usar 58.2 o similar)

  // 1. Filtro de Rango Básico
  if (rawDistanceCm < SENSOR_MIN_DISTANCE_CM || rawDistanceCm > SENSOR_MAX_DISTANCE_CM) {
    // Lectura fuera del rango operativo fiable del sensor.
    // Devolver la última lectura filtrada ESTABLE podría mantener un estado incorrecto
    // si el objeto realmente se fue o si hay un error persistente.
    return SENSOR_MAX_DISTANCE_CM + 5; // Opción más segura: devolver valor fuera de rango
  }

  // 2. Filtro de Cambio Brusco (Opcional, ayuda con picos de ruido)
  // Compara la lectura RAW actual con la última lectura filtrada *estable*
  // Solo aplicar si ya hay alguien presente para evitar filtrar la entrada inicial
  if (isPersonPresent && abs(rawDistanceCm - lastStableFilteredDistance) > MAX_READING_CHANGE_CM) {
     return lastStableFilteredDistance; // Ignorar este pico, devolver la última estable filtrada
  }

  // 3. Filtro de Promedio Móvil (Suaviza la lectura)
  readingsSum -= sensorReadings[currentReadingIndex];
  sensorReadings[currentReadingIndex] = rawDistanceCm; // Añadir la lectura RAW (o filtrada por rango/pico)
  readingsSum += rawDistanceCm;
  currentReadingIndex = (currentReadingIndex + 1) % FILTER_READINGS_COUNT;

  int filteredDistance = readingsSum / FILTER_READINGS_COUNT;

  // Devuelve la distancia promediada
  return filteredDistance;
}

// updateAlarmState: Controla el parpadeo de los LEDs y notifica estado ALARM_ACTIVE
void updateAlarmState() {
  if (!isAlarmActive) {
    // No hacer nada si la alarma no está activa.
    // Los LEDs se apagan cuando la alarma se desactiva explícitamente.
    return;
  }

  unsigned long currentTime = millis();

  // Enviar estado de alarma periódicamente mientras esté activa para asegurar que Flask lo sepa

  // Lógica de parpadeo
  if (currentTime - timeAlarmLastToggle >= ALARM_BLINK_INTERVAL_MS) {
    timeAlarmLastToggle = currentTime;
    bool currentLedState = digitalRead(LED_PINS[0]); // Leer estado de un LED para invertir
    setAllLeds(!currentLedState);
    // Notificar a Flask que la alarma sigue activa (junto con el parpadeo)
    Serial.println("ALARM_ACTIVE");
  }
}

// setAllLeds: Enciende o apaga todos los LEDs configurados
void setAllLeds(bool state) {
  byte ledState = state ? HIGH : LOW;
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], ledState);
  }
}

// loadCountFromEEPROM: Carga el contador verificando un valor mágico
void loadCountFromEEPROM() {
  uint16_t magic;
  EEPROM.get(EEPROM_ADDR_MAGIC, magic);

  if (magic == EEPROM_MAGIC_VALUE) {
    // El valor mágico coincide, la EEPROM fue inicializada por nosotros
    EEPROM.get(EEPROM_ADDR_COUNT, peopleCount);
    Serial.println("INFO: Valor mágico encontrado, cargando contador desde EEPROM.");
  } else {
    // Primera ejecución o EEPROM corrupta/diferente
    Serial.println("WARN: Valor mágico no encontrado o incorrecto. Inicializando EEPROM.");
    peopleCount = 0; // Empezar desde cero
    EEPROM.put(EEPROM_ADDR_COUNT, peopleCount);
    EEPROM.put(EEPROM_ADDR_MAGIC, EEPROM_MAGIC_VALUE);
    Serial.println("INFO: EEPROM inicializada con contador 0 y valor mágico.");
  }
}

// saveCountToEEPROMIfNeeded: Guarda el contador si ha cambiado (estrategia anti-desgaste)
void saveCountToEEPROMIfNeeded(bool forceSave = false) {
  if (countChanged || forceSave) {
     unsigned long currentCountInEEPROM;
     EEPROM.get(EEPROM_ADDR_COUNT, currentCountInEEPROM);

     // Solo escribir si el valor realmente es diferente del que está en EEPROM O si forzamos
     if (currentCountInEEPROM != peopleCount || forceSave) {
         Serial.print("INFO: Guardando contador en EEPROM: ");
         Serial.println(peopleCount);
         EEPROM.put(EEPROM_ADDR_COUNT, peopleCount);
         countChanged = false; // Resetear flag después de guardar exitosamente
     } else {
         // El valor era el mismo, no se escribió. Resetear flag igualmente.
         countChanged = false;
     }
  }
}

// handleSerialCommands: Procesa comandos recibidos por el puerto serial
void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Limpiar espacios en blanco y saltos de línea
    command.toUpperCase(); // Convertir a mayúsculas para insensibilidad

    if (command == "RESET") {
      Serial.println("COMANDO: Reset recibido.");
      peopleCount = 0;
      isPersonPresent = false; // Resetear estados también
      isPersonConfirmed = false;
      lastStableFilteredDistance = SENSOR_MAX_DISTANCE_CM; // Resetear distancia

      if(isAlarmActive){
          isAlarmActive = false;
          setAllLeds(LOW); // Apagar LEDs si la alarma estaba activa
          Serial.println("ALARM_OFF"); // Notificar a Flask
      }

      countChanged = true; // Marcar para forzar guardado a 0
      saveCountToEEPROMIfNeeded(true); // Forzar guardado inmediato del 0 en EEPROM

      Serial.println("INFO: Contador reseteado a 0 y guardado.");
      // Enviar explícitamente el nuevo contador a Flask
      Serial.print("peopleCount:");
      Serial.println(peopleCount);

    } else if (command == "STATUS") {
         Serial.println("--- ESTADO ACTUAL (Arduino) ---");
         Serial.print("Contador: "); Serial.println(peopleCount);
         Serial.print("Persona Presente: "); Serial.println(isPersonPresent ? "SI" : "NO");
         Serial.print("Persona Confirmada: "); Serial.println(isPersonConfirmed ? "SI" : "NO");
         Serial.print("Alarma Activa: "); Serial.println(isAlarmActive ? "SI" : "NO");
         Serial.print("Ultima Distancia Filtrada: "); Serial.print(lastStableFilteredDistance); Serial.println(" cm");
         Serial.print("Tiempo Entrada Zona (ms): "); Serial.println(isPersonPresent ? millis() - timeEnteredZone : 0);
         Serial.print("Tiempo Sin Movimiento (ms): "); Serial.println(isPersonPresent ? millis() - timeLastMovement : 0);
         Serial.println("-----------------------------");
    }
  }
}