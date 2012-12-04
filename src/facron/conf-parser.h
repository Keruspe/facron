/*
 *      This file is part of facron.
 *
 *      Copyright 2012 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
 *
 *      facron is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      facron is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with facron.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FACRON_CONF_PARSER_H__
#define __FACRON_CONF_PARSER_H__

typedef enum
{
    _0, /* BEGIN */
    /* FAN_ */
    F0, /* F */
    F1, /* A */
    F2, /* N */
    F3, /* _ */
    /* FAN_ACCESS */
    A0, /* A */
    A1, /* C */
    A2, /* C */
    A3, /* E */
    A4, /* S */
    A5, /* S */
    /* FAN_ACCESS_PERM */
    A6, /* _ */
    A7, /* P */
    A8, /* E */
    A9, /* R */
    AA, /* M */
    /* FAN_MODIFY */
    M0, /* M */
    M1, /* O */
    M2, /* D */
    M3, /* I */
    M4, /* F */
    M5, /* Y */
    /* FAN_CLOSE */
    C0, /* C */
    C1, /* L */
    C2, /* O */
    C3, /* S */
    C4, /* E */
    /* FAN_CLOSE_ */
    C5, /* _ */
    /* FAN_CLOSE_WRITE */
    W0, /* W */
    W1, /* R */
    W2, /* I */
    W3, /* T */
    W4, /* E */
    /* FAN_CLOSE_NOWRITE */
    N0, /* N */
    N1, /* O */
    N2, /* W */
    N3, /* R */
    N4, /* I */
    N5, /* T */
    N6, /* E */
    /* FAN_OPEN */
    O0, /* O */
    O1, /* P */
    O2, /* E */
    O3, /* N */
    /* FAN_OPEN_PERM */
    O4, /* _ */
    O5, /* P */
    O6, /* E */
    O7, /* R */
    O8, /* M */
    /* FAN_Q_OVERFLOW */
    Q0, /* Q */
    Q1, /* _ */
    Q2, /* O */
    Q3, /* V */
    Q4, /* E */
    Q5, /* R */
    Q6, /* F */
    Q7, /* L */
    Q8, /* O */
    Q9, /* W */
    /* FAN_ONDIR */
    OA, /* N */
    OB, /* D */
    OC, /* I */
    OD, /* R */
    /* FAN_EVENT_ON_CHILD */
    E0, /* E */
    E1, /* V */
    E2, /* E */
    E3, /* N */
    E4, /* T */
    E5, /* _ */
    E6, /* O */
    E7, /* N */
    E8, /* _ */
    E9, /* C */
    EA, /* H */
    EB, /* I */
    EC, /* L */
    ED, /* D */
    __ /* ERROR */
} FacronToken;

#define EMPTY _0
#define ERROR __

#define FAN_ACCESS_TOK         A5
#define FAN_MODIFY_TOK         M5
#define FAN_CLOSE_WRITE_TOK    W4
#define FAN_CLOSE_NOWRITE_TOK  N6
#define FAN_OPEN_TOK           O3
#define FAN_Q_OVERFLOW_TOK     Q9
#define FAN_OPEN_PERM_TOK      O8
#define FAN_ACCESS_PERM_TOK    AA
#define FAN_ONDIR_TOK          OD
#define FAN_EVENT_ON_CHILD_TOK ED
#define FAN_CLOSE_TOK          C4

typedef enum
{
    C_A,
    C_B,
    C_C,
    C_D,
    C_E,
    C_F,
    C_G,
    C_H,
    C_I,
    C_J,
    C_K,
    C_L,
    C_M,
    C_N,
    C_O,
    C_P,
    C_Q,
    C_R,
    C_S,
    C_T,
    C_U,
    C_V,
    C_W,
    C_X,
    C_Y,
    C_Z,
    UNDERSCORE,
    PIPE,
    SPACE,
    OTHER,
    NB_CHARS
} FacronChar;

typedef struct FacronConfEntry FacronConfEntry;

struct FacronConfEntry
{
    char *path;
    unsigned long long mask;
    char *command[512];
    FacronConfEntry *next;
};

typedef struct fanotify_event_metadata FacronMetadata;

FacronConfEntry *load_conf (void);

void unload_conf (FacronConfEntry *conf);

#endif /* __FACRON_CONF_PARSER_H_ */
