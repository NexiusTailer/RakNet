import gfx.io.GameDelegate;

returnToTitleButton.addEventListener("click", this, "ReturnToTitle");
function ReturnToTitle()
{
	// Disconnect from the server
	GameDelegate.call("f2c_Logoff", [], this);
}
