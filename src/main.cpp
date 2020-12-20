#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>

#include "config.h"
#include <util.h>
#include <linakScanManager.h>
#include <webserver.h>

#define LED_PIN 2

// NOTE: create a config.h file and in that file include the lines
// #define WIFI_SSID "myWifiSsid"
// #define WIFI_PASSWORD "mySuperSecretWifiPassword"
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
LinakScanManager *scanManager;

std::string msg_buf {};
bool shouldScan {false};
std::vector<uint32_t> scanningClients {};
JsonData deviceJson {};

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  IPAddress ip = client->remoteIP();

  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.print("Connection from: ");
    Serial.println(ip);

    bufferDeviceJson(deviceJson, msg_buf); // send JSON state on new connection
    client->text(msg_buf.c_str());
    Serial.printf("Sent to [%u]: %s\n", client->id(), msg_buf.c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.print(ip);
    Serial.println(" disconnected!");
    break;
  case WS_EVT_DATA:
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len)
    {
      // entire message in a single frame and all its data is received
      // https://github.com/me-no-dev/ESPAsyncWebServer#async-websocket-plugin
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);
      if (info->opcode == WS_TEXT)
      {
        bool sendResponse = true;
        data[len] = 0; // terminate the string
        Serial.println((char *)data);

        // check if it's a known message
        if (strcmp((char *)data, "toggleLED") == 0)
        {
          deviceJson.led_state = !deviceJson.led_state;
          Serial.printf("Toggling LED to %u\n", deviceJson.led_state);
          digitalWrite(LED_PIN, deviceJson.led_state);
        }
        else if (strcmp((char *)data, "getLEDState") == 0)
        {
          bufferDeviceJson(deviceJson, msg_buf);
          client->text(msg_buf.c_str());
          Serial.printf("Sent to [%u]: %s\n", client->id(), msg_buf.c_str());
        }
        else if (strcmp((char *)data, "getData") == 0)
        {
          Serial.println("getData called - shouldn't be needed");
        }
        else if (strcmp((char *)data, "increment") == 0)
        {
          deviceJson.current_value++;
        }
        else if (strcmp((char *)data, "decrement") == 0)
        {
          deviceJson.current_value--;
        }
        else if (strcmp((char *)data, "doScan") == 0)
        {
          // tell the loop to start scanning
          if (!shouldScan)
            shouldScan = true;
          uint32_t clientId = client->id();
          // add this client to the list of clients interested in the results
          if (std::find(scanningClients.begin(), scanningClients.end(), clientId) == scanningClients.end())
            scanningClients.push_back(clientId);
          sendResponse = false;
        }

        if (sendResponse) {
          //  send json object
          bufferDeviceJson(deviceJson, msg_buf);
          client->text(msg_buf.c_str());
          Serial.printf("Sent to [%u]: %s\n", client->id(), msg_buf.c_str());
        }
      }
      else
      {
        Serial.println("Received data that isn't WS_TEXT");
      }
    }
    else
    {
      Serial.println("Received partial message");
    }
    Serial.println("-----");
  }
  break;
  case WS_EVT_PONG:
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
    break;
  case WS_EVT_ERROR:
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
    break;
  default:
    break;
  }
}

void setup()
{
  scanManager = new LinakScanManager();
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  Serial.println();

  if (!SPIFFS.begin())
  {
    Serial.println("Error mounting SPIFFS");
    exit(1);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  scanManager->init();

  // create initial JSON document
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/style.css", HTTP_GET, onCSSRequest);
  server.onNotFound(onPageNotFound);

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.begin();

  // scan once on power-up
  deviceJson.devices = scanManager->scan();
  bufferDeviceJson(deviceJson, msg_buf);
  Serial.println("Sending data to all connected clients!");
  ws.textAll(msg_buf.c_str());
}

void loop()
{
  if (shouldScan)
  {
    if (!scanManager->isScanning)
      deviceJson.devices = scanManager->scan();
    else
    {
      // already scanning, so wait for it to finish
      while (scanManager->isScanning)
      {
        delay(500);
      }
    }
    // prepare the JSON response after results are in
    bufferDeviceJson(deviceJson, msg_buf);
    // notify all interested clients
    for (const uint32_t &clientId : scanningClients)
    {
      Serial.printf("Trying to send to client [%u]\n", clientId);
      if (ws.hasClient(clientId))
      {
        ws.text(clientId, msg_buf.c_str());
        Serial.printf("Sent %s to %u\n", msg_buf.c_str(), clientId);
      }
      else
        Serial.printf("Cannot find client [%u]\n", clientId);
    }

    scanningClients.clear();
    shouldScan = false;
  }
  ws.cleanupClients();
  delay(500);
}