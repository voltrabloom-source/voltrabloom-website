#ifndef ARDUINO_COMPAT_H
#define ARDUINO_COMPAT_H

/*
 * ==============================================================================
 * ARDUINO & FREERTOS IDE LANGUAGE SERVER COMPATIBILITY SHIM
 * ==============================================================================
 */

#ifdef ARDUINO
    #include <Arduino.h>
    #include <Wire.h>
    #include <LiquidCrystal_I2C.h>
    #include <WiFi.h>
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"
    #include "soc/soc.h"
    #include "soc/rtc_cntl_reg.h"
    #include "esp_adc_cal.h"
#else
    // Standard C/C++ Headers
    #include <cstdio>
    #include <cstdint>
    #include <cstring>
    #include <cstdlib>
    #include <cstdarg>

    #ifndef NULL
    #define NULL nullptr
    #endif

    #define pdTRUE  1
    #define pdFALSE 0
    #define portMAX_DELAY 0xFFFFFFFFUL
    #define pdMS_TO_TICKS(ms) ((uint32_t)(ms))

    typedef void* SemaphoreHandle_t;
    typedef void* TaskHandle_t;
    typedef uint32_t TickType_t;
    typedef uint32_t BaseType_t;

    typedef void (*TaskFunction_t)(void *);

    inline SemaphoreHandle_t xSemaphoreCreateMutex() { return (SemaphoreHandle_t)1; }
    inline BaseType_t xSemaphoreTake(SemaphoreHandle_t, TickType_t) { return pdTRUE; }
    inline BaseType_t xSemaphoreGive(SemaphoreHandle_t) { return pdTRUE; }
    inline void vTaskDelay(TickType_t) {}

    inline BaseType_t xTaskCreatePinnedToCore(
        TaskFunction_t,
        const char *,
        const uint32_t,
        void *,
        uint32_t,
        TaskHandle_t *,
        const BaseType_t
    ) {
        return pdTRUE;
    }

    #define WIFI_STA 1
    #define WIFI_AP  2
    #define WL_CONNECTED 3
    #define RTC_CNTL_BROWN_OUT_REG 0
    inline void WRITE_PERI_REG(uint32_t, uint32_t) {}

    inline unsigned long millis() { return 0UL; }
    inline void delay(unsigned long) {}
    inline int analogRead(uint8_t) { return 0; }

    class String {
    public:
        String() {}
        String(const char*) {}
        String(int) {}
        String(float) {}
        int length() const { return 0; }
        const char* c_str() const { return ""; }
        bool endsWith(const char*) const { return false; }
        String substring(int, int = -1) const { return *this; }
        
        String operator+(const String&) const { return *this; }
        String operator+(const char*) const { return *this; }
        String operator+(int) const { return *this; }
        String operator+(float) const { return *this; }
        
        String& operator+=(char) { return *this; }
        String& operator+=(const char*) { return *this; }
        String& operator+=(const String&) { return *this; }
        
        int indexOf(const char*) const { return -1; }
        int indexOf(const String&) const { return -1; }

        friend String operator+(const char*, const String&) { return String(); }
    };

    class HardwareSerial {
    public:
        void begin(unsigned long) {}
        void print(const char*) {}
        void print(const String&) {}
        void print(int) {}
        void print(float, int = 2) {}
        void println(const char* = "") {}
        void println(const String&) {}
        void println(int) {}
        void println(float, int = 2) {}
        void printf(const char*, ...) {}
    };
    static HardwareSerial Serial;

    class LiquidCrystal_I2C {
    public:
        LiquidCrystal_I2C(uint8_t, uint8_t, uint8_t) {}
        void init() {}
        void backlight() {}
        void clear() {}
        void setCursor(uint8_t, uint8_t) {}
        void print(const char*) {}
        void print(const String&) {}
        void print(int) {}
        void print(float, int = 2) {}
    };

    class IPAddress {
    public:
        String toString() const { return String("192.168.4.1"); }
    };

    class WiFiClass {
    public:
        void mode(int) {}
        void begin(const char*, const char*) {}
        int status() { return WL_CONNECTED; }
        void softAP(const char*, const char*) {}
        IPAddress localIP() { return IPAddress(); }
        IPAddress softAPIP() { return IPAddress(); }
    };
    static WiFiClass WiFi;

    class WiFiClient {
    public:
        operator bool() const { return false; }
        bool connected() { return false; }
        bool available() { return false; }
        char read() { return 0; }
        void print(const char*) {}
        void print(const String&) {}
        void print(int) {}
        void print(float, int = 2) {}
        void println(const char* = "") {}
        void println(const String&) {}
        void stop() {}
    };

    class WiFiServer {
    public:
        WiFiServer(int) {}
        void begin() {}
        WiFiClient available() { return WiFiClient(); }
    };

    class HTTPClient {
    public:
        bool begin(class WiFiClientSecure&, const String&) { return true; }
        void addHeader(const String&, const String&) {}
        void setTimeout(uint16_t) {}
        int POST(const char*) { return 200; }
        int POST(const uint8_t*, size_t) { return 200; }
        String errorToString(int) { return String("OK"); }
        void end() {}
    };

    class WiFiClientSecure {
    public:
        void setInsecure() {}
    };
#endif

#endif // ARDUINO_COMPAT_H
