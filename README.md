# tcpardu
This c project is small 1-client TCP server which transfer messages between TCP client and serial (Arduino) devices connected via USB. It handles also simple hot-plug support so serial device needs to be able to identify and set up communication link with this gateway.

The communication protocol and sample arduino code to be documented later.

## tcpardu -- arduino communication protocol
When arduino is connected to serial port is keeps sending `HELLO\n` string to serial port. Communication is not started until host (tcpardu) sends `OLLEH` in response.

After this handshake arduino orders commands and statuses it wants to receive. To order commands arduino sends string
```
CMD:SLAVE:MASTER:ORDER_COMMAND_FOR:UNIT1:UNIT2:...UNITn
```
If arduino wants to receive some status message it uses string
```
CMD:SLAVE:MASTER:ORDER_STATUS_FOR:UNIT1:UNIT2:...UNITn
```

# home automation
tcpardu is just one piece in whole home automation project. All pieces communicate via simple protocol. The protocol uses just 2 possible messages
- commands
- statuses
Commands are used to execute some action (switch light on, open door), statuses are sent by units to inform about status (door is open, temperature is).

## command messages
Format of command message is
```
CMD:from:to:command[:argument[...:argument]]
```

## status messages
Format of status message is
```
STS:from:to:value[:value[...:value]]
```
