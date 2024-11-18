import serial
import time

SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 115200

def send_epoch():
    try:
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
            while True:
                epoch_time = int(time.time())
                epoch_str = f"EPOCH,{epoch_time}\n"
                
                ser.write(epoch_str.encode('utf-8'))
                print(f"Sent epoch time: {epoch_str.strip()}")
                
                time.sleep(1)
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
    except KeyboardInterrupt:
        print("Exiting script.")

if __name__ == "__main__":
    send_epoch()
