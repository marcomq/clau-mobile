/* I2S: per-sample push/pop backed by SPSC rings drained/filled by glo_render(). Part of clau-mobile, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0. */
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

void glo_i2s_set_ring_frames(int frames)
{
    if (frames < 32)
        frames = 32;
    if (frames > RING_CAP - 1)
        frames = RING_CAP - 1;
    atomic_store(&ring_limit, frames);
}

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
        glo_port_check_exit();
        nanosleep(&nap, NULL);
    }
    glo_port_check_exit();
    uint32_t v;
    memcpy(&v, sample, 4);
    ring_put(&out_ring, v);
    return 4;
}

int i2s_pop_sample(i2s_port_t port, void *sample, TickType_t ticks)
{
    (void)port; (void)ticks;
    uint32_t v = 0;
    ring_get(&in_ring, &v);
    memcpy(sample, &v, 4);
    return 4;
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
