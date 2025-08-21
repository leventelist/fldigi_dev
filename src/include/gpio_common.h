#ifdef __cplusplus
extern "C" {
#endif

#include <gpiod.h>
#include <stdint.h>



typedef uint16_t gpio_num_t;
#define GPIO_COMMON_UNKNOWN           UINT16_MAX
#define GPIO_COMMON_OK                0
#define GPIO_COMMON_ERR               -1


// Public functions

void gpio_common_init(void);
gpio_num_t gpio_common_open_line(const char *chip_name, unsigned int line);
int gpio_common_close(void);
int gpio_common_set(gpio_num_t gpio_num, bool val);

#ifdef __cplusplus
}
#endif