/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WM_INPUT_H
#define WM_INPUT_H

#include <ec/wm.h>

/* functions */
extern int input_init(void); /* initialize input */
extern void input_update(void); /* update input */
extern wm_event_t *input_get_next_event(void); /* get next input event */
extern wm_event_t *input_get_last_event(void); /* get last input event */

#endif /* WM_INPUT_H */
