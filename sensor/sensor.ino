#include "DHTesp.h"
#include "secret.h"
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>


// --------------- Pinos ---------------
#define DHT_PIN        13
#define TRIG_PIN       12
#define ECHO_PIN       14

// --------------- Constantes ---------------
#define SOUND_VELOCITY  0.034
#define MAX_DISTANCE    200
#define LIMIAR_CM       80
#define INTERVALO_MS    600000   

// --------------- Constantes para simular tráfego ---------------
#define SINTETICO_MIN_MS     5000    
#define SINTETICO_MAX_MS     60000   
#define PROB_PICO            10      
#define PICO_MIN_REQS        3
#define PICO_MAX_REQS        10
#define PICO_INTERVALO_MS    300     

// --------------- NTP ---------------
#define NTP_SERVER  "pool.ntp.org"

// --------------- Objetos e variáveis ---------------
DHTesp dht;
unsigned long ultimaLeituraTemp = 0;
bool passagemAtiva = false;
unsigned long bytesEnviados = 0;
unsigned long bytesRecebidos = 0;
unsigned long proximoEventoSintetico = 0;


// Devolve hora no formato exemplo "2026-06-29 13:58:25"
String horaAtual() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "hora-desconhecida";
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  dht.setup(DHT_PIN, DHTesp::DHT11);

  randomSeed(analogRead(A0));
  proximoEventoSintetico = millis() + random(SINTETICO_MIN_MS, SINTETICO_MAX_MS);

  // --------------- Ligar ao WiFi---------------
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  // --------------- Sincronizar hora via NTP  ---------------
  setenv("TZ", "WET0WEST,M3.5.0/1,M10.5.0", 1);
  tzset();
  configTime(3600, 0, NTP_SERVER);

  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) delay(500);
}

void loop() {
  unsigned long agora = millis();
  float distancia = medirDistancia();

  // --------------- Deteção de passagem ---------------
  if (distancia > 0 && distancia < LIMIAR_CM) {
    if (!passagemAtiva) {
      passagemAtiva = true;
      Serial.print("[PASSAGEM] ");
      Serial.print(distancia);
      Serial.print(" cm | ");
      Serial.println(horaAtual());

      enviarPassagens(distancia);
    }
  } else {
    passagemAtiva = false;
  }

  // --------------- Leitura de temp/humidade + bandwidth ---------------
  if (agora - ultimaLeituraTemp >= INTERVALO_MS) {
    ultimaLeituraTemp = agora;

    TempAndHumidity leitura;
    do {
      leitura = dht.getTempAndHumidity();
      if (dht.getStatus() != 0) delay(500);
    } while (dht.getStatus() != 0);

    Serial.print("[TEMP/HUM] ");
    Serial.print(leitura.temperature);
    Serial.print(" °C | ");
    Serial.print(leitura.humidity);
    Serial.print(" % | ");
    Serial.println(horaAtual());

    enviarTemp(leitura.temperature, leitura.humidity);

    // Envia o bandwidth acumulado desde o último envio, depois reseta
    enviarBandwidth(bytesEnviados, bytesRecebidos);
    bytesEnviados = 0;
    bytesRecebidos = 0;
  }

  // --------------- Tráfego sintético ---------------
  if (agora >= proximoEventoSintetico) {
    if (random(0, 100) < PROB_PICO) {
      int numReqs = random(PICO_MIN_REQS, PICO_MAX_REQS + 1);
      Serial.print("SIMULADOR | PICO DE ");
      Serial.print(numReqs);
      Serial.println(" requisicoes");

      for (int i = 0; i < numReqs; i++) {
        simularEventoSintetico();
        delay(PICO_INTERVALO_MS);
      }
    } else {
      simularEventoSintetico();
    }
    proximoEventoSintetico = agora + random(SINTETICO_MIN_MS, SINTETICO_MAX_MS);
  }

  delay(100);
}

// ------------------------------------------------------------
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, (MAX_DISTANCE * 2 / SOUND_VELOCITY));
  if (duracao == 0) return -1;
  return (duracao * SOUND_VELOCITY) / 2.0;
}

// ------------------------------------------------------------
void simularEventoSintetico() {
  int pedidoMin = 50;
  int pedidoMax = 2000;

  int tamanhoPedido = random(pedidoMin, pedidoMax);

  String pedido = "";
  pedido.reserve(tamanhoPedido);
  for (int i = 0; i < tamanhoPedido; i++) {
    pedido += 'x';
  }

  int tamanhoResposta = random(20, 500);

  bytesEnviados += pedido.length();
  bytesRecebidos += tamanhoResposta;

  Serial.print("[SINTETICO] Evento gerado em ");
  Serial.print(horaAtual());
  Serial.print(" | enviado: ");
  Serial.print(pedido.length());
  Serial.print(" bytes | recebido (simulado): ");
  Serial.print(tamanhoResposta);
  Serial.println(" bytes");
}

//---enviar os dados em json

// Envia temperatura e humidade
void enviarTemp(float temp, float hum) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<128> doc;
  doc["tipo"]     = "temp";
  doc["temp"]     = temp;
  doc["humidade"] = hum;

  String payload;
  serializeJson(doc, payload);
  http.POST(payload);
  http.end();
}

// Enviar dados do sensor de movimento
void enviarPassagens(float distancia) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<64> doc;
  doc["tipo"] = "passagem";
  doc["distancia"] = distancia;

  String payload;
  serializeJson(doc, payload);
  http.POST(payload);
  http.end();
}

// Envia o bandwidth sintético acumulado
void enviarBandwidth(unsigned long enviados, unsigned long recebidos) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<128> doc;
  doc["tipo"]            = "bandwidth";
  doc["bytes_enviados"]  = enviados;
  doc["bytes_recebidos"] = recebidos;

  String payload;
  serializeJson(doc, payload);
  http.POST(payload);
  http.end();
}