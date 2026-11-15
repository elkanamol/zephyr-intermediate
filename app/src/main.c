#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 1024
#define THREAD_PRIORITY 5
#define ITERATIONS 1000

K_THREAD_STACK_DEFINE(stack_one, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_two, STACK_SIZE);
K_MUTEX_DEFINE(counter_mutex);

static struct k_thread thread_one;
static struct k_thread thread_two;
static volatile unsigned int counter;

static void increment_counter(void *protected, void *unused1, void *unused2)
{
    bool use_mutex = protected != NULL;

    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);

    for (int iteration = 0; iteration < ITERATIONS; iteration++)
    {
        if (use_mutex)
        {
            k_mutex_lock(&counter_mutex, K_FOREVER);
        }

        unsigned int value = counter;
        k_yield();
        counter = value + 1;

        if (use_mutex)
        {
            k_mutex_unlock(&counter_mutex);
        }
    }
}

static void run_test(bool use_mutex)
{
    counter = 0;

    k_thread_create(&thread_one, stack_one, STACK_SIZE, increment_counter,
                    use_mutex ? &counter_mutex : NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_create(&thread_two, stack_two, STACK_SIZE, increment_counter,
                    use_mutex ? &counter_mutex : NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);

    k_thread_join(&thread_one, K_FOREVER);
    k_thread_join(&thread_two, K_FOREVER);

    printk("%s: expected %u, got %u\n",
           use_mutex ? "With mutex" : "Without mutex", 2U * ITERATIONS, counter);
}

int main(void)
{
    run_test(false);
    run_test(true);
    return 0;
}

