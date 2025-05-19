@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

set pass=0
set fail=0

for %%t in (tests\*.in) do (
    set name=%%~nt
    echo Testing tests\!name!... 

    .\chesp32.exe < tests\!name!.in > tmp_actual.txt
    set "expected=tests\!name!.out"

    fc /b tmp_actual.txt !expected! >nul
    if errorlevel 1 (
        echo ❌ FAILED
        set /a fail+=1
        echo Diff:
        fc tmp_actual.txt !expected!
    ) else (
        echo ✅ PASSED
        set /a pass+=1
    )
    echo.
)

del tmp_actual.txt 2>nul
echo =========================
echo Finished
echo =========================

if %pass% neq 0 echo ✅ %pass% passed
if %fail% neq 0 echo ❌ %fail% failed
echo =========================
echo.

exit /b 0
