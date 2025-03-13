#include <Arduino.h>

int leds[] = {9, 10, 11};  // Pines donde están conectados los LEDs
int brillos[] = {0, 0, 0}; // Brillo inicial de los LEDs

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 3; i++) {
    pinMode(leds[i], OUTPUT);
  }
  Serial.println("Listo para recibir datos en el formato: LED_PIN,BRILLO (Ej: 9,128)");
}

// Función que convierte el número del pin en su índice correspondiente
int getLedIndex(int pin) {
  for (int i = 0; i < 3; i++) {
    if (leds[i] == pin) {
      return i;  // Devuelve el índice si encuentra el pin
    }
  }
  return -1;  // Retorna -1 si el pin no es válido
}

void loop() {
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    int pin, brillo;

    Serial.print("Data: ");
    Serial.println(data);

    // Intentar extraer los valores esperados (Ejemplo de entrada: "9,128")
    if (sscanf(data.c_str(), "%d,%d", &pin, &brillo) == 2) {
      int ledIndex = getLedIndex(pin);  // Convertir pin en índice

      if (ledIndex != -1 && brillo >= 0 && brillo <= 255) {
        brillos[ledIndex] = brillo;
        analogWrite(leds[ledIndex], brillo);
        Serial.print("LED en pin ");
        Serial.print(pin);
        Serial.print(" -> Brillo: ");
        Serial.println(brillo);
      } else {
        Serial.println("Error: Valores fuera de rango.");
      }
    } else {
      Serial.println("Error: Formato incorrecto. Usa 'LED_PIN,BRILLO'");
    }
  }
}
