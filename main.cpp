
#include <string>
#include <ctype.h>
#include <stdlib.h>
#include <memory>
#include <vector>
#include <map>

#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

#include <llvm/Support/TargetSelect.h>

#include <llvm/Analysis/Passes.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/Scalar.h>

#include "Kaleidoscope.hxx"

#include "common.hxx"
#include "lexer.hxx"
#include "parser.hxx"
#include "ast.hxx"

std::map<char, int> binary_op_preced;

void initialize_module_n_pass();

namespace state {
std::string identifier;
double number;
TokenT cur_token;

std::unique_ptr<llvm::Module> ll_module;
llvm::IRBuilder<> ll_builder(llvm::getGlobalContext());
std::map<std::string, llvm::Value *> ll_value_map;
std::unique_ptr<llvm::legacy::FunctionPassManager> ll_fpm;
std::unique_ptr<llvm::orc::KaleidoscopeJIT> ll_jit;
std::map<std::string, std::unique_ptr<ast_prototype>> protos;
}

TokenT get_token() {
    static TokenT last_chr = ' ';

    while (isspace(last_chr)) {
        last_chr = getchar(); }

    // identifiers & keywords
    if (isalpha(last_chr)) {
        state::identifier = last_chr;
        while (isalnum((last_chr = getchar()))) {
            state::identifier += last_chr; }
        if (state::identifier == "def") {
            return T_DEF; }
        if (state::identifier == "extern") {
            return T_EXTERN; }
        if (state::identifier == "if") {
            return T_IF; }
        if (state::identifier == "then") {
            return T_THEN; }
        if (state::identifier == "else") {
            return T_ELSE; }
        if (state::identifier == "for") {
            return T_FOR; }
        if (state::identifier == "in") {
            return T_IN; }
        if (state::identifier == "binary") {
            return T_BINARY; }
        if (state::identifier == "unary") {
            return T_UNARY; }
        return T_ID;
    }

    // (floating point) numbers
    if (isdigit(last_chr) || last_chr == '.') {
        std::string str_num = "";
        do {
            str_num += last_chr;
            last_chr = getchar();
        } while (isdigit(last_chr) || last_chr == '.');

        state::number = strtod(str_num.c_str(), 0);
        return T_NUMBER;
    }

    // comments
    if (last_chr == '#') {
        do {
            last_chr = getchar();
        } while (last_chr != EOF && last_chr != '\n' && last_chr != '\r');
        if (last_chr != EOF)
            return get_token();
    }

    // EOF
    if (last_chr == EOF) {
        return T_EOF; }

    // other characters
    TokenT this_chr = last_chr;
    last_chr = getchar();
    return this_chr;
}

TokenT next_token() {
    return (state::cur_token = get_token()); }

std::unique_ptr<ast_base> error(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    return nullptr;
}

llvm::Value *error_codegen(const char *msg) {
    error(msg);
    return 0;
}

void handle_definition() {
    if (auto ast = parse_definition()) {
        if (auto ir = ast->generate_code()) {
            fprintf(stderr, "read function definition: ");
            ir->dump();
            state::ll_jit->addModule(std::move(state::ll_module));
            initialize_module_n_pass();
        }
    } else {
        next_token();
    }
}

void handle_extern() {
    if (auto ast = parse_extern()) {
        if (auto ir = ast->generate_code()) {
            fprintf(stderr, "read extern: ");
            ir->dump();
            state::protos[ast->name] = std::move(ast);
        }
    } else {
        next_token();
    }
}

void handle_top_level_exp() {
    if (auto ast = parse_top_level_exp()) {
        if (auto ir = ast->generate_code()) {
            fprintf(stderr, "parsed a top-level expr.\n");
            ir->dump();

            auto h = state::ll_jit->addModule(std::move(state::ll_module));
            initialize_module_n_pass();
            auto expr_symbol = state::ll_jit->findSymbol("__anon_expr");
            assert(expr_symbol && "function not found");

            double (*fp)() = (double (*)())(intptr_t) expr_symbol.getAddress();
            fprintf(stderr, "evaluated to %f\n", fp());

            state::ll_jit->removeModule(h);
        }
    } else {
        fprintf(stderr, "failed to parse top-level expr.\n");
        next_token();
    }
}

void handle_token(TokenT token) {
    switch (token) {
        case ';':
            next_token();
            break;
        case T_DEF:
            handle_definition();
            break;
        case T_EXTERN:
            handle_extern();
            break;
        case T_START:
            next_token();
            handle_token(state::cur_token);
            break;
        default:
            handle_top_level_exp();
            break;
    }
}

void main_loop() {
    while (1) {
        if (state::cur_token == EOF) {
            return; }
        handle_token(state::cur_token);
        fprintf(stderr, "ready> ");
    }
}

void initialize_module_n_pass() {
    state::ll_module = llvm::make_unique<llvm::Module>(
            "my cool jit", llvm::getGlobalContext());
    state::ll_module->setDataLayout(state::ll_jit->getTargetMachine().createDataLayout());

    state::ll_fpm = llvm::make_unique<llvm::legacy::FunctionPassManager>(state::ll_module.get());
    state::ll_fpm->add(llvm::createBasicAliasAnalysisPass());
    state::ll_fpm->add(llvm::createInstructionCombiningPass());
    state::ll_fpm->add(llvm::createReassociatePass());
    state::ll_fpm->add(llvm::createGVNPass());
    state::ll_fpm->add(llvm::createCFGSimplificationPass());
    state::ll_fpm->doInitialization();
}

extern "C" double putchard(double x) {
    fputc((char) x, stderr);
    return 0;
}

extern "C" double test0() {
    printf("test0.\n");
    return 0.0;
}

extern "C" double test1() {
    printf("test1.\n");
    return 0.0;
}

int main() {

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    binary_op_preced['<'] = 10;
    binary_op_preced['+'] = 20;
    binary_op_preced['-'] = 20;
    binary_op_preced['*'] = 40;

    fprintf(stderr, "ready> ");
    next_token();

    state::ll_jit = llvm::make_unique<llvm::orc::KaleidoscopeJIT>();
    initialize_module_n_pass();

    main_loop();

    state::ll_module->dump();

    return 0;
}
