/* I2S: per-sample push/pop backed by SPSC rings drained/filled by glo_render(). Part of flono-mobile, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0. */
#include <time.h>
#include "shim_internal.h"

#define RING_CAP 8192 /* power of two, > any ring_frames limit */

typedef struct {
    uint32_t buf[RING_CAP];
    atomic_uint head; /* written by producer */
    atomic_uint tail; /* written by consumer */
} ring_t;

static ring_t out_ring, in_ring;
static atomic_int ring_limit = 512;
static atomic_int ring_floor = 512; /* what glo_render shrinks the ring to while it runs cleanly */
static atomic_int ring_primed; /* the firmware has filled the out ring since the last set_ring_frames */
static atomic_int ring_resets;
atomic_int glo_engine_rate = 50780;

static unsigned ring_count(ring_t *r)
{
    return atomic_load_explicit(&r->head, memory_order_acquire) - atomic_load_explicit(&r->tail, memory_order_acquire);
}

static void ring_put(ring_t *r, uint32_t v)
{
    unsigned h = atomic_load_explicit(&r->head, memory_order_relaxed);
    r->buf[h & (RING_CAP - 1)] = v;
    atomic_store_explicit(&r->head, h + 1, memory_order_release);
}

static int ring_get(ring_t *r, uint32_t *v)
{
    unsigned t = atomic_load_explicit(&r->tail, memory_order_relaxed);
    if (atomic_load_explicit(&r->head, memory_order_acquire) == t)
        return 0;
    *v = r->buf[t & (RING_CAP - 1)];
    atomic_store_explicit(&r->tail, t + 1, memory_order_release);
    return 1;
}

/* Called with all firmware threads gone while glo_render keeps running: only touch the firmware-owned ends.
 * The out ring drains by itself; stale mic input is dropped. */
void glo_i2s_reset(void)
{
    atomic_store_explicit(&in_ring.tail, atomic_load_explicit(&in_ring.head, memory_order_acquire), memory_order_release);
}

static int clamp_ring(int frames)
{
    return frames < 32 ? 32 : frames > RING_CAP - 1 ? RING_CAP - 1 : frames;
}

/* Starts at four times `frames` to be safe on any device; glo_render shrinks it to `frames` while it runs cleanly. */
void glo_i2s_set_ring_frames(int frames)
{
    atomic_store(&ring_floor, clamp_ring(frames));
    atomic_store(&ring_limit, clamp_ring(frames * 4));
    atomic_store(&ring_primed, 0);
    atomic_fetch_add(&ring_resets, 1);
}

int glo_i2s_ring_frames(void) { return atomic_load_explicit(&ring_limit, memory_order_relaxed); }
int glo_i2s_ring_resets(void) { return atomic_load_explicit(&ring_resets, memory_order_relaxed); }
int glo_i2s_ring_floor(void) { return atomic_load_explicit(&ring_floor, memory_order_relaxed); }
void glo_i2s_resize_ring(int frames) { atomic_store_explicit(&ring_limit, clamp_ring(frames), memory_order_relaxed); }
int glo_i2s_ring_primed(void) { return atomic_load_explicit(&ring_primed, memory_order_relaxed); }

/* Called from glo_render: the ring must hold more than one device callback, or every callback underruns. */
void glo_i2s_ensure_ring_frames(int frames)
{
    if (frames > RING_CAP - 1)
        frames = RING_CAP - 1;
    if (atomic_load_explicit(&ring_limit, memory_order_relaxed) < frames)
        atomic_store_explicit(&ring_limit, frames, memory_order_relaxed);
}

int glo_i2s_out_pop(uint32_t *frame) { return ring_get(&out_ring, frame); }

void glo_i2s_in_push(uint32_t frame)
{
    if (ring_count(&in_ring) < RING_CAP - 1)
        ring_put(&in_ring, frame);
}

int i2s_push_sample(i2s_port_t port, const void *sample, TickType_t ticks)
{
    (void)port; (void)ticks;
    const struct timespec nap = { 0, 250000 };
    while (ring_count(&out_ring) >= (unsigned)atomic_load_explicit(&ring_limit, memory_order_relaxed)) {
        atomic_store_explicit(&ring_primed, 1, memory_order_relaxed);
        glo_port_check_exit();
        nanosleep(&nap, NULL);
    }
    glo_port_check_exit();
    uint32_t v;
    memcpy(&v, sample, 4);
    ring_put(&out_ring, v);
    return 4;
}

/* The mic backlog mirrors the output ring: it fills up to the ring's size while the firmware lags, plus the block
 * a channel reads at once (e.g. 100 frames). Only twice that is stale (a channel did not read it): skip to the newest.
 * Every skip is a jump in the mic signal, followed by silence until the firmware catches up. */
static int in_get(uint32_t *v)
{
    unsigned limit = (unsigned)atomic_load_explicit(&ring_limit, memory_order_relaxed);
    if (ring_count(&in_ring) > 2 * limit)
        atomic_store_explicit(&in_ring.tail, atomic_load_explicit(&in_ring.head, memory_order_acquire) - limit,
                              memory_order_release);
    return ring_get(&in_ring, v);
}

int i2s_pop_sample(i2s_port_t port, void *sample, TickType_t ticks)
{
    (void)port; (void)ticks;
    uint32_t v = 0;
    in_get(&v);
    memcpy(sample, &v, 4);
    return 4;
}

esp_err_t i2s_write(i2s_port_t port, const void *src, size_t size, size_t *bytes_written, TickType_t ticks)
{
    for (size_t i = 0; i + 4 <= size; i += 4)
        i2s_push_sample(port, (const uint8_t *)src + i, ticks);
    if (bytes_written)
        *bytes_written = size & ~(size_t)3;
    return ESP_OK;
}

/* Frames the host has not delivered yet read as silence, so the firmware never blocks on the mic. */
esp_err_t i2s_read(i2s_port_t port, void *dest, size_t size, size_t *bytes_read, TickType_t ticks)
{
    (void)port; (void)ticks;
    for (size_t i = 0; i + 4 <= size; i += 4) {
        uint32_t v = 0;
        in_get(&v);
        memcpy((uint8_t *)dest + i, &v, 4);
    }
    if (bytes_read)
        *bytes_read = size & ~(size_t)3;
    return ESP_OK;
}

esp_err_t i2s_driver_install(i2s_port_t port, const i2s_driver_config_t *cfg, int queue_size, void *queue)
{
    (void)port; (void)queue_size; (void)queue;
    if (cfg && cfg->sample_rate > 0)
        atomic_store(&glo_engine_rate, cfg->sample_rate);
    return ESP_OK;
}

esp_err_t i2s_set_sample_rates(i2s_port_t port, uint32_t rate)
{
    (void)port;
    if (rate > 0)
        atomic_store(&glo_engine_rate, (int)rate);
    return ESP_OK;
}

esp_err_t i2s_driver_uninstall(i2s_port_t port) { (void)port; return ESP_OK; }
esp_err_t i2s_set_pin(i2s_port_t port, const i2s_pin_config_t *pins) { (void)port; (void)pins; return ESP_OK; }
esp_err_t i2s_zero_dma_buffer(i2s_port_t port) { (void)port; return ESP_OK; }
esp_err_t i2s_start(i2s_port_t port) { (void)port; return ESP_OK; }
esp_err_t i2s_stop(i2s_port_t port) { (void)port; return ESP_OK; }
