//
// Created by secondwtq <lovejay-lovemusic@outlook.com> 2015/09/09.
// Copyright (c) 2015 SCU ISDC All rights reserved.
//
// This file is part of ISDCNext.
//
// We have always treaded the borderland.
//

#ifndef KALEIDOSCOPE_AST_HXX
#define KALEIDOSCOPE_AST_HXX

#include <string>
#include <vector>
#include <memory>

#include <assert.h>

namespace llvm {
class Value;
class Function;
}

struct ast_base {
    virtual ~ast_base() { }
    virtual llvm::Value *generate_code() = 0;
};

struct ast_number : public ast_base {
    double val;
    ast_number(double v) : val(v) { }
    llvm::Value *generate_code() override;
};

struct ast_var : public ast_base {
    std::string name;
    ast_var(const std::string& n) : name(n) { }
    llvm::Value *generate_code() override;
};

struct ast_binary : public ast_base {
    char op;
    std::unique_ptr<ast_base> lhs, rhs;
    ast_binary(char o, std::unique_ptr<ast_base> l, std::unique_ptr<ast_base> r)
            : op(o), lhs(std::move(l)), rhs(std::move(r)) { }
    llvm::Value *generate_code() override;
};

struct ast_call : public ast_base {
    std::string callee;
    std::vector<std::unique_ptr<ast_base>> args;
    ast_call(const std::string& c, std::vector<std::unique_ptr<ast_base>> a)
            : callee(c), args(std::move(a)) { }
    llvm::Value *generate_code() override;
};

struct ast_if : public ast_base {
    std::unique_ptr<ast_base> cond_, then_, else_;
    ast_if(std::unique_ptr<ast_base> c, std::unique_ptr<ast_base> t, std::unique_ptr<ast_base> e)
            : cond_(std::move(c)), then_(std::move(t)), else_(std::move(e)) { }
    llvm::Value *generate_code() override;
};

struct ast_for : public ast_base {
    std::string var_name;
    std::unique_ptr<ast_base> start, end, step, body;

    ast_for(const std::string& n, std::unique_ptr<ast_base> s, std::unique_ptr<ast_base> e,
        std::unique_ptr<ast_base> st, std::unique_ptr<ast_base> b) :
            var_name(n), start(std::move(s)), end(std::move(e)),
            step(std::move(st)), body(std::move(b)) { }
    llvm::Value *generate_code() override;
};

struct ast_prototype {
    std::string name;
    std::vector<std::string> args;
    bool is_operator;
    size_t precedence;
    ast_prototype(const std::string& n, std::vector<std::string> a, bool is_op = false, size_t precde = 0)
            : name(n), args(a), is_operator(is_op), precedence(precde) { }

    bool is_unary() const {
        return is_operator && args.size() == 1; }
    bool is_binary() const {
        return is_operator && args.size() == 1; }

    char operator_name() const {
        assert(is_unary() || is_binary());
        return name[name.length() - 1]; }

    llvm::Function *generate_code();
};

struct ast_function {
    std::unique_ptr<ast_prototype> proto;
    std::unique_ptr<ast_base> body;
    ast_function(std::unique_ptr<ast_prototype> p, std::unique_ptr<ast_base> b)
            : proto(std::move(p)), body(std::move(b)) { }
    llvm::Function *generate_code();
};

std::unique_ptr<ast_base> error(const char *msg);

#endif // KALEIDOSCOPE_AST_HXX
