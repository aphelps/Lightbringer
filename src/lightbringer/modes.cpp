/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2016
 *
 * Message handling and pendant modes
 ******************************************************************************/

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_HIGH
#endif
#include "Debug.h"

#include <Arduino.h>

#include <ProgramManager.h>
#include <HMTLMessaging.h>
#include <HMTLPrograms.h>

#include <TimeSync.h>
#include <MessageHandler.h>

#include <PixelUtil.h>

#include "lightbringer.h"
#include "modes.h"

/* List of available programs */
hmtl_program_t program_functions[] = {
        // Programs from HMTLPrograms
        { HMTL_PROGRAM_NONE, NULL, NULL},
        { HMTL_PROGRAM_BLINK, program_blink, program_blink_init },
        { HMTL_PROGRAM_TIMED_CHANGE, program_timed_change, program_timed_change_init },
        { HMTL_PROGRAM_FADE, program_fade, program_fade_init },
        { HMTL_PROGRAM_SPARKLE, program_sparkle, program_sparkle_init },
        { HMTL_PROGRAM_CIRCULAR, program_circular, program_circular_init},

        { PROGRAM_BRIGHTNESS, NULL,  program_brightness },
        { PROGRAM_COLOR, NULL, program_color},

        // Custom programs
        //{ PENDANT_TEST_PIXELS, program_test_pixels, program_test_pixels_init },
};
#define NUM_PROGRAMS (sizeof (program_functions) / sizeof (hmtl_program_t))

program_tracker_t *active_programs[HMTL_MAX_OUTPUTS];
ProgramManager manager;
MessageHandler handler;

/*
 * Execute initial commands
 */
void startup_commands() {
  byte output = manager.lookup_output_by_type(HMTL_OUTPUT_PIXELS);
  DEBUG4_VALUELN("Init: sparkle ", output);
  program_sparkle_fmt(rs485.send_buffer, rs485.send_data_size,
                      config.address, output,
                      0,0,0,0,0,0,0,0,0,0);

  handler.process_msg((msg_hdr_t *)rs485.send_buffer, &rs485, NULL, &config);

}

void init_modes(Socket **sockets, byte num_sockets) {
  /* Setup the program manager */
  manager = ProgramManager(outputs, active_programs, objects, HMTL_MAX_OUTPUTS,
                           program_functions, NUM_PROGRAMS);

  /* Setup a message handler with the program manager */
  handler = MessageHandler(config.address, &manager, sockets, num_sockets);

  /* Execute any initial commands */
  startup_commands();
}

/*
 * Check for and handle incoming messages
 */
boolean messages_and_modes(void) {
  // Check and send a serial-ready message if needed
  handler.serial_ready();

  /*
   * Check the serial device and all sockets for messages, forwarding them and
   * processing them if they are for this module.
   */
  boolean update = handler.check(&config);

  /* Execute any active programs */
  if (manager.run()) {
    update = true;
  }

  if (update) {
    /*
     * An output may have been updated by message or active program,
     * update all output states.
     */
    for (byte i = 0; i < config.num_outputs; i++) {
      hmtl_update_output(outputs[i], objects[i]);
    }
  }

  return update;
}


typedef struct {
  unsigned long last_change_ms;
  uint16_t period;
  uint16_t cycle;
} mode_test_state_t;

boolean program_test_pixels_init(msg_program_t *msg,
                                 program_tracker_t *tracker,
                                 output_hdr_t *output) {
  if ((output == NULL) || (output->type != HMTL_OUTPUT_PIXELS)) {
    return false;
  }

  mode_test_state_t *state = (mode_test_state_t *)malloc(sizeof(mode_test_state_t));
  state->last_change_ms = timesync.ms();
  state->period = *(uint16_t *)msg->values;
  if (state->period == 0)
    state->period = 100;
  state->cycle = 0;

  tracker->state = state;

  DEBUG3_VALUELN("MODE: Test pixels. Period=", state->period);

  return true;
}

void setXY(PixelUtil *pixels, const uint8_t x, const uint8_t y,
           const uint32_t color) {
  uint8_t led = (x % 4) + (y % 4) * 4;
  pixels->setPixelRGB(led, color);
}

boolean program_test_pixels(output_hdr_t *output, void *object,
                            program_tracker_t *tracker) {
  mode_test_state_t *state = (mode_test_state_t *)tracker->state;
  unsigned long now = timesync.ms();

  if (now - state->last_change_ms > state->period) {
    static uint8_t x = 0, y = 0;
    setXY((PixelUtil *)object, x, y, pixel_wheel(state->cycle));

    switch (random8() % 4) {
      case 0:
        x += 1;
        break;
      case 1:
        x -= 1;
        break;
      case 2:
        y += 1;
        break;
      case 3:
        y -= 1;
        break;
    }

    state->cycle++;
    state->last_change_ms = now;

    return true;
  }

  return false;
}
