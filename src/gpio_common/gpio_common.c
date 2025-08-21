// ----------------------------------------------------------------------------
// gpio_common.c  --  GPIO control functions
//
// Copyright (C) 2025
//		Dave Freese, W1HKJ
//		   (C) Mauri Niininen, AG1LE
//    Levente Kovacs, HA5OGL
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code
// distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
//  Copyright (C) 2004
//  Lawrence Glaister (ve7it@shaw.ca)
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------


#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>

#include "gpio_common.h"

#define GPIO_MAX_LINES 32
#define GPIO_CONSUMER "FLDIGI"

//Types
typedef struct gpio_common {
  struct gpiod_line_request *request;
  unsigned int offset;
  bool used;
} gpio_common_t;

// local variables

static gpio_common_t gpio[GPIO_MAX_LINES];

// Function implementations

void gpio_common_init(void) {
  fprintf(stderr, "Initializing GPIO common structure\n");
  for (gpio_num_t i = 0; i < GPIO_MAX_LINES; i++) {
    gpio[i].used = false;
  }
}


gpio_num_t gpio_common_open_line(const char *chip_name, unsigned int line, bool active_low) {
  gpio_num_t gpio_num;
  int ret;

  struct gpiod_request_config *req_cfg = NULL;
	struct gpiod_line_settings *settings;
	struct gpiod_line_config *line_cfg;
	struct gpiod_chip *chip;

  gpio_num = GPIO_COMMON_UNKNOWN;

  if (chip_name == NULL) {
    fprintf(stderr, "No chip name supplied.\n");
    goto out;
  }

  fprintf(stderr, "Opening GPIO line %d on chip %s\n", line, chip_name);

  // Get a free slot
  for (gpio_num_t i = 0; i < GPIO_MAX_LINES; i++) {
    if (gpio[i].used == false) {
      gpio_num = i;
      break;
    }
  }

  if (gpio_num == GPIO_COMMON_UNKNOWN) {
    fprintf(stderr, "Too many GPIOs open.\n");
    goto out;
  }

  chip = gpiod_chip_open(chip_name);

  if (chip == NULL) {
    fprintf(stderr, "Failed to open GPIO chip %s\n", chip_name);
    gpio_num = GPIO_COMMON_UNKNOWN;
    goto out;
  }

  settings = gpiod_line_settings_new();

  if (settings == NULL) {
    fprintf(stderr, "Unable to allocate memory for line settings \n");
    gpio_num = GPIO_COMMON_UNKNOWN;
    goto close_chip;
  }

  gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_output_value(settings, 0);
  gpiod_line_settings_set_active_low(settings, active_low);

  line_cfg = gpiod_line_config_new();

  if (!line_cfg) {
    gpio_num = GPIO_COMMON_UNKNOWN;
    goto free_settings;
  }

  ret = gpiod_line_config_add_line_settings(line_cfg, &line, 1,
						  settings);
	if (ret < 0) {
    fprintf(stderr, "Failed to add line settings\n");
    gpio_num = GPIO_COMMON_UNKNOWN;
    goto free_line_config;
  }

  req_cfg = gpiod_request_config_new();
	if (!req_cfg) {
		goto free_line_config;
  }

  gpiod_request_config_set_consumer(req_cfg, "FLDIGI");

  gpio[gpio_num].request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

  if (gpio[gpio_num].request == NULL) {
    fprintf(stderr, "Failed to request GPIO line %d\n", gpio_num);
    gpio_num = GPIO_COMMON_UNKNOWN;
    goto free_line_config;
  } else {
    gpio[gpio_num].used = true;
    gpio[gpio_num].offset = line;
  }

free_line_config:
	gpiod_line_config_free(line_cfg);

free_settings:
	gpiod_line_settings_free(settings);

close_chip:
	gpiod_chip_close(chip);


out:

  return gpio_num;
}


int gpio_common_release_line(gpio_num_t gpio_num) {
  if (gpio_num >= GPIO_MAX_LINES) {
    return GPIO_COMMON_ERR;
  }

  if (gpio[gpio_num].request != NULL) {
    gpiod_line_request_release(gpio[gpio_num].request);
    gpio[gpio_num].request = NULL;
  }

  return GPIO_COMMON_OK;
}


int gpio_common_set(gpio_num_t gpio_num, bool val) {
  if (gpio_num >= GPIO_MAX_LINES || gpio[gpio_num].request == NULL) {
    return GPIO_COMMON_ERR;
  }
  uint16_t gpiod_val;

  if (val) {
    gpiod_val = GPIOD_LINE_VALUE_ACTIVE;
  } else {
    gpiod_val = GPIOD_LINE_VALUE_INACTIVE;
  }


  int ret = gpiod_line_request_set_value(gpio[gpio_num].request, gpio[gpio_num].offset, gpiod_val);
  if (ret < 0) {
    fprintf(stderr, "Error setting line\n");
    return GPIO_COMMON_ERR;
  }
  return GPIO_COMMON_OK;
}


int gpio_common_close(void) {

  for (gpio_num_t i = 0; i < GPIO_MAX_LINES; i++) {
    gpio_common_release_line(i);
  }

  return GPIO_COMMON_OK;
}
