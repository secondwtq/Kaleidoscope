//
// Created by secondwtq <lovejay-lovemusic@outlook.com> 2015/09/09.
// Copyright (c) 2015 SCU ISDC All rights reserved.
//
// This file is part of ISDCNext.
//
// We have always treaded the borderland.
//

#include "codegen.hxx"

#include "ll_common.hxx"
#include "ast.hxx"

#include <vector>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>

#include <llvm/IR/LegacyPassManager.h>

using namespace llvm;

Function *get_function(const std::string& name) {
    if (auto *f = state::ll_module->getFunction(name)) {
        return f; }

    auto fi = state::protos.find(name);
    if (fi != state::protos.end())
        return fi->second->generate_code();

    return nullptr;
}

Value *ast_number::generate_code() {
    return ConstantFP::get(getGlobalContext(), APFloat(val));
}

llvm::Value *ast_var::generate_code() {
    Value *ret = state::ll_value_map[name];
    if (!ret) {
        error_codegen("unknown variable name.");
        error_codegen(name.c_str());
    }
    return ret;
}

llvm::Value *ast_binary::generate_code() {
    Value *l = lhs->generate_code(),
        *r = rhs->generate_code();
    if (!l || !r) {
        return nullptr; }

    switch (op) {
        case '+':
            return state::ll_builder.CreateFAdd(l, r, "addtmp");
        case '-':
            return state::ll_builder.CreateFSub(l, r, "subtmp");
        case '*':
            return state::ll_builder.CreateFMul(l, r, "multmp");
        case '<':
            l = state::ll_builder.CreateFCmpULT(l, r, "cmptmp");
            return state::ll_builder.CreateUIToFP(l,
                    Type::getDoubleTy(getGlobalContext()), "booltmp");
        default:
            return error_codegen("invalid binary operator");
    }
}

llvm::Value *ast_call::generate_code() {
    Function *callee_func = get_function(callee);
    if (!callee_func) {
        return error_codegen("unknown function referenced."); }

    if (callee_func->arg_size() != args.size()) {
        return error_codegen("incorrect # arguments passed."); }

    std::vector<Value *> argv;
    for (size_t i = 0; i < args.size(); i++) {
        argv.push_back(args[i]->generate_code());
        if (!argv.back()) {
            return nullptr; }
    }

    return state::ll_builder.CreateCall(callee_func, argv, "calltmp");
}

llvm::Function *ast_prototype::generate_code() {
    std::vector<Type *> arg_types(args.size(), Type::getDoubleTy(getGlobalContext()));
    FunctionType *ft = FunctionType::get(Type::getDoubleTy(getGlobalContext()), arg_types, false);
    Function *f = Function::Create(ft, Function::ExternalLinkage, name, state::ll_module.get());

    size_t idx = 0;
    for (auto &arg : f->args()) {
        arg.setName(args[idx++]); }
    return f;
}

llvm::Function *ast_function::generate_code() {
    std::string name = proto->name;
    // Function *func = get_function(name);
    Function *func = get_function(proto->name);
    if (!func) {
        func = proto->generate_code(); }
    if (!func) {
        return nullptr; }
    if (!func->empty()) {
        return (Function *) error_codegen("function cannot be redefined."); }

    BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", func);
    state::ll_builder.SetInsertPoint(bb);
    state::ll_value_map.clear();
    for (auto& arg : func->args()) {
        state::ll_value_map[arg.getName()] = &arg; }
    if (Value *ret = body->generate_code()) {
        state::ll_builder.CreateRet(ret);
        verifyFunction(*func);

        state::ll_fpm->run(*func);
        state::protos[name] = std::move(proto);

        return func;
    }
    func->eraseFromParent();
    return nullptr;
}

llvm::Value *ast_if::generate_code() {
    Value *cond = cond_->generate_code();
    if (!cond) {
        return nullptr; }
    cond = state::ll_builder.CreateFCmpONE(cond,
            ConstantFP::get(getGlobalContext(), APFloat(0.0)), "ifcond");

    Function *func = state::ll_builder.GetInsertBlock()->getParent();
    BasicBlock *then_bb = BasicBlock::Create(getGlobalContext(), "then", func);
    BasicBlock *else_bb = BasicBlock::Create(getGlobalContext(), "else");
    BasicBlock *merge_bb = BasicBlock::Create(getGlobalContext(), "ifcont");

    state::ll_builder.CreateCondBr(cond, then_bb, else_bb);
    state::ll_builder.SetInsertPoint(then_bb);
    Value *then = then_->generate_code();
    if (!then) {
        return nullptr; }

    state::ll_builder.CreateBr(merge_bb);
    then_bb = state::ll_builder.GetInsertBlock();

    func->getBasicBlockList().push_back(else_bb);
    state::ll_builder.SetInsertPoint(else_bb);
    Value *else_v = else_->generate_code();
    if (!else_v) {
        return nullptr; }

    state::ll_builder.CreateBr(merge_bb);
    else_bb = state::ll_builder.GetInsertBlock();
    func->getBasicBlockList().push_back(merge_bb);
    state::ll_builder.SetInsertPoint(merge_bb);
    PHINode *pn = state::ll_builder.CreatePHI(Type::getDoubleTy(getGlobalContext()), 2, "iftmp");
    pn->addIncoming(then, then_bb);
    pn->addIncoming(else_v, else_bb);

    return pn;
}

llvm::Value *ast_for::generate_code() {
    Value *start_val = start->generate_code();
    if (start_val == 0) {
        return 0; }
    Function *func = state::ll_builder.GetInsertBlock()->getParent();
    BasicBlock *preheader_bb = state::ll_builder.GetInsertBlock();
    BasicBlock *loop_bb = BasicBlock::Create(getGlobalContext(), "loop", func);

    state::ll_builder.CreateBr(loop_bb);
    state::ll_builder.SetInsertPoint(loop_bb);
    PHINode *var = state::ll_builder.CreatePHI(Type::getDoubleTy(getGlobalContext()),
            2, var_name.c_str());
    var->addIncoming(start_val, preheader_bb);

    Value *old_val = state::ll_value_map[var_name];
    state::ll_value_map[var_name] = var;

    if (!body->generate_code()) {
        return nullptr; }

    Value *step_val = nullptr;
    if (step) {
        step_val = step->generate_code();
        if (!step_val) {
            return nullptr; }
    } else {
        step_val = ConstantFP::get(getGlobalContext(), APFloat(1.0));
    }

    Value *next_var = state::ll_builder.CreateFAdd(var, step_val, "nextvar");
    Value *end_cond = end->generate_code();
    if (!end_cond) {
        return nullptr; }
    end_cond = state::ll_builder.CreateFCmpONE(end_cond,
            ConstantFP::get(getGlobalContext(), APFloat(0.0)), "loopcond");

    BasicBlock *loop_end_bb = state::ll_builder.GetInsertBlock();
    BasicBlock *after_bb = BasicBlock::Create(getGlobalContext(), "afterloop", func);
    state::ll_builder.CreateCondBr(end_cond, loop_bb, after_bb);
    state::ll_builder.SetInsertPoint(after_bb);
    var->addIncoming(next_var, loop_end_bb);

    if (old_val) {
        state::ll_value_map[var_name] = old_val;
    } else { state::ll_value_map.erase(var_name); }
    return Constant::getNullValue(Type::getDoubleTy(getGlobalContext()));
}
