/**
 * ============================================================================
 * WIFI & WEB SERVER MODULE
 * Display Master - Smart Mosque Ecosystem v1.0
 * ============================================================================
 * 
 * Fungsi: WiFi Access Point + REST API + Web Dashboard
 * Features: Device list, control relay, statistics
 * 
 * Referensi:
 * - AsyncTCP library
 * - ESPAsyncWebServer library
 * - ArduinoJson library
 * 
 * ============================================================================
 */

#include "../include/display_master.h"

// ============================================================================
// WEB SERVER HTML PAGES
// ============================================================================

// Simple HTML dashboard (embedded)
const char HTML_DASHBOARD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Mosque Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: Arial, sans-serif; 
            background: #1C2833; 
            color: #ECF0F1; 
            padding: 20px;
        }
        .container { max-width: 1200px; margin: 0 auto; }
        h1 { color: #1ABC9C; margin-bottom: 20px; }
        .stats { 
            display: grid; 
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); 
            gap: 15px; 
            margin-bottom: 30px;
        }
        .stat-card { 
            background: #2C3E50; 
            padding: 20px; 
            border-radius: 10px;
            border: 2px solid #34495E;
        }
        .stat-value { 
            font-size: 32px; 
            color: #1ABC9C; 
            font-weight: bold;
        }
        .stat-label { 
            font-size: 14px; 
            color: #BDC3C7; 
            margin-top: 5px;
        }
        table { 
            width: 100%; 
            background: #2C3E50; 
            border-radius: 10px; 
            overflow: hidden;
            border-collapse: collapse;
        }
        th, td { 
            padding: 15px; 
            text-align: left; 
            border-bottom: 1px solid #34495E;
        }
        th { 
            background: #1ABC9C; 
            color: white; 
            font-weight: bold;
        }
        tr:hover { background: #34495E; }
        .online { color: #27AE60; }
        .offline { color: #E74C3C; }
        .btn {
            padding: 8px 16px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 14px;
            margin: 2px;
        }
        .btn-on { background: #27AE60; color: white; }
        .btn-off { background: #E74C3C; color: white; }
        .btn:hover { opacity: 0.8; }
        .refresh-btn {
            background: #3498DB;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            margin-bottom: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🕌 Smart Mosque Dashboard</h1>
        
        <button class="refresh-btn" onclick="loadData()">🔄 Refresh</button>
        
        <div class="stats">
            <div class="stat-card">
                <div class="stat-value" id="total">-</div>
                <div class="stat-label">Total Devices</div>
            </div>
            <div class="stat-card">
                <div class="stat-value" id="online">-</div>
                <div class="stat-label">Online</div>
            </div>
            <div class="stat-card">
                <div class="stat-value" id="manual">-</div>
                <div class="stat-label">Manual Mode</div>
            </div>
            <div class="stat-card">
                <div class="stat-value" id="failsafe">-</div>
                <div class="stat-label">Fail-Safe</div>
            </div>
        </div>
        
        <table>
            <thead>
                <tr>
                    <th>ID</th>
                    <th>Device Name</th>
                    <th>Profile</th>
                    <th>Status</th>
                    <th>Mode</th>
                    <th>Actions</th>
                </tr>
            </thead>
            <tbody id="deviceList">
                <tr><td colspan="6" style="text-align:center">Loading...</td></tr>
            </tbody>
        </table>
    </div>
    
    <script>
        function loadData() {
            fetch('/api/devices')
                .then(r => r.json())
                .then(data => {
                    // Update stats
                    document.getElementById('total').textContent = data.total || 0;
                    document.getElementById('online').textContent = data.online || 0;
                    document.getElementById('manual').textContent = data.manual_mode || 0;
                    document.getElementById('failsafe').textContent = data.failsafe || 0;
                    
                    // Update device list
                    const tbody = document.getElementById('deviceList');
                    if (!data.devices || data.devices.length === 0) {
                        tbody.innerHTML = '<tr><td colspan="6" style="text-align:center">No devices found</td></tr>';
                        return;
                    }
                    
                    tbody.innerHTML = data.devices.map(d => `
                        <tr>
                            <td>${d.id}</td>
                            <td>${d.name}</td>
                            <td>${d.profile}</td>
                            <td class="${d.online ? 'online' : 'offline'}">${d.online ? 'ONLINE' : 'OFFLINE'}</td>
                            <td>${d.control_source}</td>
                            <td>
                                ${d.online && d.control_source === 'AUTO' ? 
                                    `<button class="btn btn-on" onclick="allOn(${d.id})">ALL ON</button>
                                     <button class="btn btn-off" onclick="allOff(${d.id})">ALL OFF</button>` :
                                    '-'}
                            </td>
                        </tr>
                    `).join('');
                })
                .catch(e => {
                    console.error('Error:', e);
                    document.getElementById('deviceList').innerHTML = 
                        '<tr><td colspan="6" style="text-align:center;color:#E74C3C">Error loading data</td></tr>';
                });
        }
        
        function allOn(deviceId) {
            controlDevice(deviceId, true);
        }
        
        function allOff(deviceId) {
            controlDevice(deviceId, false);
        }
        
        function controlDevice(deviceId, state) {
            const payload = {
                device_id: deviceId,
                relay: 255,  // ALL relays
                state: state
            };
            
            fetch('/api/control', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(payload)
            })
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    alert('Command sent successfully');
                    setTimeout(loadData, 500);  // Reload data
                } else {
                    alert('Command failed: ' + (data.message || 'Unknown error'));
                }
            })
            .catch(e => {
                alert('Error: ' + e.message);
            });
        }
        
        // Auto-refresh setiap 5 detik
        loadData();
        setInterval(loadData, 5000);
    </script>
</body>
</html>
)rawliteral";

// ============================================================================
// WIFI INITIALIZATION
// ============================================================================

/**
 * Initialize WiFi Access Point
 * Create AP untuk web dashboard access
 */
void wifiInit() {
    Serial.println("📶 Initializing WiFi AP...");
    
    // Setup AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    
    // Get AP IP
    IPAddress IP = WiFi.softAPIP();
    
    Serial.println("✓ WiFi AP started");
    Serial.printf("  SSID: %s\n", WIFI_AP_SSID);
    Serial.printf("  Password: %s\n", WIFI_AP_PASSWORD);
    Serial.printf("  IP Address: %s\n", IP.toString().c_str());
    Serial.printf("  Dashboard: http://%s\n", IP.toString().c_str());
}

// ============================================================================
// WEB SERVER INITIALIZATION
// ============================================================================

/**
 * Initialize web server
 * Setup REST API endpoints dan web dashboard
 */
void webServerInit() {
    Serial.println("🌐 Initializing web server...");
    
    // Route: Root (dashboard HTML)
    g_webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", HTML_DASHBOARD);
    });
    
    // Route: GET /api/devices
    g_webServer.on("/api/devices", HTTP_GET, webApiGetDevices);
    
    // Route: POST /api/control
    g_webServer.on("/api/control", HTTP_POST, 
        [](AsyncWebServerRequest *request) {}, 
        nullptr, 
        webApiControlRelay
    );
    
    // Route: GET /api/stats
    g_webServer.on("/api/stats", HTTP_GET, webApiGetStats);
    
    // Route: 404 handler
    g_webServer.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "application/json", "{\"error\":\"Not found\"}");
    });
    
    // Start server
    g_webServer.begin();
    
    Serial.println("✓ Web server started on port 80");
}

// ============================================================================
// REST API HANDLERS
// ============================================================================

/**
 * API: GET /api/devices
 * Return device list sebagai JSON
 */
void webApiGetDevices(AsyncWebServerRequest* request) {
    Serial.println("📡 API: GET /api/devices");
    
    // Build JSON response
    char json_buffer[8192];  // Large buffer untuk banyak devices
    size_t len = deviceManagerExportJSON(json_buffer, sizeof(json_buffer));
    
    if (len > 0) {
        request->send(200, "application/json", json_buffer);
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to generate JSON\"}");
    }
}

/**
 * API: POST /api/control
 * Control relay via REST API
 * Body: {"device_id": 1, "relay": 0, "state": true}
 */
void webApiControlRelay(AsyncWebServerRequest* request, uint8_t* data, 
                        size_t len, size_t index, size_t total) {
    
    // Parse JSON body
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, data, len);
    
    if (error) {
        Serial.printf("⚠ JSON parse error: %s\n", error.c_str());
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
    }
    
    // Extract parameters
    uint8_t device_id = doc["device_id"] | 0;
    uint8_t relay = doc["relay"] | 255;  // 255 = ALL relays
    bool state = doc["state"] | false;
    
    Serial.printf("📡 API: POST /api/control (device=%d, relay=%d, state=%d)\n", 
                 device_id, relay, state);
    
    // Validate device_id
    if (device_id == 0 || device_id > 247) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid device ID\"}");
        return;
    }
    
    // Check if device exists
    DeviceDisplayInfo* device = deviceManagerGet(device_id);
    if (!device) {
        request->send(404, "application/json", "{\"success\":false,\"message\":\"Device not found\"}");
        return;
    }
    
    // Check if device is online
    if (!device->online) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Device offline\"}");
        return;
    }
    
    // Check if device is in AUTO mode
    if (device->control_source != 0) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Device not in AUTO mode\"}");
        return;
    }
    
    // Send command
    // Find this section in webApiControlRelay():
    bool success = false;

    if (relay == 255) {
        // ALL relays - use single command (NO DELAY!)
        int relay_count = (device->profile == PROFILE_FAN_4CH) ? 4 : 8;
        
        SetRelayPayload payload;
        payload.relay_mask = (1 << relay_count) - 1;
        for (int i = 0; i < relay_count; i++) {
            payload.values[i] = state ? 1 : 0;
        }
        memset(&payload.values[relay_count], 0, 8 - relay_count);
        
        success = commandQueueAdd(device_id, CMD_SET_RELAY, 
                                (uint8_t*)&payload, sizeof(SetRelayPayload));
        
    } else if (relay <= 7) {
        // Single relay
        success = sendRelayCommand(device_id, relay, state);
    } else {
        request->send(400, "application/json", 
                    "{\"success\":false,\"message\":\"Invalid relay index\"}");
        return;
    }

/**
 * API: GET /api/stats
 * Return system statistics
 */
void webApiGetStats(AsyncWebServerRequest* request) {
    Serial.println("📡 API: GET /api/stats");
    
    // Build JSON
    DynamicJsonDocument doc(512);
    
    doc["uptime_seconds"] = g_stats.uptime_seconds;
    doc["commands_sent"] = g_stats.commands_sent;
    doc["commands_acked"] = g_stats.commands_acked;
    doc["commands_failed"] = g_stats.commands_failed;
    doc["frames_received"] = g_stats.frames_received;
    doc["frames_invalid"] = g_stats.frames_invalid;
    doc["ui_updates"] = g_stats.ui_updates;
    
    // Device counts
    doc["total_devices"] = g_device_count;
    doc["online_devices"] = deviceManagerGetOnlineCount();
    doc["offline_devices"] = g_device_count - deviceManagerGetOnlineCount();
    doc["manual_mode_count"] = deviceManagerGetManualModeCount();
    doc["failsafe_count"] = deviceManagerGetFailSafeCount();
    
    // Profile breakdown
    doc["kipas_count"] = deviceManagerGetCountByProfile(PROFILE_FAN_4CH);
    doc["lampu_count"] = deviceManagerGetCountByProfile(PROFILE_LIGHT_8CH);
    doc["sound_count"] = deviceManagerGetCountByProfile(PROFILE_SOUND_8CH);
    
    // Uptime string
    char uptime_str[32];
    getUptimeString(uptime_str, sizeof(uptime_str));
    doc["uptime_string"] = uptime_str;
    
    // WiFi info
    doc["wifi_ssid"] = WIFI_AP_SSID;
    doc["wifi_ip"] = WiFi.softAPIP().toString();
    doc["wifi_clients"] = WiFi.softAPgetStationNum();
    
    // Serialize to string
    char json_buffer[768];
    size_t len = serializeJson(doc, json_buffer, sizeof(json_buffer));
    
    request->send(200, "application/json", json_buffer);
}

// ============================================================================
// WEBSOCKET BROADCAST (Optional - untuk real-time updates)
// ============================================================================

/**
 * Send WebSocket update
 * Broadcast status update ke semua connected clients
 * 
 * Note: Ini optional feature, perlu AsyncWebSocket library
 * Untuk saat ini pakai polling dari client side (setInterval)
 */
void webSocketBroadcast(const char* json_data) {
    // TODO: Implement WebSocket jika perlu real-time push
    // Untuk sekarang, client side pakai polling setiap 5 detik
    (void)json_data;  // Unused
}
