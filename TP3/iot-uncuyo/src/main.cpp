#include <Arduino_FreeRTOS.h>
#include <Arduino.h>
#include <semphr.h> // Biblioteca Mutex

long int unixTime = 0;
int secs = 0;
int mins = 0;
int hours = 0;

void getUnixTime(void *pvParameters);
void incrementSecond(void *pvParameters);


void setup() {
  Serial.begin(9600);
  Serial.println("Esperando tiempo Unix...");

  // Creates FreeRTOS tasks
  xTaskCreate(getUnixTime, "UnixTime", 128, NULL, 1, NULL);
  xTaskCreate(incrementSecond, "IncrementSecond", 128, NULL, 1, NULL);

  vTaskStartScheduler();

}

void loop() {}

// Tarea 1: Recibir tiempo Unix desde Serial
void getUnixTime(void *pvParameters) {
  while (1) {
    if (Serial.available() > 0) {  // Espera datos en Serial
      String input = Serial.readStringUntil('\n');  // Leer la línea hasta '\n'
      unixTime = input.toInt();  // Convertir a número
      secs = 
      Serial.print("Tiempo Unix recibido: ");
      Serial.println(unixTime);
    }
    vTaskDelay(pdMS_TO_TICKS(500));  // Pequeña espera
  }
}

// Tarea 2: Incrementar tiempo cada segundo
void incrementSecond(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  while (1) {
    // Conversión de timestamp Unix a HH:MM:SS
    hours = ((unixTime % 86400L) / 3600)-3;
    mins = (unixTime % 3600) / 60;
    secs = unixTime % 60;

    // Mostrar en Serial
    Serial.print("Hora: ");
    Serial.print(hours);
    Serial.print(":");
    Serial.print(mins);
    Serial.print(":");
    Serial.println(secs);

    unixTime++;  // Incrementa cada segundo
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));  // Espera 1 seg
  }
}