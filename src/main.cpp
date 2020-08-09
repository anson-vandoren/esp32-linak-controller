#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <BLEDevice.h>

#include "config.h"
#include <util.h>
#include <linakScanManager.h>

LinakScanManager *scanManager;

// NOTE: create a config.h file and in that file include the lines
// #define WIFI_SSID "myWifiSsid"
// #define WIFI_PASSWORD "mySuperSecretWifiPassword"
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;
const char *msg_toggle_led = "toggleLED";
const char *msg_get_led = "getLEDState";
const int led_pin = 2;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

std::string msg_buf;
bool led_state = 0;
uint16_t lastValue = 0;
uint8_t lastNumDevices = 0;

bool shouldScan = false;
std::vector<uint32_t> scanningClients;

struct JsonData
{
  bool led_state = false;
  uint16_t current_value = 0;
  std::map<std::string, std::string> devices;
  unsigned long millis;
} deviceJson;
const size_t JSON_ENTRIES = 4 + 10;

void bufferJson(std::string &buf)
{
  buf.clear();
  deviceJson.millis = millis();
  StaticJsonDocument<JSON_OBJECT_SIZE(JSON_ENTRIES)> doc;
  doc["led_state"] = deviceJson.led_state;
  doc["current_value"] = deviceJson.current_value;
  doc["time"] = deviceJson.millis;

  if (deviceJson.devices.size() != 0)
  {
    for (auto &kv : deviceJson.devices)
    {
      doc["devices"][kv.first] = kv.second;
    }
  }

  serializeJson(doc, buf);
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  IPAddress ip = client->remoteIP();

  switch (type)
  {
  case WS_EVT_CONNECT:
    // client connected
    Serial.print("Connection from: ");
    Serial.println(ip);

    bufferJson(msg_buf); // send JSON state on new connection
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
          led_state = led_state ? false : true;
          deviceJson.led_state = led_state;
          Serial.printf("Toggling LED to %u\n", led_state);
          digitalWrite(led_pin, led_state);
        }
        else if (strcmp((char *)data, "getLEDState") == 0)
        {
          bufferJson(msg_buf);
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
          bufferJson(msg_buf);
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

// callback: send homepage
void onIndexRequest(AsyncWebServerRequest *request)
{
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() + "] HTTP GET request of" + request->url());
  request->send(SPIFFS, "/index.html", "text/html");
}

// callback: send style sheet
void onCSSRequest(AsyncWebServerRequest *request)
{
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/style.css", "text/css");
}

// callback: send 404
void onPageNotFound(AsyncWebServerRequest *request)
{
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() + "] HTTP GET request of " + request->url());
  request->send(404, "text/plain", "Not found");
}

void setup()
{
  scanManager = new LinakScanManager();
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);

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
  bufferJson(msg_buf);
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
    bufferJson(msg_buf);
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