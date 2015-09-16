//
// Created by secondwtq <lovejay-lovemusic@outlook.com> 2015/09/09.
// Copyright (c) 2015 SCU ISDC All rights reserved.
//
// This file is part of ISDCNext.
//
// We have always treaded the borderland.
//

#include "parser.hxx"
#include "common.hxx"
#include "lexer.hxx"
#include "ast.hxx"

#include <memory>
#include "llvm/ADT/STLExtras.h"

#include <map>
extern std::map<char, int> binary_op_preced;

enum OperatorType {
    IDENTIFIER,
    UNARY,
    BINARY
};

std::unique_ptr<ast_prototype> error_p(const char *msg) {
    error(msg);
    return nullptr;
}

std::unique_ptr<ast_base> parse_number() {
    auto ret = llvm::make_unique<ast_number>(state::number);
    next_token();
    return std::move(ret);
}

std::unique_ptr<ast_base> parse_parenthesis() {
    next_token();
    auto v = parse_expression();
    if (!v)
        return nullptr;
    if (state::cur_token != ')') {
        return error("expect ')'."); }
    next_token();
    return v;
}

std::unique_ptr<ast_base> parse_identifier() {
    std::string name = state::identifier;
    next_token();

    if (state::cur_token != '(') {
        return llvm::make_unique<ast_var>(name); }
    next_token();
    std::vector<std::unique_ptr<ast_base>> args;
    if (state::cur_token != ')') {
        while (1) {
            if (auto arg = parse_expression()) {
                args.push_back(std::move(arg));
            } else { return nullptr; }
            if (state::cur_token == ')') {
                break; }
            if (state::cur_token != ',') {
                return error("expected ')' or ',' in argument list."); }
            next_token();
        }
    }

    next_token();
    return llvm::make_unique<ast_call>(name, std::move(args));
}

std::unique_ptr<ast_base> parse_primary() {
    switch (state::cur_token) {
        case T_ID:
            return parse_identifier();
        case T_NUMBER:
            return parse_number();
        case '(':
            return parse_parenthesis();
        case T_IF:
            return parse_if();
        case T_FOR:
            return parse_for();
        default:
            return error("unknown token when expecting an expression.");
    }
}

int token_precedence() {
    if (!isascii(state::cur_token)) {
        return -1; }

    int ret = binary_op_preced[state::cur_token];
    if (ret <= 0) {
        return -1; }
    return ret;
}

std::unique_ptr<ast_base> parse_expression() {
    auto lhs = parse_primary();
    if (!lhs) {
        return nullptr; }
    return parse_binary_op_rhs(0, std::move(lhs));
}

std::unique_ptr<ast_base> parse_binary_op_rhs(int preced, std::unique_ptr<ast_base> lhs) {
    while (1) {
        int prec = token_precedence();
        if (prec < preced) {
            return lhs; }

        int binary_op = state::cur_token;
        next_token();

        auto rhs = parse_primary();
        if (!rhs) {
            return nullptr; }

        int next_prec = token_precedence();
        if (prec < next_prec) {
            rhs = parse_binary_op_rhs(prec+1, std::move(rhs));
            if (!rhs) {
                return nullptr; }
        }

        lhs = llvm::make_unique<ast_binary>(binary_op, std::move(lhs), std::move(rhs));
    }
}

std::unique_ptr<ast_base> parse_if() {
    next_token();

    auto cond_ = parse_expression();
    printf("%s\n", ((ast_var *) cond_.get())->name.c_str());
    if (!cond_) {
        return nullptr; }
    if (state::cur_token != T_THEN) {
        return error("expected then."); }
    next_token();

    auto then_ = parse_expression();
    if (!then_) {
        return nullptr; }
    if (state::cur_token != T_ELSE) {
        return error("expected else"); }
    next_token();

    auto else_ = parse_expression();
    if (!else_) {
        return nullptr; }

    return llvm::make_unique<ast_if>(std::move(cond_),
            std::move(then_), std::move(else_));
}

std::unique_ptr<ast_base> parse_for() {
    next_token();

    if (state::cur_token != T_ID) {
        return error("expected identifier after for."); }

    std::string id_name = state::identifier;
    next_token();

    if (state::cur_token != '=') {
        return error("expected '=' after for."); }
    next_token();

    auto start = parse_expression();
    if (!start) {
        return nullptr; }
    if (state::cur_token != ',') {
        return error("expected ',' after for start value."); }
    next_token();

    auto end = parse_expression();
    if (!end) {
        return nullptr; }

    std::unique_ptr<ast_base> step;
    if (state::cur_token == ',') {
        next_token();
        step = parse_expression();
        if (!step) {
            return nullptr; }
    }

    if (state::cur_token != T_IN) {
        return error("expected 'in' after for."); }
    next_token();

    auto body = parse_expression();
    if (!body) {
        return nullptr; }

    return llvm::make_unique<ast_for>(id_name, std::move(start), std::move(end),
        std::move(step), std::move(body));
}

std::unique_ptr<ast_prototype> parse_prototype() {
    if (state::cur_token != T_ID) {
        return error_p("expect function name in prototype."); }
    std::string function_name;
    OperatorType type = IDENTIFIER;
    size_t precedence = 30;

    switch (state::cur_token) {
        case T_ID:
            function_name = state::identifier;
            type = IDENTIFIER;
            next_token();
            break;
        case T_BINARY:
            next_token();
            if (!isascii(state::cur_token)) {
                return error_p("expected binary operator."); }
            function_name = "binary";
            function_name += (char) state::cur_token;
            type = BINARY;
            next_token();
            if (state::cur_token == T_NUMBER) {
                if (state::number < 1 || state::number > 100) {
                    return error_p("invalid precedence: must be 1..100."); }
                precedence = (size_t) state::number;
                next_token();
            }
            break;
        case T_UNARY:
            break;
    }

    if (state::cur_token != '(') {
        return error_p("expected '(' in prototype."); }

    std::vector<std::string> arg_names;
    while (next_token() == T_ID) {
        arg_names.push_back(state::identifier); }
    if (state::cur_token != ')') {
        return error_p("expected ')' in prototype."); }

    next_token();
    return llvm::make_unique<ast_prototype>(function_name, std::move(arg_names));
}

std::unique_ptr<ast_function> parse_definition() {
    next_token();
    auto proto = parse_prototype();
    if (!proto) {
        return nullptr; }
    if (auto e = parse_expression()) {
        return llvm::make_unique<ast_function>(std::move(proto), std::move(e));
    } else { return nullptr; }
}

std::unique_ptr<ast_prototype> parse_extern() {
    next_token();
    return parse_prototype();
}

std::unique_ptr<ast_function> parse_top_level_exp() {
    if (auto e = parse_expression()) {
        auto proto = llvm::make_unique<ast_prototype>("__anon_expr", std::vector<std::string>());
        return llvm::make_unique<ast_function>(std::move(proto), std::move(e));
    }
    return nullptr;
}
