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

void gpio_common_init(void) {
  fprintf(stderr, "Initializing GPIO common structure\n");
  for (gpio_num_t i = 0; i < GPIO_MAX_LINES; i++) {
    gpio[i].used = false;
  }
}


gpio_num_t gpio_common_open_line(const char *chip_name, unsigned int line) {
  gpio_num_t gpio_num;
  int ret;

  struct gpiod_request_config *req_cfg = NULL;
	struct gpiod_line_settings *settings;
	struct gpiod_line_config *line_cfg;
	struct gpiod_chip *chip;

  gpio_num = UINT16_MAX;

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

  if (gpio_num == UINT16_MAX) {
    fprintf(stderr, "Too many GPIOs open.\n");
    goto out;
  }

  chip = gpiod_chip_open(chip_name);

  if (chip == NULL) {
    fprintf(stderr, "Failed to open GPIO chip %s\n", chip_name);
    gpio_num = UINT16_MAX;
    goto out;
  }

  settings = gpiod_line_settings_new();

  if (settings == NULL) {
    fprintf(stderr, "Unable to allocate memory for line settings \n");
    gpio_num = UINT16_MAX;
    goto close_chip;
  }

  gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_output_value(settings, 0);

  line_cfg = gpiod_line_config_new();

  if (!line_cfg) {
    gpio_num = UINT16_MAX;
    goto free_settings;
  }

  ret = gpiod_line_config_add_line_settings(line_cfg, &line, 1,
						  settings);
	if (ret < 0) {
    fprintf(stderr, "Failed to add line settings\n");
    gpio_num = UINT16_MAX;
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
    gpio_num = UINT16_MAX;
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
    return -1;
  }

  if (gpio[gpio_num].request != NULL) {
    gpiod_line_request_release(gpio[gpio_num].request);
    gpio[gpio_num].request = NULL;
  }

  return 0;
}


int gpio_common_set(gpio_num_t gpio_num, bool val) {
  if (gpio_num >= GPIO_MAX_LINES || gpio[gpio_num].request == NULL) {
    return -1;
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
    return -1;
  }
  return 0;
}


int gpio_common_close(void) {

  for (gpio_num_t i = 0; i < GPIO_MAX_LINES; i++) {
    gpio_common_release_line(i);
  }

  return 0;
}
