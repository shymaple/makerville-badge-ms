#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/display.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/smf.h>
#include <zephyr/input/input.h>

static const struct smf_state badge_states[];

enum badge_state {
  BADGE_STATE_INIT,
  BADGE_STATE_IDLE,
  BADGE_STATE_ERROR,
};

enum badge_event {
  BADGE_EVENT_DISPLAY_ERROR,
  BADGE_EVENT_SHORT_BUTTON_PRESS,
  BADGE_EVENT_LONG_BUTTON_PRESS,
};

struct global_context{
  int64_t button_press_time;
} gc;

K_MSGQ_DEFINE(event_msgq, sizeof(enum badge_event), 10,1);

struct s_object {
  struct smf_ctx ctx;
  enum badge_event event;
}s_obj;

LOG_MODULE_REGISTER(BADGE, LOG_LEVEL_DBG);
const struct device *display_dev;

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

/* Send a message to the display thread */
void message_display_thread(char* text)
{
  LOG_INF("Message display thread");
}

void gen_event(enum badge_event event)
{
  k_msgq_put(&event_msgq, (void*)&event, K_NO_WAIT);
}

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

int index;
#define MAX_STRINGS 3
char text[MAX_STRINGS][32]= {
  "Anuj Deshpande",
  "anujdeshpande.com",
  "Interested in developer tools!"
};

void badge_init_entry(void* arg)
{
  LOG_INF("Badge init entry");

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
    smf_set_state(SMF_CTX(&s_obj), &badge_states[BADGE_STATE_ERROR]);
    gen_event(BADGE_EVENT_DISPLAY_ERROR);
    return;
  }

  LOG_INF("SSD1306 device found\n");
  display_init();
  smf_set_state(SMF_CTX(&s_obj), &badge_states[BADGE_STATE_IDLE]);
}

void badge_init_exit(void* arg)
{
  LOG_INF("Badge init exit");
}

void badge_idle_entry(void* arg)
{
  LOG_INF("Badge idle entry");
  display_text("Makerville Badge!");
}

void badge_idle_run(void* arg)
{
  struct s_object *obj = (struct s_object *)arg;

  switch(obj->event) {
    case BADGE_EVENT_SHORT_BUTTON_PRESS:
      LOG_INF("Short button pressed when IDLE");
      display_text("Short button press");
      break;
    case BADGE_EVENT_LONG_BUTTON_PRESS:
      LOG_INF("Long button pressed when IDLE");
      display_text("Long button press");
      break;
    default:
      LOG_INF("Unhandled event in idle state %d", obj->event);
      break;
  }
}

void badge_idle_exit(void* arg)
{
  LOG_INF("Badge idle exit");
}

void badge_error_entry(void* arg)
{
  LOG_INF("Badge error entry");
}

void badge_error_run(void* arg)
{
  LOG_INF("Badge error run");
}

void badge_error_exit(void* arg)
{
  LOG_INF("Badge error exit");
}

static const struct smf_state badge_states[] = {
  [BADGE_STATE_INIT] = SMF_CREATE_STATE(badge_init_entry, NULL, NULL,NULL,NULL),
  [BADGE_STATE_IDLE] = SMF_CREATE_STATE(badge_idle_entry, badge_idle_run, badge_idle_exit,NULL,NULL),
  [BADGE_STATE_ERROR] = SMF_CREATE_STATE(badge_error_entry, badge_error_run, badge_error_exit,NULL,NULL),
};

void input_cb(struct input_event *event, void *cb_arg)
{
    if (event->type == INPUT_EV_KEY) {
        if (event->code == INPUT_KEY_0) {
            if (event->value == 1) {
                // Button pressed
                LOG_INF("Button pressed");
                gc.button_press_time = k_uptime_get();
            } else if (event->value == 0) {
                // Button released
                LOG_INF("Button released");
                int64_t press_duration = k_uptime_get() - gc.button_press_time;
                if(press_duration > 1000) {
                  LOG_INF("Long button press");
                  gen_event(BADGE_EVENT_LONG_BUTTON_PRESS);
                } else {
                  LOG_INF("Short button press");
                  gen_event(BADGE_EVENT_SHORT_BUTTON_PRESS);
                }
            }
        }
    }
}

INPUT_CALLBACK_DEFINE(NULL, input_cb,NULL);
int main(void)
{
  int32_t ret;
  int rc;
  smf_set_initial(SMF_CTX(&s_obj), &badge_states[BADGE_STATE_INIT]);
  while(1) {
    rc = k_msgq_get(&event_msgq, &s_obj.event, K_NO_WAIT);
    if(rc == 0) {
      ret = smf_run_state(SMF_CTX(&s_obj));
      if (ret != 0) {
        LOG_ERR("SMF error: %d", ret);
      }
    }
    k_msleep(10);
  }
  return 0;
}