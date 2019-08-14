/*******************************************************************************
 * Author: Adam Phelps
 * License: Creative Commons Attribution-Non-Commercial
 * Copyright: 2017
 *
 * Lightbringer controller code
 ******************************************************************************/

#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL DEBUG_HIGH
#endif
#include "Debug.h"

#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>

#include "SPI.h"
#include "FastLED.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "PixelUtil.h"
#include "Wire.h"
#include "MPR121.h"
#include "SerialCLI.h"

#include "Socket.h"
#include "RS485Utils.h"
#include "XBee.h"
#include "XBeeSocket.h"

#include "HMTLProtocol.h"
#include "HMTLMessaging.h"
#include "HMTLPrograms.h"
#include "TimeSync.h"


#include "lightbringer.h"
#include "modes.h"

/******/

/*
 * A timesync object must be defined and initialized here as some libraries
 * require it during initialization.
 */
TimeSync timesync;

config_hdr_t config;
output_hdr_t *outputs[HMTL_MAX_OUTPUTS];
config_max_t readoutputs[HMTL_MAX_OUTPUTS];
void *objects[HMTL_MAX_OUTPUTS];

TimeSync time;
PixelUtil pixels;

RS485Socket rs485;
#define SEND_BUFFER_SIZE 64 // The data size for transmission buffers
byte rs485_data_buffer[RS485_BUFFER_TOTAL(SEND_BUFFER_SIZE)];

#define MAX_SOCKETS 2
Socket *sockets[MAX_SOCKETS] = { NULL, NULL };

void setup() {
  Serial.begin(115200);

  int configOffset = -1;
  int32_t outputs_found = hmtl_setup(&config, readoutputs,
                                     outputs, objects, HMTL_MAX_OUTPUTS,
                                     &rs485,
                                     NULL,          // XBee
                                     &pixels,
                                     NULL,          // MPR121
                                     NULL,          // RGB
                                     NULL, // Value
                                     &configOffset);

  DEBUG4_VALUE("Config size:", configOffset - HMTL_CONFIG_ADDR);
  DEBUG4_VALUELN(" end:", configOffset);

  if (outputs_found & (1 << HMTL_OUTPUT_PIXELS)) {
    for (unsigned int i = 0; i < pixels.numPixels(); i++) {
      pixels.setPixelRGB(i, 0, 0, 0);
    }
    pixels.update();
  }

  /* Setup communication devices */
  byte num_sockets = 0;

  if (outputs_found & (1 << HMTL_OUTPUT_RS485)) {
    /* Setup the RS485 connection */
    rs485.setup();
    rs485.initBuffer(rs485_data_buffer, SEND_BUFFER_SIZE);
    sockets[num_sockets++] = &rs485;
  }

  if (num_sockets == 0) {
    DEBUG_ERR("No sockets configured");
    DEBUG_ERR_STATE(2);
  }

  init_modes(sockets, num_sockets);

  DEBUG2_PRINTLN("* Lightbringer Initialized *");

  // Send the ready signal to the serial port
  Serial.println(F(HMTL_READY));
}

#define PERIOD 100
unsigned long last_change = 0;
int cycle = 0;
void loop() {

  // TODO: Add sensors and such here

  /*
   * Check for messages and handle output states
   */
  boolean updated = messages_and_modes();
}
