#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/task_wdt/task_wdt.h>

LOG_MODULE_REGISTER(reliability_demo, LOG_LEVEL_INF);

#define STACK_SIZE 2048
#define QUEUE_DEPTH 8
#define PRODUCER_PERIOD_MS 100
#define HEALTH_PERIOD_MS 100
#define WATCHDOG_TIMEOUT_MS 1200
#define STALL_AFTER_SAMPLES 5
#define STALL_DURATION_MS 3000

struct sensor_sample
{
    uint32_t sequence;
    uint32_t timestamp_ms;
    int32_t value;
};

K_MSGQ_DEFINE(sensor_queue, sizeof(struct sensor_sample), QUEUE_DEPTH, 4);

static int consumer_watchdog;

static void watchdog_callback(int channel_id, void *user_data)
{
    ARG_UNUSED(user_data);
    LOG_ERR("[WATCHDOG] consumer timed out, channel=%d tick=%u",
            channel_id, k_uptime_get_32());
}

static void producer_fn(void *unused1, void *unused2, void *unused3)
{
    uint32_t sequence = 0;

    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    while (true) {
        struct sensor_sample sample = {
            .sequence = sequence,
            .timestamp_ms = k_uptime_get_32(),
            .value = 100 + (int32_t)(sequence % 100U),
        };
        int result = k_msgq_put(&sensor_queue, &sample, K_NO_WAIT);

        if (result != 0) {
            LOG_WRN("[PRODUCER] queue full, dropped seq=%u", sequence);
        }
        else
        {
            LOG_INF("[PRODUCER] seq=%u queue=%u/%u", sequence,
                    k_msgq_num_used_get(&sensor_queue), QUEUE_DEPTH);
        }
        sequence++;
        k_msleep(PRODUCER_PERIOD_MS);
    }
}

static void consumer_fn(void *unused1, void *unused2, void *unused3)
{
    unsigned int processed = 0;

    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    while (true) {
        struct sensor_sample sample;
        int result = k_msgq_get(&sensor_queue, &sample, K_FOREVER);

        if (result != 0) {
            LOG_ERR("[CONSUMER] receive failed: %d", result);
            continue;
        }
        result = task_wdt_feed(consumer_watchdog);
        if (result != 0)
        {
            LOG_ERR("[CONSUMER] watchdog feed failed: %d", result);
        }
        processed++;
        LOG_INF("[CONSUMER] seq=%u value=%d age=%u ms", sample.sequence,
                sample.value, k_uptime_get_32() - sample.timestamp_ms);

        if (processed == STALL_AFTER_SAMPLES)
        {
            LOG_WRN("[CONSUMER] simulating a %u ms stall", STALL_DURATION_MS);
            k_msleep(STALL_DURATION_MS);
            LOG_INF("[CONSUMER] resumed");
        }
    }
}

static void health_fn(void *unused1, void *unused2, void *unused3)
{
    bool high_water = false;

    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    while (true)
    {
        uint32_t used = k_msgq_num_used_get(&sensor_queue);

        if (used * 4U >= QUEUE_DEPTH * 3U)
        {
            if (!high_water)
            {
                LOG_WRN("[HEALTH] queue at or above 75%%: %u/%u", used, QUEUE_DEPTH);
                high_water = true;
            }
        }
        else if (high_water)
        {
            LOG_INF("[HEALTH] queue recovered: %u/%u", used, QUEUE_DEPTH);
            high_water = false;
        }
        k_msleep(HEALTH_PERIOD_MS);
    }
}

K_THREAD_DEFINE(producer_thread, STACK_SIZE, producer_fn,
                NULL, NULL, NULL, 5, 0, -1);
K_THREAD_DEFINE(consumer_thread, STACK_SIZE, consumer_fn,
                NULL, NULL, NULL, 5, 0, -1);
K_THREAD_DEFINE(health_thread, STACK_SIZE, health_fn,
                NULL, NULL, NULL, 6, 0, -1);

int main(void)
{
    int result = task_wdt_init(NULL);

    if (result != 0)
    {
        LOG_ERR("[WATCHDOG] initialization failed: %d", result);
        return result;
    }

    consumer_watchdog = task_wdt_add(WATCHDOG_TIMEOUT_MS, watchdog_callback, NULL);
    if (consumer_watchdog < 0)
    {
        LOG_ERR("[WATCHDOG] channel creation failed: %d", consumer_watchdog);
        return consumer_watchdog;
    }

    LOG_INF("=== Queue pressure and task watchdog demo ===");
    k_thread_start(consumer_thread);
    k_thread_start(producer_thread);
    k_thread_start(health_thread);
    return 0;
}

