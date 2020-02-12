/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include <glib.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "extmod/misc.h"


STATIC void sighandler(int signum) {
    if (signum == SIGINT) {
        if (MP_STATE_MAIN_THREAD(mp_pending_exception) == MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception))) {
            // this is the second time we are called, so die straight away
            exit(1);
        }
        mp_obj_exception_clear_traceback(MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception)));
        MP_STATE_MAIN_THREAD(mp_pending_exception) = MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception));
    }
}

void mp_hal_set_interrupt_char(char c) {
    // configure terminal settings to (not) let ctrl-C through
    if (c == CHAR_CTRL_C) {
        // enable signal handler
        struct sigaction sa;
        sa.sa_flags = 0;
        sa.sa_handler = sighandler;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
    } else {
        // disable signal handler
        struct sigaction sa;
        sa.sa_flags = 0;
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
    }
}

#if MICROPY_USE_READLINE == 1

#include <termios.h>

static struct termios orig_termios;

void mp_hal_stdio_mode_raw(void) {
    // save and set terminal settings
    tcgetattr(0, &orig_termios);
    static struct termios termios;
    termios = orig_termios;
    termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    termios.c_cflag = (termios.c_cflag & ~(CSIZE | PARENB)) | CS8;
    termios.c_lflag = 0;
    termios.c_cc[VMIN] = 1;
    termios.c_cc[VTIME] = 0;
    tcsetattr(0, TCSAFLUSH, &termios);
}

void mp_hal_stdio_mode_orig(void) {
    // restore terminal settings
    tcsetattr(0, TCSAFLUSH, &orig_termios);
}

#endif

#if MICROPY_PY_OS_DUPTERM
static int call_dupterm_read(size_t idx) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t read_m[3];
        mp_load_method(MP_STATE_VM(dupterm_objs[idx]), MP_QSTR_read, read_m);
        read_m[2] = MP_OBJ_NEW_SMALL_INT(1);
        mp_obj_t res = mp_call_method_n_kw(1, 0, read_m);
        if (res == mp_const_none) {
            return -2;
        }
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(res, &bufinfo, MP_BUFFER_READ);
        if (bufinfo.len == 0) {
            mp_printf(&mp_plat_print, "dupterm: EOF received, deactivating\n");
            MP_STATE_VM(dupterm_objs[idx]) = MP_OBJ_NULL;
            return -1;
        }
        nlr_pop();
        return *(byte*)bufinfo.buf;
    } else {
        // Temporarily disable dupterm to avoid infinite recursion
        mp_obj_t save_term = MP_STATE_VM(dupterm_objs[idx]);
        MP_STATE_VM(dupterm_objs[idx]) = NULL;
        mp_printf(&mp_plat_print, "dupterm: ");
        mp_obj_print_exception(&mp_plat_print, nlr.ret_val);
        MP_STATE_VM(dupterm_objs[idx]) = save_term;
    }

    return -1;
}
#endif

int mp_hal_stdin_rx_chr(void) {
    unsigned char c;
#if MICROPY_PY_OS_DUPTERM
    // TODO only support dupterm one slot at the moment
    if (MP_STATE_VM(dupterm_objs[0]) != MP_OBJ_NULL) {
        int c;
        do {
             c = call_dupterm_read(0);
        } while (c == -2);
        if (c == -1) {
            goto main_term;
        }
        if (c == '\n') {
            c = '\r';
        }
        return c;
    } else {
        main_term:;
#endif
        GPollFD fd = { .fd = STDIN_FILENO, .events = G_IO_IN };
        g_main_context_add_poll(g_main_context_get_thread_default(), &fd, G_PRIORITY_DEFAULT);

        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            do {
                MICROPY_EVENT_POLL_HOOK
            } while (!fd.revents);
            nlr_pop();
        }
        else {
            g_main_context_remove_poll(g_main_context_get_thread_default(), &fd);
            nlr_jump(nlr.ret_val);
        }

        g_main_context_remove_poll(g_main_context_get_thread_default(), &fd);

        int ret = read(STDIN_FILENO, &c, 1);
        if (ret == 0) {
            c = 4; // EOF, ctrl-D
        } else if (c == '\n') {
            c = '\r';
        }
        return c;
#if MICROPY_PY_OS_DUPTERM
    }
#endif
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    MP_THREAD_GIL_EXIT();
    int ret = write(1, str, len);
    MP_THREAD_GIL_ENTER();
    mp_uos_dupterm_tx_strn(str, len);
    (void)ret; // to suppress compiler warning
}

// cooked is same as uncooked because the terminal does some postprocessing
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    mp_hal_stdout_tx_strn(str, len);
}

void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}

mp_uint_t mp_hal_ticks_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

mp_uint_t mp_hal_ticks_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void mp_hal_delay_ms(mp_uint_t ms) {
    struct timespec ts = {
        .tv_sec = ms / 1000,
        .tv_nsec = ms % 1000 * 1000000,
    };
    struct timespec remain;
    for(;;) {
        MP_THREAD_GIL_EXIT();
        int ret = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &remain);
        MP_THREAD_GIL_EXIT();
        if (ret == EINTR) {
            mp_handle_pending();
            ts = remain;
            continue;
        }
        assert(ret == 0);
        break;
    }
}
