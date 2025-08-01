cmake_minimum_required(VERSION 3.1)

project(ELO VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_RELEASE "/MD")
set(CMAKE_CXX_FLAGS_DEBUG "/MD /Zi /Od")

add_executable(ELO
  # Base
  src/base/base_core.h
  src/base/base_memory.h
  src/base/base_strings.h
  src/base/base_core.cpp
  src/base/base_memory.cpp
  src/base/base_strings.cpp

  # Compiler
  src/compiler/ast.h
  src/compiler/atom.h
  src/compiler/common.h
  src/compiler/constant_value.h
  src/compiler/decl.h
  src/compiler/lexer.h
  src/compiler/llvm_backend.h
  src/compiler/OP.h
  src/compiler/parser.h
  src/compiler/report.h
  src/compiler/resolve.h
  src/compiler/source_file.h
  src/compiler/token.h
  src/compiler/types.h
  src/compiler/ast.cpp
  src/compiler/atom.cpp
  src/compiler/common.cpp
  src/compiler/compiler_main.cpp
  src/compiler/constant_value.cpp
  src/compiler/decl.cpp
  src/compiler/lexer.cpp
  src/compiler/llvm_backend.cpp
  src/compiler/parser.cpp
  src/compiler/report.cpp
  src/compiler/resolve.cpp
  src/compiler/resolve_expr.cpp
  src/compiler/resolve_stmt.cpp
  src/compiler/source_file.cpp
  src/compiler/types.cpp

  # OS
  src/os/os.h
  src/os/core/os_core.h
  src/os/core/win32/os_core_win32.h
  src/os/os.cpp
  src/os/core/os_core.cpp
  src/os/core/win32/os_core_win32.cpp

  src/path/path.h
  src/path/path.cpp
)

target_include_directories(ELO PUBLIC
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/ext
)

target_link_directories(ELO PUBLIC
  ${CMAKE_SOURCE_DIR}/bin/llvm
)

target_link_libraries(ELO
  ${CMAKE_SOURCE_DIR}/ext/libtommath/libs/tommath.lib

  LLVMCore.lib LLVMExecutionEngine.lib LLVMInterpreter.lib LLVMMC.lib LLVMMCJIT.lib LLVMSupport.lib LLVMX86AsmParser.lib LLVMX86CodeGen.lib LLVMX86Desc.lib LLVMX86Disassembler.lib LLVMX86Info.lib LLVMX86TargetMCA.lib LLVMOrcTargetProcess.lib LLVMOrcShared.lib LLVMRuntimeDyld.lib LLVMMCDisassembler.lib LLVMAsmPrinter.lib LLVMCFGuard.lib LLVMGlobalISel.lib LLVMIRPrinter.lib LLVMInstrumentation.lib LLVMSelectionDAG.lib LLVMCodeGen.lib LLVMTarget.lib  LLVMScalarOpts.lib LLVMAggressiveInstCombine.lib LLVMInstCombine.lib LLVMBitWriter.lib LLVMObjCARCOpts.lib LLVMCodeGenTypes.lib LLVMTransformUtils.lib LLVMAnalysis.lib LLVMProfileData.lib LLVMSymbolize.lib LLVMDebugInfoDWARF.lib LLVMDebugInfoPDB.lib LLVMObject.lib LLVMIRReader.lib LLVMBitReader.lib LLVMAsmParser.lib LLVMRemarks.lib LLVMBitstreamReader.lib LLVMMCParser.lib LLVMTextAPI.lib LLVMBinaryFormat.lib LLVMTargetParser.lib LLVMDebugInfoCodeView.lib LLVMDebugInfoMSF.lib LLVMDebugInfoBTF.lib LLVMDemangle.lib

  psapi.lib shell32.lib ole32.lib uuid.lib advapi32.lib ws2_32.lib delayimp.lib shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib oleaut32.lib comdlg32.lib
)

set_target_properties(ELO PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY    ${CMAKE_SOURCE_DIR}
  RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_SOURCE_DIR}
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}
)
