#include <WiFi.h>
#include <PubSubClient.h>
#include <CAN.h>
#include "ca_cert.h"  // Incluir o arquivo de certificado CA

// Credenciais WiFi
const char* ssid = "";
const char* password = "";

// Detalhes do Broker MQTT
const char* mqtt_server = "";
const int mqtt_port = 0; // Porta segura
const char* mqtt_user = "";
const char* mqtt_password = "";
const char* root_topic = "";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// Buffer para dados recebidos do CAN
String integerPart = "";
String decimalPart = "";

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Receptor CAN");

  // Conectar ao WiFi
  setup_wifi();

  // Configurar conexão segura usando certificado CA
  espClient.setCACert(ca_cert);

  // Configurar cliente MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Iniciar o barramento CAN a 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Falha ao iniciar o CAN!");
    while (1);
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Analisar o primeiro pacote para a parte inteira
  int packetSize = CAN.parsePacket();
  if (packetSize && !CAN.packetRtr() && CAN.packetId() == 0x123) {
    while (CAN.available()) {
      integerPart += (char)CAN.read();
    }
    Serial.print("Parte inteira recebida: ");
    Serial.println(integerPart);
  }

  // Analisar o segundo pacote para a parte decimal
  packetSize = CAN.parsePacket();
  if (packetSize && !CAN.packetRtr() && CAN.packetId() == 0x124) {
    while (CAN.available()) {
      decimalPart += (char)CAN.read();
    }
    Serial.print("Parte decimal recebida: ");
    Serial.println(decimalPart);
  }

  // Combinar partes se ambas foram recebidas
  if (integerPart.length() > 0 && decimalPart.length() > 0) {
    // Combinar e converter para float
    String fullNumber = integerPart + "." + decimalPart;
    float number = fullNumber.toFloat();

    // Imprimir o número float
    Serial.print("Número completo: ");
    Serial.println(number);

    // Publicar o número float no tópico MQTT
    String topic = String(root_topic) + "dados";
    client.publish(topic.c_str(), String(number).c_str());

    // Imprimir mensagem de envio
    Serial.println("Enviando Mensagem...");
    Serial.print("Distância: ");
    Serial.print(number);
    Serial.println(" cm");

    // Resetar as partes
    integerPart = "";
    decimalPart = "";
  }

  // Pequeno delay para estabilidade
  delay(100);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT...");
    if (client.connect("ArduinoClient", mqtt_user, mqtt_password)) {
      Serial.println("conectado");
      Serial.print("Conectado ao servidor ");
      Serial.println(mqtt_server);

      // Se inscrever no tópico para receber mensagens
      client.subscribe((String(root_topic) + "dados").c_str());
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentar novamente em 5 segundos");
      delay(5000);
    }
  }
}

// Função callback para lidar com novas mensagens
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nova mensagem recebida em ");
  Serial.println(mqtt_server);
  Serial.println("Mensagem recebida:");

  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.println(message);
}
