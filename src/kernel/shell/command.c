/**
 * @file command.c
 * @brief Shell command queue, worker, dispatcher, and lifecycle.
 * @author friedrichOsDev
 */
#include "_shell.h"
#include "command/_command.h"
#include <sched/sched.h>
#include <sched/thread.h>
#include <shell/output.h>

typedef struct {
    uint32_t input[SHELL_INPUT_BUFFER_SIZE];
    size_t length;
} shell_command_request_t;

/*
 * The queue is deliberately SPSC:
 * - producer: the single input_dispatcher_thread callback
 * - consumer: the single shell_command worker
 *
 * Monotonic head/tail counters make all four slots usable. Release/acquire
 * publication guarantees the slot contents are visible before the consumer
 * observes the updated tail. No spinlock is needed in the input path.
 */
typedef struct {
    shell_command_request_t requests[SHELL_COMMAND_QUEUE_SIZE];
    size_t head;
    size_t tail;
    bool accepting;
} shell_command_queue_t;

_Static_assert(SHELL_INPUT_BUFFER_SIZE == SHELL_COMMAND_MAX_LINE_CODEPOINTS,
               "Shell input and command parser limits must match");

static shell_command_queue_t command_queue;
/* Scratch storage is static so the worker does not consume most of its 16 KiB stack. */
static uint32_t worker_input[SHELL_INPUT_BUFFER_SIZE];
static size_t worker_input_length = 0U;
static shell_parse_result_t worker_parse_result;
static bool command_initialized = false;
static bool command_stop_requested = false;
static bool command_worker_stopped = true;

/** @brief True after the worker has stopped accessing command subsystem state. */
bool shell_command_is_stopped(void) {
    return __atomic_load_n(&command_worker_stopped, __ATOMIC_ACQUIRE);
}

/**
 * @brief Submit an immutable line snapshot to the non-blocking SPSC FIFO.
 *
 * This function must be called by the single input-dispatcher producer only.
 * It performs no allocation, lock acquisition, parsing, or command execution.
 */
kerr_t shell_command_submit(const uint32_t *input, size_t length) {
    if (!input && length != 0U) return E_BADADDR;
    if (length > SHELL_INPUT_BUFFER_SIZE) return E_BADSIZE;
    if (!__atomic_load_n(&command_initialized, __ATOMIC_ACQUIRE)) return E_NOINIT;
    if (length == 0U) return E_OK;
    if (!__atomic_load_n(&command_queue.accepting, __ATOMIC_ACQUIRE)) return E_NOINIT;

    size_t tail = __atomic_load_n(&command_queue.tail, __ATOMIC_RELAXED);
    size_t head = __atomic_load_n(&command_queue.head, __ATOMIC_ACQUIRE);

    if ((tail - head) >= SHELL_COMMAND_QUEUE_SIZE) {
        return E_NOMEM;
    }

    shell_command_request_t *request =
        &command_queue.requests[tail % SHELL_COMMAND_QUEUE_SIZE];
    for (size_t i = 0U; i < length; i++) request->input[i] = input[i];
    request->length = length;

    /* Publish the complete slot only after its contents have been copied. */
    __atomic_store_n(&command_queue.tail, tail + 1U, __ATOMIC_RELEASE);
    return E_OK;
}

/** @brief Dequeue one request into worker-private storage. */
static bool shell_command_queue_dequeue(void) {
    size_t head = __atomic_load_n(&command_queue.head, __ATOMIC_RELAXED);
    size_t tail = __atomic_load_n(&command_queue.tail, __ATOMIC_ACQUIRE);

    if (head == tail) return false;

    const shell_command_request_t *request =
        &command_queue.requests[head % SHELL_COMMAND_QUEUE_SIZE];
    worker_input_length = request->length;
    for (size_t i = 0U; i < worker_input_length; i++) {
        worker_input[i] = request->input[i];
    }

    /* Release the slot only after the request has been copied locally. */
    __atomic_store_n(&command_queue.head, head + 1U, __ATOMIC_RELEASE);
    return true;
}

/** @brief Report one parser failure without dispatching partial arguments. */
static void shell_command_report_parse_error(kerr_t error) {
    if (error == E_INVAL) {
        (void)shell_write_line("Syntax error: invalid or unterminated quoted argument.");
    } else if (error == E_BADCOUNT) {
        (void)shell_write_line("Too many arguments (maximum: 16).");
    } else if (error == E_BADSIZE) {
        (void)shell_write_line("Command line exceeds the parser limit.");
    } else {
        (void)shell_write_line("Unable to parse command line.");
    }
}

/** @brief Parse and execute one dequeued request. */
static void shell_command_process_request(void) {
    kerr_t err = shell_parse(worker_input, worker_input_length, &worker_parse_result);
    if (k_failed(err)) {
        shell_command_report_parse_error(err);
        return;
    }
    if (worker_parse_result.argc == 0U) return;

    err = shell_command_execute((int)worker_parse_result.argc, worker_parse_result.argv);
    if (k_failed(err) && err != E_NOTFOUND) {
        log_error("shell command '%s' failed: %s",
                  worker_parse_result.argv[0], kerr_str(err));
    }
}

/** @brief Resolve builtins first, then custom commands; call without registry lock. */
kerr_t shell_command_execute(int argc, const char *const argv[]) {
    if (argc <= 0 || !argv || !argv[0] || argv[0][0] == '\0') return E_INVAL;

    const shell_command_t *builtin = shell_builtin_find(argv[0]);
    if (builtin) return builtin->execute(argc, argv);

    shell_command_fn_t execute = NULL;
    if (shell_command_registry_lookup(argv[0], &execute) && execute) {
        return execute(argc, argv);
    }

    (void)shell_write("Unknown command: ");
    (void)shell_write_line(argv[0]);
    return E_NOTFOUND;
}

/**
 * @brief Process requests serially. Already accepted requests are drained on shutdown.
 * Scheduler ticks are configured to 1000 Hz, so one sleep tick is one millisecond.
 */
static void shell_command_worker(void *arg) {
    (void)arg;
    for (;;) {
        if (shell_command_queue_dequeue()) {
            shell_command_process_request();
            continue;
        }
        if (__atomic_load_n(&command_stop_requested, __ATOMIC_ACQUIRE)) break;
        thread_sleep_ms(SHELL_COMMAND_WORKER_SLEEP_MS);
    }

    __atomic_store_n(&command_worker_stopped, true, __ATOMIC_RELEASE);
    thread_exit();
}

/** @brief Initialize registry, queue and the single command worker. */
kerr_t shell_command_init(void) {
    if (__atomic_load_n(&command_initialized, __ATOMIC_ACQUIRE)) return E_OK;
    if (!shell_command_is_stopped()) return E_NOTAVAIL;
    K_TRY(shell_command_registry_init());

    __atomic_store_n(&command_queue.head, 0U, __ATOMIC_RELAXED);
    __atomic_store_n(&command_queue.tail, 0U, __ATOMIC_RELAXED);
    __atomic_store_n(&command_queue.accepting, true, __ATOMIC_RELEASE);
    __atomic_store_n(&command_stop_requested, false, __ATOMIC_RELEASE);
    __atomic_store_n(&command_worker_stopped, false, __ATOMIC_RELEASE);
    __atomic_store_n(&command_initialized, true, __ATOMIC_RELEASE);

    thread_t *worker = NULL;
    kerr_t err = thread_create(NULL, shell_command_worker, NULL, "shell_command", &worker);
    if (k_failed(err)) {
        __atomic_store_n(&command_initialized, false, __ATOMIC_RELEASE);
        __atomic_store_n(&command_queue.accepting, false, __ATOMIC_RELEASE);
        __atomic_store_n(&command_stop_requested, false, __ATOMIC_RELEASE);
        __atomic_store_n(&command_worker_stopped, true, __ATOMIC_RELEASE);
        __atomic_store_n(&command_queue.head, 0U, __ATOMIC_RELAXED);
        __atomic_store_n(&command_queue.tail, 0U, __ATOMIC_RELAXED);
        shell_command_registry_deinit();
        return err;
    }
    (void)worker;
    return E_OK;
}

/**
 * @brief Stop accepting, drain the FIFO and finish registry cleanup.
 * shell_input_deinit() must have completed before this is called.
 */
kerr_t shell_command_deinit(void) {
    if (__atomic_load_n(&command_initialized, __ATOMIC_ACQUIRE)) {
        __atomic_store_n(&command_initialized, false, __ATOMIC_RELEASE);
        __atomic_store_n(&command_queue.accepting, false, __ATOMIC_RELEASE);
        __atomic_store_n(&command_stop_requested, true, __ATOMIC_RELEASE);
    }
    if (!shell_command_is_stopped()) return E_NOTAVAIL;

    shell_command_registry_deinit();
    __atomic_store_n(&command_queue.head, 0U, __ATOMIC_RELAXED);
    __atomic_store_n(&command_queue.tail, 0U, __ATOMIC_RELAXED);
    __atomic_store_n(&command_queue.accepting, false, __ATOMIC_RELEASE);
    __atomic_store_n(&command_stop_requested, false, __ATOMIC_RELEASE);
    return E_OK;
}
