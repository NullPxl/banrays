#include <bluefruit.h>

/************************************************************
   nRF52840  BLE SCANNER (CHATGPT'D. NEED TO TEST MORE. PROBABLY JUST NEED TO UES NORDIC'S SDK INSTEAD)
************************************************************/

// Example identifiers (same idea as your ESP32 code)
const uint16_t META_IDS[] = {0xFD5F, 0xFEB7, 0xFEB8, 0x01AB, 0x058E, 0x0D53};
const int META_ID_COUNT = sizeof(META_IDS) / sizeof(META_IDS[0]);

/************************************************************
   Utility: Print hex bytes
************************************************************/
String hexString(const uint8_t* data, uint16_t len) {
  String out = "";
  for (uint16_t i = 0; i < len; i++) {
    char buf[4];
    sprintf(buf, "%02X", data[i]);
    out += buf;
  }
  return out;
}

/************************************************************
   Check if manufacturer ID is one of our targets
************************************************************/
bool isTargetManufacturer(uint16_t companyId) {
  for (int i = 0; i < META_ID_COUNT; i++) {
    if (META_IDS[i] == companyId) return true;
  }
  return false;
}

/************************************************************
   DEVICE CLASSIFICATION LOGIC
   Expand this to implement your Meta/Luxottica logic
************************************************************/
bool detectDevice(ble_gap_evt_adv_report_t* report) {
  uint8_t buffer[64];

  // ------------ Check Manufacturer Data ------------
  int len = Bluefruit.Scanner.parseReportByType(
      report, BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
      buffer, sizeof(buffer));

  if (len >= 2) {
    uint16_t companyId = buffer[1] << 8 | buffer[0];

    if (isTargetManufacturer(companyId)) {
      Serial.printf("\n⚡ TARGET MANUFACTURER DETECTED: 0x%04X\n", companyId);
      return true;
    }
  }

  // ------------ Check Service UUIDs ------------
  len = Bluefruit.Scanner.parseReportByType(
      report, BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE,
      buffer, sizeof(buffer));

  for (int i = 0; i < len; i += 2) {
    uint16_t uuid16 = buffer[i] | (buffer[i+1] << 8);
    if (isTargetManufacturer(uuid16)) {
      Serial.printf("\n⚡ TARGET SERVICE UUID FOUND: 0x%04X\n", uuid16);
      return true;
    }
  }

  return false;  
}

/************************************************************
   Callback: Fired for every received advertisement
************************************************************/
void scan_callback(ble_gap_evt_adv_report_t* report) {

  Serial.println("\n=======================================================");
  Serial.printf("RSSI: %d dBm\n", report->rssi);

  // ---------------------- PHY INFORMATION ----------------------
  Serial.printf("Primary PHY:   %s\n",
    report->primary_phy == BLE_GAP_PHY_1MBPS ? "1M" :
    report->primary_phy == BLE_GAP_PHY_CODED ? "Coded" : "Unknown");

  Serial.printf("Secondary PHY: %s\n",
    report->secondary_phy == BLE_GAP_PHY_1MBPS ? "1M" :
    report->secondary_phy == BLE_GAP_PHY_2MBPS ? "2M" :
    report->secondary_phy == BLE_GAP_PHY_CODED ? "Coded" : "None");

  // ---------------------- ADDRESS INFO -------------------------
  Serial.print("Address: ");
  Serial.printBufferReverse(report->peer_addr.addr, 6, ':');
  Serial.println(report->peer_addr.addr_type ? " (Random)" : " (Public)");

  // ---------------------- ADV TYPE ------------------------------
  Serial.printf("Connectable: %s\n", report->type.connectable ? "Yes" : "No");
  Serial.printf("Scannable:   %s\n", report->type.scannable ? "Yes" : "No");
  Serial.printf("Directed:    %s\n", report->type.directed ? "Yes" : "No");
  Serial.printf("Scan Resp:   %s\n", report->type.scan_response ? "Yes" : "No");
  Serial.printf("Extended:    %s\n", report->type.extended_pdu ? "Yes" : "No");

  // ---------------------- RAW PAYLOAD ---------------------------
  Serial.printf("Payload (%d bytes): ", report->data.len);
  Serial.println(hexString(report->data.p_data, report->data.len));

  // ---------------------- MANUFACTURER DATA ---------------------
  uint8_t msd[32];
  int msd_len = Bluefruit.Scanner.parseReportByType(
      report, BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA,
      msd, sizeof(msd));

  if (msd_len >= 2) {
    uint16_t companyId = msd[1] << 8 | msd[0];
    Serial.printf("Manufacturer ID: 0x%04X\n", companyId);
    Serial.printf("Manufacturer Data: %s\n", hexString(msd, msd_len).c_str());
  }

  // ---------------------- SERVICE UUIDS -------------------------
  uint8_t buffer[64];
  int len = Bluefruit.Scanner.parseReportByType(
      report, BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE,
      buffer, sizeof(buffer));

  if (len > 0) {
    Serial.print("128-bit UUIDs: ");
    Serial.print(hexString(buffer, len));
    Serial.println();
  }

  // ---------------------- DETECTION LOGIC -----------------------
  if (detectDevice(report)) {
    Serial.println("🎯 MATCH FOUND — DEVICE IS A TARGET");
  }

  // ---------------------- Resume Scanning ------------------------
  Bluefruit.Scanner.resume();
}

/************************************************************
   SETUP
************************************************************/
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("nRF52840 Advanced BLE Scanner");
  Serial.println("=======================================================\n");

  // Central only
  Bluefruit.begin(0, 1);
  Bluefruit.setTxPower(8);

  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.useActiveScan(true);
  Bluefruit.Scanner.setInterval(160, 80);

  // REMOVE setPhy() — not available in Adafruit API
  // Bluefruit.Scanner.setPhy(...);

  Bluefruit.Scanner.start(0);  // infinite scanning
}


/************************************************************
   LOOP
************************************************************/
void loop() {
  // Nothing needed — scanning is interrupt-driven
}
