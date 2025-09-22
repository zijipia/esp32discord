# ESP32 Discord API Library

Arduino library for ESP32 to communicate with Discord API v10, supporting both REST API and WebSocket Gateway.

## 🚀 Features

- ✅ **REST API**: Send/receive messages, manage channels, guilds, users
- ✅ **WebSocket Gateway**: Real-time connection with Discord
- ✅ **Rate Limiting**: Automatic API rate limit handling
- ✅ **Event Handling**: Callbacks for Discord events
- ✅ **JSON Parsing**: Uses ArduinoJson for data processing
- ✅ **Error Handling**: Automatic error handling and retry
- ✅ **WiFi Support**: Built-in ESP32 WiFi integration
- ✅ **Debug Logging**: Comprehensive debug system with multiple levels

## 📋 Requirements

- ESP32 (ESP32-S3, ESP32-C3, ESP32-S2, etc.)
- PlatformIO
- WiFi connection
- Discord Bot Token

## 🔧 Installation

### 1. Clone repository

```bash
git clone https://github.com/yourusername/esp32discord.git
cd esp32discord
```

### 2. Install dependencies

Dependencies are already configured in `platformio.ini`:

```ini
lib_deps =
    knolleary/PubSubClient@^2.8
    https://github.com/Links2004/arduinoWebSockets
    bblanchon/ArduinoJson@^7.4.2
    WiFi
    HTTPClient
    WebSocketsClient
```

### 3. Create Discord Bot

1. Visit [Discord Developer Portal](https://discord.com/developers/applications)
2. Create New Application
3. Go to "Bot" tab and create bot
4. Copy Bot Token
5. Invite bot to server with necessary permissions

### 4. Configuration

Edit `src/main.cpp`:

```cpp
// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Discord Bot Configuration
const char* botToken = "YOUR_BOT_TOKEN";
const char* channelId = "YOUR_CHANNEL_ID";
```

## 📖 Usage

### Basic initialization

```cpp
#include "DiscordAPI.h"

DiscordAPI discord;

void setup() {
    // Connect to WiFi
    WiFi.begin(ssid, password);

    // Set bot token
    discord.setBotToken("YOUR_BOT_TOKEN");

    // Connect WebSocket
    discord.connectWebSocket();
}

void loop() {
    discord.loop();
}
```

### REST API

#### Send message

```cpp
DiscordResponse response = discord.sendMessage("CHANNEL_ID", "Hello from ESP32!");
if (response.success) {
    Serial.println("Message sent successfully!");
} else {
    Serial.println("Error: " + response.error);
}
```

#### Get user information

```cpp
DiscordUser user = discord.getCurrentUser();
Serial.println("Bot name: " + user.username);
Serial.println("ID: " + user.id);
```

#### Get channel messages

```cpp
DiscordMessage* messages = discord.getChannelMessages("CHANNEL_ID", 10);
if (messages != nullptr) {
    for (int i = 0; i < 10; i++) {
        Serial.println("Message: " + messages[i].content);
    }
    delete[] messages;
}
```

### WebSocket Events

#### Handle Ready event

```cpp
void onBotReady(DiscordUser user) {
    Serial.println("Bot ready: " + user.username);
}

discord.onReady(onBotReady);
```

#### Handle new messages

```cpp
void onMessageReceived(DiscordMessage message) {
    Serial.println("Message from " + message.author.username + ": " + message.content);

    // Respond to message
    if (message.content == "!ping") {
        discord.sendMessage(message.channel_id, "Pong!");
    }
}

discord.onMessage(onMessageReceived);
```

#### Handle errors

```cpp
void onError(String error) {
    Serial.println("Error: " + error);
}

discord.onError(onError);
```

### Debug Logging

#### Setup debug callback

```cpp
void onDebug(String message, int level) {
    String prefix = "";
    switch(level) {
        case 0: // ERROR
            prefix = "🔴 [ERROR]";
            break;
        case 1: // WARNING
            prefix = "🟡 [WARNING]";
            break;
        case 2: // INFO
            prefix = "🔵 [INFO]";
            break;
        case 3: // VERBOSE
            prefix = "🟢 [VERBOSE]";
            break;
        default:
            prefix = "⚪ [DEBUG]";
            break;
    }
    Serial.println(prefix + " " + message);
}

discord.onDebug(onDebug);
```

### Utility Functions

#### Format messages

```cpp
// Mention user
String mention = discord.formatUserMention("USER_ID");

// Bold text
String bold = discord.formatBold("Bold text");

// Code block
String code = discord.formatCodeBlock("void setup() {\n  Serial.begin(115200);\n}", "cpp");

// Emoji
String emoji = discord.formatEmoji("smile", "EMOJI_ID");
```

#### Rate limiting

```cpp
if (discord.isRateLimited()) {
    Serial.println("Rate limited, try again in " + String(discord.getRateLimitReset()) + "ms");
}
```

## 🎯 Complete Example

See `src/main.cpp` for a complete bot example with commands:

- `!ping` - Check bot
- `!time` - View uptime
- `!help` - Show help
- `!status` - System status

## 📚 API Reference

### DiscordAPI Class

#### Constructor

```cpp
DiscordAPI()
```

#### Authentication

```cpp
bool setBotToken(String token)
bool setOAuth2Credentials(String clientId, String clientSecret, String redirectUri)
String getOAuth2URL(String scope = "identify")
String exchangeCodeForToken(String code)
```

#### REST API Methods

```cpp
DiscordUser getCurrentUser()
DiscordUser getUser(String userId)
DiscordGuild getGuild(String guildId)
DiscordChannel getChannel(String channelId)
DiscordMessage getMessage(String channelId, String messageId)
DiscordMessage* getChannelMessages(String channelId, int limit = 50, String before = "", String after = "", String around = "")
DiscordResponse sendMessage(String channelId, String content, bool tts = false)
DiscordResponse editMessage(String channelId, String messageId, String content)
DiscordResponse deleteMessage(String channelId, String messageId)
DiscordResponse addReaction(String channelId, String messageId, String emoji)
DiscordResponse removeReaction(String channelId, String messageId, String emoji, String userId = "@me")
DiscordResponse removeAllReactions(String channelId, String messageId)
DiscordResponse removeAllReactionsForEmoji(String channelId, String messageId, String emoji)
```

#### WebSocket Methods

```cpp
bool connectWebSocket()
void disconnectWebSocket()
void loop()
bool isWebSocketConnected()
```

#### Event Handlers

```cpp
void onReady(void (*callback)(DiscordUser user))
void onMessage(void (*callback)(DiscordMessage message))
void onGuildCreate(void (*callback)(DiscordGuild guild))
void onError(void (*callback)(String error))
void onDebug(void (*callback)(String message, int level))
```

#### Utility Methods

```cpp
String getBotInviteURL(String permissions = "0")
String formatUserMention(String userId)
String formatChannelMention(String channelId)
String formatRoleMention(String roleId)
String formatEmoji(String name, String id, bool animated = false)
String formatTimestamp(String timestamp, String style = "f")
String formatCodeBlock(String code, String language = "")
String formatInlineCode(String code)
String formatBold(String text)
String formatItalic(String text)
String formatUnderline(String text)
String formatStrikethrough(String text)
String formatSpoiler(String text)
String formatQuote(String text)
String formatBlockQuote(String text)
```

### Data Structures

#### DiscordUser

```cpp
struct DiscordUser {
    String id;
    String username;
    String discriminator;
    String global_name;
    String avatar;
    bool bot;
    bool system;
    // ... and many more fields
};
```

#### DiscordMessage

```cpp
struct DiscordMessage {
    String id;
    String channel_id;
    String guild_id;
    DiscordUser author;
    String content;
    String timestamp;
    // ... and many more fields
};
```

#### DiscordResponse

```cpp
struct DiscordResponse {
    int statusCode;
    String body;
    bool success;
    String error;
};
```

## ⚠️ Important Notes

1. **Bot Token**: Never commit bot token to Git
2. **Rate Limiting**: Discord has a limit of 50 requests per second
3. **Memory**: ESP32 has limited memory, avoid creating too many objects
4. **WiFi**: Ensure stable WiFi connection
5. **Error Handling**: Always check response.success before using data

## 🐛 Troubleshooting

### Bot cannot connect

- Check bot token
- Check WiFi connection
- Check bot permissions in server

### Rate limit error

- Reduce request frequency
- Use `discord.isRateLimited()` to check

### Memory error

- Reduce message size
- Delete unnecessary objects
- Use `ESP.getFreeHeap()` to check RAM

## 📄 License

MIT License - See LICENSE file for details.

## 🤝 Contributing

Contributions are welcome! Please create an issue or pull request.

## 📞 Support

If you encounter issues, please create an issue on GitHub or contact via Discord.

---

**Note**: This library is developed for educational and personal project purposes. Please comply with [Discord Terms of Service](https://discord.com/terms) and [Developer Policy](https://discord.com/developers/docs/policy).
