#include <PubSubClient.h>
#include <WiFi.h>

const int pinoLed = 19;
const int pinoLDR = 33;
const int pinoPIR = 34;

#define TOPICO_SUBSCRICAO_LED      "topico_ligar_desligar_led"
#define TOPICO_PUBLICAR_MOVIMENTO  "topico_status_movimento"
#define TOPICO_PUBLICAR_LUZ        "topico_status_luz"

#define ATRASO_PUBLICACAO 2000
#define ID_MQTT "esp32_mqtt"

const char *SSID = "Wokwi-GUEST";
const char *SENHA = "";

const char *BROKER_MQTT = "broker.hivemq.com";
int PORTA_BROKER = 1883;

WiFiClient espCliente;
PubSubClient cliente(espCliente);

void conectarWiFi() {
  Serial.print("Conectando ao Wi-Fi...");
  WiFi.begin(SSID, SENHA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" Conectado!");
}

void configurarMQTT() {
  cliente.setServer(BROKER_MQTT, PORTA_BROKER);
  cliente.setCallback(callbackMQTT);
}

void reconectarMQTT() {
  while (!cliente.connected()) {
    Serial.print("Tentando conectar ao MQTT...");
    if (cliente.connect(ID_MQTT)) {
      Serial.println("Conectado ao Broker MQTT");
      cliente.subscribe(TOPICO_SUBSCRICAO_LED);
    } else {
      Serial.print("Falha na conexão, tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void callbackMQTT(char* topico, byte* carga, unsigned int tamanho) {
  String mensagem = "";
  for (int i = 0; i < tamanho; i++) {
    mensagem += (char)carga[i];
  }
  if (String(topico) == TOPICO_SUBSCRICAO_LED) {
    if (mensagem == "ON") {
      digitalWrite(pinoLed, HIGH);
      Serial.println("LED LIGADO via MQTT");
    } else if (mensagem == "OFF") {
      digitalWrite(pinoLed, LOW);
      Serial.println("LED DESLIGADO via MQTT");
    }
  }
}

void lerSensores() {
  int valorLDR = analogRead(pinoLDR); // Lê o valor do sensor de luz
  int estadoPIR = digitalRead(pinoPIR); // Lê o estado do sensor de movimento

  // Publica o estado da luz (claro ou escuro)
  if (valorLDR < 100) {
    cliente.publish(TOPICO_PUBLICAR_LUZ, "Noite");
    Serial.println("Está escuro - Noite");
  } else {
    cliente.publish(TOPICO_PUBLICAR_LUZ, "Dia");
    Serial.println("Está claro - Dia");
  }

  // Publica o estado do movimento (detectado ou não)
  if (estadoPIR == HIGH) {
    cliente.publish(TOPICO_PUBLICAR_MOVIMENTO, "Movimento detectado");
    Serial.println("Movimento detectado");
  } else {
    cliente.publish(TOPICO_PUBLICAR_MOVIMENTO, "Sem movimento");
    Serial.println("Sem movimento");
  }

  // Liga o LED somente se estiver escuro (LDR < 100) e houver movimento
  if (valorLDR < 100 && estadoPIR == HIGH) { // Está escuro e há movimento
    digitalWrite(pinoLed, HIGH);
    Serial.println("Movimento detectado e está escuro - LED ligado");
    cliente.publish("topico_status_led", "LED ligado");
  } else { // Está claro ou não há movimento
    digitalWrite(pinoLed, LOW);
    Serial.println("Sem movimento ou está claro - LED desligado");
    cliente.publish("topico_status_led", "LED desligado");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(pinoLed, OUTPUT);
  pinMode(pinoLDR, INPUT);
  pinMode(pinoPIR, INPUT);

  conectarWiFi();
  configurarMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  if (!cliente.connected()) {
    reconectarMQTT();
  }
  cliente.loop();

  lerSensores();

  delay(ATRASO_PUBLICACAO);
}