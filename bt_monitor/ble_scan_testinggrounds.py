import asyncio
from bleak import BleakScanner
from datetime import datetime
import os

# for testing

# Log file path
LOG_FILE = "rayban_ble_log.txt"

# org assigned numbers (https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Assigned_Numbers/out/en/Assigned_Numbers.pdf)
# Meta:
# 0xFD5F
# 0xFEB7
# 0xFEB8
# 0x01AB
# 0x058E
# Luxottica:
# 0x0D53

# Meta and Luxottica company identifiers
META_IDENTIFIERS = {
    0xFD5F: "Meta (0xFD5F)",
    0xFEB7: "Meta (0xFEB7)",
    0xFEB8: "Meta (0xFEB8)", 
    0x01AB: "Meta (0x01AB)",
    0x058E: "Meta (0x058E)",
    0x0D53: "Luxottica (0x0D53)"
}

# Identifiers to filter out (Samsung and other noise)
BLOCKED_IDENTIFIERS = {
    0xFD5A,  # Samsung smarttag
    0xFD69,   # samsung earbuds
    0x004C,  # apple
    0xFEF3, # i think its my phone (maybe look at the data transfer tho)
    0x0006,
    0x0075
}

# RSSI threshold - adjust as needed (-60 = strong, -70 = medium, -80 = weak)
RSSI_THRESHOLD = -200

def check_meta_luxottica(device, adv_data):
    """Check if device has Meta or Luxottica identifiers"""
    matches = []
    is_blocked = False
    
    # Check manufacturer data (company ID is first 2 bytes in little-endian)
    if adv_data.manufacturer_data:
        for company_id in adv_data.manufacturer_data.keys():
            if company_id in BLOCKED_IDENTIFIERS:
                is_blocked = True
                break
            if company_id in META_IDENTIFIERS:
                matches.append(f"Manufacturer: {META_IDENTIFIERS[company_id]}")
    
    if is_blocked:
        return None  # Signal to skip this device
    
    # Check service UUIDs (16-bit identifiers embedded in 128-bit UUIDs)
    if adv_data.service_uuids:
        for uuid in adv_data.service_uuids:
            uuid_lower = uuid.lower()
            # Extract 16-bit identifier from 128-bit UUID (format: 0000XXXX-0000-1000-8000-00805f9b34fb)
            if len(uuid_lower) == 36:  # Full 128-bit UUID
                hex_part = uuid_lower[4:8]  # Extract middle 4 chars
                try:
                    identifier = int(hex_part, 16)
                    if identifier in BLOCKED_IDENTIFIERS:
                        return None
                    if identifier in META_IDENTIFIERS:
                        matches.append(f"Service UUID: {META_IDENTIFIERS[identifier]} ({uuid})")
                except ValueError:
                    pass
            # Check direct 16-bit UUIDs
            elif len(uuid_lower) == 4:
                try:
                    identifier = int(uuid_lower, 16)
                    if identifier in BLOCKED_IDENTIFIERS:
                        return None
                    if identifier in META_IDENTIFIERS:
                        matches.append(f"Service UUID: {META_IDENTIFIERS[identifier]} (0x{uuid_lower})")
                except ValueError:
                    pass
    
    # Check service data (keys are UUIDs)
    if adv_data.service_data:
        for uuid in adv_data.service_data.keys():
            uuid_lower = uuid.lower()
            if len(uuid_lower) == 36:
                hex_part = uuid_lower[4:8]
                try:
                    identifier = int(hex_part, 16)
                    if identifier in BLOCKED_IDENTIFIERS:
                        return None
                    if identifier in META_IDENTIFIERS:
                        matches.append(f"Service Data: {META_IDENTIFIERS[identifier]} ({uuid})")
                except ValueError:
                    pass
    
    return matches

async def detection_callback(device, advertisement_data):
    """Called when a BLE device is detected"""
    
    # Filter by RSSI
    if advertisement_data.rssi < RSSI_THRESHOLD:
        return
    
    # Check for Meta/Luxottica identifiers (returns None if blocked)
    meta_matches = check_meta_luxottica(device, advertisement_data)
    
    # Skip blocked devices (Samsung, etc.)
    if meta_matches is None:
        return
    
    # Print header
    timestamp = datetime.now().strftime("%H:%M:%S")
    output_lines = []
    output_lines.append(f"\n{'='*70}")
    output_lines.append(f"[{timestamp}] RSSI: {advertisement_data.rssi} dBm")
    output_lines.append(f"Address: {device.address}")
    output_lines.append(f"Name: {device.name or 'Unknown'}")
    
    # Highlight Meta/Luxottica devices
    if meta_matches:
        output_lines.append(f"\n🔍 META/LUXOTTICA DEVICE DETECTED!")
        for match in meta_matches:
            output_lines.append(f"  ✓ {match}")
    
    # Print all data
    if advertisement_data.manufacturer_data:
        output_lines.append(f"\nManufacturer Data:")
        for company_id, data in advertisement_data.manufacturer_data.items():
            company_name = META_IDENTIFIERS.get(company_id, f"Unknown (0x{company_id:04X})")
            output_lines.append(f"  Company ID: {company_name}")
            output_lines.append(f"  Data: {data.hex()}")
    
    if advertisement_data.service_uuids:
        output_lines.append(f"\nService UUIDs: {advertisement_data.service_uuids}")
    
    if advertisement_data.service_data:
        output_lines.append(f"\nService Data:")
        for uuid, data in advertisement_data.service_data.items():
            output_lines.append(f"  UUID: {uuid}")
            output_lines.append(f"  Data: {data.hex()}")
    
    if advertisement_data.local_name:
        output_lines.append(f"\nLocal Name: {advertisement_data.local_name}")
    
    output_lines.append(f"{'='*70}")
    
    # Print to console
    output_text = "\n".join(output_lines)
    print(output_text)
    
    # Write matches to log file if Meta/Luxottica device detected
    if meta_matches:
        try:
            with open(LOG_FILE, "a", encoding="utf-8") as f:
                f.write(output_text + "\n")
        except Exception as e:
            print(f"Error writing to log file: {e}")

async def continuous_scan():
    """Run continuous BLE scan"""
    print("="*70)
    print("BLE Scanner - Detecting Meta Ray-Ban Smart Glasses")
    print("="*70)
    print(f"RSSI Threshold: {RSSI_THRESHOLD} dBm (showing devices stronger than this)")
    print(f"Monitoring for Meta identifiers: {', '.join([f'0x{k:04X}' for k in META_IDENTIFIERS.keys()])}")
    print(f"Blocking: {', '.join([f'0x{k:04X}' for k in BLOCKED_IDENTIFIERS])} (Samsung)")
    print(f"Log file: {LOG_FILE}")
    print("\nScanning... (Ctrl+C to stop)\n")
    
    # Create/clear log file with header
    try:
        with open(LOG_FILE, "a", encoding="utf-8") as f:
            f.write(f"\n{'='*70}\n")
            f.write(f"NEW SCAN SESSION - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write(f"{'='*70}\n")
    except Exception as e:
        print(f"Warning: Could not write to log file: {e}\n")
    
    scanner = BleakScanner(detection_callback)
    await scanner.start()
    
    try:
        while True:
            await asyncio.sleep(1.0)
    except KeyboardInterrupt:
        print("\n\nStopping scan...")
        await scanner.stop()
        print("Scan stopped.")

if __name__ == "__main__":
    # You can adjust RSSI_THRESHOLD at the top of the file
    # -60 = very close (< 1 meter)
    # -70 = close (1-3 meters)
    # -80 = medium distance (3-10 meters)
    asyncio.run(continuous_scan())