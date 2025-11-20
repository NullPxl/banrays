#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

int scanTime = 5;
BLEScan* pBLEScan;

bool metaFound = false;
unsigned long firstDetectionTime = 0;

// Uppercase helper
void toUpper(std::string &s) {
  for (auto &c : s) c = toupper(c);
}

// ---- MANUFACTURER ID DETECTION (BEST SIGNAL) ----
bool isMetaOrLuxotticaManufacturer(BLEAdvertisedDevice& dev) {
  if (!dev.haveManufacturerData()) return false;

  std::string m = dev.getManufacturerData();
  if (m.length() < 2) return false;

  // BLE manufacturer data is little-endian
  uint16_t company = ((uint8_t)m[1] << 8) | (uint8_t)m[0];

  return (
    company == 0x01AB ||   // Meta Platforms, Inc.
    company == 0x058E ||   // Meta Platforms Technologies, LLC
    company == 0x0D53      // Luxottica Group S.p.A.
  );
}

// ---- OUI (MAC PREFIX) DETECTION ----
bool isMetaOrLuxotticaOUI(BLEAdvertisedDevice& dev) {
  std::string mac = dev.getAddress().toString();
  toUpper(mac);

  return (
    // Meta/Facebook OUIs
    mac.rfind("48:05:60", 0) == 0 ||
    mac.rfind("CC:A1:74", 0) == 0 ||
    mac.rfind("C0:DD:8A", 0) == 0 ||
    mac.rfind("D0:B3:C2", 0) == 0 ||
    mac.rfind("88:25:08", 0) == 0 ||
    mac.rfind("94:F9:29", 0) == 0 ||
    mac.rfind("D4:D6:59", 0) == 0 ||
    mac.rfind("78:C4:FA", 0) == 0 ||
    mac.rfind("B4:17:A8", 0) == 0 ||
    mac.rfind("50:99:03", 0) == 0 ||
    mac.rfind("80:F3:EF", 0) == 0 ||
    mac.rfind("84:57:F7", 0) == 0 ||
    mac.rfind("3C:28:6D", 0) == 0 ||  // Meta Platforms
    mac.rfind("AC:37:43", 0) == 0 ||  // Facebook/Meta
    // Luxottica
    mac.rfind("98:59:49", 0) == 0
  );
}

// ---- CALLBACK ----
class PrintAllCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice dev) {

    // Meta/Luxottica DETECTION
    bool mfgMatch = isMetaOrLuxotticaManufacturer(dev);
    bool ouiMatch = isMetaOrLuxotticaOUI(dev);
    
    if (mfgMatch || ouiMatch) {
      if (!metaFound) {
        Serial.println("\n!!! META / LUXOTTICA DEVICE DETECTED !!!");
        Serial.print("    Detection Method: ");
        if (mfgMatch) Serial.print("MANUFACTURER ID ");
        if (mfgMatch && ouiMatch) Serial.print("+ ");
        if (ouiMatch) Serial.print("MAC OUI");
        Serial.println();
        
        Serial.print("    Device: ");
        Serial.println(dev.haveName() ? dev.getName().c_str() : "(unnamed)");
        Serial.print("    MAC: ");
        Serial.println(dev.getAddress().toString().c_str());
        Serial.print("    RSSI: ");
        Serial.println(dev.getRSSI());
        
        // Show manufacturer data if it was the trigger
        if (mfgMatch && dev.haveManufacturerData()) {
          std::string mfg = dev.getManufacturerData();
          Serial.print("    Mfg Data: ");
          for (size_t i = 0; i < mfg.length(); i++) {
            Serial.printf("%02X ", (uint8_t)mfg[i]);
          }
          Serial.println();
          
          if (mfg.length() >= 2) {
            uint16_t company = ((uint8_t)mfg[1] << 8) | (uint8_t)mfg[0];
            Serial.printf("    Company ID: 0x%04X", company);
            if (company == 0x01AB) Serial.print(" (Meta Platforms, Inc.)");
            else if (company == 0x058E) Serial.print(" (Meta Platforms Technologies, LLC)");
            else if (company == 0x0D53) Serial.print(" (Luxottica Group S.p.A.)");
            Serial.println();
          }
        }
        
        Serial.println("    => MUTING OTHER DEVICE OUTPUT\n");
        
        metaFound = true;
        firstDetectionTime = millis();
      }
      return;
    }

    if (metaFound) return;

    // ---- PRINT EVERYTHING ----
    Serial.println("----- BLE DEVICE FOUND -----");

    // Name
    Serial.print("Name: ");
    if (dev.haveName()) Serial.println(dev.getName().c_str());
    else Serial.println("(no name)");

    // Device
    Serial.print("Device: ");
    Serial.println(dev.haveName() ? dev.getName().c_str() : "(unnamed)");

    // MAC
    Serial.print("MAC: ");
    Serial.println(dev.getAddress().toString().c_str());

    // RSSI
    Serial.print("RSSI: ");
    Serial.println(dev.getRSSI());

    // TX Power
    if (dev.haveTXPower()) {
      Serial.print("TX Power: ");
      Serial.println(dev.getTXPower());
    }

    // Service UUID
    if (dev.haveServiceUUID()) {
      Serial.print("Service UUID: ");
      Serial.println(dev.getServiceUUID().toString().c_str());
    }

    // Service Data
    if (dev.haveServiceData()) {
      Serial.print("Service Data UUID: ");
      Serial.println(dev.getServiceDataUUID().toString().c_str());

      std::string sdata = dev.getServiceData();
      Serial.print("Service Data: ");
      for (size_t i = 0; i < sdata.length(); i++) {
        Serial.printf("%02X ", (uint8_t)sdata[i]);
      }
      Serial.println();
    }

    // Manufacturer Data
    if (dev.haveManufacturerData()) {
      std::string mfg = dev.getManufacturerData();
      Serial.print("Manufacturer Data: ");
      for (size_t i = 0; i < mfg.length(); i++) {
        Serial.printf("%02X ", (uint8_t)mfg[i]);
      }
      Serial.println();
      
      // Decode company ID for debugging
      if (mfg.length() >= 2) {
        uint16_t company = ((uint8_t)mfg[1] << 8) | (uint8_t)mfg[0];
        Serial.printf("  (Company ID: 0x%04X)\n", company);
      }
    }

    // Raw payload
    uint8_t* payload = dev.getPayload();
    int len = dev.getPayloadLength();

    Serial.print("Raw Payload: ");
    if (payload && len > 0) {
      for (int i = 0; i < len; i++) Serial.printf("%02X ", payload[i]);
    } else {
      Serial.print("(none)");
    }
    Serial.println();

    Serial.println("-----------------------------------\n");
  }
};

// Global callback instance
PrintAllCallbacks callbacks;

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give serial time to initialize
  
  Serial.println("\n===========================================");
  Serial.println("Meta Ray-Ban BLE Scanner");
  Serial.println("===========================================");
  Serial.println("Scanning for ALL BLE devices...");
  Serial.println("Will mute output if Meta/Luxottica device detected\n");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(&callbacks);

  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
  pBLEScan->start(scanTime, false);
  pBLEScan->clearResults();
  
  // Optional: Show status if Meta device detected
  if (metaFound && (millis() - firstDetectionTime) % 10000 < 100) {
    Serial.println("[Status: Meta device detected - output muted]");
  }
  
  delay(50);
}