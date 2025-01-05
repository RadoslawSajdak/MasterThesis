import os
import struct
from matplotlib import pyplot as plt
import numpy as np
RES_VALUE = 0.5
TIMESTEP = 0.2e-3


def read_sd_sector(device_path, sector_number, sector_size=512):
    try:
        with open(device_path, 'rb') as device:
            device.seek(sector_number * sector_size)
            data = device.read(sector_size)
            return data

    except PermissionError:
        print(f"Permission denied. Try running with elevated privileges (e.g., sudo).")
        return None
    except FileNotFoundError:
        print(f"Device {device_path} not found.")
        return None
    except Exception as e:
        print(f"Error reading sector: {e}")
        return None

def uint_to_current(sample):
    return sample * 5e-6 / RES_VALUE * 1000

def parse_sd_data(sector_data):
    try:
        if sector_data.startswith(b"DATA--->"):
            print("Detected string: ", sector_data[8:].decode('utf-8').replace('\n',''))
            return
        timestamp = struct.unpack('<I', sector_data[:4])[0]
        data = list(struct.unpack('<254H', sector_data[4:]))
        for i, sample in enumerate(data):
            if sample & 0x8000:
                sample -= 0x10000

        return timestamp, data
    except struct.error as e:
        print(f"Error unpacking data: {e}")

def plot_data(all_samples):
    time_step = TIMESTEP
    time = [i * time_step for i in range(len(all_samples))]
    
    plt.figure(figsize=(12, 6))
    plt.plot(time, all_samples, label="Current (mA)")
    plt.xlabel("Time (s)")
    plt.ylabel("Current (mA)")
    plt.title("Current vs Time")
    plt.grid(True)
    plt.legend()

    plt.show()
    
def plot_data_with_selection(all_samples):
    time_step = TIMESTEP
    time = [i * time_step for i in range(len(all_samples))]

    fig, ax = plt.subplots(figsize=(12, 6))
    ax.plot(time, all_samples, label="Current (mA)")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Current (mA)")
    ax.set_title("Current vs Time")
    ax.grid(True)
    ax.legend()

    selected_samples = []

    def onselect(event):
        if event.name == 'button_press_event':
            onselect.start = event.xdata
        elif event.name == 'button_release_event':
            onselect.end = event.xdata

            if onselect.start and onselect.end:
                start_idx = np.searchsorted(time, onselect.start)
                end_idx = np.searchsorted(time, onselect.end)

                selected_samples = all_samples[start_idx:end_idx]
                if selected_samples:
                    avg_value = np.mean(selected_samples)
                    print(f"Selected range: {onselect.start:.6f} s to {onselect.end:.6f} s")
                    print(f"Average value in selected range: {avg_value:.6f} mA")

                ax.axvspan(onselect.start, onselect.end, color='yellow', alpha=0.3)
                fig.canvas.draw()

    onselect.start = None
    onselect.end = None

    fig.canvas.mpl_connect('button_press_event', onselect)
    fig.canvas.mpl_connect('button_release_event', onselect)

    plt.show()

if __name__ == "__main__":
    sd_device = "/dev/sdc"
    sector_size = 512
    sector = 0x00
    all_samples = []

    sector_data = read_sd_sector(sd_device, sector, sector_size)
    parse_sd_data(sector_data)
    written_sectors = struct.unpack('<I', sector_data[:4])[0]
    print("Found %d written sectors. Reading..." % (written_sectors))

    for i in range(1, written_sectors + 1):
        sector = i
        
        sector_data = read_sd_sector(sd_device, sector, sector_size)
        
        if sector_data:
            try:
                _, samples = parse_sd_data(sector_data)
            except:
                continue
            current_samples = [uint_to_current(sample) for sample in samples]            
            all_samples.extend(current_samples)
    
    plot_data_with_selection(all_samples)
