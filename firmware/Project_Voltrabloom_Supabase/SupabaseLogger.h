#ifndef SUPABASE_LOGGER_H
#define SUPABASE_LOGGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

class SupabaseLogger {
private:
    String url;
    String anonKey;
    String tableName;
    unsigned long lastUploadTime;

public:
    SupabaseLogger() : url(""), anonKey(""), tableName("telemetry"), lastUploadTime(0) {}

    // Initialize Supabase configuration credentials
    void init(const char* supabaseUrl, const char* supabaseAnonKey, const char* table = "telemetry") {
        url = String(supabaseUrl);
        anonKey = String(supabaseAnonKey);
        tableName = String(table);
        
        // Remove trailing slash if provided in URL
        if (url.endsWith("/")) {
            url = url.substring(0, url.length() - 1);
        }
        
        Serial.println("[Supabase] Initialized logger targeting table: " + tableName);
    }

    // Direct function to send telemetry JSON payload to Supabase database
    bool sendTelemetry(float vSolar, float vWind, float vSoil, float vOut, 
                       float iIn, float iOut, float battPercent, float battAh) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[Supabase] Upload skipped: WiFi not connected.");
            return false;
        }

        if (url.length() == 0 || anonKey.length() == 0) {
            Serial.println("[Supabase] Error: Supabase URL or Anon Key not configured.");
            return false;
        }

        WiFiClientSecure client;
        client.setInsecure(); // Skip TLS certificate validation for ESP32 compatibility

        HTTPClient http;
        String endpoint = url + "/rest/v1/" + tableName;

        if (!http.begin(client, endpoint)) {
            Serial.println("[Supabase] Connection failed to endpoint: " + endpoint);
            return false;
        }

        // Build RESTful API headers for Supabase PostgREST endpoint
        http.addHeader("Content-Type", "application/json");
        http.addHeader("apikey", anonKey);
        http.addHeader("Authorization", "Bearer " + anonKey);
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

        bool success = false;
        if (httpResponseCode == 201 || httpResponseCode == 200) {
            success = true;
        } else {
            Serial.printf("[Supabase] HTTP Error %d: %s\n", httpResponseCode, http.errorToString(httpResponseCode).c_str());
        }

        http.end();
        return success;
    }

    // Helper function to print telemetry to Serial Monitor and upload non-blockingly to Supabase
    void logSerialAndSupabase(float vSolar, float vWind, float vSoil, float vOut, 
                             float iIn, float iOut, float battPercent, float battAh, 
                             unsigned long intervalMs = 5000) {
        unsigned long currentMillis = millis();

        // Check if upload interval has elapsed
        if (currentMillis - lastUploadTime >= intervalMs || lastUploadTime == 0) {
            lastUploadTime = currentMillis;

            // Output structured telemetric log line to Serial Monitor
            Serial.println("================ VOLTRABLOOM TELEMETRY ================");
            Serial.printf("Solar: %.2f V | Wind: %.2f V | Soil: %.2f V | Out: %.2f V\n", vSolar, vWind, vSoil, vOut);
            Serial.printf("Arus In: %.2f A | Arus Out: %.2f A | Batt: %.1f%% (%.2f Ah)\n", iIn, iOut, battPercent, battAh);
            Serial.println("------------------------------------------------------");

            // Perform HTTPS upload to Supabase
            sendTelemetry(vSolar, vWind, vSoil, vOut, iIn, iOut, battPercent, battAh);
        }
    }
}; // End of class SupabaseLogger

// NOTE: Do NOT add an extern declaration here.
// The SupabaseLogger instance ('supabase') is defined in the main .ino sketch file.
// Adding an extern here would cause a linker conflict if this header is ever
// included from multiple compilation units.

#endif // SUPABASE_LOGGER_H
