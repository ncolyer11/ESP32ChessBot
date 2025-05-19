@echo off
setlocal enabledelayedexpansion
set pass=0
set fail=0

for %%t in (tests\*.in) do (
    set name=%%~nt
    echo Testing tests\!name!... 
    .\diyChessEngine.exe < tests\!name!.in > tmp.out
    fc /b tmp.out tests\!name!.out >nul
    if errorlevel 1 (
        echo FAILED
        set /a fail=!fail!+1
        echo Diff:
        fc tmp.out tests\!name!.out
    ) else (
        echo PASSED
        set /a pass=!pass!+1
    )
)

del tmp.out
echo !pass! passed, !fail! failed.
exit /b !fail!
