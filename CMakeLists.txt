cmake_minimum_required(VERSION 3.3)
project(Kaleidoscope)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti")

set(SOURCE_FILES main.cpp parser.hxx lexer.hxx parser.cxx ast.cxx ast.hxx common.cxx common.hxx codegen.cxx codegen.hxx ll_common.hxx Kaleidoscope.hxx)
include_directories(/usr/local/opt/llvm37/include)

add_definitions(-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS)

link_directories(/usr/local/opt/llvm37/lib)
add_executable(Kaleidoscope ${SOURCE_FILES})
set(KALEIDOSCOPE_LIBS LLVMCore LLVMSupport
        LLVMAnalysis LLVMScalarOpts LLVMTransformUtils LLVMInstCombine
        LLVMRuntimeDyld LLVMTarget LLVMObject LLVMMC LLVMExecutionEngine LLVMMCParser LLVMBitReader
        LTO LLVMCodeGen LLVMAsmPrinter LLVMSelectionDAG LLVMMCDisassembler LLVMInstrumentation
        LLVMX86AsmParser LLVMX86AsmPrinter LLVMX86CodeGen LLVMX86Info LLVMX86Desc LLVMX86Utils)
list(APPEND KALEIDOSCOPE_LIBS curses z)
target_link_libraries(Kaleidoscope ${KALEIDOSCOPE_LIBS})
