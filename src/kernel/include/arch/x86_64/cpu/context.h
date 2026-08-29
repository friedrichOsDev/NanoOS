/**
 * @file context.h
 * @brief Context switch assembly declarations
 * @author friedrichOsDev
 */

#pragma once

#include <core/thread.h>

extern void switch_context(uint64_t *prev_rsp_ptr, uint64_t next_rsp);
extern void thread_entry_stub(void);