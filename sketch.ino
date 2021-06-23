#include <Wire.h>
#include <Rtc_Pcf8563.h>
#include <MCP23017.h>
#include <Keypad.h>

#define RTCC_VERSION  "Pcf8563 v1.0.3"

//init the real time clock
Rtc_Pcf8563 rtc;
//init the extension I2C board
MCP23017 mcp;

#define DELAY_TIME 1000
#define FIRST_SECTIONS 0
#define LAST_SECTIONS 15

int actualSection = -1;

// 1, 2, 3 + A
int mode = 1;

// 1, 2, 3, .. + B
// Starts section
int cmdSection = -1;

// C stops

// 11 + D
int startTime = 1;

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
int sectionConfiguration[5][3] = {
 {0, 20, 2},
 {1, 20, 2},
 {3, 20, 1},
 {4, 20, 1},
 {5, 20, 1}
};

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

String command;

void setup()
{
  Serial.begin(9600);
  
  displayActualDateTime();
  
  // initialize i2c extension board
  initializeI2CextensionBoard();
}

void loop()
{
  char customKey = customKeypad.getKey();
    
  if (customKey && customKey != '#'){
    command += customKey;
  }

  if (customKey == 'A') {
    mode = command.toInt();
//    Serial.println("Mode:");
//    Serial.println(mode);
    command = "";
  }
  else if (customKey == 'B') {
    cmdSection = command.toInt();
//    Serial.println("Command Section:");
//    Serial.println(cmdSection);
    command = "";
  }
  else if (customKey == 'C') {
//    Serial.println("Clean");
    cmdSection = -1;
    command = "";
  }
  else if (customKey == 'D') {
    startTime = command.toInt();
    Serial.println("StartTime:");
    Serial.println(startTime);
    command = "";
  }
  
  int selectedSection = section();
  off(selectedSection);
  on(selectedSection);

//  delay(DELAY_TIME);
}

void autoMode() {
  int day = rtc.getDay();
  int hour = rtc.getHour();

  int section;
  
  if (hour == startTime) {
    for(int i=0; i<5; i++) {
      if (day % sectionConfiguration[i][2] == 0) {
        section = sectionConfiguration[i][0];
        mcp.digitalWrite(section, LOW);
        delay(sectionConfiguration[i][1]*60*1000);
        mcp.digitalWrite(section, HIGH);
      }
    }
  }
}

void on(int section) {
  if (section >= FIRST_SECTIONS && section <= LAST_SECTIONS && actualSection != section) {
    Serial.println("On");
    Serial.println(section);
    mcp.digitalWrite(section, LOW);
    actualSection = section;
  }
}

void off(int section) {
  if (actualSection != section) {
    Serial.println("Off");
    Serial.println(actualSection);
    mcp.digitalWrite(actualSection, HIGH);
    actualSection = -1;
  }
}

int section()
{
  if (mode == 3) {
    return -1;
  }
  if (mode == 2) { 
    return cmdSection;
  }
  int day = rtc.getDay();
  int hour = rtc.getHour();
  int minutes = rtc.getMinute();
  int section = -1;
  if (hour == 23 && day % 2 == 0) {
    if (minutes < 20 && 1 <= LAST_SECTIONS) { // kropelkowe front
      section = 0;
    }
    else if (minutes < 40 && 2 <= LAST_SECTIONS) { // kropelkowe skarpa
      section = 1;
    }
    else if (3 <= LAST_SECTIONS) { // kropelkowe
      section = 2;
    }
  }
  else if (hour == startTime || hour == startTime + 4) {
    if (minutes < 15 && 4 <= LAST_SECTIONS) { // S-taras
      section = 3;
    }
    else if (minutes < 30 && 5 <= LAST_SECTIONS) { // SW-donice
      section = 4;
    }
    else if(minutes < 45 && 6 <= LAST_SECTIONS) { // S-taras
      section = 5;
    }
    else if (7 <= LAST_SECTIONS) { // 15
      section = 6;
    }
  }
//  else if (hour == startTime + 1) {
//    if (minutes < 15 && 8 <= LAST_SECTIONS) { // 16
//      section = 7;
//    }
//    else if (minutes < 30 && 9 <= LAST_SECTIONS) { // 1
//      section = 8;
//    }
//    else if (minutes < 45 && 10 <= LAST_SECTIONS) { // 2
//      section = 9;
//    }
//    else if (11 <= LAST_SECTIONS) { // 3
//      section = 10;
//    }
//  }
//  else if (hour == startTime + 2) {
//    if(minutes < 15 && 12 <= LAST_SECTIONS) { // 4
//      section = 11;
//    }
//    else if (minutes < 30 && 13 <= LAST_SECTIONS) { // 5
//      section = 12;
//    }
//    else if (minutes < 45 && 14 <= LAST_SECTIONS) { // 6
//      section = 13;
//    }
//    else if (15 <= LAST_SECTIONS) { // 7
//      section = 14;
//    }
//  }
//  else if (hour == startTime + 3) {
//    if (minutes < 15 && 16 <= LAST_SECTIONS) { // 8
//      section = 15;
//    }
//  }
  return section;
}

void displayActualDateTime() {
  Serial.println("Actual date:");
  Serial.print("20");
  Serial.print(rtc.getYear());
  Serial.print("/");
  Serial.print(rtc.getMonth());
  Serial.print("/");
  Serial.println(rtc.getDay());
  Serial.println("Actual time:");
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
  rtc.setDate(19, 5, 6, 0, 21);
  //hr, min, sec
  rtc.setTime(12, 21, 0);
}

void initializeI2CextensionBoard() {
  mcp.begin(7);
  for(int i=0; i<16; i++)
  {
      mcp.pinMode(i, OUTPUT);
      mcp.digitalWrite(i, HIGH);
  }
}
