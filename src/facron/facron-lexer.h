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

#ifndef __FACRON_CONF_LEXER_H__
#define __FACRON_CONF_LEXER_H__

#include <stdbool.h>

typedef struct FacronLexer FacronLexer;

typedef enum
{
    _0, /* BEGIN */
    /* FAN_ */
    F0, /* F */
    F1, /* A */
    F2, /* N */
    F3, /* _ */
    /* FAN_A */
    A0, /* A */
    /* FAN_ACCESS */
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
    /* FAN_O */
    O0, /* O */
    /* FAN_OPEN */
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
    /* FAN_ALL_ */
    AD, /* L */
    AE, /* L */
    AF, /* _ */
    /* FAN_ALL_EVENTS */
    EG, /* E */
    EH, /* V */
    EI, /* E */
    EJ, /* N */
    EK, /* T */
    EL, /* S */
    /* FAN_ALL_PERM_EVENTS */
    P0, /* P */
    P1, /* E */
    P2, /* R */
    P3, /* M */
    P4, /* _ */
    P5, /* E */
    P6, /* V */
    P7, /* E */
    P8, /* N */
    P9, /* T */
    PA, /* S */
    /* FAN_ALL_OUTGOING_EVENTS */
    OF, /* O */
    OG, /* U */
    OH, /* T */
    OI, /* G */
    OJ, /* O */
    OK, /* I */
    OL, /* N */
    OM, /* G */
    ON, /* _ */
    OO, /* E */
    OP, /* V */
    OQ, /* E */
    OR, /* N */
    OS, /* T */
    OT, /* S */
    __ /* ERROR */
} FacronState;

#define EMPTY _0
#define ERROR __

typedef enum
{
    T_FAN_ACCESS         = A5,
    T_FAN_MODIFY         = M5,
    T_FAN_CLOSE_WRITE    = W4,
    T_FAN_CLOSE_NOWRITE  = N6,
    T_FAN_OPEN           = O3,
    T_FAN_Q_OVERFLOW     = Q9,
    T_FAN_OPEN_PERM      = O8,
    T_FAN_ACCESS_PERM    = AA,
    T_FAN_ONDIR          = OD,
    T_FAN_EVENT_ON_CHILD = ED,
    /* aliases */
    T_FAN_CLOSE               = C4,
    T_FAN_ALL_EVENTS          = EL,
    T_FAN_ALL_PERM_EVENTS     = PA,
    T_FAN_ALL_OUTGOING_EVENTS = OT,
    /* special case */
    T_EMPTY = EMPTY
} FacronToken;

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
    C_UNDERSCORE,
    C_PIPE,
    C_SPACE,
    C_COMMA,
    C_OTHER,
    NB_CHARS
} FacronChar;

typedef enum
{
    R_END,
    R_ERROR,
    R_PIPE,
    R_COMMA
} FacronResult;

bool  facron_lexer_read_line    (FacronLexer *lexer);
bool  facron_lexer_invalid_line (FacronLexer *lexer);
bool  facron_lexer_end_of_line  (FacronLexer *lexer);
char *facron_lexer_read_string  (FacronLexer *lexer);
void  facron_lexer_skip_spaces  (FacronLexer *lexer);

FacronResult facron_lexer_next_token (FacronLexer *lexer, unsigned long long *mask);

bool facron_lexer_reload_file (FacronLexer *lexer);

void facron_lexer_free (FacronLexer *lexer);

FacronLexer *facron_lexer_new (const char *filename);

#endif /* __FACRON_CONF_LEXER_H__ */
