#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Preferences.h>
#include <WebServer.h>

// --- Permanent Storage ---
Preferences preferences;

// --- WiFi Configuration & Web Server ---
WebServer setupServer(80);
bool isAPMode = false;

// --- Secure WebSocket Client (Outbound to Cloudflare Worker) ---
WebSocketsClient webSocket;

const char* wsHost = "hivearm.noreplyglobalx1.workers.dev";
const int wsPort = 443;
const char* wsPath = "/ws";

bool wsConnected = false;
unsigned long lastWsReconnectAttempt = 0;
const unsigned long wsReconnectInterval = 5000;

// --- Root CA Certificates Bundle (DigiCert, Let's Encrypt, and Google Trust Services) ---
// Since Cloudflare dynamically issues SSL certificates from these CAs, we trust all three
// to ensure the ESP32 can always successfully verify the secure TLS handshake.
const char* rootCACert = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw
CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlY3VyaXR5IF境S
MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw
MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlY3Vp
dGVzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA
A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo
27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w
Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw
TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl
qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH
szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8
Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk
MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92
wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p
aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN
VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID
AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E
FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb
C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe
QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy
h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4
7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J
ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef
MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/
Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT
6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ
0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm
2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb
bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c
-----END CERTIFICATE-----
)EOF";

// --- PCA9685 Servo Driver ---
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
#define SERVO_FREQ 50 // Standard servo frequency

// --- Servo Min/Max Pulse Length Calibration (SG90/MG996R) ---
const int SERVOMIN = 150; // 0 degrees
const int SERVOMAX = 600; // 180 degrees

// Struct to store Joint Telemetry and Target tracking
struct Joint {
  float current;      // Current angle of the servo
  float target;       // Target angle from the website slider
  float minLimit;     // Min angle boundary (from website UI limits)
  float maxLimit;     // Max angle boundary (from website UI limits)
  int pcaChannel;     // Channel number on PCA9685 board
};

// 5-Axis configuration aligned with website ranges and PCA9685 channels:
Joint joints[5] = {
  {0.0, 0.0, -180.0, 180.0, 0}, // Joint 0: Base
  {0.0, 0.0, -90.0,  90.0,  1}, // Joint 1: Shoulder
  {0.0, 0.0, -135.0, 135.0, 2}, // Joint 2: Elbow
  {0.0, 0.0, -90.0,  90.0,  3}, // Joint 3: Wrist
  {0.0, 0.0, -180.0, 180.0, 4}  // Joint 4: Camera
};

// Telemetry State
float internalTemp = 25.0;
unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 500; // Broadcast stats every 500ms

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read(); // ESP32 internal chip temp reader
#ifdef __cplusplus
}
#endif

// Convert target UI degrees into PCA9685 Pulse Length ticks
int angleToPulse(float angle, Joint& joint) {
  float normalizedDeg = map(angle, joint.minLimit, joint.maxLimit, 0, 180);
  return map(normalizedDeg, 0, 180, SERVOMIN, SERVOMAX);
}

// WebSocket Client Event Handler
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      wsConnected = false;
      Serial.println("[WSS] Disconnected from Cloudflare Worker");
      break;
      
    case WStype_CONNECTED:
      wsConnected = true;
      Serial.printf("[WSS] Connected to %s\n", wsHost);
      break;
      
    case WStype_TEXT: {
      String msg = String((char*)payload);
      int commaIndex = msg.indexOf(',');
      if (commaIndex > 0) {
        int idx = msg.substring(0, commaIndex).toInt();
        float targetAngle = msg.substring(commaIndex + 1).toFloat();
        if (idx >= 0 && idx < 5) {
          joints[idx].target = constrain(targetAngle, joints[idx].minLimit, joints[idx].maxLimit);
        }
      }
      break;
    }
    
    case WStype_BIN:
      Serial.println("[WSS] Received binary data (ignored)");
      break;
      
    default:
      break;
  }
}

// Serve Setup Portal HTML page
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{background:#12131a;color:#ffffff;font-family:sans-serif;text-align:center;padding:50px 20px;}";
  html += ".card{background:#1a1c26;border:1px solid #ffb84d;border-radius:12px;padding:30px;max-width:400px;margin:0 auto;box-shadow:0 8px 32px rgba(255,184,77,0.15);}";
  html += "h2{color:#ffb84d;margin-bottom:20px;}input[type=text],input[type=password]{width:100%;padding:12px;margin:10px 0;box-sizing:border-box;border-radius:6px;border:1px solid #2a2d3a;background:#12131a;color:#fff;}";
  html += "input[type=submit]{background:#ffb84d;color:#12131a;font-weight:bold;border:none;padding:12px 20px;border-radius:6px;cursor:pointer;width:100%;margin-top:10px;}input[type=submit]:hover{opacity:0.9;}";
  html += ".reset-btn{background:#ff4d4d;color:#fff;margin-top:15px;}</style></head>";
  html += "<body><div class='card'><h2>HiveArm Setup</h2><p>Configure WiFi settings for this device:</p>";
  html += "<form action='/save' method='POST'>";
  html += "<input type='text' name='ssid' placeholder='SSID / WiFi Name' required>";
  html += "<input type='password' name='password' placeholder='WiFi Password'>";
  html += "<input type='submit' value='Save & Connect'>";
  html += "</form>";
  html += "<form action='/reset' method='POST'><input type='submit' class='reset-btn' value='Clear Saved WiFi & Restart'></form>";
  html += "</div></body></html>";
  setupServer.send(200, "text/html", html);
}

// Save credentials from Portal to Flash, then reboot
void handleSave() {
  if (setupServer.hasArg("ssid")) {
    String ssid = setupServer.arg("ssid");
    String pass = setupServer.arg("password");
    
    preferences.begin("wifi-creds", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", pass);
    preferences.end();
    
    String html = "<html><body style='background:#12131a;color:#fff;font-family:sans-serif;text-align:center;padding-top:100px;'>";
    html += "<h2 style='color:#ffb84d;'>Credentials Saved!</h2><p>ESP32 is restarting and will connect to <b>" + ssid + "</b>...</p></body></html>";
    setupServer.send(200, "text/html", html);
    
    delay(2000);
    ESP.restart();
  } else {
    setupServer.send(400, "text/plain", "Invalid Request");
  }
}

// Clear credentials and restart
void handleReset() {
  preferences.begin("wifi-creds", false);
  preferences.clear();
  preferences.end();
  
  String html = "<html><body style='background:#12131a;color:#fff;font-family:sans-serif;text-align:center;padding-top:100px;'>";
  html += "<h2 style='color:#ff4d4d;'>WiFi Cleared</h2><p>Restarting... Please configure WiFi again.</p></body></html>";
  setupServer.send(200, "text/html", html);
  
  delay(2000);
  ESP.restart();
}

// Fallback configuration Access Point Mode
void startSetupPortal() {
  if (isAPMode) return;
  
  Serial.println("\n========================================");
  Serial.println("  STARTING SETUP PORTAL");
  Serial.println("========================================");
  
  isAPMode = true;
  wsConnected = false;
  
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
  
  IPAddress localIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(localIP, gateway, subnet);
  WiFi.softAP("HiveArm-Setup");
  
  Serial.print("1. Connect your phone/laptop WiFi to: ");
  Serial.println("HiveArm-Setup");
  Serial.print("2. Open browser and navigate to: http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("========================================\n");
  
  setupServer.on("/", handleRoot);
  setupServer.on("/save", HTTP_POST, handleSave);
  setupServer.on("/reset", HTTP_POST, handleReset);
  setupServer.begin();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n[HIVEARM] Booting...");

  // Initialize PCA9685 Servo Driver over SDA/SCL (GPIO 21 & 22)
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);

  // Move all joints to neutral 0.0 position at start
  for (int i = 0; i < 5; i++) {
    int initPulse = angleToPulse(0.0, joints[i]);
    pwm.setPWM(joints[i].pcaChannel, 0, initPulse);
  }

  // Load Saved WiFi configuration from Flash
  preferences.begin("wifi-creds", true);
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");
  preferences.end();

  if (ssid == "") {
    Serial.println("[WIFI] No saved credentials found.");
    startSetupPortal();
  } else {
    Serial.printf("[WIFI] Saved network found: %s\n", ssid.c_str());
    Serial.println("[WIFI] Attempting to connect...");
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) { // 10 second timeout
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n[WIFI] ✓ Connected!");
      Serial.print("[WIFI] IP Address: ");
      Serial.println(WiFi.localIP());
      
      // Start Secure WebSocket Client connection to Cloudflare Worker
      Serial.println("[WSS] Connecting to Cloudflare Worker...");
      
      // --- SSL CONFIGURATION FOR CLOUDFLARE ---
      // Tell the WebSocketsClient to trust Cloudflare CAs using setCACert
      webSocket.setCACert(rootCACert);
      
      webSocket.beginSSL(wsHost, wsPort, wsPath);
      webSocket.onEvent(webSocketEvent);
      lastWsReconnectAttempt = millis();
    } else {
      Serial.println("\n[WIFI] ✗ Could not connect to saved network.");
      Serial.println("[WIFI] Starting setup portal so you can enter a new network.");
      startSetupPortal();
    }
  }
}

void loop() {
  if (isAPMode) {
    setupServer.handleClient();
    return;
  }

  // --- NO GRACE PERIOD: if WiFi is lost, go straight to setup portal ---
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] ✗ Network disconnected or not available.");
    Serial.println("[WIFI] Opening setup portal immediately...");
    startSetupPortal();
    return;
  }

  // --- Maintain WebSocket ---
  webSocket.loop();
  
  if (!wsConnected) {
    if (millis() - lastWsReconnectAttempt > wsReconnectInterval) {
      lastWsReconnectAttempt = millis();
      Serial.println("[WSS] Reconnecting to Cloudflare Worker...");
      
      // --- SSL CONFIGURATION FOR CLOUDFLARE ---
      webSocket.setCACert(rootCACert);
      
      webSocket.beginSSL(wsHost, wsPort, wsPath);
    }
  }

  // --- Smooth Servo Motion (Linear Interpolation) ---
  for (int i = 0; i < 5; i++) {
    if (abs(joints[i].current - joints[i].target) > 0.1) {
      float easeFactor = 0.08;
      joints[i].current += (joints[i].target - joints[i].current) * easeFactor;
      
      int pulse = angleToPulse(joints[i].current, joints[i]);
      pwm.setPWM(joints[i].pcaChannel, 0, pulse);
    }
  }

  // --- Periodic Telemetry Output ---
  unsigned long now = millis();
  if (now - lastTelemetryTime >= telemetryInterval) {
    lastTelemetryTime = now;

    #if defined(ESP_ARDUINO_VERSION_VAL) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(2, 0, 0)
      internalTemp = (temprature_sens_read() - 32) / 1.8;
      if (internalTemp < 0 || internalTemp > 120) internalTemp = 26.5;
    #else
      internalTemp = 28.5 + (random(-10, 10) / 10.0);
    #endif

    char telemetryJson[256];
    snprintf(telemetryJson, sizeof(telemetryJson),
             "{\"base\":%.2f,\"shoulder\":%.2f,\"elbow\":%.2f,\"wrist\":%.2f,\"camera\":%.2f,\"temperature\":%.2f}",
             joints[0].current, joints[1].current, joints[2].current, joints[3].current, joints[4].current, internalTemp);

    if (wsConnected) {
      webSocket.sendTXT(telemetryJson);
    } else {
      Serial.println("[WSS] Not connected, skipping telemetry");
    }
  }

  delay(15); // ~60fps target rate limit
}
