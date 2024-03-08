
import gfx.controls.TextArea;
import gfx.controls.Button;

class Screens.ProfileScreen extends Screen
{
	private var mcEditProfile:Button;
	private var mcEditBlockList:Button;
	private var mcEditInfo:Button;
	private var tfUsername:TextField;	
	private var mcProfileImage:MovieClip;
	private var taAboutMe:TextArea;
	private var taActivities:TextArea;
	private var taInterests:TextArea;
	private var taFavoriteGames:TextArea;
	private var taFavoriteMovies:TextArea;
	private var taFavoriteBooks:TextArea;
	private var taFavoriteQuotes:TextArea;
	
	private static var mInstance:ProfileScreen;
			
	public function ProfileScreen()
	{
		ConsoleWindow.Trace("Constructing ProfileScreen");
		
		mScreenId = ScreenID.PROFILE;
		mScreenTabId = ScreenTab.ID_PROFILE;
		
		mInstance = this;
	}
	
	public static function get Instance():ProfileScreen { return mInstance; }
	
	public function VOnFinishedLoading():Void
	{	
		//Add click event for buttons
		mcEditBlockList.addEventListener("click", this, "OnClickEditBlockList");
		mcEditProfile.addEventListener("click", this, "OnClickEditProfile");
		mcEditInfo.addEventListener("click", this, "OnClickEditInfo");
		
		//Add callbacks for C++
		
		super.VOnFinishedLoading();
	}
	
	public function OnShow():Void
	{
		if ( !mcProfileImage.mcImageContainer.mcImage )
		{
			var imageIndex:Number = LobbyInterface.Instance.GetProfileImageIndex();
			mcProfileImage.attachMovie( "ProfileImage" + imageIndex, "mcImage", mcProfileImage.getNextHighestDepth() );			
		}
		tfUsername.text = LobbyInterface.Instance.GetUsername();
	}
	
	public function OnReceivedPlayerInfo():Void
	{		
		taAboutMe.text = AccountInfo.Instance.GetAboutMe();
		taActivities.text = AccountInfo.Instance.GetActivities();
		taInterests.text = AccountInfo.Instance.GetInterests();
		taFavoriteGames.text = AccountInfo.Instance.GetFavoriteGames();
		taFavoriteMovies.text = AccountInfo.Instance.GetFavoriteBooks();
		taFavoriteBooks.text = AccountInfo.Instance.GetFavoriteMovies();
		taFavoriteQuotes.text = AccountInfo.Instance.GetFavoriteQuotes();
		ConsoleWindow.Trace("wtf??" + AccountInfo.Instance.GetAboutMe());		
	}
	
	public function GetAboutMe():String 	{ return taAboutMe.text; }
	public function GetActivities():String 	{ return taActivities.text; }
	public function GetInterests():String 	{ return taInterests.text; }
	public function GetFavoriteGames():String { return taFavoriteGames.text; }
	public function GetFavoriteBooks():String { return taFavoriteMovies.text; }
	public function GetFavoriteMovies():String { return taFavoriteBooks.text; }
	public function GetFavoriteQuotes():String { return taFavoriteQuotes.text; }
	
	public function OnClickEditProfile():Void
	{
		LobbyInterface.Instance.ShowScreen( ScreenID.CHANGE_PHOTO );
	}
	
	public function OnClickEditBlockList():Void
	{
		LobbyInterface.Instance.ShowScreen( ScreenID.BLOCK_LIST );
	}
	
	public function OnClickEditInfo():Void
	{
		LobbyInterface.Instance.ShowScreen( ScreenID.REGISTER_ACCOUNT_PERSONAL );
	}	
}