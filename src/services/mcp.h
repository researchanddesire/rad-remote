#ifndef MCP_H
#define MCP_H

#include <Adafruit_MCP23X17.h>

extern Adafruit_MCP23X17 mcp;

// Initialize MCP23017 with all pin configurations and interrupts
bool initMCP();

void mcpTask(void *pvParameters);
#endif