//
// Created by secondwtq <lovejay-lovemusic@outlook.com> 2015/09/09.
// Copyright (c) 2015 SCU ISDC All rights reserved.
//
// This file is part of ISDCNext.
//
// We have always treaded the borderland.
//

#ifndef KALEIDOSCOPE_LL_COMMON_HXX
#define KALEIDOSCOPE_LL_COMMON_HXX

#include <string>
#include <map>
#include <memory>

#include <llvm/IR/IRBuilder.h>

namespace llvm {
namespace legacy {
class FunctionPassManager;
}
}

class ast_prototype;

namespace state {
extern std::unique_ptr<llvm::Module> ll_module;
extern std::map<std::string, llvm::Value *> ll_value_map;

// extern variable with an initializer
extern llvm::IRBuilder<> ll_builder;
extern std::unique_ptr<llvm::legacy::FunctionPassManager> ll_fpm;

extern std::map<std::string, std::unique_ptr<ast_prototype>> protos;
}

#endif // KALEIDOSCOPE_LL_COMMON_HXX
