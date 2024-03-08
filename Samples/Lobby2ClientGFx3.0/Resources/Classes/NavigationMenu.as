
import gfx.controls.Button;

class NavigationMenu extends MovieClip
{	
	private var mcLogIn:Button;
	private var mcLogOut:Button;
	private var mcProfile:Button;
	private var mcFriends:Button;
	private var mcEmail:Button;
	private var mcClan:Button;
	private var mcExit:Button;
	
	public function NavigationMenu()
	{
	}
	
	public function onLoad()
	{		
		OnPlayerLoggedOut();
		mcLogIn.addEventListener("click", this, "OnClickedLogInButton");
		mcLogOut.addEventListener("click", this, "OnClickedLogoutButton");
		mcProfile.addEventListener("click", this, "OnClickedProfileButton");
		mcFriends.addEventListener("click", this, "OnClickedFriendsButton");
		mcEmail.addEventListener("click", this, "OnClickedEmailButton");
		mcClan.addEventListener("click", this, "OnClickedClanButton");
		mcExit.addEventListener("click", this, "OnClickedExitButton");
	}
	
	public function OnClickedLogInButton():Void
	{		
	}
	
	public function OnClickedLogoutButton():Void
	{
		LobbyInterface.Instance.GetScreen(ScreenID.LOGGED_IN)["f2c_Logoff"]();
	}
	
	public function OnClickedProfileButton():Void
	{
		LobbyInterface.Instance.ShowScreen( ScreenID.PROFILE );
	}
	
	public function OnClickedFriendsButton():Void
	{
		LobbyInterface.Instance.ShowScreen( ScreenID.FRIENDS );
	}
	
	public function OnClickedEmailButton():Void
	{
		LobbyInterface.Instance.ShowScreen( ScreenID.EMAIL );
	}
	
	public function OnClickedClanButton():Void
	{
		LobbyInterface.Instance.ShowScreen( ScreenID.CLAN_ROOT );
	}
	
	public function OnClickedExitButton():Void
	{
		LobbyInterface.Instance.GetScreen(ScreenID.LOGGED_IN)["f2c_Logoff"]();		
	}
	
	public function OnPlayerLoggedIn():Void
	{
		mcLogIn.visible = false;
		mcLogOut.visible = true;
		mcProfile.visible = true;
		mcFriends.visible = true;
		mcClan.visible = true;
		mcEmail.visible = true;
		mcFriends.visible = true;	
		mcExit.visible = true;	
	}
	
	public function OnPlayerLoggedOut():Void
	{
		mcLogIn.visible = true;
		mcLogOut.visible = false;
		mcProfile.visible = false;
		mcFriends.visible = false;
		mcClan.visible = false;
		mcEmail.visible = false;
		mcFriends.visible = false;
		mcExit.visible = false;
	}
}