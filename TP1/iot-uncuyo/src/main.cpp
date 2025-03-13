#include <Arduino.h>

int leds[] = {9, 10, 11};  // Pines donde están conectados los LEDs
int brillos[] = {0, 0, 0}; // Brillo inicial de los LEDs
const int led13 = 13;      // Pin del led en la placa
const int ldrPin = A3;     // Pin analógico donde está conectado el LDR

void setup()
{
  Serial.begin(9600);
  for (int i = 0; i < 3; i++)
  {
    pinMode(leds[i], OUTPUT);
  }
  pinMode(led13, OUTPUT);
  Serial.println("Listo para recibir datos en el formato: LED,BRILLO (Ej: 9,128)");
  Serial.println("Listo para recibir datos en el formato: 13,ON/OFF (Ej: 13,ON)");
}

// Función que convierte el número del pin en su índice correspondiente
int getLedIndex(int pin)
{
  for (int i = 0; i < 3; i++)
  {
    if (leds[i] == pin)
    {
      return i; // Devuelve el índice si encuentra el pin
    }
  }
  return -1; // Retorna -1 si el pin no es válido
}

void loop()
{
  if (Serial.available())
  {
    String data = Serial.readStringUntil('\n');
    int pin, brillo;
    char estado[4];

    Serial.print("Data: ");
    Serial.println(data);

    // Intentar extraer los valores esperados (Ejemplo de entrada: "9,128")
    if (sscanf(data.c_str(), "%d,%d", &pin, &brillo) == 2)
    {
      int ledIndex = getLedIndex(pin); // Convertir pin en índice

      if (ledIndex != -1 && brillo >= 0 && brillo <= 255)
      {
        brillos[ledIndex] = brillo;
        analogWrite(leds[ledIndex], brillo);
        Serial.print("LED en pin ");
        Serial.print(pin);
        Serial.print(" -> Brillo: ");
        Serial.println(brillo);
      }
      else
      {
        Serial.println("Error: Valores fuera de rango.");
      }
    }
    else if (sscanf(data.c_str(), "%d,%3s", &pin, estado) == 2 && pin == 13)
    {
      if (strcmp(estado, "ON") == 0)
      {
        digitalWrite(led13, HIGH);
        Serial.println("LED 13 encendido.");
      }
      else if (strcmp(estado, "OFF") == 0)
      {
        digitalWrite(led13, LOW);
        Serial.println("LED 13 apagado.");
      }
      else
      {
        Serial.println("Error: Usa '13,ON' o '13,OFF' para controlar el LED 13.");
      }
    }
    else
    {
      Serial.println("Error: Formato incorrecto. Usa 'LED,BRILLO' o '13,ON/OFF'.");
    }
  }

  // Leer la intensidad luminosa del LDR
  int ldrValue = analogRead(ldrPin);
  Serial.print("Intensidad luminosa (LDR): ");
  Serial.println(ldrValue);
  
  delay(1000); // Leer cada segundo
}
