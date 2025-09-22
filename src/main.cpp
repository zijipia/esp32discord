#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

const char *ssid = "Ziji";
const char *password = "1335555777777";
const char *botToken = "MTMxNjAxOTI1NDU5OTg4MDcwNA.GTikCj.QWKl3cJ32k9qL-EpHOnfmrH9R_gpc0FywtieaE";

WebSocketsClient webSocket;

uint64_t lastHeartbeat = 0;
uint32_t heartbeat_interval = 0;
int sequenceNumber = 0;

const int ledPin = 2;

void sendIdentify()
{
  StaticJsonDocument<1024> doc;
  doc["op"] = 2;
  JsonObject d = doc.createNestedObject("d");
  d["token"] = botToken;
  // d["intents"] = 3276799;
  d["intents"] = 513; // GUILDS (1) + GUILD_MESSAGES (512)

  JsonObject props = d.createNestedObject("properties");
  props["os"] = "esp32";
  props["browser"] = "esp32";
  props["device"] = "esp32";

  String json;
  serializeJson(doc, json);
  Serial.print("📤 IDENTIFY Payload: ");
  Serial.println(json);
  webSocket.sendTXT(json);
  Serial.println("📡 Sent IDENTIFY");
}

void sendHeartbeat()
{
  StaticJsonDocument<256> doc;
  doc["op"] = 1;
  if (sequenceNumber >= 0)
  {
    doc["d"] = sequenceNumber;
  }
  else
  {
    doc["d"] = nullptr;
  }

  String json;
  serializeJson(doc, json);
  Serial.print("📤 HEARTBEAT Payload: ");
  Serial.println(json);
  webSocket.sendTXT(json);
  Serial.println("❤️ Sent HEARTBEAT");
}

void handleMessage(JsonObject &d)
{
  const char *content = d["content"];
  const char *author = d["author"]["username"];
  Serial.printf("📨 Message from %s: %s\n", author, content);

  if (strcmp(content, "led on") == 0)
  {
    digitalWrite(ledPin, HIGH);
    Serial.println("🔆 LED ON");
  }
  else if (strcmp(content, "led off") == 0)
  {
    digitalWrite(ledPin, LOW);
    Serial.println("🌑 LED OFF");
  }
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  if (type == WStype_TEXT)
  {
    Serial.print("📥 Received raw: ");
    Serial.write(payload, length);
    Serial.println();

    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err)
    {
      Serial.println("❌ JSON Parse Error");
      return;
    }

    if (!doc["s"].isNull())
    {
      sequenceNumber = doc["s"];
    }

    int op = doc["op"];
    if (op == 10)
    { // HELLO
      heartbeat_interval = doc["d"]["heartbeat_interval"];
      lastHeartbeat = millis();
    }
    else if (op == 11)
    {
      Serial.println("✅ HEARTBEAT ACKNOWLEDGED");
    }

    const char *t = doc["t"];
    if (t && strcmp(t, "GUILD_CREATE") == 0)
    {
      Serial.println("📥 Skipped GUILD_CREATE");
      return; // bỏ qua hoàn toàn
    }
    if (t && strcmp(t, "MESSAGE_CREATE") == 0)
    {
      JsonObject d = doc["d"].as<JsonObject>();
      handleMessage(d);
    }
  }
  else if (type == WStype_DISCONNECTED)
  {
    Serial.println("🔌 WebSocket disconnected");
  }
  else if (type == WStype_CONNECTED)
  {
    sendIdentify();
    Serial.println("🔗 WebSocket connected");
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.print("🔄 Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");

  webSocket.beginSSL("gateway.discord.gg", 443, "/?v=10&encoding=json");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop()
{
  webSocket.loop();

  if (heartbeat_interval > 0 && millis() - lastHeartbeat > heartbeat_interval)
  {
    sendHeartbeat();
    lastHeartbeat = millis();
  }
}
