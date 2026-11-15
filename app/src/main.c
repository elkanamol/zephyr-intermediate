#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

LOG_MODULE_REGISTER(sensor_demo, LOG_LEVEL_INF);

#define STACK_SIZE 2048
#define SENSOR_PERIOD_MS 100
#define LOGGER_PERIOD_MS 500
#define LOGGER_QUEUE_DEPTH 8

struct sensor_data {
    uint32_t sequence;
    uint32_t timestamp_ms;
    int32_t temperature_mc;
};

static void display_listener(const struct zbus_channel *channel);

ZBUS_LISTENER_DEFINE(display, display_listener);
ZBUS_SUBSCRIBER_DEFINE(logger, LOGGER_QUEUE_DEPTH);

ZBUS_CHAN_DEFINE(sensor_channel, struct sensor_data, NULL, NULL,
                 ZBUS_OBSERVERS(display, logger),
                 ZBUS_MSG_INIT(.sequence = 0, .timestamp_ms = 0,
                               .temperature_mc = 0));

static void display_listener(const struct zbus_channel *channel)
{
    const struct sensor_data *sample = zbus_chan_const_msg(channel);

    LOG_INF("[DISPLAY] seq=%u temp=%d mC tick=%u",
            sample->sequence, sample->temperature_mc, sample->timestamp_ms);
}

static void sensor_sim_fn(void *unused1, void *unused2, void *unused3)
{
    uint32_t sequence = 0;

    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    while (true) {
        struct sensor_data sample = {
            .sequence = sequence,
            .timestamp_ms = k_uptime_get_32(),
            .temperature_mc = 24000 + (int32_t)(sequence % 200U) * 50,
        };
        int result = zbus_chan_pub(&sensor_channel, &sample, K_NO_WAIT);

        if (result != 0) {
            LOG_WRN("[SENSOR] publish seq=%u failed: %d", sequence, result);
        }
        sequence++;
        k_msleep(SENSOR_PERIOD_MS);
    }
}

static void logger_fn(void *unused1, void *unused2, void *unused3)
{
    const struct zbus_channel *channel;

    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    while (true) {
        int result = zbus_sub_wait(&logger, &channel, K_FOREVER);

        if (result != 0) {
            LOG_WRN("[LOGGER] wait failed: %d", result);
            continue;
        }
        while (zbus_sub_wait(&logger, &channel, K_NO_WAIT) == 0) {
        }

        struct sensor_data sample;

        result = zbus_chan_read(channel, &sample, K_MSEC(10));
        if (result == 0) {
            LOG_INF("[LOGGER] seq=%u temp=%d mC sample_tick=%u log_tick=%u",
                    sample.sequence, sample.temperature_mc,
                    sample.timestamp_ms, k_uptime_get_32());
        } else {
            LOG_WRN("[LOGGER] read failed: %d", result);
        }

        k_msleep(LOGGER_PERIOD_MS);
    }
}

K_THREAD_DEFINE(sensor_thread, STACK_SIZE, sensor_sim_fn,
                NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(logger_thread, STACK_SIZE, logger_fn,
                NULL, NULL, NULL, 6, 0, 0);

int main(void)
{
    LOG_INF("=== Zbus sensor demo ===");
    return 0;
}

