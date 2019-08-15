/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2016
 *
 * Message handling and pendant modes
 ******************************************************************************/

#ifndef LIGHTBRINGER_MODES_H
#define LIGHTBRINGER_MODES_H

#include <HMTLPrograms.h>

/* Initialize the message and mode handlers */
void init_modes(Socket **sockets, byte num_sockets);

/* Check for messages and handle program modes */
boolean messages_and_modes(void);

/*******************************************************************************
 * Lightbringer-specific program modes
 */

/* Wireless pendant programs */
#define PENDANT_TEST_PIXELS 0x20
#define TRIANGES_FADE       0x21



/* Mode program prototypes */
boolean program_test_pixels_init(msg_program_t *msg,
                                 program_tracker_t *tracker,
                                 output_hdr_t *output, void *object,
                                 ProgramManager *manager);

boolean program_test_pixels(output_hdr_t *output, void *object,
                            program_tracker_t *tracker);



#define DEFAULT_STRIP_LENGTH 69

typedef struct __attribute__((packed)) {
  uint32_t period;         //  4B - Milliseconds for fade to next color
  uint8_t  num_colors;     //  1B - Number of colors that are defined
  uint8_t  strip_length;   //  1B - Number of LEDs in each color strip
  uint8_t  flags;          //  1B
  CRGB     colors[8];      // 24B (3x8)
} triangles_fade_msg_t;    // 31B Total (32B limit)

typedef struct {
  triangles_fade_msg_t msg;
  unsigned long last_change_ms;
  uint8_t start_color;
} triangles_fade_state_t;

boolean program_triangles_fade_init(msg_program_t *msg,
                                    program_tracker_t *tracker,
                                    output_hdr_t *output, void *object,
                                    ProgramManager *manager);

boolean program_triangles_fade(output_hdr_t *output, void *object,
                               program_tracker_t *tracker);
uint16_t program_triangles_fade_fmt(byte *buffer, uint16_t buffsize,
                                    uint16_t address, uint8_t output,
                                    uint32_t period,
                                    uint8_t  num_colors,
                                    uint8_t  strip_length,
                                    uint8_t  flags,
                                    CRGB     *colors);


#endif // LIGHTBRINGER_MODES_H
