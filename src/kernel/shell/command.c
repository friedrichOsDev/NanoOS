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

typedef struct {
    shell_command_request_t requests[SHELL_COMMAND_QUEUE_SIZE];
    size_t read_index;
    size_t write_index;
    size_t count;
    spinlock_t lock;
    bool accepting;
} shell_command_queue_t;

_Static_assert(SHELL_INPUT_BUFFER_SIZE == SHELL_COMMAND_MAX_LINE_CODEPOINTS,
               "Shell input and command parser limits must match");

static shell_command_queue_t command_queue = {.lock = SPINLOCK_INIT};
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

/** @brief Submit an immutable input snapshot to the fixed-size FIFO. */
kerr_t shell_command_submit(const uint32_t *input, size_t length) {
    if (!input && length != 0U) return E_BADADDR;
    if (length > SHELL_INPUT_BUFFER_SIZE) return E_BADSIZE;
    if (!__atomic_load_n(&command_initialized, __ATOMIC_ACQUIRE)) return E_NOINIT;
    if (length == 0U) return E_OK;

    irq_flags_t flags = spinlock_acquire_irqsave(&command_queue.lock);
    if (!command_queue.accepting) {
        spinlock_release_irqrestore(&command_queue.lock, flags);
        return E_NOINIT;
    }
    if (command_queue.count >= SHELL_COMMAND_QUEUE_SIZE) {
        spinlock_release_irqrestore(&command_queue.lock, flags);
        return E_NOMEM;
    }

    shell_command_request_t *request = &command_queue.requests[command_queue.write_index];
    for (size_t i = 0U; i < length; i++) request->input[i] = input[i];
    request->length = length;
    command_queue.write_index = (command_queue.write_index + 1U) % SHELL_COMMAND_QUEUE_SIZE;
    command_queue.count++;
    spinlock_release_irqrestore(&command_queue.lock, flags);
    return E_OK;
}

/** @brief Dequeue one request into worker-private storage. */
static bool shell_command_queue_dequeue(void) {
    irq_flags_t flags = spinlock_acquire_irqsave(&command_queue.lock);
    if (command_queue.count == 0U) {
        spinlock_release_irqrestore(&command_queue.lock, flags);
        return false;
    }

    const shell_command_request_t *request = &command_queue.requests[command_queue.read_index];
    worker_input_length = request->length;
    for (size_t i = 0U; i < worker_input_length; i++) worker_input[i] = request->input[i];

    command_queue.read_index = (command_queue.read_index + 1U) % SHELL_COMMAND_QUEUE_SIZE;
    command_queue.count--;
    spinlock_release_irqrestore(&command_queue.lock, flags);
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

    irq_flags_t flags = spinlock_acquire_irqsave(&command_queue.lock);
    command_queue.read_index = 0U;
    command_queue.write_index = 0U;
    command_queue.count = 0U;
    command_queue.accepting = true;
    spinlock_release_irqrestore(&command_queue.lock, flags);

    __atomic_store_n(&command_stop_requested, false, __ATOMIC_RELEASE);
    __atomic_store_n(&command_worker_stopped, false, __ATOMIC_RELEASE);
    __atomic_store_n(&command_initialized, true, __ATOMIC_RELEASE);

    thread_t *worker = NULL;
    kerr_t err = thread_create(NULL, shell_command_worker, NULL, "shell_command", &worker);
    if (k_failed(err)) {
        __atomic_store_n(&command_initialized, false, __ATOMIC_RELEASE);
        __atomic_store_n(&command_stop_requested, false, __ATOMIC_RELEASE);
        __atomic_store_n(&command_worker_stopped, true, __ATOMIC_RELEASE);
        flags = spinlock_acquire_irqsave(&command_queue.lock);
        command_queue.accepting = false;
        command_queue.count = 0U;
        command_queue.read_index = 0U;
        command_queue.write_index = 0U;
        spinlock_release_irqrestore(&command_queue.lock, flags);
        shell_command_registry_deinit();
        return err;
    }
    (void)worker;
    return E_OK;
}

/**
 * @brief Stop accepting, drain the FIFO and finish registry cleanup.
 * Caller retries while the worker is still executing.
 */
kerr_t shell_command_deinit(void) {
    if (__atomic_load_n(&command_initialized, __ATOMIC_ACQUIRE)) {
        __atomic_store_n(&command_initialized, false, __ATOMIC_RELEASE);
        irq_flags_t flags = spinlock_acquire_irqsave(&command_queue.lock);
        command_queue.accepting = false;
        spinlock_release_irqrestore(&command_queue.lock, flags);
        __atomic_store_n(&command_stop_requested, true, __ATOMIC_RELEASE);
    }
    if (!shell_command_is_stopped()) return E_NOTAVAIL;

    shell_command_registry_deinit();
    irq_flags_t flags = spinlock_acquire_irqsave(&command_queue.lock);
    command_queue.accepting = false;
    command_queue.read_index = 0U;
    command_queue.write_index = 0U;
    command_queue.count = 0U;
    spinlock_release_irqrestore(&command_queue.lock, flags);
    __atomic_store_n(&command_stop_requested, false, __ATOMIC_RELEASE);
    return E_OK;
}
