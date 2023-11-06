#pragma once

#include <stdint.h>

/* definitions from zile - modifiers */
#define KBD_CTRL                        01000
#define KBD_META                        02000

/* Common non-alphanumeric keys. */
#define KBD_CANCEL                      (KBD_CTRL | 'g')
#define KBD_TAB                         00402
#define KBD_RET                         00403
#define KBD_PGUP                        00404
#define KBD_PGDN                        00405
#define KBD_HOME                        00406
#define KBD_END                         00407
#define KBD_DEL                         00410
#define KBD_BS                          00411
#define KBD_INS                         00412
#define KBD_LEFT                        00413
#define KBD_RIGHT                       00414
#define KBD_UP                          00415
#define KBD_DOWN                        00416
#define KBD_F1                          00420
#define KBD_F2                          00421
#define KBD_F3                          00422
#define KBD_F4                          00423
#define KBD_F5                          00424
#define KBD_F6                          00425
#define KBD_F7                          00426
#define KBD_F8                          00427
#define KBD_F9                          00430
#define KBD_F10                         00431
#define KBD_F11                         00432
#define KBD_F12                         00433

#define KBD_NOKEY                       03777

int32_t deemacs_next_key( void );

char* deemacs_key_to_str_representation( int32_t key );
