#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(homework, LOG_LEVEL_DBG);

#define STACK_SIZE 1024
#define SENSOR_MS 100
#define EVENT_COUNT 10
#define BURST_SIZE 5
#define BURST_INTERVAL_MS 4
#define DEBOUNCE_MS 30

static unsigned int processed_events;
static unsigned int processed_bursts;

static void sensor_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    processed_events++;
    LOG_INF("[HANDLER] event %u tick=%u", processed_events, k_uptime_get_32());
}

static void debounce_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    processed_bursts++;
    LOG_INF("[DEBOUNCE] burst %u tick=%u", processed_bursts, k_uptime_get_32());
}

K_WORK_DEFINE(sensor_work, sensor_handler);
K_WORK_DELAYABLE_DEFINE(debounce_work, debounce_handler);

static void sensor_sim_fn(void *unused1, void *unused2, void *unused3)
{
    struct k_work_sync sync;

    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    for (int event = 1; event <= EVENT_COUNT; event++)
    {
        k_msleep(SENSOR_MS);
        LOG_INF("[SENSOR] event %d tick=%u", event, k_uptime_get_32());
        int result = k_work_submit(&sensor_work);
        if (result < 0)
        {
            LOG_ERR("work submission failed: %d", result);
        }
        else if (result == 0)
        {
            LOG_WRN("sensor work was already queued");
        }
    }

    k_work_flush(&sensor_work, &sync);
    LOG_INF("[SUMMARY] events=%d handlers=%u", EVENT_COUNT, processed_events);

    for (int burst = 1; burst <= EVENT_COUNT; burst++)
    {
        k_msleep(SENSOR_MS);
        for (int event = 1; event <= BURST_SIZE; event++)
        {
            LOG_INF("[SENSOR] burst %d event %d tick=%u",
                    burst, event, k_uptime_get_32());
            int result = k_work_reschedule(&debounce_work, K_MSEC(DEBOUNCE_MS));
            if (result < 0)
            {
                LOG_ERR("work reschedule failed: %d", result);
            }
            if (event < BURST_SIZE)
            {
                k_msleep(BURST_INTERVAL_MS);
            }
        }
    }
}

K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_sim_fn, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
    LOG_INF("=== L3 Homework: Event-driven Workqueue ===");
    return 0;
}

