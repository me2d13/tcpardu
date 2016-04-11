# Background
Suppose you're home automation maniac and want to monitor and control things using web (or any other) application. You want to monitor temprature, sun activity, humidity, doors opening and control lights, heating, jalousies and other stuff. One of the cheap and easiy options is to use Arduino on hardware side. But you probably don't want to equip each Arduino with some kind of network (wifi or ethernet) shield. So how to connect more Arduinos to your home network?

Solution I have chosen is use Raspberry (or other simple computer with Linux distro) as USB host and connect Arduinos to this host. Rasberry Pi has no problems with network connectivty and Arduinos are connected directly or via USB hub. My home automation application is written in Java so the task was how to control those Arduino devices (sensors, relays) from central Java application running at the same network but different host. Solution is this tcpardu application agregating serial communication between Arduinos and Raspberry to TCP communication between Java application and Raspberry.

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

## http mode
For quick testing of controling endpoints there's also http mode where commands can be entered not through telnet (tcp socket) but as http requests. In this mode command is written as url with : replaced by /.
TODO: more documentation needed

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
