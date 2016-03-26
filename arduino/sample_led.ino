/*
 * Sample Arduino code for tcpardu communication
 * Built in LED can be turned on and off with tcp commands:
 * 
 * CMD::TEST_LED:ON
 * CMD::TEST_LED:OFF
 *  
 *  Author: Petr Medek
 *  
 *  https://github.com/me2d13/tcpardu
 * 
 */

#define LEDPIN 13
#define WHOAMI "TCP_ARDU_TEST"

// 1 unit (controlled device) to control - built in LED for this test
#define NUMBER_OF_UNITS 1
#define UNIT_INDEX_LED 0
char *cUnits[NUMBER_OF_UNITS] = { "TEST_LED" };

// maximum message length
#define IN_BUFFER_LENGTH 100

// buffer for incoming mesage
char gSerialInput[IN_BUFFER_LENGTH];

// pointers to parsed part of message
char *gMessageType;
char *gFrom;
char *gTo;
char *gCommand;
char *gValue;

// was connection established?
boolean gConnected = false;

void setup() {
  pinMode(LEDPIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  if (gConnected) {
    // communication established, wait for commands
    checkAndProcessMessage();
    delay(100);
  } else {
    // send Hello message to master, wait check answer
    sendHello();
    // wait 5 sec
    delay(5000);
  }
}

// ***** HW handling functions - what should be updated for other devices (pins) *****

// find target unit and call appropriate handling method. Here we have just one for simlicity
void processCommand() {
  for (int i = 0; i < NUMBER_OF_UNITS; i++) {
    if (compareStr(cUnits[i], gTo)) {
      switch (i) {
      case UNIT_INDEX_LED:
        processLedCommand();
        break;
      }
    }
  }
}

// check command for LED control, set LED value, send status response back to master
void processLedCommand() {
  if (compareStr("ON", gCommand)) {
    digitalWrite(LEDPIN, HIGH);
    sendStatusInt(cUnits[UNIT_INDEX_LED], 1);
  } else if (compareStr("OFF", gCommand)) {
    digitalWrite(LEDPIN, LOW);
    sendStatusInt(cUnits[UNIT_INDEX_LED], 0);
  }
  // error handling would be here
}





// ***** Support functions for tcpardu communication *****

// send HELLO to serial line and call answer checking
void sendHello() {
  Serial.print("HELLO\n");
  int numRead = readSerialStringToBufferAndParseIt();
  if (numRead > 0) {
    checkHelloAnswer();
  }
}

// read data from serial port and store to buffer; then call parse function
int readSerialStringToBufferAndParseIt() {
  int i = 0;
  while (Serial.available() > 0) {
    if (i+1 == IN_BUFFER_LENGTH) {
      break;
    }
    gSerialInput[i] = Serial.read();
    i++;
  }
  gSerialInput[i] = 0;
  parseReceivedBufferToParts();
  return i;
}

// parse received message in buffer, set pointers to correct places in buffer,
// replace delimiters : with string ends
void parseReceivedBufferToParts() {
  gMessageType = NULL;
  gFrom = NULL;
  gTo = NULL;
  gCommand = NULL;
  gValue = NULL;
  int i = 0;
  for (int part = 0; part < 5; part++) {
    if (gSerialInput[i] == 0) return;
    if (part == 0) {
      gMessageType = gSerialInput;
    } else if (part == 1) {
      gFrom = gSerialInput + i;
    } else if (part == 2) {
      gTo = gSerialInput + i;
    } else if (part == 3) {
      gCommand = gSerialInput + i;
    } else if (part == 4) {
      gValue = gSerialInput + i;
    }
    while (gSerialInput[i] != 0) {
      if (gSerialInput[i] == ':') {
        gSerialInput[i] = 0;
        i++;
        break;
      }
      i++;
    }
  }
}

// check if received data is answer to HELLO message, if yes, set connected flag and
// inform masters what commands I want to receive
void checkHelloAnswer() {
  if (compareStr("OLLEH", gMessageType)) {
    gConnected = true;
    orderMessages();
  }
}

// send command to master with my units (controlled devices) names
void orderMessages() {
  Serial.print("CMD:");
  Serial.print(WHOAMI);
  Serial.print(":MASTER:ORDER_COMMAND_FOR");
  for (int i = 0; i < NUMBER_OF_UNITS; i++) {
    Serial.print(":");
    Serial.print(cUnits[i]);
  }
  Serial.print("\n");
}

// when communication is established and we receive real command, parse it and call handling function
void checkAndProcessMessage() {
  int numRead = readSerialStringToBufferAndParseIt();
  if (numRead > 0 && gMessageType != NULL && gFrom != NULL && gTo != NULL && gCommand != NULL) {
    if (compareStr("CMD", gMessageType)) {
      processCommand();
    }
    // Status (STS) processing and error handling would be here as well
  }
}

// send status message back to master (tcpardu)
void sendStatusInt(char* unit, int value) {
    Serial.print("STS:");
    Serial.print(unit);
    Serial.print("::");
    Serial.println(value);
}

// compare 2 null terminated strings (char arrays)
boolean compareStr(char *str1, char *str2) {
  int i = 0;
  while (str1[i] != 0) {
    if (str1[i] != str2[i] || str2[i] == 0) {
      return false;
    }
    i++;
  }
  return str2[i] == 0;
}


