@ECHO MSVC and Git needs to be previously installed for this script to work

@ECHO vcpkg will be installed in %USERPROFILE%
@pushd %USERPROFILE%

REM Install and build vcpkg packet manager
git clone https://github.com/Microsoft/vcpkg
cd vcpkg
call bootstrap-vcpkg.bat

@popd

REM Install cpprest with vcpkg
%USERPROFILE%\vcpkg\vcpkg install --triplet x86-windows cpprestsdk