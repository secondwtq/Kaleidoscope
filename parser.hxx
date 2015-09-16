//
// Created by secondwtq <lovejay-lovemusic@outlook.com> 2015/09/09.
// Copyright (c) 2015 SCU ISDC All rights reserved.
//
// This file is part of ISDCNext.
//
// We have always treaded the borderland.
//

#ifndef KALEIDOSCOPE_PARSER_HXX
#define KALEIDOSCOPE_PARSER_HXX

#include <memory>

class ast_base;
class ast_function;
class ast_prototype;

std::unique_ptr<ast_base> parse_number();
std::unique_ptr<ast_base> parse_parenthesis();
std::unique_ptr<ast_base> parse_identifier();
std::unique_ptr<ast_base> parse_primary();
std::unique_ptr<ast_base> parse_expression();
std::unique_ptr<ast_base> parse_binary_op_rhs(int preced, std::unique_ptr<ast_base> lhs);
std::unique_ptr<ast_base> parse_if();
std::unique_ptr<ast_base> parse_for();
std::unique_ptr<ast_prototype> parse_prototype();
std::unique_ptr<ast_function> parse_definition();
std::unique_ptr<ast_prototype> parse_extern();
std::unique_ptr<ast_function> parse_top_level_exp();

std::unique_ptr<ast_base> parse_expression();
std::unique_ptr<ast_base> parse_binary_op_rhs(
        int preced, std::unique_ptr<ast_base> lhs);

#endif // KALEIDOSCOPE_PARSER_HXX
