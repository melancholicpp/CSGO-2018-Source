//===== Copyright � 1996-2009, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#include "mm_title.h"
#include "mm_title_richpresence.h"
#include "swarm.spa.h"
#include "matchext_swarm.h"

#include "fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CMatchTitle::CMatchTitle()
{
	;
}

CMatchTitle::~CMatchTitle()
{
	;
}


//
// Init / shutdown
//

InitReturnVal_t CMatchTitle::Init()
{
	if ( IGameEventManager2 *mgr = g_pMatchExtensions->GetIGameEventManager2() )
	{
		mgr->AddListener( this, "server_pre_shutdown", false );
		mgr->AddListener( this, "game_newmap", false );
		mgr->AddListener( this, "finale_start", false );
		mgr->AddListener( this, "round_start", false );
		mgr->AddListener( this, "round_end", false );
		mgr->AddListener( this, "difficulty_changed", false );
	}

	// Initialize all the campaigns
	g_pMatchExtSwarm->Initialize();

#ifdef _X360
	// Initialize Title Update version
	extern ConVar mm_tu_string;
	mm_tu_string.SetValue( "00000000" );
#endif

	return INIT_OK;
}

void CMatchTitle::Shutdown()
{
	if ( IGameEventManager2 *mgr = g_pMatchExtensions->GetIGameEventManager2() )
	{
		mgr->RemoveListener( this );
	}
}


//
// Implementation
//

uint64 CMatchTitle::GetTitleID()
{
#ifdef _X360
	#ifndef _DEMO
		return TITLEID_SWARM;
	#else
		return TITLEID_SWARM_DEMO;
	#endif
#elif !defined( SWDS )
	static uint64 uiAppID = 0ull;
	if ( !uiAppID && steamapicontext && steamapicontext->SteamUtils() )
	{
		uiAppID = steamapicontext->SteamUtils()->GetAppID();
	}
	return uiAppID;
#else
	return 0ull;
#endif
}

uint64 CMatchTitle::GetTitleServiceID()
{
#ifdef _X360
	return 0x45410880ull; // Left 4 Dead 1 Service ID
#else
	return 0ull;
#endif
}

bool CMatchTitle::IsMultiplayer()
{
	return true;
}

void CMatchTitle::PrepareNetStartupParams( void *pNetStartupParams )
{
#ifdef _X360
	XNetStartupParams &xnsp = *( XNetStartupParams * ) pNetStartupParams;

	xnsp.cfgQosDataLimitDiv4 = 128; // 512 bytes
	xnsp.cfgSockDefaultRecvBufsizeInK = 64; // Increase receive size for UDP to 64k
	xnsp.cfgSockDefaultSendBufsizeInK = 64; // Keep send size at 64k too

	int numGamePlayersMax = GetTotalNumPlayersSupported();

	int numConnections = 4 * ( numGamePlayersMax - 1 );
	//   - the max number of connections to members of your game party
	//   - the max number of connections to members of your social party
	//   - the max number of connections to a pending game party (if you are joining a new one ).
	//   - matchmakings client info structure also creates a connection per client for the lobby.

	//   1 - the main game session
	int numTotalConnections = 1 + numConnections;

	//   29 - total Connections (XNADDR/XNKID pairs) ,using 5 sessions (XNKID/XNKEY pairs).

	xnsp.cfgKeyRegMax = 16; //adding some extra room because of lazy dealocation of these pairs.
	xnsp.cfgSecRegMax = MAX( 64, numTotalConnections ); //adding some extra room because of lazy dealocation of these pairs.
	
	xnsp.cfgSockMaxDgramSockets = xnsp.cfgSecRegMax;
	xnsp.cfgSockMaxStreamSockets = xnsp.cfgSecRegMax;
#endif
}

int CMatchTitle::GetTotalNumPlayersSupported()
{
	// Alien Swarm is a 4-player game
	return 4;
}

// Get a guest player name
char const * CMatchTitle::GetGuestPlayerName( int iUserIndex )
{
	if ( vgui::ILocalize *pLocalize = g_pMatchExtensions->GetILocalize() )
	{
		if ( wchar_t* wStringTableEntry = pLocalize->Find( "#L4D360UI_Character_Guest" ) )
		{
			static char szName[ MAX_PLAYER_NAME_LENGTH ] = {0};
			pLocalize->ConvertUnicodeToANSI( wStringTableEntry, szName, ARRAYSIZE( szName ) );
			return szName;
		}
	}
	
	return "";
}

// Sets up all necessary client-side convars and user info before
// connecting to server
void CMatchTitle::PrepareClientForConnect( KeyValues *pSettings )
{
#ifndef SWDS
	int numPlayers = 1;
#ifdef _X360
	numPlayers = XBX_GetNumGameUsers();
#endif

	//
	// Now we set the convars
	//

	for ( int k = 0; k < numPlayers; ++ k )
	{
		int iController = k;
#ifdef _X360
		iController = XBX_GetUserId( k );
#endif
		IPlayerLocal *pPlayerLocal = g_pPlayerManager->GetLocalPlayer( iController );
		if ( !pPlayerLocal )
			continue;

		// Set "name"
		static SplitScreenConVarRef s_cl_name( "name" );
		char const *szName = pPlayerLocal->GetName();
		s_cl_name.SetValue( k, szName );

		// Set "networkid_force"
		if ( IsX360() )
		{
			static SplitScreenConVarRef s_networkid_force( "networkid_force" );
			uint64 xid = pPlayerLocal->GetXUID();
			s_networkid_force.SetValue( k, CFmtStr( "%08X:%08X:", uint32( xid >> 32 ), uint32( xid ) ) );
		}
	}
#endif
}

bool CMatchTitle::StartServerMap( KeyValues *pSettings )
{
	int numPlayers = 1;
#ifdef _X360
	numPlayers = XBX_GetNumGameUsers();
#endif

	KeyValues *pInfoMission = NULL;
	KeyValues *pInfoChapter = g_pMatchExtSwarm->GetMapInfo( pSettings, &pInfoMission );
	Assert( pInfoChapter );
	if ( !pInfoChapter || !pInfoMission )
		return false;

	// Check that we have the server interface and that the map is valid
	if ( !g_pMatchExtensions->GetIVEngineServer() )
		return false;
	if ( !g_pMatchExtensions->GetIVEngineServer()->IsMapValid( pInfoChapter->GetString( "map" ) ) )
		return false;

	//
	// Prepare game dll reservation package
	//
	KeyValues *pGameDllReserve = g_pMatchFramework->GetMatchNetworkMsgController()->PackageGameDetailsForReservation( pSettings );
	KeyValues::AutoDelete autodelete( pGameDllReserve );

	pGameDllReserve->SetString( "map/mapcommand", ( numPlayers <= 1 ) ? "map" : "ss_map" );

	if ( !Q_stricmp( "commentary", pSettings->GetString( "options/play", "" ) ) )
		pGameDllReserve->SetString( "map/mapcommand", "map_commentary" );

	// Run map based off the faked reservation packet
	g_pMatchExtensions->GetIServerGameDLL()->ApplyGameSettings( pGameDllReserve );

	return true;
}

static KeyValues * GetCurrentMatchSessionSettings()
{
	IMatchSession *pIMatchSession = g_pMatchFramework->GetMatchSession();
	return pIMatchSession ? pIMatchSession->GetSessionSettings() : NULL;
}

#ifndef SWDS
static void SendPreConnectClientDataToServer( int nSlot )
{
	// We have just connected to the server,
	// send our avatar information

	int iController = nSlot;
#ifdef _X360
	iController = XBX_GetUserId( nSlot );
#endif

	// Swarm for now has no preconnect data
	if ( 1 )
		return;

	KeyValues *pPreConnectData = new KeyValues( "preconnectdata" );

	//
	// Now we prep the keyvalues for server
	//
	if ( IPlayerLocal *pPlayerLocal = g_pPlayerManager->GetLocalPlayer( iController ) )
	{
		// Set session-specific user info
		XUID xuid = pPlayerLocal->GetXUID();
		pPreConnectData->SetUint64( "xuid", xuid );

		KeyValues *pSettings = GetCurrentMatchSessionSettings();

		KeyValues *pMachine = NULL;
		if ( KeyValues *pPlayer = SessionMembersFindPlayer( pSettings, xuid, &pMachine ) )
		{
		}
	}

	// Deliver the keyvalues to the server
	int nRestoreSlot = g_pMatchExtensions->GetIVEngineClient()->GetActiveSplitScreenPlayerSlot();
	g_pMatchExtensions->GetIVEngineClient()->SetActiveSplitScreenPlayerSlot( nSlot );
	g_pMatchExtensions->GetIVEngineClient()->ServerCmdKeyValues( pPreConnectData );
	g_pMatchExtensions->GetIVEngineClient()->SetActiveSplitScreenPlayerSlot( nRestoreSlot );
}
#endif

void CMatchTitle::OnEvent( KeyValues *pEvent )
{
	char const *szEvent = pEvent->GetName();
	
	if ( !Q_stricmp( "OnPlayerRemoved", szEvent ) ||
		 !Q_stricmp( "OnPlayerUpdated", szEvent ) )
	{
		MM_Title_RichPresence_PlayersChanged( GetCurrentMatchSessionSettings() );
	}
	else if ( !Q_stricmp( "OnMatchSessionUpdate", szEvent ) )
	{
		if ( !Q_stricmp( pEvent->GetString( "state" ), "updated" ) )
		{
			if ( KeyValues *kvUpdate = pEvent->FindKey( "update" ) )
			{
				MM_Title_RichPresence_Update( GetCurrentMatchSessionSettings(), kvUpdate );
			}
		}
		else if ( !Q_stricmp( pEvent->GetString( "state" ), "created" ) ||
				  !Q_stricmp( pEvent->GetString( "state" ), "ready" ) )
		{
			MM_Title_RichPresence_Update( GetCurrentMatchSessionSettings(), NULL );
		}
		else if ( !Q_stricmp( pEvent->GetString( "state" ), "closed" ) )
		{
			MM_Title_RichPresence_Update( NULL, NULL );
		}
	}
#ifndef SWDS
	else if ( !Q_stricmp( "OnEngineClientSignonStateChange", szEvent ) )
	{
		int nSlot = pEvent->GetInt( "slot" );
		int iOldState = pEvent->GetInt( "old" );
		int iNewState = pEvent->GetInt( "new" );

		if ( iOldState < SIGNONSTATE_CONNECTED &&
			 iNewState >= SIGNONSTATE_CONNECTED )
		{
			SendPreConnectClientDataToServer( nSlot );
		}
	}
	else if ( !Q_stricmp( "OnEngineSplitscreenClientAdded", szEvent ) )
	{
		int nSlot = pEvent->GetInt( "slot" );
		SendPreConnectClientDataToServer( nSlot );
	}
	else if ( !Q_stricmp( "Client::CmdKeyValues", szEvent ) )
	{
		KeyValues *pCmd = pEvent->GetFirstTrueSubKey();
		if ( !pCmd )
			return;

		char const *szCmd = pCmd->GetName();
		szCmd;
	}
#endif
}

//
//
//

int CMatchTitle::GetEventDebugID( void )
{
	return EVENT_DEBUG_ID_INIT;
}

void CMatchTitle::FireGameEvent( IGameEvent *pIGameEvent )
{
	// Check if the current match session is on an active game server
	IMatchSession *pMatchSession = g_pMatchFramework->GetMatchSession();
	if ( !pMatchSession )
		return;
	KeyValues *pSessionSettings = pMatchSession->GetSessionSettings();
	char const *szGameServer = pSessionSettings->GetString( "server/server", "" );
	char const *szSystemLock = pSessionSettings->GetString( "system/lock", "" );
	if ( ( !szGameServer || !*szGameServer ) &&
		( !szSystemLock || !*szSystemLock ) )
		return;

	char const *szGameMode = pSessionSettings->GetString( "game/mode", "coop" );
	szGameMode;

	// Also don't run on the client when there's a host
	char const *szSessionType = pMatchSession->GetSessionSystemData()->GetString( "type", NULL );
	if ( szSessionType && !Q_stricmp( szSessionType, "client" ) )
		return;

	// Parse the game event
	char const *szGameEvent = pIGameEvent->GetName();
	if ( !szGameEvent || !*szGameEvent )
		return;

	if ( !Q_stricmp( "round_start", szGameEvent ) )
	{
		pMatchSession->UpdateSessionSettings( KeyValues::AutoDeleteInline( KeyValues::FromString(
			"update",
			" update { "
				" game { "
					" state game "
				" } "
			" } "
			) ) );
	}
	else if ( !Q_stricmp( "round_end", szGameEvent ) )
	{
		g_pMatchFramework->GetEventsSubscription()->BroadcastEvent( new KeyValues(
			"OnProfilesWriteOpportunity", "reason", "checkpoint"
			) );
	}
	else if ( !Q_stricmp( "finale_start", szGameEvent ) )
	{
		pMatchSession->UpdateSessionSettings( KeyValues::AutoDeleteInline( KeyValues::FromString(
			"update",
			" update { "
				" game { "
					" state finale "
				" } "
			" } "
			) ) );
	}
	else if ( !Q_stricmp( "game_newmap", szGameEvent ) )
	{
		const char *szMapName = pIGameEvent->GetString( "mapname", "" );

		KeyValues *kvUpdate = KeyValues::FromString(
			"update",
			" update { "
				" game { "
					" state game "
				" } "
			" } "
			);
		KeyValues::AutoDelete autodelete( kvUpdate );

		KeyValues *pInfoMission = NULL;
		KeyValues *pInfoChapter = g_pMatchExtSwarm->GetMapInfoByBspName( pSessionSettings, szMapName, &pInfoMission );
		Assert( pInfoChapter && pInfoMission );
		if ( pInfoChapter && pInfoMission )
		{
			kvUpdate->SetString( "update/game/campaign", pInfoMission->GetString( "name" ) );
			kvUpdate->SetInt( "update/game/chapter", pInfoChapter->GetInt( "chapter" ) );
		}

		pMatchSession->UpdateSessionSettings( kvUpdate );

		g_pMatchFramework->GetEventsSubscription()->BroadcastEvent( new KeyValues(
			"OnProfilesWriteOpportunity", "reason", "checkpoint"
			) );
	}
	else if ( !Q_stricmp( "difficulty_changed", szGameEvent ) )
	{
		char const *szDifficulty = pIGameEvent->GetString( "strDifficulty", "normal" );

		KeyValues *kvUpdate = KeyValues::FromString(
			"update",
			" update { "
				" game { "
					" difficulty = "
				" } "
			" } "
			);
		KeyValues::AutoDelete autodelete( kvUpdate );

		kvUpdate->SetString( "update/game/difficulty", szDifficulty );

		pMatchSession->UpdateSessionSettings( kvUpdate );
	}
	else if ( !Q_stricmp( "server_pre_shutdown", szGameEvent ) )
	{
		char const *szReason = pIGameEvent->GetString( "reason", "quit" );
		if ( !Q_stricmp( szReason, "quit" ) )
		{
			DevMsg( "Received server_pre_shutdown notification - server is shutting down...\n" );

			// Transform the server shutdown event into game end event
			g_pMatchFramework->GetEventsSubscription()->BroadcastEvent( new KeyValues(
				"OnEngineDisconnectReason", "reason", "Server shutting down"
				) );
		}
	}
}

