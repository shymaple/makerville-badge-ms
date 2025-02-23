#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/display.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>

LOG_MODULE_REGISTER(BADGE, LOG_LEVEL_DBG);
const struct device *display_dev;

int display_text()
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

	for (int idx = 0; idx < 42; idx++) {
		if (cfb_get_font_size(display_dev, idx, &font_width, &font_height)) {
			break;
		}
		cfb_framebuffer_set_font(display_dev, idx);
		printf("font width %d, font height %d\n",
		       font_width, font_height);
    if (idx==2)
      break;
	}

	printf("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
	       x_res,
	       y_res,
	       ppt,
	       rows,
	       cfb_get_display_parameter(display_dev, CFB_DISPLAY_COLS));

	/* cfb_framebuffer_invert(display_dev); */

	cfb_set_kerning(display_dev, 0);
  cfb_print(display_dev,
            "Anuj D",
            0, 0);
  cfb_framebuffer_finalize(display_dev);

	/* while (1) { */
	/* 	for (int i = 0; i < MIN(x_res, y_res); i++) { */
	/* 		cfb_framebuffer_clear(display_dev, false); */
	/* 		if (cfb_print(display_dev, */
	/* 			      "Anuj D", */
	/* 			      i, i)) { */
	/* 			printf("Failed to print a string\n"); */
	/* 			continue; */
	/* 		} */

	/* 		cfb_framebuffer_finalize(display_dev); */
	/* 	} */
	/* } */
}

int main(void)
{

  display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  if (!device_is_ready(display_dev)) {
    printk("Display_Device %s not ready\n", display_dev->name);
    return 0;
  }

  LOG_INF("SSD1306 device found\n");
  display_text();
  return 0;
}
