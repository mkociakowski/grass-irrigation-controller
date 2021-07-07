#include <Wire.h>
#include <Rtc_Pcf8563.h>
#include <MCP23017.h>
#include <Keypad.h>
#include <Arduino_JSON.h>
#include <SoftwareSerial.h>

SoftwareSerial linkSerial(12, 11);

#define RTCC_VERSION  "Pcf8563 v1.0.3"

//init the real time clock
Rtc_Pcf8563 rtc;
//init the extension I2C board
MCP23017 mcp;

#define FIRST_SECTIONS 0
#define LAST_SECTIONS 15

// keyboard
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
 {'1','2','3','A'},
 {'4','5','6','B'},
 {'7','8','9','C'},
 {'*','0','#','D'}
};
byte rowPins[ROWS] = {10, 9, 8, 7}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 5, 4, 3}; //connect to the column pinouts of the keypad

int actualSection = -1;

// 0, 1, 2, 3 + A
int mode = 1;

// 1, 2, 3, .. + B
// Starts section
int cmdSection = -1;

// C stops

// 11 + D
int startTime = 0;
int endTime = startTime;

int synchronizationDay;

String command;

const int sections = 8;
int sectionConf[sections][8] = {
 {0, 1, 2, -1, -1, -1, -1, 0},
 {1, 1, 2, -1, -1, -1, -1, 0},
 {3, 1, 1, -1, -1, -1, -1, 0},
 {4, 1, 1, -1, -1, -1, -1, 0},
 {5, 1, 1, -1, -1, -1, -1, 0},
 {6, 1, 1, -1, -1, -1, -1, 0},
 {7, 1, 1, -1, -1, -1, -1, 0},
 {8, 1, 1, -1, -1, -1, -1, 0}
};

int sectionConf1[sections][8] = {
 {0, 20, 2, -1, -1, -1, -1, 0},
 {1, 20, 2, -1, -1, -1, -1, 0},
 {3, 30, 1, -1, -1, -1, -1, 0},
 {4, 25, 1, -1, -1, -1, -1, 0},
 {5, 25, 1, -1, -1, -1, -1, 0},
 {6, 30, 1, -1, -1, -1, -1, 0},
 {7, 30, 1, -1, -1, -1, -1, 0},
 {8, 30, 1, -1, -1, -1, -1, 0}
};

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

void setup()
{
  Serial.begin(9600);
  linkSerial.begin(9600);
  delay(2000);

  

  Serial.println("------------------------------------");
//  setDateTime();
//  displayActualDateTime();
//  delay(12000);
  displayActualDateTime();
  
  // initialize i2c extension board
  initializeI2CextensionBoard();

  initializeEndTime();

  initializeStartEndHoursForSections();
}

void loop() {
  runCommand(readCommand());

  if (mode == 2) {
    manualMode();
  }

  if (mode == 1) {
    autoMode();
  }
}

void runCommand(String cmd) {
  if (cmd == "A") {
    mode = command.toInt();
    command = "";
    Serial.print("Run command A, mode: ");
    Serial.println(mode);
  }
  else if (cmd == "B") {
    cmdSection = command.toInt();
    command = "";
    Serial.print("Run command B, section: ");
    Serial.println(cmdSection);
  }
  else if (cmd == "C") {
    cmdSection = -1;
    command = "";
    Serial.println("Run command C, clean ");
  }
  else if (cmd == "D") {
    startTime = command.toInt();
    initializeEndTime();
    initializeStartEndHoursForSections();
    command = "";
    Serial.print("Run command D, set start hour to: ");
    Serial.println(startTime);
  }
  else {
    if (cmd && cmd != "#"){
      command += cmd;
    }
  }
}

String readCommand() {
  String cmd = readSerialCommand();
  if (cmd != "") {
    return cmd;
  }
  return readKeypadCommand();
}

String readKeypadCommand() {
  char tmpCmd = customKeypad.getKey();
  if (tmpCmd) {
    return String(tmpCmd);
  }

  return "";
}

String readSerialCommand() {
  String cmd = "";
  if (linkSerial.available()) {
    JSONVar myObject = JSON.parse(linkSerial.readString());

    // JSON.typeof(jsonVar) can be used to get the type of the var
    if (JSON.typeof(myObject) != "undefined") {
      if (myObject.hasOwnProperty("action")) {
        cmd = myObject["action"];
      }
      if (myObject.hasOwnProperty("n")) {
        command = myObject["n"];
      }
    }
  }

  return cmd;
}

void autoMode() {
  int day = rtc.getDay();
  int hour = rtc.getHour();
  int minute = rtc.getMinute();
  
  if (synchronizationDay != day && hour == startTime) {
    initializeStartEndHoursForSections();
  }
  
  if (hour >= startTime && hour <= endTime) {
    for(int i=0; i<sections; i++) {
      if (sectionConf[i][7] == 1 && !shouldRun(i, hour, minute)) {
        off(sectionConf[i][0]);
        sectionConf[i][7] = 0;
      }
    }
    
    for(int i=0; i<sections; i++) {
      if (day % sectionConf[i][2] == 0) {
        if (sectionConf[i][7] == 0 && shouldRun(i, hour, minute)) {
          sectionConf[i][7] = 1;
          on(sectionConf[i][0]);
        }
      }
    }
  }
}

boolean shouldRun(int actualProgramIteratio, int hour, int minute) {
  return hour == sectionConf[actualProgramIteratio][3] && minute >= sectionConf[actualProgramIteratio][4]
    && hour <= sectionConf[actualProgramIteratio][5] && minute < sectionConf[actualProgramIteratio][6];
}

void manualMode() {
  int tmpSection = -1;
  if (mode == 2) {
    tmpSection = cmdSection;
  }
  
  if (actualSection != tmpSection) {
    off(actualSection);
    actualSection = -1;
  }
  
  if (tmpSection >= FIRST_SECTIONS && tmpSection <= LAST_SECTIONS && actualSection != tmpSection) {
    on(tmpSection);
    actualSection = tmpSection;
  }
}

void on(int section) {
  mcp.digitalWrite(section, LOW);
  Serial.print("On: ");
  Serial.println(section);
}

void off(int section) {
  mcp.digitalWrite(section, HIGH);
  Serial.print("Off: ");
  Serial.println(section);
}

void displayActualDateTime() {
  Serial.print("Actual date: ");
  Serial.print("20");
  Serial.print(rtc.getYear());
  Serial.print("/");
  Serial.print(rtc.getMonth());
  Serial.print("/");
  Serial.println(rtc.getDay());
  Serial.print("Actual time: ");
  Serial.print(rtc.getHour());
  Serial.print(":");
  Serial.print(rtc.getMinute());
  Serial.print(":");
  Serial.println(rtc.getSecond());
}

void setDateTime() {
  //clear out the registers
  rtc.initClock();
  //set a time to start with.
  //day, weekday, month, century(1=1900, 0=2000), year(0-99)
  rtc.setDate(7, 3, 7, 0, 21);
  //hr, min, sec
  rtc.setTime(23, 59, 50);
}

void initializeI2CextensionBoard() {
  mcp.begin(7);
  for(int i=0; i<16; i++)
  {
      mcp.pinMode(i, OUTPUT);
      mcp.digitalWrite(i, HIGH);
  }
}

void initializeEndTime() {
  int sum = 0;
  for(int i=0; i<sections; i++) {
    sum += sectionConf[i][1];
  }
  endTime = (sum / 60) + startTime;

    
  Serial.print("Start time: ");
  Serial.println(startTime);
  Serial.print("End time: ");
  Serial.println(endTime);
}

void initializeStartEndHoursForSections() {
  int sum = 0;
  int hour = startTime;
  int minutes = sum % 60;
  synchronizationDay = rtc.getDay();

  for(int i=0; i<sections; i++) {
    sectionConf[i][3] = hour;
    sectionConf[i][4] = minutes;

    sum += sectionConf[i][1];
    hour = startTime + (sum / 60);
    minutes = sum % 60;

    sectionConf[i][5] = hour;
    sectionConf[i][6] = minutes;

    Serial.print("Section ");
    Serial.print(sectionConf[i][0]);
    Serial.print(" starts at ");
    Serial.print(sectionConf[i][3]);
    Serial.print(":");
    Serial.print(sectionConf[i][4]);
    Serial.print(", ends at ");
    Serial.print(sectionConf[i][5]);
    Serial.print(":");
    Serial.println(sectionConf[i][6]);
  }
}
