import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import find_peaks

# Prompted the heck out of this based on what i saw in time series data

# -------------------------------------------------------------------
# CONFIG
# -------------------------------------------------------------------
CSV_FILE = "ir_log.csv"  # your logged data (columns: ms, diff)

# Heuristic thresholds (tune these!)
HEIGHT_MIN   = 10.0   # minimum height above local baseline to count as "strong"
WIDTH_MAX_MS = 180.0  # maximum FWHM width (camera spikes are narrow)
RISE_MIN     = 0.15    # minimum rise slope (diff units per ms)
FALL_MIN     = 0.15   # minimum fall slope magnitude

# t_ms,peak_value,baseline,height,width_ms,rise_slope,fall_slope,is_lens_like
# 13943.0,28.0,14.0,14.0,161.0,0.23333333333333334,-0.13861386138613863,False
# -------------------------------------------------------------------
# LOAD DATA
# -------------------------------------------------------------------
df = pd.read_csv(CSV_FILE)   # expects columns: ms, diff
df = df.sort_values("ms").reset_index(drop=True)

t = df["ms"].values.astype(float)
y = df["diff"].values.astype(float)

# Approximate sample period (for info only)
dt = np.diff(t)
mean_dt = np.median(dt[dt > 0]) if np.any(dt > 0) else 5.0
print(f"Approx. sample interval: {mean_dt:.2f} ms")


# -------------------------------------------------------------------
# FIND CANDIDATE PEAKS (local maxima)
# -------------------------------------------------------------------
# We set a small minimum height so we catch everything and filter later.
peaks, props = find_peaks(y, height=5)  # height=3 to ignore tiny noise bumps

print(f"Found {len(peaks)} candidate peaks")


# -------------------------------------------------------------------
# PER-PEAK FEATURE EXTRACTION
# -------------------------------------------------------------------
records = []

for p in peaks:
    t_peak = t[p]
    peak_val = y[p]

    # --- Find half-max crossing points on left/right for width ---
    half = peak_val * 0.5

    # search left until we fall below half
    left = p
    while left > 0 and y[left] > half:
        left -= 1

    # search right until we fall below half
    right = p
    while right < len(y) - 1 and y[right] > half:
        right += 1

    width_ms = t[right] - t[left]

    # --- Local baseline: valley near the edges of the peak ---
    baseline_val = min(y[left], y[right])
    height = peak_val - baseline_val

    # --- Slopes: how fast we rise and fall relative to baseline ---
    # Protect against division by zero if timing is weird
    rise_dt = max(t[p] - t[left], 1e-6)
    fall_dt = max(t[right] - t[p], 1e-6)

    rise_slope = (peak_val - baseline_val) / rise_dt      # positive
    fall_slope = (baseline_val - peak_val) / fall_dt      # negative

    # --- Classification: is this peak "camera-lens-like"? ---
    is_lens = (
        height >= HEIGHT_MIN and
        width_ms <= WIDTH_MAX_MS and
        rise_slope >= RISE_MIN and
        (-fall_slope) >= FALL_MIN
    )

    records.append({
        "t_ms": t_peak,
        "peak_value": peak_val,
        "baseline": baseline_val,
        "height": height,
        "width_ms": width_ms,
        "rise_slope": rise_slope,
        "fall_slope": fall_slope,
        "is_lens_like": is_lens
    })

peaks_df = pd.DataFrame(records)
print("\n=== Peak features ===")
print(peaks_df)

# Save for inspection / tweaking
peaks_df.to_csv("detected_peaks_simple.csv", index=False)


# -------------------------------------------------------------------
# VISUALIZE
# -------------------------------------------------------------------
plt.figure(figsize=(16, 6))
plt.plot(t, y, label="diff (lit - dark)", lw=1)

# Mark all peaks
plt.scatter(peaks_df["t_ms"], peaks_df["peak_value"],
            color="gray", s=40, alpha=0.5, label="all peaks")

# Highlight only those classified as lens-like
lens_peaks = peaks_df[peaks_df["is_lens_like"]]
plt.scatter(lens_peaks["t_ms"], lens_peaks["peak_value"],
            color="red", s=100, marker="x", label="camera-lens-like")

plt.title("IR Differential Reflection with Simple Lens Peak Detection")
plt.xlabel("Time (ms)")
plt.ylabel("diff (lit - dark)")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()
