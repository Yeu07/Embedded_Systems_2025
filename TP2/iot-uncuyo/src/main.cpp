#include <Arduino_FreeRTOS.h>
#include <semphr.h> // Biblioteca Mutex

#define BUTTON_START 2  // Pin para iniciar lectura
#define BUTTON_STOP 3   // Pin para detener lectura

int sensorValue = 0; // Variable para almacenar el valor
bool lecturaHabilitada = false;  // Estado de la lectura
SemaphoreHandle_t xMutex;  // Mutex para proteger la variable compartida

void TaskAnalogRead(void *pvParameters);
void TaskSerialWrite(void *pvParameters);
void TaskBlinkLed11(void *pvParameters);
void TaskStartAlarm(void *pvParameters);
void TaskSerialCommand(void *pvParameters);

const int led11 = 11;
const int led12 = 12;

void iniciarLectura();
void detenerLectura();

void setup() {
  Serial.begin(9600);
  pinMode(led11, OUTPUT);
  pinMode(led12, OUTPUT);
  pinMode(BUTTON_START, INPUT);
  pinMode(BUTTON_STOP, INPUT);

  attachInterrupt(digitalPinToInterrupt(BUTTON_START), iniciarLectura, RISING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_STOP), detenerLectura, RISING);

  xMutex = xSemaphoreCreateMutex();
  if (xMutex == NULL) {
    Serial.println("Error creando el mutex");
    while (1) {}
  }

  xTaskCreate(TaskAnalogRead, "AnalogRead", 128, NULL, 1, NULL);
  xTaskCreate(TaskSerialWrite, "SerialWrite", 128, NULL, 2, NULL);
  xTaskCreate(TaskBlinkLed11, "BlinkLed", 128, NULL, 2, NULL);
  xTaskCreate(TaskStartAlarm, "StartAlarm", 128, NULL, 2, NULL);
  xTaskCreate(TaskSerialCommand, "SerialCommand", 128, NULL, 2, NULL);
}

void loop() {}

void TaskAnalogRead(void *pvParameters) {
  for (;;) {
    if (lecturaHabilitada) {
      int tempValue = analogRead(A3);
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        sensorValue = tempValue;
        xSemaphoreGive(xMutex);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(15));
  }
}

void TaskSerialWrite(void *pvParameters) {
  for (;;) {
    if (lecturaHabilitada) {
      int valueToSend;
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        valueToSend = sensorValue;
        xSemaphoreGive(xMutex);
      }
      Serial.println(valueToSend);
    } else {
      Serial.println("--");
    }
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void TaskBlinkLed11(void *pvParameters) {
  for (;;) {
    if (lecturaHabilitada) {
      digitalWrite(led11, HIGH);
      vTaskDelay(pdMS_TO_TICKS(1000));
      digitalWrite(led11, LOW);
      vTaskDelay(pdMS_TO_TICKS(1000));
    } else {
      digitalWrite(led11, LOW);
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

void TaskStartAlarm(void *pvParameters) {
  static bool alarmaActiva = false;
  for (;;) {
    if (lecturaHabilitada) {
      int actualIntensity;
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        actualIntensity = sensorValue;
        xSemaphoreGive(xMutex);
      }
      if (actualIntensity > 800) {
        alarmaActiva = true;
      }
      if (alarmaActiva) {
        digitalWrite(led12, HIGH);
        vTaskDelay(pdMS_TO_TICKS(100));
        digitalWrite(led12, LOW);
        vTaskDelay(pdMS_TO_TICKS(100));
      } else {
        vTaskDelay(pdMS_TO_TICKS(50));
      }
    } else {
      alarmaActiva = false;
      digitalWrite(led12, LOW);
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

void TaskSerialCommand(void *pvParameters) {
  for (;;) {
    if (Serial.available() > 0) {
      String command = Serial.readStringUntil('\n');
      command.trim();
      if (command == "ON") {
        iniciarLectura();
      } else if (command == "OFF") {
        detenerLectura();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void iniciarLectura() {
  lecturaHabilitada = true;
}

void detenerLectura() {
  lecturaHabilitada = false;
}
