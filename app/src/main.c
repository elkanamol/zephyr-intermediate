#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/tracing/tracing.h>

LOG_MODULE_REGISTER(homework, LOG_LEVEL_INF);

#define STACK_SIZE 2048
#ifndef CONTROL_PRIORITY
#define CONTROL_PRIORITY 3
#endif
#define MAINTENANCE_PRIORITY 4
#define EVENT_PERIOD_MS 250
#define MAINTENANCE_LOAD_US 45000
#define DEADLINE_MS 10

struct control_event
{
    uint32_t seq;
    uint32_t ready_ms;
};

K_MSGQ_DEFINE(control_queue, sizeof(struct control_event), 4, 4);
K_SEM_DEFINE(maintenance_start, 0, 1);

static uint32_t dropped_events;

static void event_timer_expiry(struct k_timer *timer)
{
    static uint32_t sequence;
    struct control_event event = {
        .seq = sequence++,
        .ready_ms = k_uptime_get_32(),
    };

    ARG_UNUSED(timer);
    sys_trace_named_event("event_ready", event.seq, event.ready_ms);
    LOG_INF("event_ready seq=%u ready_ms=%u", event.seq, event.ready_ms);

    if (k_msgq_put(&control_queue, &event, K_NO_WAIT) != 0)
    {
        dropped_events++;
        LOG_WRN_RATELIMIT("event_dropped seq=%u count=%u", event.seq, dropped_events);
        return;
    }

    k_sem_give(&maintenance_start);
}

K_TIMER_DEFINE(event_timer, event_timer_expiry, NULL);

static void control_fn(void *unused1, void *unused2, void *unused3)
{
    uint32_t deadline_misses = 0;

    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    while (true) {
        struct control_event event;
        int result = k_msgq_get(&control_queue, &event, K_FOREVER);

        if (result != 0) {
            LOG_ERR("control receive failed: %d", result);
            continue;
        }

        uint32_t latency_ms = k_uptime_get_32() - event.ready_ms;

        sys_trace_named_event("event_start", event.seq, latency_ms);
        if (latency_ms > DEADLINE_MS)
        {
            deadline_misses++;
            LOG_WRN_RATELIMIT("deadline_miss count=%u seq=%u latency_ms=%u",
                              deadline_misses, event.seq, latency_ms);
        }
        LOG_INF("event_start seq=%u latency_ms=%u misses=%u",
                event.seq, latency_ms, deadline_misses);
    }
}

static void maintenance_fn(void *unused1, void *unused2, void *unused3)
{
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    while (true)
    {
        k_sem_take(&maintenance_start, K_FOREVER);
        k_busy_wait(MAINTENANCE_LOAD_US);
    }
}

K_THREAD_DEFINE(control, STACK_SIZE, control_fn,
                NULL, NULL, NULL, CONTROL_PRIORITY, 0, 0);
K_THREAD_DEFINE(maintenance, STACK_SIZE, maintenance_fn,
                NULL, NULL, NULL, MAINTENANCE_PRIORITY, 0, 0);

int main(void)
{
    LOG_INF("Control must start within %u ms of event readiness", DEADLINE_MS);
    k_timer_start(&event_timer, K_MSEC(500), K_MSEC(EVENT_PERIOD_MS));
    return 0;
}