#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/display.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(BADGE, LOG_LEVEL_DBG);
const struct device *display_dev;
struct k_work button_work;

// Define work queue for display operations
K_THREAD_STACK_DEFINE(display_stack, 1024);
struct k_work_q display_work_q;

// Structure to hold display text work
struct display_work_item {
    struct k_work work;
    char text[32];
    bool update_text;
};

// Create a single work item that we'll reuse
static struct display_work_item display_item;

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
  cfb_framebuffer_set_font(display_dev, 2);

	printf("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
	       x_res,
	       y_res,
	       ppt,
	       rows,
	       cfb_get_display_parameter(display_dev, CFB_DISPLAY_COLS));

	/* cfb_framebuffer_invert(display_dev); */

	cfb_set_kerning(display_dev, 0);
  return 0;
}

void display_work_handler(struct k_work *work)
{
    struct display_work_item *display_item = CONTAINER_OF(work, struct display_work_item, work);
    int x_res = cfb_get_display_parameter(display_dev, CFB_DISPLAY_WIDTH);
    int text_len = strlen(display_item->text);
    int char_width = 20; // Approximate width of each character
    int total_width = text_len * char_width;
    int x_pos = x_res; // Start from right edge
    
    // Clear the display once at the start
    cfb_framebuffer_clear(display_dev, true);
    
    while (1) {
        // Only clear the area that needs updating
        cfb_framebuffer_clear(display_dev, false);
        
        // Scroll the text from right to left
        while (x_pos > -total_width) {
            // Clear only the area that needs updating
            cfb_framebuffer_clear(display_dev, false);
            cfb_print(display_dev, display_item->text, x_pos, 0);
            cfb_framebuffer_finalize(display_dev);
            x_pos -= 4; // Faster scroll speed
            k_msleep(10); // Shorter delay between updates
        }
        
        // Reset position for next iteration
        x_pos = x_res;
        
        // If text was updated, break the loop to restart with new text
        if (display_item->update_text) {
            display_item->update_text = false;
            // Clear the display for the new text
            cfb_framebuffer_clear(display_dev, true);
            break;
        }
    }
    
    // Resubmit the work to continue scrolling
    k_work_submit_to_queue(&display_work_q, &display_item->work);
}

void display_text(char* text)
{
    // Copy the text to the work item
    strncpy(display_item.text, text, sizeof(display_item.text) - 1);
    display_item.text[sizeof(display_item.text) - 1] = '\0';
    display_item.update_text = true;
    
    // Submit the work to the display queue
    k_work_submit_to_queue(&display_work_q, &display_item.work);
}

void button_pressed(const struct device *dev, struct gpio_callback *cb,
                    uint32_t pins)
{
	int val = gpio_pin_get_dt(&button);
    if (val == 1) {
        // Button pressed (assuming active low)
		  k_work_submit(&button_work);
    } else {
        // Button released
    }

}

int index;
#define MAX_STRINGS 3
char text[MAX_STRINGS][32]= {
  "Anuj Deshpande",
  "anujdeshpande.com",
  "Interested in developer tools!"
};

void button_work_cb(struct k_work *work)
{
  LOG_INF("Button pressed");
  cfb_framebuffer_set_font(display_dev, 2);
  display_text(text[index]);

  index = index + 1;
  if (index == MAX_STRINGS)
    index = 0;
	return;
}

int main(void)
{
  int ret;
  k_work_init(&button_work, button_work_cb);

  // Initialize display work queue
  k_work_queue_init(&display_work_q);
  k_work_queue_start(&display_work_q, display_stack, K_THREAD_STACK_SIZEOF(display_stack),
                    K_PRIO_COOP(7), NULL);
  
  // Initialize display work item
  k_work_init(&display_item.work, display_work_handler);
  display_item.update_text = false;

  display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  if (!device_is_ready(display_dev)) {
    printk("Display_Device %s not ready\n", display_dev->name);
    /* return 0; */
  }

  LOG_INF("SSD1306 device found\n");
  display_init();
  display_text("Makerville Badge! Press button to change text");

	if (!gpio_is_ready_dt(&button)) {
		LOG_INF("Error: button device %s is not ready\n",
		       button.port->name);
	}
  /* TODO Uncommenting everything below this causes issues in I2C */
	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		LOG_INF("Error %d: failed to configure %s pin %d\n",
		       ret, button.port->name, button.pin);
	}
	ret = gpio_pin_interrupt_configure_dt(&button,
                                        GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_INF("Error %d: failed to configure interrupt on %s pin %d\n",
           ret, button.port->name, button.pin);
		return 0;
	}
	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
	LOG_INF("Set up button at %s pin %d\n", button.port->name, button.pin);

  return 0;
}
