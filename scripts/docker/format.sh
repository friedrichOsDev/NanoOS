git ls-files '*.c' '*.cpp' '*.h' '*.hpp' | xargs clang-format -i
git ls-files '*.asm' '*.s' '*.S' | xargs asmfmt -w