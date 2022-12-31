// -----------------------------------------------------------------------------
// MIT License
//
// Copyright 2022-Present Miguel A. Guerrero
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal # in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------
// This module implements a wrapper over log.c/h event logger
// The module defines a buffer for events, a log data structure,
// and to provide times-stamping.
// The code here can be called from interrupt context, as no calls to std c lib
// are used.
// ----------------------------------------------------------------------------
#include "log.h"
#include "emb_log.h"
#include "msgs_auto.h"
#include "debug.h"
#include "emb_assert.h"
#include "debug_hw_specific.h"

uint64_t get_time_stamp();

#if defined(EMB_LOG_XTENSA)
    // Tensilica implementation assumed here (tested a number of years ago, may need updates)
    #include "xtensa_api.h"
    #include <xtensa/config/core.h>
    #include <xtensa/xtruntime.h>
    #include <xtensa/hal.h>
    uint64_t get_time_stamp() { return (uint64_t) xthal_get_ccount(); }
    // can be mapped to .text if desired as access is always in multiples of 32-bits
    static int32_t emb_log_buf[EMB_LOG_ENTRIES] __attribute__((section(".text")));
#else
    #include <sys/time.h>
    // default is an x86 implementation, for easy testing
    #include "rdtsc.h"
    uint64_t get_time_stamp() { 
        return rdtsc();
    }
    static int32_t emb_log_buf[EMB_LOG_ENTRIES];
#endif


static log_t log;

// initialize log data structure
void emb_log_init()
{
    log_init(&log, emb_log_buf, EMB_LOG_ENTRIES);
}

// enable or disable logging
void emb_log_set_enable(int on)
{
    log_set_enable(&log, on);
}

// specify an initial number of log messages to skip
void emb_log_start_after_cnt_msgs(int cnt)
{
    log_start_after_cnt_msgs(&log, cnt);
}

// if val is != 0, we don't wrap around
void emb_log_set_one_shot(int val) {
    log_set_one_shot(&log, val);
}

// specify an number of log messages to log
void emb_log_stop_after_cnt_capt_msgs(int cnt)
{
    log_stop_after_cnt_capt_msgs(&log, cnt);
}

// add a pre-created log entry
void emb_log_add(void *msg, int msg_byte_len)
{
    // This restriction could be removed but keeping it makes it
    // compatible with Tensilica logging in .text
    emb_assert ((((long)msg) & 0x3) == 0); // msg must be word aligned

    EMB_LOG_ENTER_CRITICAL_SECT; // the following sequence shouldn't be interrupted

    uint64_t ts = get_time_stamp();
    log_add(&log, ts, msg, msg_byte_len);

    EMB_LOG_EXIT_CRITICAL_SECT;
}

// printing in hex, 8 words per line
static void entry_dump_raw(int i, int entry, int buf_ofs)
{
    if (i % 8 == 0) {
        DEBUG_putchar('\n');
    }
    DEBUG_print_hex((uint32_t)entry);
    DEBUG_putchar(' ');
}

// dump the buffer into console (not using std lib)
void emb_log_dump(int format)
{
    if (0 == format) {
        DEBUG_print("\ncursor=");      DEBUG_print_dec(log.cur);
        DEBUG_print("\nwrapped=");     DEBUG_print_dec(log.wrapped);
        DEBUG_print("\nenabled=");     DEBUG_print_dec(log.enabled);
        DEBUG_print("\nevnt_cnt=");    DEBUG_print_dec(log.cnt);
        DEBUG_print("\nmax_entries="); DEBUG_print_dec(log.max_entries);
        DEBUG_print("\n=== Start buffer dump. Most recent first ===");
        log_dump_raw(&log, entry_dump_raw);
        DEBUG_println("\n=== End buffer dump ===");
    }
    else {
        // just a place holder for other possible formats
        DEBUG_println("\nemb_log_dump this format is not implemented");
        DEBUG_print_hex(format);
        DEBUG_println("");
    }
}

