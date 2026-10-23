/* FreeRTOS tasks on pthreads, cooperative exit for esp_restart()/power-off. Part of flono, Copyright (C) 2026 Marco Mengelkoch, GPL-3.0. */
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#ifdef __ANDROID__
#include <sys/resource.h>
#endif
#include <sys/time.h>
#include <time.h>
#include "shim_internal.h"
#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#endif

atomic_int glo_kill;
atomic_int glo_powered_off;

static pthread_mutex_t threads_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t threads_cond = PTHREAD_COND_INITIALIZER;
static int threads_alive;
static _Thread_local int is_fw_thread;
static struct timespec boot_time;

typedef struct {
    TaskFunction_t fn;
    void *param;
} task_start_t;

static void now(struct timespec *ts) { clock_gettime(CLOCK_MONOTONIC, ts); }

static int64_t us_since_boot(void)
{
    struct timespec ts;
    now(&ts);
    return (int64_t)(ts.tv_sec - boot_time.tv_sec) * 1000000 + (ts.tv_nsec - boot_time.tv_nsec) / 1000;
}

void glo_rtos_reset_clock(void) { now(&boot_time); }

static void thread_done(void)
{
    pthread_mutex_lock(&threads_lock);
    threads_alive--;
    pthread_cond_broadcast(&threads_cond);
    pthread_mutex_unlock(&threads_lock);
}

static _Noreturn void exit_fw_thread(void)
{
    thread_done();
    pthread_exit(NULL);
}

void glo_port_check_exit(void)
{
    if (is_fw_thread && atomic_load_explicit(&glo_kill, memory_order_relaxed))
        exit_fw_thread();
}

void glo_request_kill(int power_off)
{
    if (power_off)
        atomic_store(&glo_powered_off, 1);
    atomic_store(&glo_kill, 1);
}

static void *task_trampoline(void *arg)
{
    task_start_t start = *(task_start_t *)arg;
    free(arg);
    is_fw_thread = 1;
    start.fn(start.param);
    thread_done();
    return NULL;
}

static int spawn(TaskFunction_t fn, void *param, size_t stack, pthread_t *out)
{
    task_start_t *start = malloc(sizeof *start);
    if (!start)
        return ENOMEM;
    start->fn = fn;
    start->param = param;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, stack);

    pthread_mutex_lock(&threads_lock);
    threads_alive++;
    pthread_mutex_unlock(&threads_lock);

    pthread_t th;
    int rc = pthread_create(&th, &attr, task_trampoline, start);
    pthread_attr_destroy(&attr);
    if (rc != 0) {
        free(start);
        thread_done();
        return rc;
    }
    if (out)
        *out = th;
    return 0;
}

static void app_entry(void *fn)
{
#ifdef __ANDROID__
    setpriority(PRIO_PROCESS, 0, -16); /* THREAD_PRIORITY_URGENT_AUDIO: app_main renders every sample */
#elif defined(__APPLE__)
    /* Real-time like a CoreAudio thread: an ordinary thread's short sleeps can last 30 ms and drain the ring. */
    mach_timebase_info_data_t tb;
    mach_timebase_info(&tb);
    uint32_t ms = (uint32_t)(1000000.0 * tb.denom / tb.numer);
    thread_time_constraint_policy_data_t rt = { .period = 2 * ms, .computation = ms, .constraint = 2 * ms, .preemptible = 1 };
    thread_policy_set(pthread_mach_thread_np(pthread_self()), THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&rt,
                      THREAD_TIME_CONSTRAINT_POLICY_COUNT);
#endif
    ((void (*)(void))fn)();
}

void glo_rtos_spawn_app(void (*entry)(void))
{
    spawn(app_entry, (void *)entry, 8 * 1024 * 1024, NULL);
}

void glo_rtos_wait_all_exited(void)
{
    pthread_mutex_lock(&threads_lock);
    while (threads_alive > 0)
        pthread_cond_wait(&threads_cond, &threads_lock);
    pthread_mutex_unlock(&threads_lock);
}

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t fn, const char *name, uint32_t stack, void *param,
                                   UBaseType_t prio, TaskHandle_t *handle, BaseType_t core)
{
    (void)name; (void)stack; (void)prio; (void)core;
    pthread_t th;
    if (spawn(fn, param, 1024 * 1024, &th) != 0)
        return pdFAIL;
    if (handle)
        *handle = (TaskHandle_t)(uintptr_t)th;
    return pdPASS;
}

BaseType_t xTaskCreate(TaskFunction_t fn, const char *name, uint32_t stack, void *param,
                       UBaseType_t prio, TaskHandle_t *handle)
{
    return xTaskCreatePinnedToCore(fn, name, stack, param, prio, handle, tskNO_AFFINITY);
}

void vTaskDelete(TaskHandle_t handle)
{
    if (handle == NULL || handle == (TaskHandle_t)(uintptr_t)pthread_self())
        exit_fw_thread();
}

void vTaskDelay(const TickType_t ticks)
{
    glo_port_check_exit();
    struct timespec deadline, ts;
    now(&deadline);
    deadline.tv_sec += ticks / 1000;
    deadline.tv_nsec += (long)(ticks % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    for (;;) {
        now(&ts);
        int64_t left_ns = (int64_t)(deadline.tv_sec - ts.tv_sec) * 1000000000LL + (deadline.tv_nsec - ts.tv_nsec);
        if (left_ns <= 0)
            break;
        if (left_ns > 10000000LL)
            left_ns = 10000000LL;
        struct timespec nap = { 0, (long)left_ns };
        nanosleep(&nap, NULL);
        glo_port_check_exit();
    }
}

void glo_task_yield(void)
{
    glo_port_check_exit();
    sched_yield();
}

TickType_t xTaskGetTickCount(void) { return (TickType_t)(us_since_boot() / 1000); }
TaskHandle_t xTaskGetCurrentTaskHandle(void) { return (TaskHandle_t)(uintptr_t)pthread_self(); }
TaskHandle_t xTaskGetIdleTaskHandleForCPU(UBaseType_t cpu) { return (TaskHandle_t)(uintptr_t)(cpu + 1); }
BaseType_t xPortGetCoreID(void) { return 0; }
size_t xPortGetFreeHeapSize(void) { return 160 * 1024; }
size_t esp_get_free_heap_size(void) { return 160 * 1024; }
size_t heap_caps_get_largest_free_block(uint32_t caps) { (void)caps; return 0; }

int64_t esp_timer_get_time(void) { return us_since_boot(); }
uint32_t esp_log_timestamp(void) { return (uint32_t)(us_since_boot() / 1000); }

int glo_gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;
    int64_t us = us_since_boot();
    tv->tv_sec = (time_t)(us / 1000000);
    tv->tv_usec = (suseconds_t)(us % 1000000);
    return 0;
}
