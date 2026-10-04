@echo off
setlocal

echo === Baixando projeto principal ===
git pull

if errorlevel 1 (
    echo.
    echo ERRO: git pull falhou.
    pause
    exit /b 1
)

echo.
echo === Baixando subtree core ===
git subtree pull --prefix=core yume-core master --squash

if errorlevel 1 (
    echo.
    echo ERRO: subtree pull falhou.
    pause
    exit /b 1
)

echo.
echo === Pull concluido com sucesso ===
pause