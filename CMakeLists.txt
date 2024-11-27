cmake_minimum_required(VERSION 3.14)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang")

project(
    coatc
    VERSION 0.1.0
    DESCRIPTION "the coat programming language"
    HOMEPAGE_URL "https://github.com/barnii77/coat"
    LANGUAGES CXX
)

# ---- Declare library ----

add_library(
    compiler_lib
    source/lib.cpp
    source/lexer.cpp
    source/ast.cpp
    source/parser.cpp
    source/LLVMCodeGen/codegen.cpp
    source/LLVMCodeGen/optimization.cpp
    source/LLVMCodeGen/cleanup_pass.cpp
    source/LLVMCodeGen/lowering.cpp
    source/LLVMCodeGen/external_linking.cpp
)

target_include_directories(
    compiler_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(compiler_lib PUBLIC cxx_std_17)
target_link_libraries(compiler_lib PRIVATE stdc++)

# ---- Declare executable ----

add_executable(compiler_exe source/main.cpp)

set_property(TARGET compiler_exe PROPERTY OUTPUT_NAME compiler)

target_compile_features(compiler_exe PUBLIC cxx_std_17)
target_link_libraries(compiler_exe PRIVATE stdc++)

target_link_libraries(compiler_exe PRIVATE compiler_lib)

find_package(LLVM REQUIRED)

# TODO check if it still compiles with llvm 18 (because that is what you get using apt)
set(LLVM_MIN_VERSION "19.0.0")

if(${LLVM_VERSION} VERSION_LESS ${LLVM_MIN_VERSION})
    message(FATAL_ERROR "LLVM version must be ${LLVM_MIN_VERSION} or higher")
endif()

target_link_libraries(compiler_lib PRIVATE
    LLVMWindowsManifest
    LLVMXRay
    LLVMLibDriver
    LLVMDlltoolDriver
    LLVMTextAPIBinaryReader
    LLVMCoverage
    LLVMLineEditor
    LLVMXCoreDisassembler
    LLVMXCoreCodeGen
    LLVMXCoreDesc
    LLVMXCoreInfo
    LLVMX86TargetMCA
    LLVMX86Disassembler
    LLVMX86AsmParser
    LLVMX86CodeGen
    LLVMX86Desc
    LLVMX86Info
    LLVMWebAssemblyDisassembler
    LLVMWebAssemblyAsmParser
    LLVMWebAssemblyCodeGen
    LLVMWebAssemblyUtils
    LLVMWebAssemblyDesc
    LLVMWebAssemblyInfo
    LLVMVEDisassembler
    LLVMVEAsmParser
    LLVMVECodeGen
    LLVMVEDesc
    LLVMVEInfo
    LLVMSystemZDisassembler
    LLVMSystemZAsmParser
    LLVMSystemZCodeGen
    LLVMSystemZDesc
    LLVMSystemZInfo
    LLVMSparcDisassembler
    LLVMSparcAsmParser
    LLVMSparcCodeGen
    LLVMSparcDesc
    LLVMSparcInfo
    LLVMRISCVTargetMCA
    LLVMRISCVDisassembler
    LLVMRISCVAsmParser
    LLVMRISCVCodeGen
    LLVMRISCVDesc
    LLVMRISCVInfo
    LLVMPowerPCDisassembler
    LLVMPowerPCAsmParser
    LLVMPowerPCCodeGen
    LLVMPowerPCDesc
    LLVMPowerPCInfo
    LLVMNVPTXCodeGen
    LLVMNVPTXDesc
    LLVMNVPTXInfo
    LLVMMSP430Disassembler
    LLVMMSP430AsmParser
    LLVMMSP430CodeGen
    LLVMMSP430Desc
    LLVMMSP430Info
    LLVMMipsDisassembler
    LLVMMipsAsmParser
    LLVMMipsCodeGen
    LLVMMipsDesc
    LLVMMipsInfo
    LLVMLoongArchDisassembler
    LLVMLoongArchAsmParser
    LLVMLoongArchCodeGen
    LLVMLoongArchDesc
    LLVMLoongArchInfo
    LLVMLanaiDisassembler
    LLVMLanaiCodeGen
    LLVMLanaiAsmParser
    LLVMLanaiDesc
    LLVMLanaiInfo
    LLVMHexagonDisassembler
    LLVMHexagonCodeGen
    LLVMHexagonAsmParser
    LLVMHexagonDesc
    LLVMHexagonInfo
    LLVMBPFDisassembler
    LLVMBPFAsmParser
    LLVMBPFCodeGen
    LLVMBPFDesc
    LLVMBPFInfo
    LLVMAVRDisassembler
    LLVMAVRAsmParser
    LLVMAVRCodeGen
    LLVMAVRDesc
    LLVMAVRInfo
    LLVMARMDisassembler
    LLVMARMAsmParser
    LLVMARMCodeGen
    LLVMARMDesc
    LLVMARMUtils
    LLVMARMInfo
    LLVMAMDGPUTargetMCA
    LLVMAMDGPUDisassembler
    LLVMAMDGPUAsmParser
    LLVMAMDGPUCodeGen
    LLVMAMDGPUDesc
    LLVMAMDGPUUtils
    LLVMAMDGPUInfo
    LLVMAArch64Disassembler
    LLVMAArch64AsmParser
    LLVMAArch64CodeGen
    LLVMAArch64Desc
    LLVMAArch64Utils
    LLVMAArch64Info
    LLVMOrcDebugging
    LLVMOrcJIT
    LLVMWindowsDriver
    LLVMMCJIT
    LLVMJITLink
    LLVMInterpreter
    LLVMExecutionEngine
    LLVMRuntimeDyld
    LLVMOrcTargetProcess
    LLVMOrcShared
    LLVMDWP
    LLVMDebugInfoLogicalView
    LLVMDebugInfoGSYM
    LLVMOption
    LLVMObjectYAML
    LLVMObjCopy
    LLVMMCA
    LLVMMCDisassembler
    LLVMLTO
    LLVMPasses
    LLVMHipStdPar
    LLVMCFGuard
    LLVMCoroutines
    LLVMipo
    LLVMVectorize
    LLVMLinker
    LLVMInstrumentation
    LLVMFrontendOpenMP
    LLVMFrontendOffloading
    LLVMFrontendOpenACC
    LLVMFrontendHLSL
    LLVMFrontendDriver
    LLVMExtensions
    LLVMDWARFLinkerParallel
    LLVMDWARFLinkerClassic
    LLVMDWARFLinker
    LLVMGlobalISel
    LLVMMIRParser
    LLVMAsmPrinter
    LLVMSelectionDAG
    LLVMCodeGen
    LLVMTarget
    LLVMObjCARCOpts
    LLVMCodeGenTypes
    LLVMIRPrinter
    LLVMInterfaceStub
    LLVMFileCheck
    LLVMFuzzMutate
    LLVMScalarOpts
    LLVMInstCombine
    LLVMAggressiveInstCombine
    LLVMTransformUtils
    LLVMBitWriter
    LLVMAnalysis
    LLVMProfileData
    LLVMSymbolize
    LLVMDebugInfoBTF
    LLVMDebugInfoPDB
    LLVMDebugInfoMSF
    LLVMDebugInfoDWARF
    LLVMObject
    LLVMTextAPI
    LLVMMCParser
    LLVMIRReader
    LLVMAsmParser
    LLVMMC
    LLVMDebugInfoCodeView
    LLVMBitReader
    LLVMFuzzerCLI
    LLVMCore
    LLVMRemarks
    LLVMBitstreamReader
    LLVMBinaryFormat
    LLVMTargetParser
    LLVMTableGen
    LLVMSupport
    LLVMDemangle
)
# ---- Developer mode ----

if(NOT compiler_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of this project"
  )
endif()

include(cmake/dev-mode.cmake)
