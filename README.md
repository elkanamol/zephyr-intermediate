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

## L6 task 1: control response time

Control must start within 10 ms of `event_ready`, measured from the timer
callback's readiness timestamp to `event_start` after the queue receive; this
does not measure completion. Originally, maintenance at priority 4 runs its
45 ms busy wait before control at priority 7. Raising control to priority 3
makes it run first (lower numbers mean higher priority).

With the same workload and detector, the first eight events show 40 ms and
eight misses in the [original output](evidence/l6-task1/original-console.txt),
versus 0 ms and no misses in the
[corrected output](evidence/l6-task1/corrected-console.txt). Both include
`kernel thread list` snapshots. Deferred logs appear twice because two log
backends are enabled.

The Babeltrace scheduler excerpts show the ordering:
[original](evidence/l6-task1/original-scheduler.txt) and
[corrected](evidence/l6-task1/corrected-scheduler.txt). Raw CTF streams are
[original](evidence/l6-task1/original/channel0_0) and
[corrected](evidence/l6-task1/corrected/channel0_0). The captures use
`native_sim/native/64`, nRF Connect SDK v3.3.0 (Zephyr v4.3.99-fd9204a02d52),
and Babeltrace 1.5.11. Both builds use `app/native.conf`; the original uses
`-DCMAKE_C_FLAGS=-DCONTROL_PRIORITY=7`. Decode each stream in a separate
directory alongside the SDK's `subsys/tracing/ctf/tsdl/metadata`.

The 100 Hz uptime clock reports latency in 10 ms steps: the original trace
shows a 45 ms gap, while the detector reports 40 ms. These simulator results
do not establish a worst-case response-time guarantee on hardware, and
quantization can hide a delay just over 10 ms.
