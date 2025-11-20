import serial
import time

PORT = "COM5"
BAUD = 115200
OUTFILE = "ir_log.csv"

# FOR BT
# PORT = "COM7"
# BAUD = 115200
# OUTFILE = "bt_log.txt"

def main():
    ser = serial.Serial(PORT, BAUD, timeout=1)
    time.sleep(2)

    print(f"Logging from {PORT} at {BAUD} baud...")
    print(f"Saving to {OUTFILE}")
    print("Press Ctrl+C to stop.\n")

    with open(f"./logs/{OUTFILE}", "w", encoding="utf-8") as f:
        while True:
            try:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if line:
                    print(line)
                    f.write(line + "\n")
            except KeyboardInterrupt:
                print("\nStopped logging.")
                break

    ser.close()

if __name__ == "__main__":
    main()
