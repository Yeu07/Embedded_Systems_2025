#include <Arduino_FreeRTOS.h>
#include <Arduino.h>

// Define task to analog read
void TaskAnalogRead( void *pvParameters );

void setup(){
  Serial.begin(9600);

  //while(!Serial){
    ; // wait serial to connect
  //}

  xTaskCreate(
    TaskAnalogRead
    ,  "AnalogRead"
    ,  128  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL );

}

void loop(){}

void TaskAnalogRead(void *pvParameters){
  (void) pvParameters;

  for(;;){
    // read the input on analog pin 0:
    int sensorValue = analogRead(A3);
    // print out the value you read:
    Serial.println(sensorValue);
    vTaskDelay(1);  // one tick delay (15ms) in between reads for stability
  }
}