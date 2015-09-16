//
// Created by secondwtq <lovejay-lovemusic@outlook.com> 2015/09/09.
// Copyright (c) 2015 SCU ISDC All rights reserved.
//
// This file is part of ISDCNext.
//
// We have always treaded the borderland.
//

#ifndef KALEIDOSCOPE_LEXER_HXX
#define KALEIDOSCOPE_LEXER_HXX

#include "common.hxx"

enum Token {
    T_EOF = -1,
    T_DEF = -2,

    T_EXTERN = -3,
    T_ID = -4,
    T_NUMBER = -5,

    T_IF = -6,
    T_THEN = -7,
    T_ELSE = -8,

    T_FOR = -9,
    T_IN = -10,

    T_BINARY = -11,
    T_UNARY = -12,

    T_START = -13,
};

TokenT get_token();
TokenT next_token();

#endif // KALEIDOSCOPE_LEXER_HXX
