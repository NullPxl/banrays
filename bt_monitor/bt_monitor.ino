#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ========== CONFIGURATION ==========
const int RSSI_THRESHOLD = -150;        // Only show devices stronger than this
const int DETECTION_PIN = 13;
const int SCAN_TIME = 5;               // Scan duration in seconds

// Meta and Luxottica company identifiers
const uint16_t META_IDENTIFIERS[] = {
  0xFD5F,  // Meta (0xFD5F)
  0xFEB7,  // Meta (0xFEB7)
  0xFEB8,  // Meta (0xFEB8)
  0x01AB,  // Meta (0x01AB)
  0x058E,  // Meta (0x058E)
  0x0D53   // Luxottica (0x0D53)
};
const int META_ID_COUNT = 6;

// MAC addresses are randomized, so not checking OUI here (maybe do it for BTC)
    // mac.rfind("48:05:60", 0) == 0 ||
    // mac.rfind("CC:A1:74", 0) == 0 ||
    // mac.rfind("C0:DD:8A", 0) == 0 ||
    // mac.rfind("D0:B3:C2", 0) == 0 ||
    // mac.rfind("88:25:08", 0) == 0 ||
    // mac.rfind("94:F9:29", 0) == 0 ||
    // mac.rfind("D4:D6:59", 0) == 0 ||
    // mac.rfind("78:C4:FA", 0) == 0 ||
    // mac.rfind("B4:17:A8", 0) == 0 ||
    // mac.rfind("50:99:03", 0) == 0 ||
    // mac.rfind("80:F3:EF", 0) == 0 ||
    // mac.rfind("84:57:F7", 0) == 0 ||
    // mac.rfind("3C:28:6D", 0) == 0 ||
    // mac.rfind("AC:37:43", 0) == 0 ||
    // // Luxottica
    // mac.rfind("98:59:49", 0) == 0

// Identifiers to filter out (from local test setup)
const uint16_t BLOCKED_IDENTIFIERS[] = {
  0xFD5A,  // Samsung
  0xFD69,   // Samsung
  0x004C, //apple
  0x0006, // microsoft
  0xFEF3, // phone
};
const int BLOCKED_ID_COUNT = 5;

// ========== GLOBALS ==========
BLEScan* pBLEScan;
unsigned long sessionStartTime = 0;

// ========== DETECTION RESULT STRUCT ==========
struct DetectionResult {
  bool isBlocked;
  bool isMetaDevice;
  String matches[10];  // Store up to 10 match descriptions
  int matchCount;
};

// ========== HELPER FUNCTIONS ==========

String getCompanyName(uint16_t companyId) {
  switch(companyId) {
    case 0xFD5F: return "Meta (0xFD5F)";
    case 0xFEB7: return "Meta (0xFEB7)";
    case 0xFEB8: return "Meta (0xFEB8)";
    case 0x01AB: return "Meta (0x01AB)";
    case 0x058E: return "Meta (0x058E)";
    case 0x0D53: return "Luxottica (0x0D53)";
    default: {
      char buf[32];
      sprintf(buf, "Unknown (0x%04X)", companyId);
      return String(buf);
    }
  }
}

bool isMetaIdentifier(uint16_t id) {
  for(int i = 0; i < META_ID_COUNT; i++) {
    if(META_IDENTIFIERS[i] == id) return true;
  }
  return false;
}

bool isBlockedIdentifier(uint16_t id) {
  for(int i = 0; i < BLOCKED_ID_COUNT; i++) {
    if(BLOCKED_IDENTIFIERS[i] == id) return true;
  }
  return false;
}

String formatTimestamp() {
  unsigned long ms = millis() - sessionStartTime;
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  
  char buf[16];
  sprintf(buf, "%02lu:%02lu:%02lu", hours % 24, minutes % 60, seconds % 60);
  return String(buf);
}

String hexString(uint8_t* data, size_t len) {
  String result = "";
  for(size_t i = 0; i < len; i++) {
    char buf[4];
    sprintf(buf, "%02x", data[i]);
    result += buf;
  }
  return result;
}

// Extract 16-bit identifier from 128-bit UUID (format: 0000XXXX-0000-1000-8000-00805f9b34fb)
uint16_t extract16BitFromUUID(String uuid) {
  if(uuid.length() == 36) {
    // Extract characters at positions 4-7 (the XXXX part)
    String hex = uuid.substring(4, 8);
    return (uint16_t)strtol(hex.c_str(), NULL, 16);
  }
  return 0;
}

// ========== DETECTION LOGIC ==========

DetectionResult checkDevice(BLEAdvertisedDevice& device) {
  DetectionResult result = {false, false, {}, 0};
  
  // Check manufacturer data
  if(device.haveManufacturerData()) {
    std::string mfgData = device.getManufacturerData();
    if(mfgData.length() >= 2) {
      // Little-endian: first byte is LSB, second is MSB
      uint16_t companyId = ((uint8_t)mfgData[1] << 8) | (uint8_t)mfgData[0];
      
      if(isBlockedIdentifier(companyId)) {
        result.isBlocked = true;
        return result;
      }
      
      if(isMetaIdentifier(companyId)) {
        result.isMetaDevice = true;
        result.matches[result.matchCount++] = "Manufacturer: " + getCompanyName(companyId);
      }
    }
  }
  
  // Check service UUIDs
  if(device.haveServiceUUID()) {
    BLEUUID serviceUUID = device.getServiceUUID();
    String uuidStr = String(serviceUUID.toString().c_str());
    uuidStr.toLowerCase();
    
    uint16_t identifier = extract16BitFromUUID(uuidStr);
    if(identifier != 0) {
      if(isBlockedIdentifier(identifier)) {
        result.isBlocked = true;
        return result;
      }
      
      if(isMetaIdentifier(identifier)) {
        result.isMetaDevice = true;
        result.matches[result.matchCount++] = "Service UUID: " + getCompanyName(identifier) + " (" + uuidStr + ")";
      }
    }
  }
  
  return result;
}

// ========== BLE CALLBACK ==========

class MetaDetectionCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice device) {
    // Filter by RSSI
    if(device.getRSSI() < RSSI_THRESHOLD) return;
    
    // Check for Meta/Luxottica identifiers
    DetectionResult detection = checkDevice(device);
    
    // Skip blocked devices
    if(detection.isBlocked) return;
    
    // Build output
    String output = "\n";
    output += "======================================================================\n";
    output += "[" + formatTimestamp() + "] RSSI: " + String(device.getRSSI()) + " dBm\n";
    output += "Address: " + String(device.getAddress().toString().c_str()) + "\n";
    output += "Name: " + String(device.haveName() ? device.getName().c_str() : "Unknown") + "\n";
    
    // Highlight Meta/Luxottica devices
    if(detection.isMetaDevice) {
      output += "\n🔍 META/LUXOTTICA DEVICE DETECTED!\n";
      for(int i = 0; i < detection.matchCount; i++) {
        output += "  ✓ " + detection.matches[i] + "\n";
      }
      
      // Set detection pin HIGH
      digitalWrite(DETECTION_PIN, HIGH);
    }
    
    // Print manufacturer data
    if(device.haveManufacturerData()) {
      std::string mfgData = device.getManufacturerData();
      output += "\nManufacturer Data:\n";
      
      if(mfgData.length() >= 2) {
        uint16_t companyId = ((uint8_t)mfgData[1] << 8) | (uint8_t)mfgData[0];
        output += "  Company ID: " + getCompanyName(companyId) + "\n";
      }
      
      output += "  Data: " + hexString((uint8_t*)mfgData.c_str(), mfgData.length()) + "\n";
    }
    
    // Print service UUIDs
    if(device.haveServiceUUID()) {
      output += "\nService UUIDs: ['" + String(device.getServiceUUID().toString().c_str()) + "']\n";
    }
    
    // Print service data if available
    if(device.haveServiceData()) {
      std::string svcData = device.getServiceData();
      BLEUUID svcUUID = device.getServiceDataUUID();
      output += "\nService Data:\n";
      output += "  UUID: " + String(svcUUID.toString().c_str()) + "\n";
      output += "  Data: " + hexString((uint8_t*)svcData.c_str(), svcData.length()) + "\n";
    }
    
    // Print local name if different from advertised name
    if(device.haveName()) {
      output += "\nLocal Name: " + String(device.getName().c_str()) + "\n";
    }
    
    output += "======================================================================\n";
    
    Serial.print(output);
  }
};

MetaDetectionCallbacks callbacks;

// ========== SETUP ==========

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Configure detection pin
  pinMode(DETECTION_PIN, OUTPUT);
  digitalWrite(DETECTION_PIN, LOW);
  
  sessionStartTime = millis();
  
  Serial.println("\n======================================================================");
  Serial.println("BLE Scanner - Detecting Meta Ray-Ban Smart Glasses");
  Serial.println("======================================================================");
  Serial.print("RSSI Threshold: ");
  Serial.print(RSSI_THRESHOLD);
  Serial.println(" dBm (showing devices stronger than this)");
  
  Serial.print("Monitoring for Meta identifiers: ");
  for(int i = 0; i < META_ID_COUNT; i++) {
    Serial.printf("0x%04X", META_IDENTIFIERS[i]);
    if(i < META_ID_COUNT - 1) Serial.print(", ");
  }
  Serial.println();
  
  Serial.print("Blocking: ");
  for(int i = 0; i < BLOCKED_ID_COUNT; i++) {
    Serial.printf("0x%04X", BLOCKED_IDENTIFIERS[i]);
    if(i < BLOCKED_ID_COUNT - 1) Serial.print(", ");
  }
  Serial.println(" (Samsung)");
  
  Serial.print("Detection Pin: GPIO ");
  Serial.println(DETECTION_PIN);
  
  Serial.println("\n======================================================================");
  Serial.print("NEW SCAN SESSION - ");
  Serial.println(formatTimestamp());
  Serial.println("======================================================================");
  Serial.println("\nScanning...\n");
  
  // Initialize BLE
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(&callbacks);
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

// ========== LOOP ==========

void loop() {
  // Reset detection pin at start of each scan
  digitalWrite(DETECTION_PIN, LOW);
  
  // Scan for devices
  BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME, false);
  pBLEScan->clearResults();
  
  delay(50);
}
