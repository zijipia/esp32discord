# 🛡️ Memory Safety Guide for ESP32 Discord Bot

## 🚨 Common Memory Issues

### 1. LoadProhibited Error

**Symptoms:**

```
Guru Meditation Error: Core 1 panic'ed (LoadProhibited). Exception was unhandled.
EXCVADDR: 0x00000000
```

**Causes:**

- Null pointer dereference
- Accessing freed memory
- Buffer overflow
- Uninitialized pointers

**Solutions:**

- Always check for null pointers
- Use safe memory allocation
- Validate JSON objects before access
- Implement proper error handling

## 🔧 Memory Safety Features Added

### 1. Null Pointer Checks

```cpp
// Before (unsafe)
String message = String((char*)payload);

// After (safe)
if (payload != nullptr && length > 0) {
    String message = String((char*)payload);
    // Process message
} else {
    _debugLog("Received empty WebSocket message", DEBUG_LEVEL_WARNING);
}
```

### 2. JSON Validation

```cpp
// Before (unsafe)
int op = doc["op"];
String eventType = doc["t"];

// After (safe)
if (doc.isNull() || !doc.is<JsonObject>()) {
    _debugLog("Invalid JSON document received", DEBUG_LEVEL_ERROR);
    return;
}

int op = doc["op"].as<int>();
String eventType = doc["t"].as<String>();
```

### 3. Safe Memory Allocation

```cpp
// Before (unsafe)
DiscordMessage* messageArray = new DiscordMessage[messageCount];

// After (safe)
DiscordMessage* messageArray = new (std::nothrow) DiscordMessage[messageCount];
if (messageArray == nullptr) {
    _debugLog("Failed to allocate memory for messages", DEBUG_LEVEL_ERROR);
    return nullptr;
}
```

### 4. Array Bounds Checking

```cpp
// Before (unsafe)
for (int i = 0; i < message.mentions_count; i++) {
    message.mentions[i] = mentions[i]["id"].as<String>();
}

// After (safe)
if (message.mentions_count > 0) {
    message.mentions = new (std::nothrow) String[message.mentions_count];
    if (message.mentions != nullptr) {
        for (int i = 0; i < message.mentions_count; i++) {
            message.mentions[i] = mentions[i]["id"].as<String>();
        }
    } else {
        _debugLog("Failed to allocate memory for mentions", DEBUG_LEVEL_ERROR);
        message.mentions_count = 0;
    }
}
```

## 📊 Memory Monitoring

### 1. Check Free Heap

```cpp
void checkMemory() {
    Serial.println("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("Largest free block: " + String(ESP.getMaxAllocHeap()) + " bytes");
}
```

### 2. Monitor Memory Usage

```cpp
void onDebug(String message, int level) {
    if (message.indexOf("heap") != -1) {
        Serial.println("💾 " + message);
    }
}
```

### 3. Memory Leak Detection

```cpp
void detectMemoryLeaks() {
    static unsigned long lastCheck = 0;
    static size_t lastFreeHeap = 0;

    if (millis() - lastCheck > 30000) { // Check every 30 seconds
        size_t currentFreeHeap = ESP.getFreeHeap();
        if (currentFreeHeap < lastFreeHeap - 1000) { // Lost more than 1KB
            Serial.println("⚠️ Potential memory leak detected!");
            Serial.println("Previous: " + String(lastFreeHeap) + " bytes");
            Serial.println("Current: " + String(currentFreeHeap) + " bytes");
        }
        lastFreeHeap = currentFreeHeap;
        lastCheck = millis();
    }
}
```

## 🛠️ Best Practices

### 1. Always Check Pointers

```cpp
if (pointer != nullptr) {
    // Safe to use pointer
    pointer->method();
} else {
    // Handle null pointer
    _debugLog("Null pointer detected", DEBUG_LEVEL_ERROR);
}
```

### 2. Validate JSON Objects

```cpp
if (jsonObj.is<JsonObject>()) {
    // Safe to access properties
    String value = jsonObj["key"].as<String>();
} else {
    // Handle invalid JSON
    _debugLog("Invalid JSON object", DEBUG_LEVEL_ERROR);
}
```

### 3. Use Safe Allocation

```cpp
// Use nothrow to prevent exceptions
String* array = new (std::nothrow) String[size];
if (array == nullptr) {
    // Handle allocation failure
    return false;
}
```

### 4. Free Memory Properly

```cpp
// Always free allocated memory
if (array != nullptr) {
    delete[] array;
    array = nullptr;
}
```

### 5. Use RAII Principles

```cpp
class SafeStringArray {
private:
    String* data;
    size_t size;

public:
    SafeStringArray(size_t s) : size(s) {
        data = new (std::nothrow) String[size];
    }

    ~SafeStringArray() {
        delete[] data;
    }

    String& operator[](size_t index) {
        return data[index];
    }
};
```

## 🔍 Debugging Memory Issues

### 1. Enable Verbose Debugging

```cpp
void onDebug(String message, int level) {
    // Show all debug levels for memory issues
    if (level <= DEBUG_LEVEL_VERBOSE) {
        Serial.println(message);
    }
}
```

### 2. Add Memory Checks

```cpp
void debugMemoryUsage() {
    Serial.println("=== Memory Status ===");
    Serial.println("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("Largest free block: " + String(ESP.getMaxAllocHeap()) + " bytes");
    Serial.println("Min free heap: " + String(ESP.getMinFreeHeap()) + " bytes");
}
```

### 3. Monitor Allocation Patterns

```cpp
void trackAllocation(size_t size) {
    static size_t totalAllocated = 0;
    totalAllocated += size;
    Serial.println("Total allocated: " + String(totalAllocated) + " bytes");
}
```

## ⚠️ Common Pitfalls

### 1. Dangling Pointers

```cpp
// BAD: Pointer becomes invalid
String* getString() {
    String local = "Hello";
    return &local; // Returns pointer to local variable
}

// GOOD: Return by value or use dynamic allocation
String getString() {
    return "Hello"; // Returns copy
}
```

### 2. Double Free

```cpp
// BAD: Freeing same memory twice
delete[] array;
delete[] array; // Error!

// GOOD: Set to nullptr after freeing
delete[] array;
array = nullptr;
```

### 3. Memory Leaks

```cpp
// BAD: Not freeing allocated memory
String* array = new String[100];
// Forgot to delete[] array;

// GOOD: Always free memory
String* array = new String[100];
// ... use array ...
delete[] array;
```

## 🚀 Performance Tips

### 1. Use Stack When Possible

```cpp
// GOOD: Stack allocation (faster)
String localString = "Hello";

// AVOID: Heap allocation when not needed
String* heapString = new String("Hello");
delete heapString;
```

### 2. Reuse Objects

```cpp
// GOOD: Reuse objects
static JsonDocument doc;
doc.clear();
deserializeJson(doc, jsonString);
```

### 3. Limit String Operations

```cpp
// BAD: Many string concatenations
String result = "";
for (int i = 0; i < 100; i++) {
    result += "item" + String(i);
}

// GOOD: Use StringBuffer or pre-allocate
StringBuffer buffer(1000);
for (int i = 0; i < 100; i++) {
    buffer.append("item");
    buffer.append(i);
}
```

## 📈 Memory Optimization

### 1. Reduce String Usage

```cpp
// Use const char* when possible
const char* message = "Hello World";

// Instead of
String message = "Hello World";
```

### 2. Use References

```cpp
// GOOD: Pass by reference
void processMessage(const DiscordMessage& message);

// AVOID: Pass by value (creates copy)
void processMessage(DiscordMessage message);
```

### 3. Implement Object Pooling

```cpp
class MessagePool {
private:
    std::vector<DiscordMessage> pool;
    std::vector<bool> inUse;

public:
    DiscordMessage* getMessage() {
        for (size_t i = 0; i < pool.size(); i++) {
            if (!inUse[i]) {
                inUse[i] = true;
                return &pool[i];
            }
        }
        return nullptr;
    }

    void returnMessage(DiscordMessage* msg) {
        // Find and mark as available
    }
};
```

## 🎯 Summary

The Discord API library now includes comprehensive memory safety features:

- ✅ Null pointer checks
- ✅ JSON validation
- ✅ Safe memory allocation
- ✅ Array bounds checking
- ✅ Proper error handling
- ✅ Memory leak detection
- ✅ Debug logging for memory issues

These improvements should prevent the LoadProhibited error and make the bot more stable! 🎉
