#include <Arduino.h>
#include <Preferences.h>
#include "buzzer.h"

#define LED_BUILTIN 2
#define BUTTON_PIN 26
#define BUZZER_PIN 27
#define BAUDRATE 115200

// Wifi Setup
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);

const char *ap_ssid = "PatroWifi";
const char *ap_password = "12345678";

#define MAX_NETWORKS 20
String ssidList[MAX_NETWORKS];
int rssiList[MAX_NETWORKS];
int foundNetworks = 0;

Preferences preferences;

void WebServerTask(void *pvParameters)
{
    while (true)
    {
        dnsServer.processNextRequest();
        server.handleClient();
        vTaskDelay(1);
    }
}

void handleRoot()
{
    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<style>body{font-family:sans-serif;background:#f0f0f0;}form{background:#fff;padding:20px;border-radius:8px;max-width:350px;margin:auto;box-shadow:0 2px 8px #0002;}label{display:block;margin-top:10px;}select,input[type=password]{width:100%;padding:8px;margin-top:5px;}input[type=submit]{margin-top:15px;padding:10px 20px;background:#2196F3;color:#fff;border:none;border-radius:4px;cursor:pointer;}input[type=submit]:hover{background:#1976D2;}</style>";
    html += "<h2 style='text-align:center;'>Configuração WiFi</h2>";
    html += "<form action='/save'>";
    html += "<label for='ssid'>Rede WiFi:</label>";
    html += "<select name='ssid' id='ssid'>";
    for (int i = 0; i < foundNetworks; ++i)
    {
        html += "<option value='" + ssidList[i] + "'>" + ssidList[i] + " (" + String(rssiList[i]) + " dBm)</option>";
    }
    html += "</select>";
    html += "<label for='pass'>Senha:</label>";
    html += "<input name='pass' id='pass' type='password'>";
    html += "<input type='submit' value='Salvar'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
}

void handleSave()
{
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("pass", pass);
    preferences.end();

    String html = "<html><body><h2>Configuração recebida!</h2>";
    html += "<p>SSID: " + ssid + "</p>";
    html += "<p>Senha: " + pass + "</p>";
    html += "<p>Reiniciando o dispositivo...</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);

    Serial.println("[Portal] Configuração salva. Reiniciando...");
    beepSaved();
    delay(2000);
    ESP.restart();
}

void handleNotFound()
{
    server.sendHeader("Location", "/", true); // Redireciona para "/"
    server.send(302, "text/plain", "");
}

int buttonState = 0;
unsigned long buttonPressStartTime = 0;
bool isResetting = false;

void setup()
{
    Serial.begin(BAUDRATE);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    buzzer_init(BUZZER_PIN); // Inicializa o buzzer

    preferences.begin("wifi", true);
    String savedSsid = preferences.getString("ssid", "");
    String savedPass = preferences.getString("pass", "");
    preferences.end();

    bool wifiConnected = false;
    if (savedSsid.length() > 0)
    {
        Serial.println("[WiFi] Credenciais encontradas. Tentando conectar...");
        WiFi.mode(WIFI_STA);
        WiFi.begin(savedSsid.c_str(), savedPass.c_str());
        beepConnecting();
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
        {
            delay(500);
            Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("\n[WiFi] Conectado com sucesso!");
            Serial.print("[WiFi] IP: ");
            Serial.println(WiFi.localIP());
            beepSuccess();
            wifiConnected = true;
        }
        else
        {
            Serial.println("\n[WiFi] Falha ao conectar. Iniciando modo Access Point.");
            beepFailAP();
        }
    }
    else
    {
        Serial.println("[WiFi] Nenhuma credencial salva. Iniciando modo Access Point.");
        beepFailAP();
    }

    if (!wifiConnected)
    {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ap_ssid, ap_password);
        Serial.print("[AP] Access Point iniciado. IP: ");
        Serial.println(WiFi.softAPIP());

        // Scan de redes para o portal
        foundNetworks = WiFi.scanNetworks();
        if (foundNetworks > MAX_NETWORKS)
            foundNetworks = MAX_NETWORKS;
        for (int i = 0; i < foundNetworks; ++i)
        {
            ssidList[i] = WiFi.SSID(i);
            rssiList[i] = WiFi.RSSI(i);
        }
        Serial.print("[AP] ");
        Serial.print(foundNetworks);
        Serial.println(" redes WiFi encontradas para exibir no portal.");

        // Initialize DNS to redirect all requests to the ESP
        dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

        // Main Page
        server.on("/", handleRoot);
        server.on("/save", handleSave);
        server.onNotFound(handleNotFound);

        // Start Web Server
        server.begin();
        Serial.println("[AP] Servidor web iniciado.");

        // Create task for Web Server at Core 0
        xTaskCreatePinnedToCore(WebServerTask, "WebServerTask", 4096, NULL, 1, NULL, 0);
    }
}

void loop()
{
    // Read Inputs
    buttonState = digitalRead(BUTTON_PIN);

    if (buttonState == LOW)
    {
        if (buttonPressStartTime == 0)
        {
            buttonPressStartTime = millis();
            Serial.println("[Botão] Pressionado. Segure por 5 segundos para resetar.");
        }
        else if (millis() - buttonPressStartTime > 5000 && !isResetting)
        {
            isResetting = true;
            Serial.println("[Reset] Resetando configurações em 3 segundos...");

            // Alerta sonoro antes do reset
            beepResetWarning();

            preferences.begin("wifi", false);
            preferences.clear();
            preferences.end();
            Serial.println("[Reset] Configurações apagadas. Reiniciando...");
            ESP.restart();
        }
    }
    else
    {
        if (buttonPressStartTime > 0)
        {
            Serial.println("[Botão] Liberado.");
            buttonPressStartTime = 0;
        }
    }

    digitalWrite(LED_BUILTIN, WiFi.status() == WL_CONNECTED ? HIGH : (millis() % 1000 < 500));

    delay(10);
}
