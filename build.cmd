@echo off
cls

clang.exe -DDEVELOPMENT_ -O0 -g -std=c99 -masm=intel -mavx2 -o p.o -c p.c  || exit /b 1

lld-link.exe /subsystem:console /entry:THREAD0 /debug /nologo p.o || exit /b 1
