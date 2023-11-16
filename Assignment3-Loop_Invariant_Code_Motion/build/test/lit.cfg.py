import lit.llvm

lit.llvm.initialize(lit_config, config)

from lit.llvm import llvm_config


config.name = "LICM"
config.test_format = lit.formats.ShTest()
config.test_source_root = "/mnt/test"
config.test_exec_root = "/mnt/build/test"
config.suffixes = [".c", ".cpp", ".ll"]

config.substitutions.append((r"%testdir", "/mnt/build/test"))
config.substitutions.append((r"%dylibdir", "/mnt/build/lib"))
config.substitutions.append((r"%llvm_cxxflags", "-I/usr/lib/llvm-16/include -std=c++17   -fno-exceptions -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS"))

config.llvm_config_bindir = "/usr/lib/llvm-16/bin"
llvm_config.add_tool_substitutions(
    ["clang", "opt", "llc", "FileCheck"], config.llvm_config_bindir
)
