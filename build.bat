@echo off

set MODE=%1
if [%1]==[] (
  set MODE=debug
)

set PROJECT=ELO

if not exist build mkdir build
set WARNING_FLAGS=/W4 /wd4100 /wd4101 /wd4189 /wd4200 /wd4201 /wd4456 /wd4505 /wd4706
set INCLUDES=/Isrc\ /Iext\
set COMMON_COMPILER_FLAGS=/nologo /FC /EHsc /Fdbuild\ /Fobuild\ %WARNING_FLAGS% %INCLUDES% /std:c++17
set COMMON_LINKER_FLAGS=/INCREMENTAL:NO /OPT:REF

set COMPILER_FLAGS=%COMMON_COMPILER_FLAGS%
if %MODE%==release (
  set COMPILER_FLAGS=/O2 /MD /Z7 %COMPILER_FLAGS%
) else if %mode%==debug (
  set COMPILER_FLAGS=/Od /MD /Zi /DBUILD_DEBUG %COMPILER_FLAGS%
) else (
  echo Unkown build mode
  exit /B 2
)

set LIBS=/LIBPATH:bin\llvm\ LLVMCore.lib LLVMExecutionEngine.lib LLVMInterpreter.lib LLVMMC.lib LLVMMCJIT.lib LLVMSupport.lib LLVMX86AsmParser.lib LLVMX86CodeGen.lib LLVMX86Desc.lib LLVMX86Disassembler.lib LLVMX86Info.lib LLVMX86TargetMCA.lib LLVMOrcTargetProcess.lib LLVMOrcShared.lib LLVMRuntimeDyld.lib LLVMMCDisassembler.lib LLVMAsmPrinter.lib LLVMCFGuard.lib LLVMGlobalISel.lib LLVMIRPrinter.lib LLVMInstrumentation.lib LLVMSelectionDAG.lib LLVMCodeGen.lib LLVMTarget.lib  LLVMScalarOpts.lib LLVMAggressiveInstCombine.lib LLVMInstCombine.lib LLVMBitWriter.lib LLVMObjCARCOpts.lib LLVMCodeGenTypes.lib LLVMTransformUtils.lib LLVMAnalysis.lib LLVMProfileData.lib LLVMSymbolize.lib LLVMDebugInfoDWARF.lib LLVMDebugInfoPDB.lib LLVMObject.lib LLVMIRReader.lib LLVMBitReader.lib LLVMAsmParser.lib LLVMRemarks.lib LLVMBitstreamReader.lib LLVMMCParser.lib LLVMTextAPI.lib LLVMBinaryFormat.lib LLVMTargetParser.lib LLVMDebugInfoCodeView.lib LLVMDebugInfoMSF.lib LLVMDebugInfoBTF.lib LLVMDemangle.lib psapi.lib shell32.lib ole32.lib uuid.lib advapi32.lib ws2_32.lib delayimp.lib -delayload:shell32.dll -delayload:ole32.dll shlwapi.lib kernel32.lib user32.lib gdi32.lib winspool.lib oleaut32.lib comdlg32.lib

set LINKER_FLAGS=%COMMON_LINKER_FLAGS% %LIBS%

:: Build Project
CL %COMPILER_FLAGS% src\compiler/compiler_main.cpp /Fe%PROJECT%.exe /link %LINKER_FLAGS%
