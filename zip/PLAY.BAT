@ECHO OFF
:START
CLS
11UP
ECHO.
ECHO  11UP for Microsoft Windows
ECHO.
ECHO  Developed by donnaken15
ECHO.
ECHO  Made in Visual C++
ECHO.
ECHO  Images and Sounds
ECHO  by Megatouch LLC.
ECHO.
ECHO  NEW SCORE: %errorlevel%
ECHO  THANKS FOR PLAYING!
ECHO.
CHOICE /C YN /M "Play again?"
IF %ERRORLEVEL% EQU 1 GOTO START