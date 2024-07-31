// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <CAN.h>
#define cs 53

const int triggerPin = 2;
const int echoPin = 4;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Preparando sonar
  // Set the trigger pin as an output
  pinMode(triggerPin, OUTPUT);

  // Set the echo pin as an input
  pinMode(echoPin, INPUT);

  // Ajustes para o Arduino Mega
  CAN.setPins(cs);

  Serial.println("CAN Sender");

  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
}

union FloatByteConverter {
  float floatValue;
  byte byteValue[sizeof(float)];
};

float resultadoSonar() { 
    // Trigger Pin no LOW
    digitalWrite(triggerPin, LOW);

    delayMicroseconds(2);

    // Gera um pulso de 10us no Trigger Pin 
    digitalWrite(triggerPin, HIGH);

    delayMicroseconds(10);

    digitalWrite(triggerPin, LOW);

    // Faz a leitura da duracao do pulso do Echo Pin
    long duration = pulseIn(echoPin, HIGH);

    // Calcula a distancia em centimetros
    float distance = (duration * 0.0343) / 2;
    
    return distance;
}

void loop() {

  Serial.print("Sending packet ... ");

  CAN.beginPacket(0x12);

  // Converte float para bytes
  float resultado = resultadoSonar();

  // Maximo de 20 bytes para a 
  // representacao do resultado
  char floatStr[20];

  // Converte o float para bytes com no
  // maximo 10 algarismos e 2 decimais
  dtostrf(resultado, 10, 2, floatStr);  

  // Envia todos os algarismos nao decimais para a rede. 
  // Qualquer algarismos decimal sera enviado como pacote extendido.
  int extended_index = 0;
  for (size_t i = 0; i < strlen(floatStr); ++i) {
    if (floatStr[i] == '.') {
      extended_index = i;
      break;
    }
    CAN.write(floatStr[i]);
  }

  CAN.endPacket();

  Serial.println("done");

  delay(1000);

  // Envia pacote extendido contendo os algarismos apos o decimal
  if (extended_index) {
    CAN.beginExtendedPacket(0x13);
    for (size_t i = extended_index; i < strlen(floatStr); ++i) {
      CAN.write(floatStr[i]);
    }
    CAN.endPacket();
  }

  Serial.println("done");

  delay(1000);
}