#ifndef Irrigation_controller_h
#define Irrigation_controller_h

#include "Arduino.h"
#include "MCP23017.h"

class IrrigationController
{
  public:
    IrrigationController(int pin);
    void on(int sec);
    void off(int sec);
  private:
    //init the extension I2C board
    MCP23017 _mcp;
    int _mcpBeginPin;
    int _section;
};

#endif
