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
// This module implements a simple circular buffer event logger.
// the buffer is defined in terms of 32-bit words with the purpose to
// facilitate the underlaying buffering to be in RAM w/ 32b access capability
// only. Other than that this module does not care about the contents of the 
// msgs or have any other dependency on project or underlaying hardware
// ----------------------------------------------------------------------------
#include "log.h"
#include "msgs_auto.h" // for EMB_LOG_TS_SHIFT/EMB_LOG_TS_MAX

#define HI_WORD_MASK 0xFFFFFFFF00000000ULL
#define LO_WORD_MASK 0x00000000FFFFFFFFULL

// Init the log passing the underlaying pre-allocated buffer and length
// defined in words and word aligned
void log_init(log_t* log, int32_t* bufin, int bufin_nwords)
{
    log->buf = bufin;
    log->max_entries = bufin_nwords;
    log->cur = 0;
    log->cnt = 0;
    log->start_cnt = -1;
    log->stop_cnt = -1;
    log->wrapped = 0;
    log->enabled = 1;
    log->first = 1;
    log->one_shot = 0;
}

void log_set_enable(log_t* log, int on)
{
    log->enabled = on;
}

void log_start_after_cnt_msgs(log_t* log, int c)
{
    log->enabled = 0;
    log->start_cnt = c;
}

void log_stop_after_cnt_capt_msgs(log_t* log, int c)
{
    log->stop_cnt = c;
}

// if val is != 0, we don't wrap around
void log_set_one_shot(log_t* log, int val) {
    log->one_shot = val;
}

// This code attempts to be constant time (avoid conditionals)
#define EMIT_WORD(w) do {\
        log->buf[log->cur++] = (w); \
        int in_range = log->cur < log->max_entries; \
        log->cur *= in_range; /* zero-out if out of range */ \
        log->wrapped |= !in_range; \
    } while(0)


// Add an entry to the log specificying bufer and length in bytes
// the orignal buffer is expected to be word aligned
void log_add(log_t* log, uint64_t tsin, void *msgin, int byte_len_in)
{
    // Update total message count pushed
    log->cnt++;

    // check whether there is a delayed log enable
    if (log->start_cnt >= 0) {
        if (log->start_cnt == 0) // start capture after cntr expires
            log->enabled = 1;

        log->start_cnt--;
    }

    // exit if temporarily disabled capture
    if (!log->enabled || (log->one_shot && log->wrapped)) {
        return;
    }

    if (log->first) {
        log->last_ts = tsin;
        log->first = 0;
    }

    // Push message to circular queue as words
    int32_t *msg = (int32_t *) msgin;
    int word_len = (byte_len_in + 3) / 4;  // len is padded to word boundary

    int i;
    for (i=0; i < word_len - 1; i++) { // all but last
        EMIT_WORD(msg[i]);
    }

    // insert time-stamp as relative to prev event
    uint64_t ts = tsin - log->last_ts;
    log->last_ts = tsin;

    uint32_t flag_ts_is_64b = 0;
    // this happens very rarelly
    if ((ts & HI_WORD_MASK) != 0) { // if bigger than 32 bits
        EMIT_WORD((ts >> 32) & LO_WORD_MASK);
        EMIT_WORD(ts & LO_WORD_MASK);
        flag_ts_is_64b = EMB_LOG_TS64_MASK; // indicate it is a 64 bit time-stamp
        ts = EMB_LOG_TS_MAX; // and flag ts as maxed out
    }
    else if (ts >= EMB_LOG_TS_MAX) {
        // if ts is bigger than what fits on msg id word, add extra word
        EMIT_WORD(ts & LO_WORD_MASK);
        ts = EMB_LOG_TS_MAX; // and flag ts as maxed out
    }
    // last has id and embedded ts
    EMIT_WORD(msg[i] | flag_ts_is_64b | (ts << EMB_LOG_TS_SHIFT));

    // check whether there is a delayed log disable
    if (log->stop_cnt >= 0 && log->enabled) {
        log->stop_cnt--;

        if (log->stop_cnt == 0) { // stop capture after cntr expires
            log->enabled = 0;
        }
    }
}

// Circular buffer dump happens most-recent first. The dump function
// is user provided. If the messages are defined so that the message
// type is at the end of it, it sould be parseable regardless of
// where the wrap-around happens even for variable size messages.
void log_dump_raw(log_t *log, dump_f dump_func)
{
    int cnt=0;
    int end = log->cur;
    int cur = log->cur;
    while (--cur >= 0) {
        dump_func(cnt++, log->buf[cur], cur);
    }

    if (log->wrapped) {
        cur = log->max_entries;
        while (--cur >= end) {
            dump_func(cnt++, log->buf[cur], cur);
        }
    }
}
