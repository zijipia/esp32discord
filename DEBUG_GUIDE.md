# 🔍 Debug Guide for ESP32 Discord Bot

## 📋 Overview

The Discord API library for ESP32 has been integrated with a detailed debug logging system to help you easily monitor and troubleshoot issues.

## 🎯 Debug Levels

### DEBUG_LEVEL_ERROR (0) - 🔴 Error

- Critical errors that need immediate attention
- Examples: Invalid bot token, connection failures

### DEBUG_LEVEL_WARNING (1) - 🟡 Warning

- Issues that may affect operation
- Examples: WebSocket disconnection, rate limiting

### DEBUG_LEVEL_INFO (2) - 🔵 Info

- Important events during operation
- Examples: Successful connection, bot ready

### DEBUG_LEVEL_VERBOSE (3) - 🟢 Verbose

- Detailed debug information for development
- Examples: WebSocket messages, HTTP requests

## 🚀 Usage

### 1. Setup Debug Callback

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

void setup() {
    // ... other setup code ...

    discord.onDebug(onDebug);
}
```

### 2. Filter Debug Messages

```cpp
void onDebug(String message, int level) {
    // Only show ERROR and WARNING
    if (level <= DEBUG_LEVEL_WARNING) {
        Serial.println("[" + String(level) + "] " + message);
    }
}
```

### 3. Log Debug to File (if SD card available)

```cpp
void onDebug(String message, int level) {
    String timestamp = String(millis() / 1000);
    String logEntry = "[" + timestamp + "] [" + String(level) + "] " + message;

    Serial.println(logEntry);

    // Write to file if SD card available
    if (SD.begin()) {
        File logFile = SD.open("/debug.log", FILE_APPEND);
        if (logFile) {
            logFile.println(logEntry);
            logFile.close();
        }
    }
}
```

## 📊 Important Debug Messages

### Connection and Authentication

```
🔵 [INFO] Bot token set successfully
🔵 [INFO] Connecting to WebSocket...
🔵 [INFO] WebSocket connected
🔵 [INFO] Sent IDENTIFY packet
🔵 [INFO] Bot ready! Session ID: abc123
```

### WebSocket Events

```
🟢 [VERBOSE] Processing WebSocket event: OP=10, Type=READY
🟢 [VERBOSE] Received WebSocket message: {"op":0,"t":"MESSAGE_CREATE"...
🟢 [VERBOSE] Sent heartbeat, sequence: 123
🟢 [VERBOSE] Heartbeat ACK received
```

### HTTP Requests

```
🟢 [VERBOSE] Sending request: POST /channels/123/messages
🟢 [VERBOSE] Request successful: 200
🟡 [WARNING] Request rate limited: /channels/123/messages
🔴 [ERROR] Request failed: HTTP 401: Unauthorized
```

### Errors and Warnings

```
🔴 [ERROR] Invalid bot token (empty)
🟡 [WARNING] WebSocket disconnected
🟡 [WARNING] Invalid session, retrying identify...
🟡 [WARNING] Received reconnect command from Discord
```

## 🔧 Troubleshooting with Debug

### 1. Bot cannot connect

```
🔴 [ERROR] Invalid bot token (empty)
```

**Solution**: Check bot token in code

### 2. WebSocket disconnects frequently

```
🟡 [WARNING] WebSocket disconnected
🟡 [WARNING] Invalid session, retrying identify...
```

**Solution**: Check WiFi connection and bot permissions

### 3. Rate limit

```
🟡 [WARNING] Request rate limited: /channels/123/messages
```

**Solution**: Reduce request frequency

### 4. HTTP 401 Unauthorized

```
🔴 [ERROR] Request failed: HTTP 401: Unauthorized
```

**Solution**: Check bot token and permissions

## 📈 Performance Monitoring

### Memory Usage

```cpp
void onDebug(String message, int level) {
    if (level == DEBUG_LEVEL_INFO) {
        Serial.println("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
    }
    Serial.println(message);
}
```

### Network Quality

```cpp
void onDebug(String message, int level) {
    if (level == DEBUG_LEVEL_WARNING) {
        Serial.println("WiFi RSSI: " + String(WiFi.RSSI()) + " dBm");
    }
    Serial.println(message);
}
```

## 🎛️ Advanced Debug Configuration

### Conditional Debug

```cpp
#define DEBUG_LEVEL 2  // Only show ERROR, WARNING, INFO

void onDebug(String message, int level) {
    if (level <= DEBUG_LEVEL) {
        Serial.println(message);
    }
}
```

### Debug with Timestamp

```cpp
void onDebug(String message, int level) {
    String timestamp = String(millis() / 1000);
    Serial.println("[" + timestamp + "] " + message);
}
```

### Debug with Function Name

```cpp
void onDebug(String message, int level) {
    String funcName = __FUNCTION__;  // Only works in C++
    Serial.println("[" + funcName + "] " + message);
}
```

## 📝 Best Practices

1. **Always use onDebug** in development
2. **Filter debug level** appropriate for environment
3. **Monitor memory usage** regularly
4. **Log network quality** to debug connections
5. **Use timestamp** to track timeline
6. **Disable verbose logging** in production

## 🚨 Important Notes

- Debug logging may affect performance
- Don't log sensitive data like bot token
- Use conditional compilation for production
- Monitor memory usage when enabling verbose logging

---

With this debug system, you can easily monitor and troubleshoot issues during Discord bot development! 🎉
