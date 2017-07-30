/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2016
 *
 * Wireless pendants
 ******************************************************************************/

#ifndef WIRELESSPENDANT_PENDANT_H
#define WIRELESSPENDANT_PENDANT_H

#include <HMTLTypes.h>

extern config_hdr_t config;
extern output_hdr_t *outputs[HMTL_MAX_OUTPUTS];
extern void *objects[HMTL_MAX_OUTPUTS];

extern RS485Socket rs485;

#endif //WIRELESSPENDANT_PENDANT_H
