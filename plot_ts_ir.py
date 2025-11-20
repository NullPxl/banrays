import pandas as pd
import matplotlib.pyplot as plt

CSV_FILE = "./logs/ir_log.csv"

def main():
    df = pd.read_csv(CSV_FILE)

    plt.figure(figsize=(12, 5))
    plt.plot(df["ms"], df["diff"], linewidth=1)

    plt.title("IR Differential Reflection Waveform")
    plt.xlabel("Time (ms)")
    plt.ylabel("diff (lit - dark)")

    plt.grid(True)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
