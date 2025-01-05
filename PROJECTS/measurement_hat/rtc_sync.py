import asyncio
import struct
import time
from bleak import BleakScanner, BleakClient

SERVICE_UUID = "0000DEAD-0000-1000-8000-00805F9B34FB"
CHARACTERISTIC_UUID = "0000BEEF-0000-1000-8000-00805F9B34FB"

async def scan_for_devices():
    print("Scanning for Bluetooth devices...")
    devices = await BleakScanner.discover()
    
    filtered_devices = [device for device in devices if "RS00" in device.name]
    
    if not filtered_devices:
        print("No devices found with name 'RS00*'.")
        return None
    
    print("\nDevices found:")
    for i, device in enumerate(filtered_devices):
        print("\t%i: %s (Address: %s)" % (i, device.name, device.address))
    
    return filtered_devices

async def connect_and_read(device):
    print("Connecting to device %s (Address: %s)..." % (device.name, device.address))
    async with BleakClient(device.address) as client:
        if client.is_connected:
            print("Successfully connected to %s!" % (device.name))
            try:
                current_epoch = int(time.time())
                epoch_data = struct.pack('<I', current_epoch)
                await client.write_gatt_char(CHARACTERISTIC_UUID, epoch_data)
                print("Wrote epoch time: ", current_epoch)

                data = await client.read_gatt_char(CHARACTERISTIC_UUID)
                read_epoch = struct.unpack('<I', data[:4])[0]
                print("Read epoch time: ", read_epoch)

                diff = read_epoch - current_epoch
                if  diff <= 1 and diff >= -1:
                    print("Verification successful: Epoch time matches.")
                else:
                    print("Verification failed: Epoch time does not match.")
            except Exception as e:
                print(f"Error during operation: {e}")
        else:
            print("Failed to connect to ", device.name)


async def main():
    devices = await scan_for_devices()
    if not devices:
        return
    
    try:
        choice = int(input("Enter the number of the device you want to connect to: "))
        if 0 <= choice < len(devices):
            await connect_and_read(devices[choice])
        else:
            print("Invalid choice.")
    except ValueError:
        print("Invalid input. Please enter a number.")

if __name__ == "__main__":
    asyncio.run(main())
