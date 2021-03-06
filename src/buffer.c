//
// Created by David Nugent on 4/02/2016.
//

#include <string.h>
#include <stdlib.h>

#include "buffer.h"

#define BUFFER_ALLOC    0x51a5a25


buffer_t *
buffer_init(buffer_t *buffer, size_t size) {
    if (buffer != NULL)
        memset(buffer, '\0', sizeof(buffer_t));
    else {
        buffer = calloc(1, sizeof(buffer_t));
        buffer->alloc = BUFFER_ALLOC;
    }
    buffer->b_size = size;
    buffer->b_hi = buffer->b_lo = 0;
    buffer->data = size ? malloc(buffer->b_size) : NULL;
    return buffer;
}


void
buffer_free(buffer_t *buffer) {
    buffer->b_size = buffer->b_lo = buffer->b_hi = 0;
    if (buffer->data) {
        free(buffer->data);
        buffer->data = NULL;
    }
    if (buffer->alloc == BUFFER_ALLOC)
        free(buffer);
}


size_t buffer_size(buffer_t *buffer) { return buffer->b_size; }
size_t buffer_hi(buffer_t *buffer) { return buffer->b_hi; }
size_t buffer_lo(buffer_t *buffer) { return buffer->b_lo; }
void * buffer_base(buffer_t *buffer) { return buffer->data; }


void
buffer_flush(buffer_t *buffer) {
    buffer->b_hi = buffer->b_lo = 0;
}


size_t
buffer_used(buffer_t *buffer) {
    if (buffer->b_hi >= buffer->b_lo)                       // |_^....v_|
        return buffer->b_hi - buffer->b_lo;
    return buffer->b_size - buffer->b_lo + buffer->b_hi;    // |..v__^..|
}


size_t
buffer_available(buffer_t *buffer) {
    return buffer->b_size ? buffer->b_size - buffer_used(buffer) - 1 : 0;
}


static inline size_t
buffer_inc(buffer_t *buffer, size_t val, size_t by) {
    val += by;
    return val >= buffer->b_size ? 0 : val;
}


void
buffer_compact(buffer_t *buffer) {
    if (buffer->b_lo == 0) {                    // |^....v__|
        // already compact, nothing to do
    } else {
        size_t size;
        if (buffer->b_lo >= buffer->b_hi) {     // |_^....v_| -> |^....v__|
            // not wrapped
            size = buffer->b_lo - buffer->b_hi;
            if (size)
                memmove(buffer->data, buffer->data + buffer->b_lo, size);
            // empty, just reset i/o ptrs
        } else {                                // |..v__^..| -> |^....v__|
            // duplicating+replacing the buffer is the simplest way
            // avoids dealing with temp buffers and overlapping regions
            void *data = malloc(buffer->b_size);
            size = buffer->b_size - buffer->b_lo;
            if (size)
                memcpy(data, buffer->data + buffer->b_lo, size);
            if (buffer->b_hi)
                memcpy(data, buffer->data, buffer->b_hi);
            size += buffer->b_hi;
            free(buffer->data);
            buffer->data = data;
        }
        buffer->b_lo = 0;
        buffer->b_hi = size;
    }
}


// put data into the buffer
size_t
buffer_put(buffer_t *buffer, void const *buf, size_t len) {
    size_t avail = buffer_available(buffer);
    if (len >= avail)    // partial put only if insufficient space
        len = avail;
    else
        avail = len;
    size_t top = buffer->b_size - buffer->b_hi;         // |_^....v_|
    if (buffer->b_lo > buffer->b_hi)                    // |..v__^..|
        top -= (buffer->b_size - buffer->b_lo);
    if (top > avail)
        top = avail;
    if (top > 0) {
        memcpy(buffer->data + buffer->b_hi, buf, top);
        avail -= top;
        buf += top;
        buffer->b_hi = buffer_inc(buffer, buffer->b_hi, top);
    }
    if (avail) {
        memcpy(buffer->data, buf, avail);
        buffer->b_hi = buffer_inc(buffer, buffer->b_hi, avail);
    }
    return len;
}


size_t
buffer_peek(buffer_t *buffer, void *buf, size_t len) {
    size_t avail = buffer_used(buffer);
    if (len > avail)    // partial put only if insufficient data
        len = avail;
    else
        avail = len;
    size_t lo = buffer->b_lo;
    size_t hi = buffer->b_hi;
    if (lo > hi) {
        size_t top = buffer->b_size - lo;
        if (top > avail)
            top = avail;
        if (top)
            memcpy(buf, buffer->data + lo, top);
        buf += top;
        avail -= top;
        lo = buffer_inc(buffer, lo, top);
    }
    if (avail)
        memcpy(buf, buffer->data + lo, avail);
    return len;
}


size_t
buffer_get(buffer_t *buffer, void *buf, size_t len) {
    size_t avail = buffer_used(buffer);
    if (len > avail)    // partial put only if insufficient data
        len = avail;
    else
        avail = len;
    if (buffer->b_lo > buffer->b_hi) {
        size_t top = buffer->b_size - buffer->b_lo;
        if (top > avail)
            top = avail;
        if (top && buf) {
            memcpy(buf, buffer->data + buffer->b_lo, top);
            buf += top;
        }
        avail -= top;
        buffer->b_lo = buffer_inc(buffer, buffer->b_lo, top);
    }
    if (avail) {
        if (buf)
            memcpy(buf, buffer->data + buffer->b_lo, avail);
        buffer->b_lo = buffer_inc(buffer, buffer->b_lo, avail);
    }
    return len;
}


static size_t
buffer_copymove(buffer_t *dst, buffer_t *src, size_t len, int move) {
    // ajdust length for max bytes available in dst buffer
    size_t d_avail = buffer_available(dst);
    if (len > d_avail)
        len = d_avail;
    else d_avail = len;
    // adjust length for max bytes available in the src buffer
    size_t s_avail = buffer_used(src);
    if (len > s_avail)
        len = s_avail;
    else s_avail = len;
    // finally, trim if we have more bytes in src then can fit in dst
    if (s_avail > d_avail)
        s_avail = d_avail;
    // dup hi and lo from source
    size_t lo = src->b_lo;
    size_t hi = src->b_hi;
    // copy top half (if there is one)
    if (lo > hi) {
        size_t top = src->b_size - lo;
        if (top > s_avail)
            top = s_avail;
        if (top)
            buffer_put(dst, src->data + lo, top);
        s_avail -= top;
        lo = buffer_inc(src, lo, top);
    }
    // copy bottom half
    if (s_avail) {
        buffer_put(dst, src->data + lo, s_avail);
        lo = buffer_inc(src, lo, s_avail);
    }
    // only advance the ptr in src buffer if we are moving
    if (move) {
        src->b_lo = lo;
        src->b_hi = hi;
    }
    return len;
}


size_t
buffer_copy(buffer_t *dst, buffer_t *src, size_t size) {
    return buffer_copymove(dst, src, size, 0);
}


size_t
buffer_move(buffer_t *dst, buffer_t *src, size_t size) {
    return buffer_copymove(dst, src, size, 1);
}
