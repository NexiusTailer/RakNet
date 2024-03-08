copy /Y main_ppapi.cpp main_ppapi.bak
copy /Y ..\..\..\Source\*.cpp

d:/nacl_sdk/pepper_canary/toolchain/win_x86_glibc/bin/i686-nacl-g++.exe -T"D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib\ldscripts\elf_nacl.x.static" -g -I"./../../../Source" -ID:\nacl_sdk\pepper_canary\include -g -Wall -Werror -m32 -I"d:/nacl_sdk/pepper_canary/toolchain/win_x86_glibc/x86_64-nacl/include" -I"d:/nacl_sdk/pepper_canary/toolchain/win_x86_glibc/x86_64-nacl/include/c++/4.4.3" -LD:\nacl_sdk\pepper_canary\lib\win_x86_32_host\Debug -MMD -O3 -fno-strict-aliasing -funswitch-loops -D"_DEBUG" -D"_RAKNET_LIB" -D"__GCC__" -D"_CRT_NONSTDC_NO_DEPRECATE" -D"_CRT_SECURE_NO_DEPRECATE" -DRAKPEER_USER_THREADED -D_RAKNET_SUPPORT_TCPInterface=0 -D_RAKNET_SUPPORT_PacketizedTCP=0 -D_RAKNET_SUPPORT_EmailSender=0 -D_RAKNET_SUPPORT_HTTPConnection=0 -D_RAKNET_SUPPORT_TelnetTransport=0 -D_RAKNET_SUPPORT_UDPForwarder=0 -D_RAKNET_SUPPORT_NatPunchthroughServer=0 -D_RAKNET_SUPPORT_NatTypeDetectionServer=0 -D_RAKNET_SUPPORT_NatTypeDetectionClient=0 -D__native_client__ -fno-exceptions -frtti *.cpp -o chatexampleclient_x86_32_Debug.nexe -lpthread -lppapi -lppapi_cpp

d:/nacl_sdk/pepper_canary/toolchain/win_x86_glibc/bin/i686-nacl-g++.exe -T"D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib\ldscripts\elf64_nacl.x.static" -g -I"./../../../Source" -ID:\nacl_sdk\pepper_canary\include -g -Wall -Werror -m64 -I"d:/nacl_sdk/pepper_canary/toolchain/win_x86_glibc/x86_64-nacl/include" -I"d:/nacl_sdk/pepper_canary/toolchain/win_x86_glibc/x86_64-nacl/include/c++/4.4.3" -LD:\nacl_sdk\pepper_canary\lib\win_x86_32_host\Debug -MMD -O3 -fno-strict-aliasing -funswitch-loops -D"_DEBUG" -D"_RAKNET_LIB" -D"__GCC__" -D"_CRT_NONSTDC_NO_DEPRECATE" -D"_CRT_SECURE_NO_DEPRECATE" -DRAKPEER_USER_THREADED -D_RAKNET_SUPPORT_TCPInterface=0 -D_RAKNET_SUPPORT_PacketizedTCP=0 -D_RAKNET_SUPPORT_EmailSender=0 -D_RAKNET_SUPPORT_HTTPConnection=0 -D_RAKNET_SUPPORT_TelnetTransport=0 -D_RAKNET_SUPPORT_UDPForwarder=0 -D_RAKNET_SUPPORT_NatPunchthroughServer=0 -D_RAKNET_SUPPORT_NatTypeDetectionServer=0 -D_RAKNET_SUPPORT_NatTypeDetectionClient=0 -D__native_client__ -fno-exceptions -frtti *.cpp -o chatexampleclient_x86_64_Debug.nexe -lpthread -lppapi -lppapi_cpp

REM newlib doesn't compile, C only?
REM "D:\nacl_sdk\pepper_canary\toolchain\win_x86_newlib\bin\i686-nacl-g++.exe" -g -I"./../../../Source" -ID:\nacl_sdk\pepper_canary\include -g -Wall -Werror -m32 -I"d:/nacl_sdk/pepper_canary/toolchain/win_x86_newlib/x86_64-nacl/include" -I"d:/nacl_sdk/pepper_canary/toolchain/win_x86_newlib/x86_64-nacl/include/c++/4.4.3" -LD:\nacl_sdk\pepper_canary\lib\win_x86_32_host\Debug -MMD -O3 -fno-strict-aliasing -funswitch-loops -D"_DEBUG" -D"_RAKNET_LIB" -D"__GCC__" -D"_CRT_NONSTDC_NO_DEPRECATE" -D"_CRT_SECURE_NO_DEPRECATE" -DRAKPEER_USER_THREADED -D_RAKNET_SUPPORT_TCPInterface=0 -D_RAKNET_SUPPORT_PacketizedTCP=0 -D_RAKNET_SUPPORT_EmailSender=0 -D_RAKNET_SUPPORT_HTTPConnection=0 -D_RAKNET_SUPPORT_TelnetTransport=0 -D_RAKNET_SUPPORT_UDPForwarder=0 -D_RAKNET_SUPPORT_NatPunchthroughServer=0 -D_RAKNET_SUPPORT_NatTypeDetectionServer=0 -D_RAKNET_SUPPORT_NatTypeDetectionClient=0 -D__native_client__ -fno-exceptions -frtti *.cpp -o chatexampleclient_x86_32_Debug.nexe -lpthread -lppapi -lppapi_cpp

REM "D:\nacl_sdk\pepper_canary\toolchain\win_x86_newlib\bin\i686-nacl-g++.exe" -g -I"./../../../Source" -ID:\nacl_sdk\pepper_canary\include -g -Wall -Werror -m64 -I"d:/nacl_sdk/pepper_canary/toolchain/win_x86_newlib/x86_64-nacl/include" -I"d:/nacl_sdk/pepper_canary/toolchain/win_x86_newlib/x86_64-nacl/include/c++/4.4.3" -LD:\nacl_sdk\pepper_canary\lib\win_x86_32_host\Debug -MMD -O3 -fno-strict-aliasing -funswitch-loops -D"_DEBUG" -D"_RAKNET_LIB" -D"__GCC__" -D"_CRT_NONSTDC_NO_DEPRECATE" -D"_CRT_SECURE_NO_DEPRECATE" -DRAKPEER_USER_THREADED -D_RAKNET_SUPPORT_TCPInterface=0 -D_RAKNET_SUPPORT_PacketizedTCP=0 -D_RAKNET_SUPPORT_EmailSender=0 -D_RAKNET_SUPPORT_HTTPConnection=0 -D_RAKNET_SUPPORT_TelnetTransport=0 -D_RAKNET_SUPPORT_UDPForwarder=0 -D_RAKNET_SUPPORT_NatPunchthroughServer=0 -D_RAKNET_SUPPORT_NatTypeDetectionServer=0 -D_RAKNET_SUPPORT_NatTypeDetectionClient=0 -D__native_client__ -fno-exceptions -frtti *.cpp -o chatexampleclient_x86_64_Debug.nexe -lpthread -lppapi -lppapi_cpp

del *.cpp
copy /Y main_ppapi.bak main_ppapi.cpp
del main_ppapi.bak
del *.d

mkdir lib32
mkdir lib64
cd lib32
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib32\libc.so.51fe1ff9"
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib32\libgcc_s.so.1"
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib32\libm.so.51fe1ff9"
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib32\libppapi_cpp.so"
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib32\libpthread.so.51fe1ff9"
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib32\runnable-ld.so"
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib32\libstdc++.so.6"

cd ..
cd lib64
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib\libc.so.51fe1ff9"
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib\libgcc_s.so.1"
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib\libm.so.51fe1ff9"
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib\libppapi_cpp.so"
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib\libpthread.so.51fe1ff9"
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib\runnable-ld.so"
copy "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\x86_64-nacl\lib\libstdc++.so.6"
cd ..

REM "D:\nacl_sdk\pepper_canary\toolchain\win_x86_glibc\bin\i686-nacl-objdump.exe" -p chatexampleclient_x86_32_Debug.nexe > objdump.txt