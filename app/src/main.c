#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(BADGE, LOG_LEVEL_DBG);

int main(void)
{
  while(true)
    {
      LOG_INF("Hello World");
      k_msleep(10000);
    }
}

