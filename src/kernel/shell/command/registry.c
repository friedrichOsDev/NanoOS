/**
 * @file registry.c
 * @brief Fixed-capacity registry for custom kernel shell commands.
 * @author friedrichOsDev
 */

#include "_command.h"

#include <lib/util/str.h>

typedef struct {
    char name[SHELL_COMMAND_NAME_MAX];
    char description[SHELL_COMMAND_DESCRIPTION_MAX];
    shell_command_fn_t execute;
    bool active;
} shell_registered_command_t;

static shell_registered_command_t registry[SHELL_MAX_CUSTOM_COMMANDS];
static bool registry_initialized = false;
static spinlock_t registry_lock = SPINLOCK_INIT;

/** @brief Return a string length, stopping at the supplied bound. */
static size_t shell_command_bounded_length(const char *text, size_t maximum) {
    for (size_t i = 0U; i < maximum; i++) {
        if (text[i] == '\0') {
            return i;
        }
    }
    return maximum;
}

/** @brief Check one character against the command-name alphabet. */
static bool shell_command_name_character_valid(char character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9') ||
           character == '_' || character == '-';
}

/** @brief Validate a command name and its maximum length. */
static kerr_t shell_command_validate_name(const char *name, size_t *out_length) {
    if (!name || !out_length) {
        return E_BADADDR;
    }

    size_t length = shell_command_bounded_length(name, SHELL_COMMAND_NAME_MAX);
    if (length == 0U) {
        return E_INVAL;
    }
    if (length >= SHELL_COMMAND_NAME_MAX) {
        return E_BADSIZE;
    }

    for (size_t i = 0U; i < length; i++) {
        if (!shell_command_name_character_valid(name[i])) {
            return E_INVAL;
        }
    }

    *out_length = length;
    return E_OK;
}

/** @brief Clear every registry slot. The caller must hold registry_lock. */
static void shell_command_registry_clear_locked(void) {
    for (size_t i = 0U; i < SHELL_MAX_CUSTOM_COMMANDS; i++) {
        registry[i] = (shell_registered_command_t){0};
    }
}

/** @brief Initialize the registry before the worker is started. */
kerr_t shell_command_registry_init(void) {
    irq_flags_t flags = spinlock_acquire_irqsave(&registry_lock);
    shell_command_registry_clear_locked();
    registry_initialized = true;
    spinlock_release_irqrestore(&registry_lock, flags);
    return E_OK;
}

/** @brief Reset the registry after the command worker has stopped. */
void shell_command_registry_deinit(void) {
    irq_flags_t flags = spinlock_acquire_irqsave(&registry_lock);
    registry_initialized = false;
    shell_command_registry_clear_locked();
    spinlock_release_irqrestore(&registry_lock, flags);
}

/**
 * @brief Register a custom command after validating and copying its metadata.
 */
kerr_t shell_command_register(const shell_command_t *command) {
    if (!command || !command->description || !command->execute) {
        return E_BADADDR;
    }

    size_t name_length = 0U;
    K_TRY(shell_command_validate_name(command->name, &name_length));

    size_t description_length =
        shell_command_bounded_length(command->description, SHELL_COMMAND_DESCRIPTION_MAX);
    if (description_length >= SHELL_COMMAND_DESCRIPTION_MAX) {
        return E_BADSIZE;
    }

    if (shell_builtin_find(command->name)) {
        return E_NOTAVAIL;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&registry_lock);
    if (!registry_initialized) {
        spinlock_release_irqrestore(&registry_lock, flags);
        return E_NOINIT;
    }

    size_t free_slot = SHELL_MAX_CUSTOM_COMMANDS;
    for (size_t i = 0U; i < SHELL_MAX_CUSTOM_COMMANDS; i++) {
        if (!registry[i].active) {
            if (free_slot == SHELL_MAX_CUSTOM_COMMANDS) {
                free_slot = i;
            }
            continue;
        }
        if (strcmp(registry[i].name, command->name) == 0) {
            spinlock_release_irqrestore(&registry_lock, flags);
            return E_INVAL;
        }
    }

    if (free_slot >= SHELL_MAX_CUSTOM_COMMANDS) {
        spinlock_release_irqrestore(&registry_lock, flags);
        return E_NOMEM;
    }

    for (size_t i = 0U; i <= name_length; i++) {
        registry[free_slot].name[i] = command->name[i];
    }
    for (size_t i = 0U; i <= description_length; i++) {
        registry[free_slot].description[i] = command->description[i];
    }
    registry[free_slot].execute = command->execute;
    registry[free_slot].active = true;

    spinlock_release_irqrestore(&registry_lock, flags);
    return E_OK;
}

/**
 * @brief Remove a custom command and make its slot available again.
 */
kerr_t shell_command_unregister(const char *name) {
    size_t name_length = 0U;
    K_TRY(shell_command_validate_name(name, &name_length));
    (void)name_length;

    if (shell_builtin_find(name)) {
        return E_NOTAVAIL;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&registry_lock);
    if (!registry_initialized) {
        spinlock_release_irqrestore(&registry_lock, flags);
        return E_NOINIT;
    }

    for (size_t i = 0U; i < SHELL_MAX_CUSTOM_COMMANDS; i++) {
        if (registry[i].active && strcmp(registry[i].name, name) == 0) {
            registry[i] = (shell_registered_command_t){0};
            spinlock_release_irqrestore(&registry_lock, flags);
            return E_OK;
        }
    }

    spinlock_release_irqrestore(&registry_lock, flags);
    return E_NOTFOUND;
}

/**
 * @brief Find a callback while holding the registry lock, then return it by value.
 */
bool shell_command_registry_lookup(const char *name, shell_command_fn_t *out_execute) {
    if (!name || !out_execute) {
        return false;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&registry_lock);
    if (!registry_initialized) {
        spinlock_release_irqrestore(&registry_lock, flags);
        return false;
    }

    for (size_t i = 0U; i < SHELL_MAX_CUSTOM_COMMANDS; i++) {
        if (registry[i].active && strcmp(registry[i].name, name) == 0) {
            *out_execute = registry[i].execute;
            spinlock_release_irqrestore(&registry_lock, flags);
            return true;
        }
    }

    spinlock_release_irqrestore(&registry_lock, flags);
    return false;
}

/**
 * @brief Copy registered metadata into a caller-owned snapshot.
 *
 * No registry lock is held while the caller later performs shell output.
 */
size_t shell_command_registry_snapshot(shell_command_info_t *out, size_t capacity) {
    if (!out || capacity == 0U) {
        return 0U;
    }

    irq_flags_t flags = spinlock_acquire_irqsave(&registry_lock);
    if (!registry_initialized) {
        spinlock_release_irqrestore(&registry_lock, flags);
        return 0U;
    }

    size_t copied = 0U;
    for (size_t i = 0U; i < SHELL_MAX_CUSTOM_COMMANDS && copied < capacity; i++) {
        if (!registry[i].active) {
            continue;
        }

        for (size_t j = 0U; j < SHELL_COMMAND_NAME_MAX; j++) {
            out[copied].name[j] = registry[i].name[j];
        }
        for (size_t j = 0U; j < SHELL_COMMAND_DESCRIPTION_MAX; j++) {
            out[copied].description[j] = registry[i].description[j];
        }
        copied++;
    }

    spinlock_release_irqrestore(&registry_lock, flags);
    return copied;
}
