# LTF Serial Example

- Connect any serial device.

- (Optionally you can make it send "Hello" if you want to test reading from it)

- Check the serial port name (e.g. `/dev/ttyACM0`)

- Run:

```bash
ltf test -v port_name=/dev/ttyACM0
```

- You can also configure some options:

```bash
ltf test -v port_name=/dev/ttyACM0,baudrate=9600,bits=7,parity=odd,stopbits=2
```
