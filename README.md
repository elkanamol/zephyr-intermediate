# Zephyr Training Environment

Welcome to the Zephyr RTOS training! This repository includes a ready-to-use
development environment based on Zephyr **4.4.0**, which you can set up in one of
three ways:

---

## Manual Zephyr Setup

Follow the following guide:
- [Getting Started Guide](https://docs.zephyrproject.org/4.4.0/develop/getting_started/index.html#).

Make sure to select appropriate OS and to perform all steps till
[Build the Blinky Sample](https://docs.zephyrproject.org/4.4.0/develop/getting_started/index.html#build-the-blinky-sample).

## Thread scheduling example

The cooperative thread runs five busy iterations and then yields before
returning. At priority -1, its `k_yield()` only gives ready threads of equal or
higher priority a chance to run; the preemptible threads at priorities 3, 5, and
7 can run after it returns.

The preemptible threads sleep for 100, 200, and 300 ms respectively, which
controls how often they become ready to run. When multiple threads are ready,
the scheduler runs the highest-priority one first (a lower number means a higher
priority), so their output can interleave as they wake up.
