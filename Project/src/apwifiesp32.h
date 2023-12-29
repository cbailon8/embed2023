#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

void handleRoot()
{
    String html = "<html><body>";
    html += "<form method='POST' action='/'>";
    html += "Luces: <input type='text' name='light'><br>";
    html += "Puerta: <input type='text' name='door'><br>";
    html += "Persianas: <input type='text' name='window'><br>";
    html += "<input type='submit' value='Enviar'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
}

void initAP(const char *apSsid, const char *apPassword)
{ // Nombre de la red Wi-Fi y  Contraseña creada por el ESP32
    Serial.begin(115200);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid, apPassword);

    server.on("/", handleRoot);

    server.begin();
    Serial.print("Ip de esp32...");
    Serial.println(WiFi.softAPIP());
    Serial.println("Servidor web iniciado");
}

void loopAP()
{
    server.handleClient();
}