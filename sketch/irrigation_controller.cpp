#include "Arduino.h"
#include "Morse.h"

IrrigationController::IrrigationController(int pin)
{
  pinMode(pin, OUTPUT);
  _mcpBeginPin = pin;
  initializeI2CextensionBoard();
}

void IrrigationController::on(int sec)
{
  mcp.digitalWrite(sec, LOW);
  Serial.print("On: ");
  Serial.println(sec);
}

void IrrigationController::off(int sec)
{
  mcp.digitalWrite(sec, HIGH);
  Serial.print("Off: ");
  Serial.println(sec);
}

void initializeI2CextensionBoard() {
  mcp.begin(_mcpBeginPin);
  for(int i=0; i<16; i++)
  {
      mcp.pinMode(i, OUTPUT);
      mcp.digitalWrite(i, HIGH);
  }
}
