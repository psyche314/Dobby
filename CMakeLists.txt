cmake_minimum_required(VERSION 3.18)
project(Dobby VERSION 1.0.0 LANGUAGES C CXX ASM)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
option(DOBBY_DEBUG "Enable diagnostic logging" OFF)
option(DOBBY_GENERATE_SHARED "Build shared and static libraries" ON)
option(NearBranch "Prefer a near branch trampoline" ON)
option(FullFloatingPointRegisterPack "Save all ARM64 floating-point registers" OFF)
option(Plugin.SymbolResolver "Build DobbySymbolResolver" ON)
option(DOBBY_BUILD_TEST "Build native end-to-end tests" OFF)
if(MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "Dobby requires Clang on Windows.")
endif()
if(DOBBY_BUILD_KERNEL_MODE OR Plugin.ImportTableReplace OR Plugin.Android.BionicLinkerUtil OR Private.Obfuscation)
  message(FATAL_ERROR "This user-mode build does not support the requested optional plugin.")
endif()
set(dobby_sources
  source/TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.cc
  source/core/assembler/assembler-arm.cc
  source/core/assembler/assembler-ia32.cc
  source/core/assembler/assembler-x64.cc
  source/core/codegen/codegen-arm.cc
  source/core/codegen/codegen-ia32.cc
  source/InstructionRelocation/arm/InstructionRelocationARM.cc
  source/InstructionRelocation/arm64/InstructionRelocationARM64.cc
  source/InstructionRelocation/x86/InstructionRelocationX86.cc
  source/InstructionRelocation/x86/InstructionRelocationX86Shared.cc
  source/InstructionRelocation/x64/InstructionRelocationX64.cc
  source/InstructionRelocation/x86/x86_insn_decode/x86_insn_decode.c
  source/InterceptRouting/InterceptRouting.h
  source/InterceptRouting/InlineHookRouting.h
  source/InterceptRouting/InstrumentRouting.h
  source/InterceptRouting/RoutingPlugin.h
  source/InterceptRouting/InstrumentRouting/instrument_routing_handler.cpp
  source/InterceptRouting/NearBranchTrampoline/NearBranchTrampoline.h
  source/TrampolineBridge/Trampoline/trampoline_arm.cc
  source/TrampolineBridge/Trampoline/trampoline_arm64.cc
  source/TrampolineBridge/Trampoline/trampoline_x86.cc
  source/TrampolineBridge/Trampoline/trampoline_x64.cc
  source/TrampolineBridge/ClosureTrampolineBridge/arm/helper_arm.cc
  source/TrampolineBridge/ClosureTrampolineBridge/arm/closure_bridge_arm.cc
  source/TrampolineBridge/ClosureTrampolineBridge/arm/ClosureTrampolineARM.cc
  source/TrampolineBridge/ClosureTrampolineBridge/arm64/helper_arm64.cc
  source/TrampolineBridge/ClosureTrampolineBridge/arm64/closure_bridge_arm64.cc
  source/TrampolineBridge/ClosureTrampolineBridge/arm64/ClosureTrampolineARM64.cc
  source/TrampolineBridge/ClosureTrampolineBridge/arm64/closure_bridge_arm64.asm
  source/TrampolineBridge/ClosureTrampolineBridge/arm64/closure_trampoline_arm64.asm
  source/TrampolineBridge/ClosureTrampolineBridge/x86/helper_x86.cc
  source/TrampolineBridge/ClosureTrampolineBridge/x86/closure_bridge_x86.cc
  source/TrampolineBridge/ClosureTrampolineBridge/x86/ClosureTrampolineX86.cc
  source/TrampolineBridge/ClosureTrampolineBridge/x64/helper_x64.cc
  source/TrampolineBridge/ClosureTrampolineBridge/x64/closure_bridge_x64.cc
  source/TrampolineBridge/ClosureTrampolineBridge/x64/ClosureTrampolineX64.cc
  source/TrampolineBridge/ClosureTrampolineBridge/x64/closure_bridge_x64.asm
  source/TrampolineBridge/ClosureTrampolineBridge/x64/closure_trampoline_x64.asm
  source/dobby.cpp
  )

if(WIN32)
  set(dobby_platform Windows)
  set(dobby_backend windows)
elseif(APPLE)
  set(dobby_platform Darwin)
  set(dobby_backend posix)
else()
  set(dobby_platform Linux)
  set(dobby_backend posix)
endif()
list(APPEND dobby_sources
  source/Backend/UserMode/PlatformUtil/${dobby_platform}/ProcessRuntime.cc
  source/Backend/UserMode/UnifiedInterface/platform-${dobby_backend}.cc
  source/Backend/UserMode/ExecMemory/clear-cache-tool-all.c
  external/logging/logging.cc
  source/InterceptRouting/NearBranchTrampoline/near_trampoline_arm64.cc)
if(APPLE)
  list(APPEND dobby_sources source/Backend/UserMode/ExecMemory/code-patch-tool-darwin.cc)
else()
  list(APPEND dobby_sources source/Backend/UserMode/ExecMemory/code-patch-tool-${dobby_backend}.cc)
endif()
if(Plugin.SymbolResolver)
  if(APPLE)
    list(APPEND dobby_sources
      builtin-plugin/SymbolResolver/macho/macho_ctx.cc
      builtin-plugin/SymbolResolver/macho/dobby_symbol_resolver.cc
      builtin-plugin/SymbolResolver/macho/macho_file_symbol_resolver.cpp
      builtin-plugin/SymbolResolver/macho/shared_cache_ctx.cpp)
  elseif(WIN32)
    list(APPEND dobby_sources builtin-plugin/SymbolResolver/pe/dobby_symbol_resolver.cc)
  else()
    list(APPEND dobby_sources builtin-plugin/SymbolResolver/elf/dobby_symbol_resolver.cc)
  endif()
endif()

# Compile once and leave the embedding project's compiler/toolchain flags intact.
add_library(dobby_objects OBJECT ${dobby_sources})
target_compile_features(dobby_objects PRIVATE cxx_std_17)
set_target_properties(dobby_objects PROPERTIES
  C_STANDARD 11 C_STANDARD_REQUIRED YES POSITION_INDEPENDENT_CODE YES
  C_VISIBILITY_PRESET hidden CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
target_include_directories(dobby_objects PRIVATE
  . include source source/dobby external external/logging builtin-plugin
  builtin-plugin/SymbolResolver source/Backend/UserMode)
target_compile_definitions(dobby_objects PRIVATE
  DOBBY_BUILDING BUILD_WITH_TRAMPOLINE_ASM
  __DOBBY_BUILD_VERSION__="${PROJECT_VERSION}"
  $<$<BOOL:${DOBBY_DEBUG}>:DOBBY_DEBUG>
  $<$<NOT:$<BOOL:${DOBBY_DEBUG}>>:DOBBY_LOGGING_DISABLE>
  $<$<BOOL:${FullFloatingPointRegisterPack}>:FULL_FLOATING_POINT_REGISTER_PACK>
  DOBBY_NEAR_BRANCH_DEFAULT=$<BOOL:${NearBranch}>)
if(WIN32)
  target_compile_definitions(dobby_objects PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()
foreach(source IN LISTS dobby_sources)
  if(source MATCHES "[.]asm$")
    set_source_files_properties(${source} PROPERTIES LANGUAGE ASM COMPILE_FLAGS "-x assembler-with-cpp")
  endif()
endforeach()
if(DOBBY_GENERATE_SHARED)
  add_library(dobby SHARED $<TARGET_OBJECTS:dobby_objects>)
  add_library(dobby_static STATIC $<TARGET_OBJECTS:dobby_objects>)
  set_target_properties(dobby_static PROPERTIES OUTPUT_NAME dobby)
  if(WIN32)
    set_target_properties(dobby PROPERTIES ARCHIVE_OUTPUT_NAME dobby_import)
  endif()
  set(dobby_targets dobby dobby_static)
else()
  add_library(dobby STATIC $<TARGET_OBJECTS:dobby_objects>)
  add_library(dobby_static ALIAS dobby)
  set(dobby_targets dobby)
endif()
find_package(Threads REQUIRED)
foreach(target IN LISTS dobby_targets)
  target_include_directories(${target} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  target_link_libraries(${target} PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
  if(ANDROID)
    target_link_libraries(${target} PRIVATE log)
  endif()
endforeach()
install(TARGETS ${dobby_targets} EXPORT DobbyTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES include/dobby.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT DobbyTargets NAMESPACE Dobby:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Dobby)
if(DOBBY_BUILD_TEST)
  enable_testing()
  add_subdirectory(tests/e2e)
endif()

configure_package_config_file(cmake/DobbyConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/DobbyConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Dobby)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/DobbyConfigVersion.cmake"
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/DobbyConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/DobbyConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Dobby)
