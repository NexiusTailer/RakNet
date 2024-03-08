import gfx.io.GameDelegate;
import gfx.controls.TextInput;
import gfx.controls.Button;

class Screens.RecoverPasswordScreen extends Screen
{	
	private var userNameEdit:TextInput;	
	private var recoverPasswordByUsername:Button;		
	private var goBackToConnectedToServer:Button;
	
	private var bForgotPassword:Boolean;
	private var btnBack:Button;
	
	private static var mInstance:RecoverPasswordScreen;
		
	public function RecoverPasswordScreen() 
	{
		ConsoleWindow.Trace("Constructing RecoverPasswordScreen");							
		
		mScreenId = ScreenID.RECOVER_PASSWORD;			
		mScreenTabId = ScreenTab.ID_LOGIN;
		
		mInstance = this;
	}
	
	public static function get Instance():RecoverPasswordScreen
	{
		return mInstance;
	}
	
	public function VOnFinishedLoading():Void
	{		
		//Add click event for buttons
		recoverPasswordByUsername.addEventListener("click", this, "f2c_RecoverPasswordByUsername");
		goBackToConnectedToServer.addEventListener("click", this, "goBackToConnectedToServerFunc");
		btnBack.addEventListener("click", this, "Back");
				
		//Add callbacks for C++
		GameDelegate.addCallBack("c2f_RecoverPasswordByUsername", this, "c2f_RecoverPasswordByUsername");
		
		super.VOnFinishedLoading();
	}
	
	public function SetMode( forgotPassword:Boolean ):Void
	{
		ConsoleWindow.Trace("SetMode..." + forgotPassword);
		bForgotPassword = forgotPassword;
	}
	
	public function OnShow():Void
	{
		if ( bForgotPassword )
		{
			ConsoleWindow.Trace("hm...");
			gotoAndStop("Password1");
		}
		else
		{
			gotoAndStop("Username");
		}
	}
	
	public function f2c_RecoverPasswordByUsername():Void
	{
		GameDelegate.call("f2c_RecoverPasswordByUsername", [userNameEdit.text], _root);
	}

	public function goBackToConnectedToServerFunc():Void
	{
		//_root.gotoAndPlay("ConnectedToServer");
		LobbyInterface.Instance.ShowScreen( ScreenID.LOGIN );
	}

	public function c2f_RecoverPasswordByUsername(resultIdentifier:String, username:String, emailaddr:String ):Void
	{
		switch (resultIdentifier)
		{
			case "L2RC_UNKNOWN_USER":
			break;
			
			case "L2RC_DATABASE_CONSTRAINT_FAILURE":
			break;
			
			case "L2RC_SUCCESS":
				// Not yet implemented to actually email you
			break;
		}	
	}
	
	public function Back():Void
	{
		LobbyInterface.Instance.ShowScreen( ScreenID.LOGIN );
	}
}