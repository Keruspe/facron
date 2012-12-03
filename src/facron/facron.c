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

#include "config.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/fanotify.h>

#include <linux/fanotify.h>
#include <linux/limits.h>

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

static FacronChar
to_FacronChar (char c)
{
    switch (c)
    {
    case 'a': case 'A':
        return C_A;
    case 'b': case 'B':
        return C_B;
    case 'c': case 'C':
        return C_C;
    case 'd': case 'D':
        return C_D;
    case 'e': case 'E':
        return C_E;
    case 'f': case 'F':
        return C_F;
    case 'g': case 'G':
        return C_G;
    case 'h': case 'H':
        return C_H;
    case 'i': case 'I':
        return C_I;
    case 'j': case 'J':
        return C_J;
    case 'k': case 'K':
        return C_K;
    case 'l': case 'L':
        return C_L;
    case 'm': case 'M':
        return C_M;
    case 'n': case 'N':
        return C_N;
    case 'o': case 'O':
        return C_O;
    case 'p': case 'P':
        return C_P;
    case 'q': case 'Q':
        return C_Q;
    case 'r': case 'R':
        return C_R;
    case 's': case 'S':
        return C_S;
    case 't': case 'T':
        return C_T;
    case 'u': case 'U':
        return C_U;
    case 'v': case 'V':
        return C_V;
    case 'w': case 'W':
        return C_W;
    case 'x': case 'X':
        return C_X;
    case 'y': case 'Y':
        return C_Y;
    case 'z': case 'Z':
        return C_Z;
    case '_':
        return UNDERSCORE;
    case '|':
        return PIPE;
    case ' ': case '\t': case '\n': case '\r':
        return SPACE;
    default:
        return OTHER;
    }
}

static const char syms[ERROR][NB_CHARS] =
{
          /*  A,  B,  C,  D,  E,  F,  G,  H,  I,  J,  K,  L,  M,  N,  O,  P,  Q,  R,  S,  T,  U,  V,  W,  X,  Y,  Z,  _,  |, \ , ... */
    [_0] = { __, __, __, __, __, F0, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, __ },
    /* FAN_ */
    [F0] = { F1, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [F1] = { __, __, __, __, __, __, __, __, __, __, __, __, __, F2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [F2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, F3, __, __, __ },
    [F3] = { A0, __, C0, __, E0, __, __, __, __, __, __, __, M0, __, O0, __, Q0, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    /* FAN_ACCESS */
    [A0] = { __, __, A1, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [A1] = { __, __, A2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [A2] = { __, __, __, __, A3, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [A3] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, A4, __, __, __, __, __, __, __, __, __, __, __ },
    [A4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, A5, __, __, __, __, __, __, __, __, __, __, __ },
    [A5] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, A6, _0, _0, __ },
    /* FAN_ACCESS_PERM */
    [A6] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, A7, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [A7] = { __, __, __, __, A8, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [A8] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, A9, __, __, __, __, __, __, __, __, __, __, __, __ },
    [A9] = { __, __, __, __, __, __, __, __, __, __, __, __, AA, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [AA] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_MODIFY */
    [M0] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, M1, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [M1] = { __, __, __, M2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [M2] = { __, __, __, __, __, __, __, __, M3, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [M3] = { __, __, __, __, __, M4, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [M4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, M5, __, __, __, __, __ },
    [M5] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_CLOSE */
    [C0] = { __, __, __, __, __, __, __, __, __, __, __, C1, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [C1] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, C2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [C2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, C3, __, __, __, __, __, __, __, __, __, __, __ },
    [C3] = { __, __, __, __, C4, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [C4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, C5, _0, _0, __ },
    /* FAN_CLOSE_ */
    [C5] = { __, __, __, __, __, __, __, __, __, __, __, __, __, N0, __, __, __, __, __, __, __, __, W0, __, __, __, __, __, __, __ },
    /* FAN_CLOSE_WRITE */
    [W0] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, W1, __, __, __, __, __, __, __, __, __, __, __, __ },
    [W1] = { __, __, __, __, __, __, __, __, W2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [W2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, W3, __, __, __, __, __, __, __, __, __, __ },
    [W3] = { __, __, __, __, W4, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [W4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_CLOSE_NOWRITE */
    [N0] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, N1, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [N1] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, N2, __, __, __, __, __, __, __ },
    [N2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, N3, __, __, __, __, __, __, __, __, __, __, __, __ },
    [N3] = { __, __, __, __, __, __, __, __, N4, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [N4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, N5, __, __, __, __, __, __, __, __, __, __ },
    [N5] = { __, __, __, __, N6, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [N6] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_OPEN */
    [O0] = { __, __, __, __, __, __, __, __, __, __, __, __, __, OA, __, O1, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O1] = { __, __, __, __, O2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, O3, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O3] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, O4, _0, _0, __ },
    /* FAN_OPEN_PERM */
    [O4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, O5, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O5] = { __, __, __, __, O6, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O6] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, O7, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O7] = { __, __, __, __, __, __, __, __, __, __, __, __, O8, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [O8] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_Q_OVERFLOW */
    [Q0] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, Q1, __, __, __ },
    [Q1] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, Q2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [Q2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, Q3, __, __, __, __, __, __, __, __ },
    [Q3] = { __, __, __, __, Q4, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [Q4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, Q5, __, __, __, __, __, __, __, __, __, __, __, __ },
    [Q5] = { __, __, __, __, __, Q6, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [Q6] = { __, __, __, __, __, __, __, __, __, __, __, Q7, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [Q7] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, Q8, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [Q8] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, Q9, __, __, __, __, __, __, __ },
    [Q9] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_ONDIR */
    [OA] = { __, __, __, OB, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [OB] = { __, __, __, __, __, __, __, __, OC, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [OC] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, OD, __, __, __, __, __, __, __, __, __, __, __, __ },
    [OD] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ },
    /* FAN_EVENT_ON_CHILD */
    [E0] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, E1, __, __, __, __, __, __, __, __ },
    [E1] = { __, __, __, __, E2, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [E2] = { __, __, __, __, __, __, __, __, __, __, __, __, __, E3, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [E3] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, E4, __, __, __, __, __, __, __, __, __, __ },
    [E4] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, E5, __, __, __ },
    [E5] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, E6, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [E6] = { __, __, __, __, __, __, __, __, __, __, __, __, __, E7, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [E7] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, E8, __, __, __ },
    [E8] = { __, __, E9, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [E9] = { __, __, __, __, __, __, __, EA, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [EA] = { __, __, __, __, __, __, __, __, EB, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [EB] = { __, __, __, __, __, __, __, __, __, __, __, EC, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [EC] = { __, __, __, ED, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __ },
    [ED] = { __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, _0, _0, __ }
          /*  A,  B,  C,  D,  E,  F,  G,  H,  I,  J,  K,  L,  M,  N,  O,  P,  Q,  R,  S,  T,  U,  V,  W,  X,  Y,  Z,  _,  |, \ , ... */
};

typedef struct
{
    char *path;
    unsigned long mask;
} FacronEntry;

typedef struct fanotify_event_metadata FacronMetadata;

static bool
is_space (char c)
{
    return (c == ' ' || c == '\t');
}

static unsigned long
FacronToken_to_mask (FacronToken t)
{
    switch (t)
    {
    case FAN_ACCESS_TOK:
        return FAN_ACCESS;
    case FAN_MODIFY_TOK:
        return FAN_MODIFY;
    case FAN_CLOSE_WRITE_TOK:
        return FAN_CLOSE_WRITE;
    case FAN_CLOSE_NOWRITE_TOK:
        return FAN_CLOSE_NOWRITE;
    case FAN_OPEN_TOK:
        return FAN_OPEN;
    case FAN_Q_OVERFLOW_TOK:
        return FAN_Q_OVERFLOW;
    case FAN_OPEN_PERM_TOK:
        return FAN_OPEN_PERM;
    case FAN_ACCESS_PERM_TOK:
        return FAN_ACCESS_PERM;
    case FAN_ONDIR_TOK:
        return FAN_ONDIR;
    case FAN_EVENT_ON_CHILD_TOK:
        return FAN_EVENT_ON_CHILD;
    case FAN_CLOSE_TOK:
        return FAN_CLOSE;
    case EMPTY:
        return 0;
    default:
        fprintf (stderr, "Warning: unknown token: %d\n", t);
        return 0;
    }
}

static bool
read_next (FacronEntry *entry, FILE *conf)
{
    entry->path = NULL;
    entry->mask = 0;

    size_t len;
    if (getline (&entry->path, &len, conf) < 1)
        return false;
    char *line = entry->path;

    for (size_t i = 0; i < len; ++i)
    {
        if (is_space (line[i]))
        {
            line[i] = '\0';
            line += i + 1;
            len -= (i + 1);
            printf ("path to monitor: \"%s\"\n", entry->path);
            break;
        }
    }

    FacronToken token = EMPTY;
    FacronToken prev_token;
    for (size_t i = 0; i < len; ++i)
    {
        prev_token = token;
        FacronChar c = to_FacronChar (line[i]);
        token = syms[token][c];
        switch (token)
        {
        case ERROR:
            fprintf (stderr, "Error at char %c: \"%s\" not understood\n", line[i], line);
            return false;
        case EMPTY:
            entry->mask |= FacronToken_to_mask (prev_token);
            if (c == SPACE && entry->mask != 0)
                return true;
        default:
            break;
        }
    }

    return true;
}

static bool
watch_next (int fanotify_fd, FILE *conf)
{
    FacronEntry entry;
    if (!read_next (&entry, conf))
        return false;

    /* TODO: FAN_MARK_REMOVE */
    return fanotify_mark (fanotify_fd, FAN_MARK_ADD, entry.mask, AT_FDCWD, entry.path);
}

static void
load_conf (int fanotify_fd)
{
    FILE *conf = fopen ("/etc/facron.conf", "r");

    while (watch_next (fanotify_fd, conf));

    fclose (conf);
}

int
main (void)
{
    int fanotify_fd = fanotify_init (FAN_CLASS_NOTIF, O_RDONLY | O_LARGEFILE);

    if (fanotify_fd < 0)
    {
        fprintf (stderr, "Could not initialize fanotify\n");
        return -1;
    }

    load_conf (fanotify_fd);

    char buf[4096];
    size_t len;

    while ((len = read (fanotify_fd, buf, sizeof (buf))) > 0)
    {
        char path[PATH_MAX];
        int path_len;

        for (FacronMetadata *metadata = (FacronMetadata *) buf; FAN_EVENT_OK (metadata, len); metadata = FAN_EVENT_NEXT (metadata, len))
        {
            if (metadata->vers < 2)
            {
                fprintf (stderr, "Kernel fanotify version too old\n");
                close (metadata->fd);
                goto fail;
            }

            if (metadata->fd < 0)
                continue;

            sprintf (path, "/proc/self/fd/%d", metadata->fd);
            path_len = readlink (path, path, sizeof (path) - 1);
            if (path_len < 0)
                goto next;
            path[path_len] = '\0';

next:
            close (metadata->fd);
        }
    }

fail:
    close (fanotify_fd);

    return 0;
}
