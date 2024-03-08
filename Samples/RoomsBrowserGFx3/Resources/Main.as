import gfx.io.GameDelegate;

startButton.addEventListener("click", this, "Start");
function Start()
{
	GameDelegate.call("f2c_QueryPlatform", [], this, "c2f_QueryPlatform_Main_Callback");
}


function c2f_QueryPlatform_Main_Callback(platform:String):Void
{
	if (platform=="RakNet")
	{
		gotoAndStop("connectToServer_RakNet");
		
	}
}


stop();
