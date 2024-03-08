Set the working directory: to C:\Program Files (x86)\Scaleform\GFx SDK 3.0\Bin\CLIKLobby or your equivalent

If you get this crash:
 	RoomsAndLobbyGFx3.exe!GPtr<LobbyDataProvider>::operator->()  Line 334 + 0x26 bytes	C++
 	RoomsAndLobbyGFx3.exe!LobbyController::ProcessTasks()  Line 98 + 0xb bytes	C++
>	RoomsAndLobbyGFx3.exe!FxPlayerApp::Run()  Line 1568	C++
 	RoomsAndLobbyGFx3.exe!FxApp::AppMain(int argc=1, char * * argv=0x00255ec8)  Line 182 + 0xf bytes	C++
 	RoomsAndLobbyGFx3.exe!main(int argc=1, char * * argv=0x00255ec8)  Line 701 + 0x13 bytes	C++

You didn't set the working directory.