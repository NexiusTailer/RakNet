import gfx.io.GameDelegate;
import gfx.controls.TextInput;
import gfx.controls.Button;

class Screens.LoggingInScreen extends Screen
{	
	private var loginFailureResult:Button;		
	private var cancelFromLoggingIn:Button;		
		
	public function LoggingInScreen() 
	{
		ConsoleWindow.Trace("Constructing LoggingInScreen");						
		
		mScreenId = ScreenID.LOGGING_IN;		
		mScreenTabId = ScreenTab.ID_LOGIN;		
	}
	
	public function VOnFinishedLoading():Void
	{		
		loginFailureResult.visible = false;

		//Add click event for buttons
		cancelFromLoggingIn.addEventListener("click", this, "cancelFromLoggingInFunc");
				
		//Add callbacks for C++
		GameDelegate.addCallBack("c2f_NotifyLoginResultFailure", this, "c2f_NotifyLoginResultFailure");
		GameDelegate.addCallBack("c2f_NotifyLoginResultSuccess", this, "c2f_NotifyLoginResultSuccess");
		
		super.VOnFinishedLoading();
	}
	
	function cancelFromLoggingInFunc():Void
	{
		GameDelegate.call("f2c_DisconnectFromServer", [], _root);
		//_root.gotoAndPlay("Disconnected");
		LobbyInterface.Instance.ShowScreen( ScreenID.CONNECTION );
	}

	function c2f_NotifyLoginResultFailure(reasonIdentifier:String, bannedReason:String, whenBanned:String, bannedExpiration:String ):Void
	{
		switch (reasonIdentifier)
		{
			case "Client_Login_HANDLE_NOT_IN_USE_OR_BAD_SECRET_KEY":
			break;
			case "Client_Login_CANCELLED":
			break;
			case "Client_Login_CABLE_NOT_CONNECTED":
			break;
			case "Client_Login_NET_NOT_CONNECTED":
			break;
			case "Client_Login_BANNED":
			// banned parameters used here, not otherwise
			break;
			case "Client_Login_CDKEY_STOLEN":
			break;
			case "Client_Login_EMAIL_ADDRESS_NOT_VALIDATED":
			break;
			case "Client_Login_BAD_TITLE_OR_TITLE_SECRET_KEY":
			break;
		}
				
		loginFailureResult.visible=true;
		loginFailureResult.label=reasonIdentifier;
	}

	function c2f_NotifyLoginResultSuccess( ):Void
	{
		//_root.gotoAndPlay("LoggedIn");
		LobbyInterface.Instance.ShowScreen( ScreenID.LOGGED_IN );
	}
	
}