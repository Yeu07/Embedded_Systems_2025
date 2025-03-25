#include <Arduino_FreeRTOS.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <semphr.h>

long int unixTime = 0;
int secs = 0;
int mins = 0;
int hours = 0;
int eepromAddress = 0;
SemaphoreHandle_t xMutex;

void getUnixTime(void *pvParameters);
void incrementSecond(void *pvParameters);
void storeEvent(int pin);
void retrieveEEPROM();
void clearEEPROM();

void setup() {
  Serial.begin(9600);
  Serial.println("Esperando tiempo Unix...");

  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), []() { storeEvent(2); }, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), []() { storeEvent(3); }, FALLING);

  xMutex = xSemaphoreCreateMutex();

  xTaskCreate(getUnixTime, "UnixTime", 128, NULL, 1, NULL);
  xTaskCreate(incrementSecond, "IncrementSecond", 128, NULL, 1, NULL);

  vTaskStartScheduler();
}

void loop() {}

void getUnixTime(void *pvParameters) {
  while (1) {
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      long int newTime = input.toInt();
      if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
        unixTime = newTime;
        xSemaphoreGive(xMutex);
      }
      Serial.print("Tiempo Unix recibido: ");
      Serial.println(unixTime);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void incrementSecond(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  while (1) {
    if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
      unixTime++;
      hours = ((unixTime % 86400L) / 3600) - 3;
      mins = (unixTime % 3600) / 60;
      secs = unixTime % 60;
      xSemaphoreGive(xMutex);
    }
    Serial.print("Hora: ");
    Serial.print(hours);
    Serial.print(":");
    Serial.print(mins);
    Serial.print(":");
    Serial.println(secs);
    
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
  }
}

void storeEvent(int pin) {
  if (xSemaphoreTake(xMutex, portMAX_DELAY)) {
    EEPROM.put(eepromAddress, unixTime);
    EEPROM.put(eepromAddress + sizeof(long int), pin);
    eepromAddress += sizeof(long int) + sizeof(int);
    xSemaphoreGive(xMutex);

    Serial.print("Evento guardado en EEPROM: Pin ");
    Serial.print(pin);
    Serial.print(" a tiempo ");
    Serial.println(unixTime);
  }
}

void retrieveEEPROM() {
  Serial.println("Eventos guardados:");
  int address = 0;
  while (address < EEPROM.length()) {
    long int storedTime;
    int storedPin;
    EEPROM.get(address, storedTime);
    EEPROM.get(address + sizeof(long int), storedPin);
    if (storedTime == 0xFFFFFFFF) break;

    Serial.print("Pin: ");
    Serial.print(storedPin);
    Serial.print(", Tiempo: ");
    Serial.println(storedTime);
    
    address += sizeof(long int) + sizeof(int);
  }
}

void clearEEPROM() {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0xFF);
  }
  eepromAddress = 0;
  Serial.println("EEPROM borrada.");
}
