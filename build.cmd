@echo off
cls

rem clang 18.1.8
rem lld   18.1.8

clang.exe -DDEVELOPMENT_ -O0 -g -std=c99 -masm=intel -mavx2 -mbmi -mbmi2 -o p.o -c p.c  || exit /b 1

lld-link.exe /subsystem:console /entry:THREAD0 /debug /nologo p.o || exit /b 1
