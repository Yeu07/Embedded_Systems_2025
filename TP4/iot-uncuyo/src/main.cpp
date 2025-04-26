#include <EEPROM.h>
#include <Arduino.h>

// Pines del sensor HCSR04
const int trigPin = 5;
const int echoPin = 4;

// Pines de los LEDs 
const int ledPin6 = 6;
const int ledPin7 = 7;
const int ledPin8 = 8;
const int ledPin9 = 9;
const int ledPin10 = 10;
const int ledPin11 = 11;
const int ledPin12 = 12;
const int ledPin13 = 13;

// Variables para el sensor
long duration;
int distance;
int lastDistance = 0;
int stableDistance = 0;

// Variables para el conteo
int peopleCount = 0;
bool personDetected = false;
unsigned long detectionStartTime = 0;
unsigned long lastMovementTime = 0;
const unsigned long minDetectionTime = 500; // 0.5 segundos
const unsigned long alarmThreshold = 3000; // 3 segundos para alarma

// Umbrales de distancia
const int minDistance = 10; // 10 cm (distancia mínima para detectar)
const int maxDistance = 200; // 200 cm (distancia máxima para detectar)
const int distanceThreshold = 30; // 30 cm (umbral para considerar que hay alguien)

// Variables para filtrado
const int numReadings = 5;
int readings[numReadings];
int readIndex = 0;
int total = 0;
int average = 0;

int getFilteredDistance() {
  // Limpiar el trigger
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Activar el trigger por 10 microsegundos
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Leer el echo y calcular distancia en cm
  duration = pulseIn(echoPin, HIGH);
  int rawDistance = duration * 0.034 / 2;
  
  // Filtrar lecturas erróneas (fuera de rango)
  if (rawDistance < minDistance || rawDistance > maxDistance) {
    rawDistance = lastDistance; // Mantener última distancia válida
  }
  
  // Filtro de promedio móvil para suavizar lecturas
  total = total - readings[readIndex];
  readings[readIndex] = rawDistance;
  total = total + readings[readIndex];
  readIndex = (readIndex + 1) % numReadings;
  
  int filteredDistance = total / numReadings;
  lastDistance = filteredDistance;
  
  return filteredDistance;
}

void triggerAlarm() {
  // Encender todos los LEDs
  digitalWrite(ledPin6, HIGH);
  digitalWrite(ledPin7, HIGH);
  digitalWrite(ledPin8, HIGH);
  digitalWrite(ledPin9, HIGH);
  digitalWrite(ledPin10, HIGH);
  digitalWrite(ledPin11, HIGH);
  digitalWrite(ledPin12, HIGH);
  digitalWrite(ledPin13, HIGH);
  
  // Enviar señal de alarma a la página web
  Serial.println("ALARM_EVENT");
}

void resetAlarm() {
  // Apagar todos los LEDs
  digitalWrite(ledPin6, LOW);
  digitalWrite(ledPin7, LOW);
  digitalWrite(ledPin8, LOW);
  digitalWrite(ledPin9, LOW);
  digitalWrite(ledPin10, LOW);
  digitalWrite(ledPin11, LOW);
  digitalWrite(ledPin12, LOW);
  digitalWrite(ledPin13, LOW);
}

void setup() {
  // Inicializar pines
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin6, OUTPUT);
  pinMode(ledPin7, OUTPUT);
  pinMode(ledPin8, OUTPUT);
  pinMode(ledPin9, OUTPUT);
  pinMode(ledPin10, OUTPUT);
  pinMode(ledPin11, OUTPUT);
  pinMode(ledPin12, OUTPUT);
  pinMode(ledPin13, OUTPUT);
  
  // Inicializar comunicación serial
  Serial.begin(9600);
  
  // Inicializar filtro de promedio móvil
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
  
  // Cargar conteo desde EEPROM
  peopleCount = EEPROM.read(0);
  if (peopleCount == 255) { // Valor inicial de EEPROM
    peopleCount = 0;
  }
  
  // Apagar todos los LEDs al inicio
  resetAlarm();
}



void loop() {
  // Leer distancia del sensor
  int currentDistance = getFilteredDistance();
  
  // Verificar si hay una persona en el rango de detección
  if (currentDistance < distanceThreshold && currentDistance > minDistance) {
    if (!personDetected) {
      // Primera detección de persona
      personDetected = true;
      detectionStartTime = millis();
      lastMovementTime = millis();
      stableDistance = currentDistance;
      Serial.println("Persona detectada");
    } else {
      // Persona ya detectada, verificar movimiento
      if (abs(currentDistance - stableDistance) > 5) { // Hay movimiento
        lastMovementTime = millis();
        stableDistance = currentDistance;
      }
      
      // Verificar si la persona se ha detenido demasiado tiempo
      if (millis() - lastMovementTime > alarmThreshold) {
        triggerAlarm();
      } else {
        resetAlarm();
      }
    }
  } else {
    // No hay persona detectada o se fue
    if (personDetected) {
      // Verificar si estuvo el tiempo mínimo para contar como persona
      if (millis() - detectionStartTime >= minDetectionTime) {
        peopleCount++;
        EEPROM.write(0, peopleCount); // Guardar en EEPROM
        Serial.print("Persona contada. Total: ");
        Serial.println(peopleCount);
        
        // Enviar señal a la página web
        Serial.println("DETECTION_EVENT");
      }
      personDetected = false;
      resetAlarm();
    }
  }
  
  // Verificar si hay un comando de reset desde la página web
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    if (command == "RESET") {
      peopleCount = 0;
      EEPROM.write(0, peopleCount);
      Serial.println("Contador reiniciado");
    }
  }
  
  delay(100); // Pequeña pausa entre lecturas
}

