from serial import Serial
from serial.tools.miniterm import ask_for_port


def main():
    DIRECTION = 1
    position = 0                # < Current encoder position from 0 to 36000
    port = ask_for_port()

    with Serial(port, baudrate=19200) as ser:
        print("Starting communication with controller")

        while ser:
            dat = ser.read()
            dat = dat.decode()

            print(f"received reading, my position: {position}")
            
            if dat.startswith('R'):
                pos_as_str = f"{position}\r\n".encode()
                ser.write(pos_as_str)

            position += 10 * DIRECTION
            if position > 36000 or position < 0:
                DIRECTION = -DIRECTION


if __name__ == '__main__':
    main()