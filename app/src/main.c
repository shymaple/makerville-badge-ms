#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/display.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(BADGE, LOG_LEVEL_DBG);
const struct device *display_dev;

#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
                                                              {0});
static struct gpio_callback button_cb_data;

int display_init()
{
  uint16_t x_res;
	uint16_t y_res;
	uint16_t rows;
	uint8_t ppt;
	uint8_t font_width;
	uint8_t font_height;


	if (display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO10) != 0) {
		if (display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO01) != 0) {
			printf("Failed to set required pixel format");
			return 0;
		}
	}

	printf("Initialized %s\n", display_dev->name);

	if (cfb_framebuffer_init(display_dev)) {
		printf("Framebuffer initialization failed!\n");
		return 0;
	}

	cfb_framebuffer_clear(display_dev, true);

	display_blanking_off(display_dev);

	x_res = cfb_get_display_parameter(display_dev, CFB_DISPLAY_WIDTH);
	y_res = cfb_get_display_parameter(display_dev, CFB_DISPLAY_HEIGH);
	rows = cfb_get_display_parameter(display_dev, CFB_DISPLAY_ROWS);
	ppt = cfb_get_display_parameter(display_dev, CFB_DISPLAY_PPT);

	/* for (int idx = 0; idx < 42; idx++) { */
	/* 	if (cfb_get_font_size(display_dev, idx, &font_width, &font_height)) { */
	/* 		break; */
	/* 	} */
	/* 	cfb_framebuffer_set_font(display_dev, idx); */
	/* 	printf("font width %d, font height %d\n", */
	/* 	       font_width, font_height); */
  /*   if (idx==2) */
  /*     break; */
	/* } */
  cfb_framebuffer_set_font(display_dev, 0);

	printf("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
	       x_res,
	       y_res,
	       ppt,
	       rows,
	       cfb_get_display_parameter(display_dev, CFB_DISPLAY_COLS));

	/* cfb_framebuffer_invert(display_dev); */

	cfb_set_kerning(display_dev, 0);
  cfb_print(display_dev,
            "ANUJ        DESHPANDE",
            0, 0);
  cfb_framebuffer_finalize(display_dev);
}

int count = 0;
void button_pressed(const struct device *dev, struct gpio_callback *cb,
                    uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
  /* char text[32]; */
  /* sprintf(text,"Anuj %d",count++); */
  /* display_text(text); */
}


int main(void)
{
  int ret;
  display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  if (!device_is_ready(display_dev)) {
    printk("Display_Device %s not ready\n", display_dev->name);
    /* return 0; */
  }

  LOG_INF("SSD1306 device found\n");
  display_init();

	if (!gpio_is_ready_dt(&button)) {
		LOG_INF("Error: button device %s is not ready\n",
		       button.port->name);
	}
  /* TODO Uncommenting everything below this causes issues in I2C */
	/* ret = gpio_pin_configure_dt(&button, GPIO_INPUT); */
	/* if (ret != 0) { */
	/* 	LOG_INF("Error %d: failed to configure %s pin %d\n", */
	/* 	       ret, button.port->name, button.pin); */
	/* } */
	/* ret = gpio_pin_interrupt_configure_dt(&button, */
  /*                                       GPIO_INT_EDGE_TO_ACTIVE); */
	/* if (ret != 0) { */
	/* 	LOG_INF("Error %d: failed to configure interrupt on %s pin %d\n", */
  /*          ret, button.port->name, button.pin); */
	/* 	return 0; */
	/* } */
	/* gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin)); */
	/* gpio_add_callback(button.port, &button_cb_data); */
	/* LOG_INF("Set up button at %s pin %d\n", button.port->name, button.pin); */

  /* k_msleep(1000); */

  return 0;
}
