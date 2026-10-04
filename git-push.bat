@echo off
setlocal

echo === Enviando subtree core ===
git subtree push --prefix=core yume-core master

if errorlevel 1 (
    echo.
    echo ERRO: subtree push falhou.
    pause
    exit /b 1
)

echo.
echo === Enviando projeto principal ===
git push


if errorlevel 1 (
    echo.
    echo ERRO: git push falhou.
    pause
    exit /b 1
)

echo.
echo === Push concluido com sucesso ===
pause