const int leds[] = {7, 9, 11, 13};
const int numLeds = 4;

enum GameState { IDLE, WAITING, READY, GAMEOVER };
GameState currentState = IDLE;

String inputBuffer = "";
int sequence[100];
int level = 0;
int inputIndex = 0;
unsigned long lastIdleBlink = 0;
bool idleBlinkState = false;
unsigned long lastGameOverBlink = 0;
int gameOverBlinks = 0;

void setup() {
  for (int i = 0; i < numLeds; i++) {
    pinMode(leds[i], OUTPUT);
  }
  Serial.begin(9600);
  randomSeed(analogRead(0));
  printStatus();
}

void loop() {
  switch (currentState) {
    case IDLE:
      handleIdle();
      break;
    case WAITING:
      playSequence();
      break;
    case READY:
      handleInput();
      break;
    case GAMEOVER:
      handleGameOver();
      break;
  }

  readSerialInput();
}

void handleIdle() {
  if (millis() - lastIdleBlink >= 500) {
    idleBlinkState = !idleBlinkState;
    digitalWrite(leds[0], idleBlinkState); // 7
    digitalWrite(leds[2], idleBlinkState); // 11
    digitalWrite(leds[1], !idleBlinkState); // 9
    digitalWrite(leds[3], !idleBlinkState); // 13
    lastIdleBlink = millis();
  }
}

void turnOffLeds() {
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(leds[i], LOW);
  }
}

void startGame() {
  level = 1;
  inputIndex = 0;
  generateSequence();
  currentState = WAITING;
  printStatus();
}

void generateSequence() {
  sequence[level - 1] = leds[random(0, numLeds)];
}

void playSequence() {
  int delayTime = max(200, 700 - level * 50); // reduce delay each level
  for (int i = 0; i < level; i++) {
    digitalWrite(sequence[i], HIGH);
    delay(delayTime);
    digitalWrite(sequence[i], LOW);
    delay(delayTime);
  }
  inputIndex = 0;
  currentState = READY;
  Serial.print("STATUS,Ready,");
  Serial.print(sequence[0]);
  Serial.print(",");
  Serial.println(level);
}

void handleInput() {
  // espera comandos del jugador por monitor serial
}

void readSerialInput() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        processInput(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
}

void processInput(String input) {
  input.trim();
  if (currentState == IDLE && input.equalsIgnoreCase("start")) {
    turnOffLeds();
    delay(1000);
    startGame();
  } else if (currentState == READY) {
    int ledInput = input.toInt();
    if (ledInput == sequence[inputIndex]) {
      digitalWrite(ledInput,HIGH);
      delay(200);
      digitalWrite(ledInput,LOW);
      inputIndex++;
      if (inputIndex < level) {
        Serial.print("STATUS,Ready,");
        Serial.print(sequence[inputIndex]);
        Serial.print(",");
        Serial.println(level);
      } else {
        level++;
        stairLeds();
        generateSequence();
        currentState = WAITING;
        delay(500);
        printStatus();
      }
    } else {
      currentState = GAMEOVER;
      gameOverBlinks = 0;
      lastGameOverBlink = millis();
      printStatus();
    }
  }
}

void stairLeds() {
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(leds[i], HIGH);
    delay(200);
  }
  delay(200);
  for (int i = 0; i < numLeds; i++) {
      digitalWrite(leds[i], LOW);
  }
  delay(200);
}

void handleGameOver() {
  if (millis() - lastGameOverBlink >= 250) {
    gameOverBlinks++;
    bool state = (gameOverBlinks % 2 == 0);
    for (int i = 0; i < numLeds; i++) {
      digitalWrite(leds[i], state);
    }
    lastGameOverBlink = millis();
  }

  if (gameOverBlinks >= 16) { // 2 segundos de parpadeo
    currentState = IDLE;
    level = 0;
    printStatus();
  }
}

void printStatus() {
  String stateStr;
  switch (currentState) {
    case IDLE: stateStr = "Idle"; break;
    case WAITING: stateStr = "Waiting"; break;
    case READY: stateStr = "Ready"; break;
    case GAMEOVER: stateStr = "GameOver"; break;
  }

  int led = 0;
  if (currentState == WAITING || currentState == READY) {
    // No hacer nada aquí, los leds se imprimen en otro lugar
    return;
  }

  Serial.print("STATUS,");
  Serial.print(stateStr);
  Serial.print(",");
  Serial.print(led);
  Serial.print(",");
  Serial.println(currentState == GAMEOVER ? 0 : level);
}