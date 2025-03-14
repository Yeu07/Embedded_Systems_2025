#include <Arduino_FreeRTOS.h>
#include <Arduino.h>
#include <semphr.h> // Biblioteca Mutex

// Definición de pines para los pulsadores
#define BUTTON_START 2  // Pin para iniciar lectura
#define BUTTON_STOP 3   // Pin para detener lectura

int sensorValue = 0; // Variable para almacenar el valor
bool lecturaHabilitada = false;  // Estado de la lectura
SemaphoreHandle_t xMutex;  // Mutex para proteger la variable compartida

// Task para lectura análoga y para escribir en el serial
void TaskAnalogRead( void *pvParameters );
void TaskSerialWrite(void *pvParameters);

void iniciarLectura();
void detenerLectura();

void setup(){
  Serial.begin(9600);

  //while(!Serial){
    ; // wait serial to connect
  //}

  // Configurar pines de los pulsadores como entrada 
  pinMode(BUTTON_START, INPUT);
  pinMode(BUTTON_STOP, INPUT);

  // Configurar interrupciones para los pulsadores
  attachInterrupt(digitalPinToInterrupt(BUTTON_START), iniciarLectura, RISING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_STOP), detenerLectura, RISING);

  // Crear el mutex
  xMutex = xSemaphoreCreateMutex();

  //Chequeo si se creeo el mutex
  if(xMutex == NULL){
    Serial.println("Error creando el mutex");
    while (1){}
    
  }

  // Crear la tarea de lectura del sensor
  xTaskCreate(
    TaskAnalogRead, "AnalogRead",
    128, NULL, 1, NULL
  );

  // Crear la tarea de envío de datos al puerto serie
  xTaskCreate(
    TaskSerialWrite, "SerialWrite",
    128, NULL, 2, NULL
  );

}

void loop(){}

// **Tarea que lee el sensor cuando está habilitada**
void TaskAnalogRead(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    if (lecturaHabilitada) {  // Solo lee si está habilitado
      int tempValue = analogRead(A3);

      // Proteger la variable compartida con el mutex
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        sensorValue = tempValue;
        xSemaphoreGive(xMutex);
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(15));  // Esperar 15 ms entre lecturas
  }
}

// **Tarea que envía los datos al puerto serial cada 3 segundos**
void TaskSerialWrite(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    if (lecturaHabilitada) {  // Solo envía datos si está habilitado
      int valueToSend;

      // Acceder a la variable compartida de forma segura
      if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
        valueToSend = sensorValue;
        xSemaphoreGive(xMutex);
      }

      Serial.println(valueToSend);
    }
    
    vTaskDelay(pdMS_TO_TICKS(3000));  // Esperar 3 segundos antes del siguiente envío
  }
}
// **Interrupción para iniciar la lectura**
void iniciarLectura() {
  lecturaHabilitada = true;
}

// **Interrupción para detener la lectura**
void detenerLectura() {
  lecturaHabilitada = false;
}