import gfx.io.GameDelegate;

connectToServerButton.addEventListener("click", this, "connectToServer");
function connectToServer()
{
	GameDelegate.call("f2c_ConnectToServer", [ipAddressTextInput.text, portTextInput.text], this, "c2f_connectToServer_callback");
}

function c2f_connectToServer_callback(callSucceeded : Boolean)
{
	if (callSucceeded==false)
		gotoAndStop("Main");	
}

GameDelegate.addCallBack("c2f_NotifyConnectionAttemptToServerSuccess", this, "c2f_NotifyConnectionAttemptToServerSuccess");
function c2f_NotifyConnectionAttemptToServerSuccess():Void
{
	gotoAndStop("Accounts_RakNet");
}

GameDelegate.addCallBack("c2f_NotifyConnectionAttemptToServerFailure", this, "c2f_NotifyConnectionAttemptToServerFailure");
function c2f_NotifyConnectionAttemptToServerFailure(resultCode:String, systemAddress:String):Void
{
	// Result codes are:
	// CONNECTION_ATTEMPT_FAILED
	// ALREADY_CONNECTED
	// NO_FREE_INCOMING_CONNECTIONS
	// RSA_PUBLIC_KEY_MISMATCH
	// CONNECTION_BANNED
    // INVALID_PASSWORD
	trace(resultCode);
	gotoAndStop("Main");
}

stop();