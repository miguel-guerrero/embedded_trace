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
// -----------------------------------------------------------------------------
#pragma once

#ifndef EMB_LOG_ONE_SHOT
# define EMB_LOG_ONE_SHOT 0   // if 1 disallow circular wrap
#endif

#include <stdint.h>

typedef void (*dump_f)(int i, int entry, int buf_ofs);

// data structure that keeps track of where we are in the trace
// buffer and other control info
typedef struct {
    int32_t *buf;     // Assuming word aligned
    int max_entries;  // Max space in words in log buffer
    int cur;          // Place to enter next message
    int cnt;          // Total messages captured so far
    int start_cnt;    // Msgs till starting log, <=0 to ignore
    int stop_cnt;     // Msgs till stopping log, <=0 to ignore
    uint64_t last_ts; // Last seen time-stamp
    int wrapped;      // Did the buffer ever wrapped around
    int enabled;      // If non zero, we record log events
    int first;        // True for 1st event only
    int one_shot;     // If set, we won't wrap around, just stop
                      // logging
} log_t;

void log_init(log_t* log, int32_t* bufin, int bufin_nwords);
void log_add(log_t* log, uint64_t ts, void *msgin, int byte_len_in);
void log_dump_raw(log_t *log, dump_f dump_func);
void log_set_one_shot(log_t* log, int val);

void log_set_enable(log_t* log, int on);
void log_start_after_cnt_msgs(log_t* log, int cnt);
void log_stop_after_cnt_capt_msgs(log_t* log, int cnt);

