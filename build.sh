#! /bin/env sh

set -e

cc=clang
ld=ld.lld

$cc -mavx2 -mbmi -O1 -std=c90 -o p.o -nostartfiles -nostdlib -fno-asynchronous-unwind-tables -c p.c
$ld -L$VULKAN_SDK/lib -lvulkan -e THREAD0 -o p p.o

