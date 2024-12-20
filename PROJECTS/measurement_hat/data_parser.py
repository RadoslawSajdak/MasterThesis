import os
import struct
from matplotlib import pyplot as plt

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
    lsb = 50e-3 / 32768
    return sample * lsb * 1000

def parse_sd_data(sector_data):
    try:
        timestamp = struct.unpack('<I', sector_data[:4])[0]
        data = list(struct.unpack('<254H', sector_data[4:]))
        for i, sample in enumerate(data):
            # It's required to remove noise near when no current flows
            if sample > 0xf000:
                if i > 0:
                    data[i] = data[i - 1]
                else:
                    data[i] = 0.0
        return timestamp, data
    except struct.error as e:
        print(f"Error unpacking data: {e}")

def plot_data(all_samples):
    time_step = 50e-6
    time = [i * time_step for i in range(len(all_samples))]
    
    plt.figure(figsize=(12, 6))
    plt.plot(time, all_samples, label="Current (mA)")
    plt.xlabel("Time (s)")
    plt.ylabel("Current (mA)")
    plt.title("Current vs Time")
    plt.grid(True)
    plt.legend()

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

    for i in range(1, written_sectors):
        sector = i
        
        sector_data = read_sd_sector(sd_device, sector, sector_size)
        
        if sector_data:
            _, samples = parse_sd_data(sector_data)
            current_samples = [uint_to_current(sample) for sample in samples]            
            all_samples.extend(current_samples)
    
    plot_data(all_samples)
