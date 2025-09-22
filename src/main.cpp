#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

const char *ssid = "Ziji";
const char *password = "1335555777777";
const char *botToken = "MTMxNjAxOTI1NDU5OTg4MDcwNA.GTikCj.QWKl3cJ32k9qL-EpHOnfmrH9R_gpc0FywtieaE";

WebSocketsClient webSocket;

uint64_t lastHeartbeat = 0;
uint32_t heartbeat_interval = 0;
int sequenceNumber = 0;
bool isReady = false;
uint32_t reconnectDelay = 5000;
uint64_t lastReconnectAttempt = 0;

// FreeRTOS variables
SemaphoreHandle_t webSocketMutex;
TaskHandle_t webSocketTaskHandle = NULL;
TaskHandle_t heartbeatTaskHandle = NULL;

const int ledPin = 2;
void sendHeartbeat()
{
  // Check if WebSocket is ready before sending
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("⚠️ WiFi not connected, skipping heartbeat");
    return;
  }

  // Use smaller JSON document to save stack space
  StaticJsonDocument<128> doc;
  doc["op"] = 1;
  if (sequenceNumber > 0)
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
  Serial.printf("💓 Sending heartbeat, sequence: %d\n", sequenceNumber);

  if (xSemaphoreTake(webSocketMutex, pdMS_TO_TICKS(1000))) // Timeout 1 second
  {
    bool sent = webSocket.sendTXT(json);
    if (sent)
    {
      Serial.println("✅ HEARTBEAT sent successfully");
    }
    else
    {
      Serial.println("❌ Failed to send HEARTBEAT");
    }
    xSemaphoreGive(webSocketMutex);
  }
  else
  {
    Serial.println("⚠️ HEARTBEAT mutex timeout");
  }
}
// FreeRTOS Task Functions
void webSocketTask(void *parameter)
{
  Serial.println("🔄 WebSocket task started");
  Serial.printf("🔄 WebSocket task stack size: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));

  // Wait for WebSocket to be initialized
  vTaskDelay(pdMS_TO_TICKS(2000));
  Serial.println("🔄 WebSocket task ready to run");

  while (true)
  {
    // Check if WiFi is connected before running WebSocket loop
    if (WiFi.status() == WL_CONNECTED)
    {
      // Run webSocket.loop() without mutex - it's thread-safe
      webSocket.loop();
    }
    else
    {
      Serial.println("⚠️ WiFi not connected, waiting...");
    }
    
    // Small delay to prevent task from consuming too much CPU
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void heartbeatTask(void *parameter)
{
  Serial.println("💓 Heartbeat task started");
  Serial.printf("💓 Heartbeat task stack size: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));

  while (true)
  {
    if (isReady && heartbeat_interval > 0)
    {
      uint32_t timeSinceLastHeartbeat = millis() - lastHeartbeat;
      if (timeSinceLastHeartbeat > heartbeat_interval)
      {
        Serial.printf("⏰ Time since last heartbeat: %d ms (interval: %d ms)\n", timeSinceLastHeartbeat, heartbeat_interval);

        if (xSemaphoreTake(webSocketMutex, pdMS_TO_TICKS(1000))) // Timeout 1 second
        {
          sendHeartbeat();
          lastHeartbeat = millis();
          xSemaphoreGive(webSocketMutex);
        }
        else
        {
          Serial.println("⚠️ Heartbeat mutex timeout");
        }
      }
    }

    // Check every 1 second
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void sendIdentify()
{
  Serial.println("🔍 Bot Token (first 20 chars): " + String(botToken).substring(0, 20) + "...");

  // Use smaller JSON document to save stack space
  StaticJsonDocument<512> doc;
  doc["op"] = 2;
  JsonObject d = doc.createNestedObject("d");
  d["token"] = botToken;
  // Thử với intents tối thiểu
  d["intents"] = 1; // Chỉ GUILDS intent

  JsonObject props = d.createNestedObject("properties");
  props["os"] = "esp32";
  props["browser"] = "esp32";
  props["device"] = "esp32";

  String json;
  serializeJson(doc, json);
  Serial.print("📤 IDENTIFY Payload: ");
  Serial.println(json);

  // Check if WebSocket is ready before sending
  if (WiFi.status() == WL_CONNECTED)
  {
    bool sent = webSocket.sendTXT(json);
    if (sent)
    {
      Serial.println("✅ IDENTIFY sent successfully");
    }
    else
    {
      Serial.println("❌ Failed to send IDENTIFY");
    }
  }
  else
  {
    Serial.println("⚠️ WiFi not connected, skipping IDENTIFY");
  }
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

    // Use smaller JSON document to save stack space
    StaticJsonDocument<1024> doc;
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
      Serial.printf("💓 Heartbeat interval: %d ms\n", heartbeat_interval);
      // Không set lastHeartbeat ở đây, sẽ set sau khi READY
    }
    else if (op == 11)
    {
      Serial.println("✅ HEARTBEAT ACKNOWLEDGED");
    }
    else if (op == 9)
    {
      Serial.println("❌ INVALID SESSION - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4000)
    {
      Serial.println("❌ UNKNOWN ERROR - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4001)
    {
      Serial.println("❌ UNKNOWN OPCODE - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4002)
    {
      Serial.println("❌ DECODE ERROR - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4003)
    {
      Serial.println("❌ NOT AUTHENTICATED - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4004)
    {
      Serial.println("❌ AUTHENTICATION FAILED - Check bot token!");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4005)
    {
      Serial.println("❌ ALREADY AUTHENTICATED - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4007)
    {
      Serial.println("❌ INVALID SEQUENCE - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4008)
    {
      Serial.println("❌ RATE LIMITED - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4009)
    {
      Serial.println("❌ SESSION TIMED OUT - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4010)
    {
      Serial.println("❌ INVALID SHARD - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4011)
    {
      Serial.println("❌ SHARDING REQUIRED - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4012)
    {
      Serial.println("❌ INVALID API VERSION - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4013)
    {
      Serial.println("❌ INVALID INTENTS - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 4014)
    {
      Serial.println("❌ DISALLOWED INTENTS - Reconnecting...");
      isReady = false;
      webSocket.disconnect();
    }
    else if (op == 0)
    { // DISPATCH
      const char *t = doc["t"];
      if (t && strcmp(t, "READY") == 0)
      {
        Serial.println("🎉 READY event received - Bot is online!");
        // Bắt đầu heartbeat sau khi READY
        lastHeartbeat = millis();
        isReady = true;
        Serial.println("💓 Started heartbeat timer");

        // Gửi heartbeat đầu tiên ngay lập tức
        Serial.println("💓 Sending initial heartbeat immediately...");
        sendHeartbeat();
        lastHeartbeat = millis();
      }
      else if (t && strcmp(t, "GUILD_CREATE") == 0)
      {
        Serial.println("📥 Skipped GUILD_CREATE");
        return; // bỏ qua hoàn toàn
      }
      else if (t && strcmp(t, "MESSAGE_CREATE") == 0)
      {
        JsonObject d = doc["d"].as<JsonObject>();
        handleMessage(d);
      }
    }
  }
  else if (type == WStype_DISCONNECTED)
  {
    Serial.println("🔌 WebSocket disconnected");
    // Reset heartbeat timer khi disconnect
    lastHeartbeat = 0;
    isReady = false;
    lastReconnectAttempt = millis();
  }
  else if (type == WStype_CONNECTED)
  {
    Serial.println("🔗 WebSocket connected");
    Serial.println("📤 Sending IDENTIFY...");
    sendIdentify();
  }
  else if (type == WStype_ERROR)
  {
    Serial.println("❌ WebSocket ERROR");
  }
  else if (type == WStype_FRAGMENT_TEXT_START)
  {
    Serial.println("📥 WebSocket FRAGMENT_TEXT_START");
  }
  else if (type == WStype_FRAGMENT_BIN_START)
  {
    Serial.println("📥 WebSocket FRAGMENT_BIN_START");
  }
  else if (type == WStype_FRAGMENT)
  {
    Serial.println("📥 WebSocket FRAGMENT");
  }
  else if (type == WStype_FRAGMENT_FIN)
  {
    Serial.println("📥 WebSocket FRAGMENT_FIN");
  }
  else if (type == WStype_PING)
  {
    Serial.println("📥 WebSocket PING");
  }
  else if (type == WStype_PONG)
  {
    Serial.println("📥 WebSocket PONG");
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  // Initialize FreeRTOS mutex
  webSocketMutex = xSemaphoreCreateMutex();
  if (webSocketMutex == NULL)
  {
    Serial.println("❌ Failed to create mutex");
    return;
  }

  WiFi.begin(ssid, password);
  Serial.print("🔄 Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");

  Serial.println("🔗 Starting WebSocket connection...");
  webSocket.beginSSL("gateway.discord.gg", 443, "/?v=10&encoding=json");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  webSocket.setExtraHeaders("User-Agent: DiscordBot (https://github.com/discord/discord-api-docs, 1.0)");
  Serial.println("✅ WebSocket configuration complete");
  
  // Wait a bit for connection to establish
  delay(1000);
  Serial.println("📤 Sending initial IDENTIFY...");
  sendIdentify();

  // Wait a bit more for WebSocket to be ready
  delay(2000);
  Serial.println("🚀 Creating FreeRTOS tasks...");

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(
      webSocketTask,        // Task function
      "WebSocketTask",      // Task name
      8192,                 // Stack size (increased from 4096)
      NULL,                 // Parameters
      2,                    // Priority
      &webSocketTaskHandle, // Task handle
      1                     // Core (0 or 1)
  );

  xTaskCreatePinnedToCore(
      heartbeatTask,        // Task function
      "HeartbeatTask",      // Task name
      4096,                 // Stack size (increased from 2048)
      NULL,                 // Parameters
      1,                    // Priority
      &heartbeatTaskHandle, // Task handle
      0                     // Core (0 or 1)
  );

  Serial.println("✅ FreeRTOS tasks created");
}

void loop()
{
  // Debug connection state mỗi 5 giây
  static uint64_t lastDebug = 0;
  if (millis() - lastDebug > 5000)
  {
    Serial.printf("🔍 Debug - isReady: %s, heartbeat_interval: %d, WiFi: %s\n",
                  isReady ? "true" : "false",
                  heartbeat_interval,
                  WiFi.status() == WL_CONNECTED ? "connected" : "disconnected");
    lastDebug = millis();
  }

  // Tự động reconnect nếu bị disconnect
  if (!isReady && WiFi.status() == WL_CONNECTED &&
      millis() - lastReconnectAttempt > reconnectDelay)
  {
    Serial.println("🔄 Attempting to reconnect...");

    // Reconnect without mutex - it's called from main thread
    webSocket.beginSSL("gateway.discord.gg", 443, "/?v=10&encoding=json");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    webSocket.setExtraHeaders("User-Agent: DiscordBot (https://github.com/discord/discord-api-docs, 1.0)");

    lastReconnectAttempt = millis();
  }

  // Small delay to prevent main loop from consuming too much CPU
  delay(100);
}
