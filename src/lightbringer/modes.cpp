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
        { PENDANT_TEST_PIXELS, program_test_pixels, program_test_pixels_init },
        { TRIANGES_FADE, program_triangles_fade, program_triangles_fade_init },
        { SECTION_TWINKLE, section_twinkle, section_twinkle_init},
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
//  DEBUG4_VALUELN("Init: sparkle ", output);
//  program_sparkle_fmt(rs485.send_buffer, rs485.send_data_size,
//                      config.address, output,
//                      0,0,0,0,0,0,0,0,0,0);

  DEBUG3_VALUELN("Init: triangles fade ", output);
#if 0
  CRGB colors[] = {
          CRGB(00,00,00),
          //CRGB(50,00,00),
          //CRGB(25,25,00),
          //CRGB(00,50,00),
          //CRGB(00,25,25),
          CRGB(00,00,255),
          CRGB(00,00,50),
          CRGB(25,00,25),
          CRGB(00,25,25),
  };
  program_triangles_fade_fmt(rs485.send_buffer, rs485.send_data_size,
                             config.address, output,
                             1000, // Period
                             sizeof(colors) / sizeof(CRGB),
                             0, // Strip length (0 == default)
                             1,
                             colors);
#else
  section_twinkle_fmt(rs485.send_buffer, rs485.send_data_size,
                      config.address, output,
                      1000, // Period MS
                      50,  // Twinkle MS
                      0, // Strip length (0 == default)
                      CRGB(0,0,0), // Background color
                      CRGB(255,255,255) // Foreground color
                      );
#endif

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
                                 output_hdr_t *output, void *object,
                                 ProgramManager *manager) {
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

/******************************************************************************
 * This color mode splits the pixels into segments, and then fades each segment
 * through a defined color sequence.
 */

uint16_t program_triangles_fade_fmt(byte *buffer, uint16_t buffsize,
                                    uint16_t address, uint8_t output,
                                    uint32_t period,
                                    uint8_t  num_colors,
                                    uint8_t  strip_length,
                                    uint8_t  flags,
                                    CRGB     *colors){
  msg_hdr_t *msg_hdr = (msg_hdr_t *)buffer;
  msg_program_t *msg_program = (msg_program_t *)(msg_hdr + 1);

  hmtl_program_fmt(msg_program, output, TRIANGES_FADE, buffsize);

  triangles_fade_msg_t *program =
          (triangles_fade_msg_t *)msg_program->values;

  /* If the other values are set then don't memset */
  memset(program, 0, MAX_PROGRAM_VAL);
  program->period = period;
  program->num_colors = num_colors;
  program->strip_length = strip_length;
  program->flags = flags;
  memcpy(program->colors, colors, sizeof(CRGB) * num_colors);

  hmtl_msg_fmt(msg_hdr, address, HMTL_MSG_PROGRAM_LEN, MSG_TYPE_OUTPUT);
  return HMTL_MSG_PROGRAM_LEN;

}

boolean program_triangles_fade_init(msg_program_t *msg,
                                    program_tracker_t *tracker,
                                    output_hdr_t *output, void *object,
                                    ProgramManager *manager) {
  if ((output == NULL) ||(output->type != HMTL_OUTPUT_PIXELS)) {
    DEBUG4_PRINTLN("traingles_fade: bad config");
    return false;
  }

  DEBUG3_PRINT("Initializing triangle fade program");
  triangles_fade_state_t *state =
          (triangles_fade_state_t *)manager->get_program_state(tracker, sizeof(triangles_fade_state_t));

  DEBUG4_VALUE(" msgsz:", sizeof (state->msg));

  memcpy(&state->msg, msg->values, sizeof (state->msg)); // ??? Correct size?
  if (state->msg.period == 0) state->msg.period = 50;
  if (state->msg.strip_length == 0) state->msg.strip_length = DEFAULT_STRIP_LENGTH;

  state->last_change_ms = 0;
  state->start_color = 0;

  DEBUG4_VALUE(" change_period:", state->msg.period);
  DEBUG4_VALUE(" num_colors:", state->msg.num_colors);
  DEBUG4_VALUE(" strip_length:", state->msg.strip_length);
  DEBUG4_VALUE(" flags:", state->msg.flags);
  for (byte i = 0; i < state->msg.num_colors; i++) {
    DEBUG4_VALUELN(" ", state->msg.colors[i].r);
    DEBUG4_VALUELN(",", state->msg.colors[i].g);
    DEBUG4_VALUELN(",", state->msg.colors[i].b);
  }
  DEBUG_ENDLN();

  return true;
}

/*
 * Divide the pixels into sections of equal length and apply a color to the
 * indicated section.
 */
void set_led_section(PixelUtil *pixels, uint8_t section, uint8_t section_length,
                     CRGB color) {
  pixel_range_t range = {
          .start = (PIXEL_ADDR_TYPE)((PIXEL_ADDR_TYPE)section * section_length),
          .length = (PIXEL_ADDR_TYPE)section_length
  };
  pixels->setRangeRGB(range, color);
}

boolean program_triangles_fade(output_hdr_t *output, void *object,
                               program_tracker_t *tracker) {
  unsigned long now = timesync.ms();
  auto *state = (triangles_fade_state_t *)tracker->state;
  PixelUtil *pixels = (PixelUtil*)object;
  uint8_t color;
  CRGB current;
  fract8 fraction;

  unsigned long elapsed = now - state->last_change_ms;
  if (elapsed >= state->msg.period) {
    /* Advance to the next phase */
    elapsed = state->msg.period;
    state->last_change_ms = now;
    state->start_color++;
  }

  fraction = (fract8)map(elapsed, 0, state->msg.period, 0, 255);

  for (uint8_t section = 0;
       section < pixels->numPixels()/state->msg.strip_length;
       section++) {
    color = (state->start_color + section) % state->msg.num_colors;
    current = blend(state->msg.colors[color],
                    state->msg.colors[(color + 1) % state->msg.num_colors],
                    fraction);
    set_led_section(pixels, section, state->msg.strip_length, current);
  }

  return true;
}

/******************************************************************************
 * This mode splits the pixels into sections, and then flashes the segments
 * between a background and foreground color
 */
uint16_t section_twinkle_fmt(byte *buffer, uint16_t buffsize,
                             uint16_t address, uint8_t output,
                             uint32_t period,
                             uint16_t twinkle_ms,
                             uint8_t  strip_length,
                             CRGB     bgColor,
                             CRGB     fgColor) {
  auto *msg_hdr = (msg_hdr_t *)buffer;
  auto *msg_program = (msg_program_t *)(msg_hdr + 1);

  hmtl_program_fmt(msg_program, output, SECTION_TWINKLE, buffsize);

  auto *program = (section_twinkle_t *)msg_program->values;

  /* If the other values are set then don't memset */
  memset(program, 0, MAX_PROGRAM_VAL);
  program->period_ms = period;
  program->twinkle_ms = twinkle_ms;
  program->strip_length = strip_length;
  program->bgColor = bgColor;
  program->fgColor = fgColor;

  hmtl_msg_fmt(msg_hdr, address, HMTL_MSG_PROGRAM_LEN, MSG_TYPE_OUTPUT);
  return HMTL_MSG_PROGRAM_LEN;
}

boolean section_twinkle_init(msg_program_t *msg,
                             program_tracker_t *tracker,
                             output_hdr_t *output, void *object,
                             ProgramManager *manager) {
  if ((output == NULL) ||(output->type != HMTL_OUTPUT_PIXELS)) {
    DEBUG4_PRINTLN("section_twinkle: bad config");
    return false;
  }
  PixelUtil *pixels = (PixelUtil *) object;
  uint8_t num_sections = pixels->numPixels() / ((section_twinkle_t *)msg)->strip_length;

  DEBUG3_PRINT("Initializing section twinkle program");
  auto *state = (section_twinkle_state_t *)manager->get_program_state(tracker, SECTION_TWINKLE_STATE_SIZE(num_sections));

  DEBUG4_VALUE(" msgsz:", sizeof (state->msg));

  memset(state, 0, SECTION_TWINKLE_STATE_SIZE(num_sections));
  memcpy(&state->msg, msg->values, sizeof(state->msg));
  if (state->msg.period_ms == 0) state->msg.period_ms = 1000;
  if (state->msg.twinkle_ms == 0) state->msg.twinkle_ms = (uint16_t)(state->msg.period_ms / 10);
  if (state->msg.strip_length == 0) state->msg.strip_length = DEFAULT_STRIP_LENGTH;
  state->previous_time = timesync.ms();

  DEBUG4_VALUE(" period:", state->msg.period_ms);
  DEBUG4_VALUE(" twinkle_ms:", state->msg.twinkle_ms);
  DEBUG4_VALUE(" strip_length:", state->msg.strip_length);
  DEBUG4_VALUE(" bg:", state->msg.bgColor.r);
  DEBUG4_VALUE(",", state->msg.bgColor.g);
  DEBUG4_VALUE(",", state->msg.bgColor.b);
  DEBUG4_VALUE(" fg:", state->msg.fgColor.r);
  DEBUG4_VALUE(",", state->msg.fgColor.g);
  DEBUG4_VALUE(",", state->msg.fgColor.b);
  DEBUG_ENDLN();

  return true;
}

boolean section_twinkle(output_hdr_t *output, void *object,
                        program_tracker_t *tracker) {
  unsigned long now = timesync.ms();
  auto *state = (section_twinkle_state_t *) tracker->state;
  unsigned long elapsed = now - state->previous_time;
  PixelUtil *pixels = (PixelUtil *) object;
  CRGB current;

  for (uint8_t section = 0;
       section < pixels->numPixels() / state->msg.strip_length;
       section++) {
    bool twinkle = false;
    /*
     * If the section is not currently twinkling then determine if it should
     * start.
     */
    if (state->start_periods[section] < now - state->msg.twinkle_ms * 2) {
      /* Not currently twinkling */
      if (random(state->msg.period_ms) < elapsed) {
        state->start_periods[section] = now;
        twinkle = true;
        DEBUG4_VALUE("twinkle: start:", section);
      }
    } else {
      /* Currently twinkling */
      twinkle = true;
      DEBUG4_VALUE("twinkle: on:", section);
    }

    // If this section is twinkling then set its color
    if (twinkle) {
      unsigned long section_elapsed = now - state->start_periods[section];
      fract8 fraction;

      if (section_elapsed < state->msg.twinkle_ms) {
        // Fading up
        fraction = (fract8)map(section_elapsed, 0, state->msg.twinkle_ms, 0, 255);
        DEBUG4_VALUE(" up:", fraction);
      } else {
        // Fading down
        fraction = (fract8)map(section_elapsed - state->msg.twinkle_ms, 0, state->msg.twinkle_ms, 255, 0);
        DEBUG4_VALUE(" down:", fraction);
      }

      current = blend(state->msg.bgColor, state->msg.fgColor, fraction);
      set_led_section(pixels, section, state->msg.strip_length, current);
    } else {
      set_led_section(pixels, section, state->msg.strip_length, state->msg.bgColor);
    }

    DEBUG_ENDLN();
  }

  state->previous_time = now;

  return true;
}