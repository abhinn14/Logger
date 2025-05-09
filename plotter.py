import serial
import matplotlib.pyplot as plt
import time
from collections import deque
import numpy as np
from sklearn.ensemble import IsolationForest

# Serial config
bt_port = 'COM12'
baud_rate = 9600

# Calibration: raw voltage ≈40 V → true 220 V
VOLT_CALIB = 220.0 / 45.69  # ≈5.5

# Data window size
window_size = 200
data_buffers = {
    "Current (A)":     deque(maxlen=window_size),
    "Voltage (V)":     deque(maxlen=window_size),
    "Humidity1 (%)":   deque(maxlen=window_size),
    "Temp1 (°C)":      deque(maxlen=window_size),
    "Humidity2 (%)":   deque(maxlen=window_size),
    "Temp2 (°C)":      deque(maxlen=window_size),
    "Vibration":       deque(maxlen=window_size),
    "Power (W)":       deque(maxlen=window_size),
}

# Initialize IsolationForest models for each data series
models = { key: IsolationForest(contamination=0.01, random_state=42)
           for key in data_buffers }

# Set up subplots
plt.ion()
fig, axs_mat = plt.subplots(4, 2, figsize=(12, 10))
axs = axs_mat.flat

try:
    ser = serial.Serial(bt_port, baud_rate, timeout=1)
    print(f"Connected to {bt_port}")

    while True:
        line = ser.readline().decode(errors='ignore').strip()
        if not line:
            continue

        parts = line.split()
        if len(parts) != 7:
            print("Invalid line:", line)
            continue

        # parse floats
        try:
            values = list(map(float, parts))
        except ValueError:
            print("Could not parse:", line)
            continue

        # 1) scale the raw voltage reading
        values[1] *= VOLT_CALIB

        # 2) compute power using scaled voltage
        power = values[0] * values[1]
        values.append(power)

        # 3) append to buffers
        keys = list(data_buffers.keys())
        for i in range(8):
            data_buffers[keys[i]].append(values[i])

        # 4) anomaly detection & plotting
        for i, key in enumerate(keys):
            ax    = axs[i]
            data  = np.array(data_buffers[key])
            ax.clear()

            if len(data) < 5:
                # too little data, just plot line
                ax.plot(data, '-', color='blue', label='Data')
            else:
                # fit and predict
                X      = data.reshape(-1, 1)
                model  = models[key]
                model.fit(X)
                preds  = model.predict(X)           #  1 = normal, -1 = anomaly

                x = np.arange(len(data))
                # plot continuous line
                ax.plot(x, data, '-', color='blue', label='Data')
                # overlay anomalies
                anomaly_idx = np.where(preds == -1)[0]
                ax.plot(x[anomaly_idx], data[anomaly_idx], 'ro', label='Anomaly')

            ax.set_title(key)
            ax.set_xlabel("Sample")
            ax.grid(True)
            ax.legend(loc='upper left')

        plt.tight_layout()
        plt.pause(0.01)

except serial.SerialException as e:
    print("Serial error:", e)

except KeyboardInterrupt:
    print("\nDisconnected")

finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
