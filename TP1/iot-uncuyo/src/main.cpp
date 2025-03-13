#include <Arduino.h>

int leds[] = {9, 10, 11};
int brillos[] = {0, 0, 0};
int duration = 1000;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 3; i++) {
    pinMode(leds[i], OUTPUT);
  }
  Serial.println("Listo para recibir datos en el formato: LED,BRILLO");
}

void loop() {

  if (Serial.available()) {  // Si hay datos en el puerto serial
    String data = Serial.readStringUntil('\n'); // Leer hasta el salto de línea
    int ledIndex, brillo;

    Serial.print("Data: ");
    Serial.println(data);

    // Intentar extraer los valores esperados (Ejemplo de entrada: "1,128")
    if (sscanf(data.c_str(), "%d,%d", &ledIndex, &brillo) == 2) {
      if (ledIndex >= 0 && ledIndex < 3 && brillo >= 0 && brillo <= 255) {
        brillos[ledIndex] = brillo;
        analogWrite(leds[ledIndex], brillo);
        Serial.print("LED ");
        Serial.print(ledIndex);
        Serial.print(" -> Brillo: ");
        Serial.println(brillo);
      } else {
        Serial.println("Error: Valores fuera de rango.");
      }
    } else {
      Serial.println("Error: Formato incorrecto. Usa 'LED,BRILLO'");
    }
  }
}