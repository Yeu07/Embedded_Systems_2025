#include <Arduino_FreeRTOS.h>
#include <Arduino.h>
#include <semphr.h> // Biblioteca Mutex

int sensorValue = 0; // Variable para almacenar el valor
SemaphoreHandle_t xMutex;  // Mutex para proteger la variable compartida

// Task para lectura análoga y para escribir en el serial
void TaskAnalogRead( void *pvParameters );
void TaskSerialWrite(void *pvParameters);

void setup(){
  Serial.begin(9600);

  //while(!Serial){
    ; // wait serial to connect
  //}

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

// **Tarea que lee el sensor constantemente**
void TaskAnalogRead(void *pvParameters){
  (void) pvParameters;

  for(;;){
    
    int tempValue = analogRead(A3); //Leer el sensor

    // Proteger la variable compartida con el mutex
    if(xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
      sensorValue = tempValue; //Guardo valor leido
      xSemaphoreGive(xMutex); //Liberar MUtex
    }

    vTaskDelay(pdMS_TO_TICKS(15));  // Esperar 15 ms entre lecturas
    
  }
}

// **Tarea que envía los datos al puerto serial cada 3 segundos**
void TaskSerialWrite(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    int valueToSend;

    // Acceder a la variable compartida de forma segura
    if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      valueToSend = sensorValue;
      xSemaphoreGive(xMutex);
    }

    Serial.println(valueToSend);  // Enviar el dato por Serial

    vTaskDelay(pdMS_TO_TICKS(3000));  // Esperar 3 segundos antes de la siguiente transmisión
  }
}