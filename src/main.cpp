#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char *ssid = "CABANA 8883";
const char *password = "eusouamor88";

const int RELE_PIN = 0;
bool releEstado = false;

ESP8266WebServer server(80);

// ── Template HTML em PROGMEM (Flash, 0 B de RAM) ─────────────────
static const char PAGE_TPL[] PROGMEM =
  "<!DOCTYPE html>"
  "<html><head><meta charset=\"UTF-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<title>Controle Rele</title>"
  "<style>"
  "body{font-family:Arial,sans-serif;display:flex;flex-direction:column;"
  "align-items:center;justify-content:center;min-height:100vh;margin:0;"
  "background:#1a1a2e;color:#fff}"
  "h1{margin-bottom:40px;font-size:24px}"
  ".s{font-size:20px;margin-bottom:20px;font-weight:700}"
  ".on{color:#4caf50}.off{color:#f44336}"
  ".w{position:relative;width:120px;height:60px}"
  ".w input{opacity:0;width:0;height:0}"
  ".l{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;"
  "background:#f44336;transition:.4s;border-radius:60px}"
  ".l:before{position:absolute;content:\"\";height:48px;width:48px;"
  "left:6px;bottom:6px;background:#fff;transition:.4s;border-radius:50%%}"
  "input:checked+.l{background:#4caf50}"
  "input:checked+.l:before{transform:translateX(60px)}"
  "</style></head><body>"
  "<h1>Controle do Rele</h1>"
  "<div class=\"s %s\">%s</div>"
  "<label class=\"w\">"
  "<input type=\"checkbox\" onchange=\"location.href=this.checked?'/ligar':'/desligar'\" %s>"
  "<span class=\"l\"></span></label></body></html>";

// ── Buffer no stack: 1 envio TCP, Content-Length correto ─────────
void handleRoot() {
  char buf[900];
  int len = snprintf_P(buf, sizeof(buf), PAGE_TPL,
    releEstado ? "on"      : "off",
    releEstado ? "LIGADO"  : "DESLIGADO",
    releEstado ? "checked" : "");
  server.send(200, F("text/html"), buf);
}

void handleLigar() {
  releEstado = true;
  digitalWrite(RELE_PIN, LOW);
  server.sendHeader(F("Location"), F("/"));
  server.send(302);
}

void handleDesligar() {
  releEstado = false;
  digitalWrite(RELE_PIN, HIGH);
  server.sendHeader(F("Location"), F("/"));
  server.send(302);
}

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();

  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(RELE_PIN, HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print(F("Conectando ao Wi-Fi"));

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }

  Serial.println(F(" Conectado!"));
  Serial.print(F("IP: "));
  Serial.println(WiFi.localIP());

  server.on(F("/"), handleRoot);
  server.on(F("/ligar"), handleLigar);
  server.on(F("/desligar"), handleDesligar);
  server.begin();
  Serial.println(F("Servidor HTTP iniciado."));
}

void loop() {
  server.handleClient();
}
