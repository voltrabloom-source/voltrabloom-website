#ifndef SUPABASE_LOGGER_H
#define SUPABASE_LOGGER_H

#include "ArduinoCompat.h"
#ifdef ARDUINO
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#endif

class SupabaseLogger {
private:
    String url;
    String anonKey;
    String tableName;
    String endpoint;  // Pre-computed to avoid per-upload heap allocation
    unsigned long lastUploadTime;

public:
    SupabaseLogger() : url(""), anonKey(""), tableName("telemetry"), endpoint(""), lastUploadTime(0) {}

    // Initialize Supabase configuration credentials
    void init(const char* supabaseUrl, const char* supabaseAnonKey, const char* table = "telemetry") {
        url = String(supabaseUrl);
        anonKey = String(supabaseAnonKey);
        tableName = String(table);
        
        // Remove trailing slash if provided in URL
        if (url.endsWith("/")) {
            url = url.substring(0, url.length() - 1);
        }
        
        // Pre-compute endpoint to avoid heap allocation on every upload
        endpoint = url + "/rest/v1/" + tableName;
        
        Serial.println("[Supabase] Initialized cloud logger for table: " + tableName);
    }

    // Direct function to send telemetry JSON payload to Supabase database
    bool sendTelemetry(float vSolar, float vWind, float vSoil, float vOut, 
                       float iIn, float iOut, float battPercent, float battAh) {
        // Skip upload if not in station mode or not connected to internet
        if (WiFi.status() != WL_CONNECTED) {
            return false;
        }

        if (url.length() == 0 || anonKey.length() == 0) {
            return false;
        }

        WiFiClientSecure client;
        client.setInsecure(); // Skip TLS verification for ESP32 compatibility

        HTTPClient http;

        if (!http.begin(client, endpoint)) {
            Serial.println("[Supabase] Connection failed to endpoint: " + endpoint);
            return false;
        }

        // Build RESTful API headers for Supabase PostgREST endpoint
        http.addHeader("Content-Type", "application/json");
        http.addHeader("apikey", anonKey);
        // Use char[] for Authorization header to avoid String heap allocation per upload
        char authHeader[256];
        snprintf(authHeader, sizeof(authHeader), "Bearer %s", anonKey.c_str());
        http.addHeader("Authorization", authHeader);
        http.addHeader("Prefer", "return=minimal");
        http.setTimeout(3000); // 3-second non-blocking timeout

        // Construct JSON Payload
        char jsonBuffer[384];
        snprintf(jsonBuffer, sizeof(jsonBuffer),
            "{"
            "\"solar_v\":%.2f,"
            "\"wind_v\":%.2f,"
            "\"soil_v\":%.2f,"
            "\"output_v\":%.2f,"
            "\"current_in_a\":%.2f,"
            "\"current_out_a\":%.2f,"
            "\"battery_percent\":%.1f,"
            "\"battery_ah\":%.2f"
            "}",
            vSolar, vWind, vSoil, vOut, iIn, iOut, battPercent, battAh
        );

        int httpResponseCode = http.POST(jsonBuffer);
        bool success = (httpResponseCode == 201 || httpResponseCode == 200);

        if (success) {
            Serial.printf("[Supabase] Upload OK (HTTP %d)\n", httpResponseCode);
        } else {
            Serial.printf("[Supabase] Upload Error (HTTP %d): %s\n", httpResponseCode, http.errorToString(httpResponseCode).c_str());
        }

        http.end();
        return success;
    }

    // Helper function to upload non-blockingly at a set interval
    void logSerialAndSupabase(float vSolar, float vWind, float vSoil, float vOut, 
                             float iIn, float iOut, float battPercent, float battAh, 
                             unsigned long intervalMs = 5000) {
        unsigned long currentMillis = millis();

        if (currentMillis - lastUploadTime >= intervalMs || lastUploadTime == 0) {
            lastUploadTime = currentMillis;

            Serial.println("================ VOLTRABLOOM TELEMETRY ================");
            Serial.printf("Solar: %.2f V | Wind: %.2f V | Soil: %.2f V | Out: %.2f V\n", vSolar, vWind, vSoil, vOut);
            Serial.printf("Arus In: %.2f A | Arus Out: %.2f A | Batt: %.1f%% (%.2f Ah)\n", iIn, iOut, battPercent, battAh);
            Serial.println("------------------------------------------------------");

            sendTelemetry(vSolar, vWind, vSoil, vOut, iIn, iOut, battPercent, battAh);
        }
    }
};

#endif // SUPABASE_LOGGER_H
