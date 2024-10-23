# DiyIrTower
# Copyright (C) 2024 maehw
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# This script sends the serial bytes given on the command line and also prints the received response (if any).

import serial
import argparse


def send_receive_serial_data(serial_device, data_to_send):
    # Initialize serial connection
    ser = serial.Serial(serial_device, 2400, parity=serial.PARITY_ODD, timeout=0.5)

    # Convert data to bytes
    data_bytes = bytes.fromhex(data_to_send)

    # Send data
    print("[TX] ", end="")
    for b in data_bytes:
        print(f"{b:02x} ", end="")
    print()
    ser.write(data_bytes)

    # Receive response (or timeout)
    print("[RX] ", end="")
    while True:
        response = ser.read()

        if response:
            # Print response
            print(f"{response.hex()} ", end="")
        else:
            break
    print()

    # Close serial connection
    ser.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Send binary data over serial and receive response.")
    parser.add_argument("serial_device", help="Name of the serial device (e.g., '/dev/ttyUSB0')")
    parser.add_argument("data_to_send", help="Binary data to send in HEX format (e.g., '55FF00609F609F')")
    args = parser.parse_args()

    send_receive_serial_data(args.serial_device, args.data_to_send)

