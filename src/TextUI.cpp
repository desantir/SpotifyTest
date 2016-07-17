/* 
Copyright (C) 2011-2016 Robert DeSantis
hopluvr at gmail dot com

This file is part of DMX Studio.
 
DMX Studio is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.
 
DMX Studio is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.
 
You should have received a copy of the GNU General Public License
along with DMX Studio; see the file _COPYING.txt.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.
*/

#include "stdafx.h"
#include "TextUI.h"

static const char * LINE_CLEAR = "\r                                                                                                    \r";

static HandlerMap function_map;

LPCSTR spotifyUsername = "123598159";

int PlaylistField::last_playlist_selection = 1;

static CString track_time( DWORD time );

const unsigned SAMPLE_SIZE = 16384;

// ----------------------------------------------------------------------------
//
TextUI::TextUI( MusicPlayer* player ) :
    m_player( player ),
    m_running( false )
{
	function_map[ "q" ] = HandlerInfo( &TextUI::quit,						false,	"Quit" );
	function_map[ "lp" ] = HandlerInfo( &TextUI::listPlaylists,				false,	"List Playlists" );
	function_map[ "lt" ] = HandlerInfo( &TextUI::listTracks,				false,	"List Tracks" );
	function_map[ "sti" ] = HandlerInfo( &TextUI::showTrackAudioInfo,		false,	"Show track audio info" );
    function_map[ "sta" ] = HandlerInfo( &TextUI::showTrackAnalysis,   	    false,	"Show track analysis (if available)" );
	function_map[ "pt" ] = HandlerInfo( &TextUI::playTrack,	    			false,	"Play Track" );
    function_map[ "pts" ] = HandlerInfo( &TextUI::playTrackSeek,	  		false,	"Play Track with Seek" );
	function_map[ "qt" ] = HandlerInfo( &TextUI::queueTrack,	  			false,	"Queue Track" );
	function_map[ "qp" ] = HandlerInfo( &TextUI::queuePlaylist,  			false,	"Queue Playlist" );
	function_map[ "pp" ] = HandlerInfo( &TextUI::playPlaylist,	    		false,	"Play Playlist" );
	function_map[ "?" ] = HandlerInfo( &TextUI::help,					    false,	"Help" );
    function_map[ "p" ] = HandlerInfo( &TextUI::pauseTrack,				    false,	"Toggle track pause" );
    function_map[ "s" ] = HandlerInfo( &TextUI::stopTrack,				    false,	"Stop track" );
    function_map[ "f" ] = HandlerInfo( &TextUI::forwardTrack,				false,	"Forward track" );
    function_map[ "b" ] = HandlerInfo( &TextUI::backTrack,				    false,	"Back track" );
    function_map[ "qx" ] = HandlerInfo( &TextUI::showQueuedTracks,		    false,	"Show track queue" );
    function_map[ "px" ] = HandlerInfo( &TextUI::showPlayedTracks,		    false,	"Show played tracks" );
    function_map[ "ee" ] = HandlerInfo( &TextUI::enableEvents,  		    false,	"Enable events" );
    function_map[ "de" ] = HandlerInfo( &TextUI::disableEvents,  		    false,	"Disable events" );
}

// ----------------------------------------------------------------------------
//
TextUI::~TextUI(void)
{
}

// ----------------------------------------------------------------------------
//
void TextUI::run()
{
	m_running = true;

	m_text_io.printf( "\n\nSpotify API Test Bench - type ? for command list\n\n" );

    m_player->connect();                   // Try to connect using stored credentials

	while ( m_running ) {
        // See if we need to login
        if ( !m_player->isLoggedIn() && !spotify_login( ) )
           break;

        CString label;

        DWORD length, remaining;
        CString track_link;

        if ( m_player->getPlayingTrack( track_link, &length, &remaining ) ) {
            label.Format( "Now %s: %s", "Playing", m_player->getTrackFullName( track_link ) );
            label.AppendFormat( " | length %s remaining %s", track_time(length), track_time(remaining) );

            if ( m_player->isTrackPaused() )
                label.Append( " | PAUSED" );

		    m_text_io.printf( "\n%s\n", (LPCSTR)label );
        }

		m_text_io.clear();

		m_text_io.printf( "> ", (LPCSTR)label );

		CString cmd;
		int retcode = m_text_io.getString( cmd );
		m_text_io.printf( "\n" );

        if ( !m_player->isLoggedIn() )
            continue;
		if ( retcode != INPUT_SUCCESS )
			continue;
		m_text_io.tokenize( cmd );
		if ( !m_text_io.nextToken( cmd ) )
			continue;

		cmd.MakeLower();

		HandlerMap::iterator it = function_map.find( cmd );
		if ( it == function_map.end() ) {
			m_text_io.printf( "Unrecognized command '%s' - Type ? for list of commands\n", (LPCTSTR)cmd );
		}
		else if ( !m_running && (*it).second.m_running ) {
			m_text_io.printf( "UI must be running to use '%s'\n", (LPCTSTR)cmd );
		}
		else {
		    (this->*(*it).second.m_funcptr)();
		}
	}
}

// ----------------------------------------------------------------------------
//
static CString track_time( DWORD time )
{
    CString time_str;

    if ( time > 1000*60*60 ) {  // Hours
        time_str.Format( "%d:", time / (1000*60*60) );
        time = time % (1000*60*60);
    }

    time_str.AppendFormat( "%d:", time / (1000*60) );
    time = time % (1000*60);

    time_str.AppendFormat( "%02d", time / (1000) );

    return time_str;
}

// ----------------------------------------------------------------------------
//
bool TextUI::spotify_login()
{
    InputField username_field( "Spotify Username", spotifyUsername );
    InputField password_field( "Spotify Password", "" );
    password_field.setPassword( true );

    Form form( &m_text_io );
    form.setStopOnLastField( false );
    form.add( username_field );
    form.add( password_field );

    for ( ;; ) {
        if ( !form.play() )
            return false;
    
        if ( m_player->signon( username_field.getValue(), password_field.getValue() ) )
            break;

        printf( "%s\n", (LPCSTR)m_player->getLastPlayerError() );

        password_field.setValue( "" );
    }

    password_field.setValue( "               " );

    return true;
}

// ----------------------------------------------------------------------------
//
bool TextUI::isUserSure() {
	ConfirmForm form( &m_text_io );
	return form.play();
}

// ----------------------------------------------------------------------------
//
void TextUI::help() {
	for ( HandlerMap::iterator it=function_map.begin(); it != function_map.end(); it++ ) {
		m_text_io.printf( "%-4s - %s\n", (LPCTSTR)(*it).first, (*it).second.m_desc );
	}
}

// ----------------------------------------------------------------------------
//
void TextUI::quit() {
	m_running = !isUserSure();
}

// ----------------------------------------------------------------------------
//
void TextUI::listPlaylists()
{
    PlayerItems     playlists;

    m_player->getPlaylists( playlists );

    for ( PlayerItems::iterator it=playlists.begin(); it != playlists.end(); it++ ) {
        CString name = m_player->getPlaylistName( (*it) );
        name.Remove( '\n' );

        PlayerItems tracks;
        m_player->getTracks( (*it), tracks );
        printf( "%s (%d)\n", (LPCSTR)name, tracks.size() );
    }
}

// ----------------------------------------------------------------------------
//
void TextUI::listTracks(void)
{
	PlaylistField playlist_field( "Playlist", m_player );

	Form form( &m_text_io );
	form.add( playlist_field );

    if ( form.play() ) {
        PlayerItems tracks;
        m_player->getTracks( playlist_field.getPlaylist(), tracks );

        for ( PlayerItems::iterator it=tracks.begin(); it != tracks.end(); it++ ) {
            CString title, artist;
            DWORD duration = 0;
            bool starred = false;
            CString detailedInfo;

            if ( m_player->getTrackInfo( (*it), &title, &artist, NULL, &duration, &starred ) ) {
                duration /= 1000;
                int minutes = duration/60;
                int seconds = duration%60;

                printf( "%s by %s [%d:%02d] %s\n", (LPCSTR)title, (LPCSTR)artist, minutes, seconds, (starred) ? "*" : "" );
            }
            else
                printf( "Error readin track info \n" );
        }
    }
}

// ----------------------------------------------------------------------------
//
void TextUI::selectTrack( bool queue, bool seek )
{
	PlaylistField playlist_field( "Playlist", m_player );
    TrackListField tracks_field( "Track", m_player );
    IntegerField start_field( "Start (ms)", 0 );

	class MyForm : public Form {
		void fieldLeaveNotify( size_t field_num ) {
			if ( field_num == 0 ) {
				PlaylistField* playlist_field = getField<PlaylistField>( 0 );
				TrackListField* tracks_field = getField<TrackListField>( 1 );
                tracks_field->setPlaylist( playlist_field->getPlaylist() );
			}
		}

	public:
		MyForm( TextIO* input_stream ) :
            Form( input_stream ) {}
	};

	MyForm form( &m_text_io );
	form.add( playlist_field );
	form.add( tracks_field );

    if ( seek ) 
        form.add( start_field );

    if ( form.play() ) {
        if ( queue )
            m_player->queueTrack( tracks_field.getTrack() );
        else
            m_player->playTrack( tracks_field.getTrack(), start_field.getLongValue() );

        // Delay so the track starts 
        Sleep( 500 );
    }
}

// ----------------------------------------------------------------------------
//
void TextUI::showTrackAudioInfo( )
{
    static char * key[] = { "C","C#","D","D#","E","F","F#","G","B#","B","B#","B" };

	PlaylistField playlist_field( "Playlist", m_player );
    TrackListField tracks_field( "Track", m_player );

	class MyForm : public Form {
		void fieldLeaveNotify( size_t field_num ) {
			if ( field_num == 0 ) {
				PlaylistField* playlist_field = getField<PlaylistField>( 0 );
				TrackListField* tracks_field = getField<TrackListField>( 1 );
                tracks_field->setPlaylist( playlist_field->getPlaylist() );
			}
		}

	public:
		MyForm( TextIO* input_stream ) :
            Form( input_stream ) {}
	};

	MyForm form( &m_text_io );
	form.add( playlist_field );
	form.add( tracks_field );

    if ( form.play() ) {
        CString title, artist;
        DWORD duration = 0;
        bool starred = false;
        CString detailedInfo;
        AudioInfo audio_info;

        m_player->playTrack( tracks_field.getTrack(), 0L );

        m_player->getTrackInfo( tracks_field.getTrack(), &title, &artist, NULL, &duration, &starred );
        AudioStatus status = m_player->getTrackAudioInfo( tracks_field.getTrack(), &audio_info, 5000 );
         
        duration /= 1000;
        int minutes = duration/60;
        int seconds = duration%60;

        m_text_io.printf( "%s [%d:%02d] %s by %s\n\n", (LPCSTR)title, minutes, seconds, (starred) ? "*" : "", (LPCSTR)artist );

        m_text_io.printf( "               link = %s\n", tracks_field.getTrack() );
       
        switch (status) {
            case OK:
                m_text_io.printf( "                 id = %s\n", audio_info.id );
                m_text_io.printf( "           duration = %f seconds\n", audio_info.duration );
                m_text_io.printf( "          song type = %s\n", audio_info.song_type );
                m_text_io.printf( "                key = %s\n", key[audio_info.key] );
                m_text_io.printf( "               mode = %s\n", audio_info.mode ? "major" : "minor" );
                m_text_io.printf( "     time_signature = %d\n", audio_info.time_signature );
                m_text_io.printf( "             energy = %f\n", audio_info.energy );
                m_text_io.printf( "           liveness = %f\n", audio_info.liveness );
                m_text_io.printf( "              tempo = %f BPM\n", audio_info.tempo );
                m_text_io.printf( "        speechiness = %f\n", audio_info.speechiness );
                m_text_io.printf( "       acousticness = %f\n", audio_info.acousticness );
                m_text_io.printf( "   instrumentalness = %f\n", audio_info.instrumentalness );
                m_text_io.printf( "           loudness = %f dB\n", audio_info.loudness );
                m_text_io.printf( "            valence = %f\n", audio_info.valence );
                m_text_io.printf( "       danceability = %f\n", audio_info.danceability );
                break;

            case FAILED:
                m_text_io.printf( "               status = FAILED\n" );
                break;

            case NOT_AVAILABLE:
                m_text_io.printf( "               status = NOT AVAILABLE\n" );
                break;

            case QUEUED:
                m_text_io.printf( "               status = QUEUED\n" );
                break;
            }

        m_text_io.printf( "\n" );
    }
}

// ----------------------------------------------------------------------------
//
void TextUI::showTrackAnalysis()
{
    PlaylistField playlist_field( "Playlist", m_player );
    TrackListField tracks_field( "Track", m_player );

    class MyForm : public Form {
        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                PlaylistField* playlist_field = getField<PlaylistField>( 0 );
                TrackListField* tracks_field = getField<TrackListField>( 1 );
                tracks_field->setPlaylist( playlist_field->getPlaylist() );
            }
        }

    public:
        MyForm( TextIO* input_stream ) :
            Form( input_stream ) {}
    };

    MyForm form( &m_text_io );
    form.add( playlist_field );
    form.add( tracks_field );

    if ( form.play() ) {
        AnalyzeInfo* info;

        if ( !m_player->getTrackAnalysis( tracks_field.getTrack(), &info ) ) {
            m_text_io.printf( "Analysis not currently available for this track - play complete track first\n" );
            return;
        }

        m_text_io.printf( "%s - %d amplitude samples\n", (LPCSTR)info->link, info->data_count );

        static const char* level_meter = "====================================================================================================";

        UINT  ms = 0;
        for ( size_t i=0; i < info->data_count; i++ ) {
            uint16_t amplitude = info->data[i];

            uint16_t amp = (uint16_t)(amplitude / 32767.0 * 100 + .5);

            //printf( "%d %.2f\n", m_amplitude, amp );

            const char *meter = &level_meter[ 100 - amp ];
            m_text_io.printf( "%4.1f: %4d %s\n", ms/1000.0, amp, meter );

            ms += info->duration_ms;
        }
    }
}

// ----------------------------------------------------------------------------
//
void TextUI::selectPlaylist( bool queue )
{
	PlaylistField playlist_field( "Playlist", m_player );

	Form form( &m_text_io );
	form.add( playlist_field );

    if ( form.play() ) {
        m_player->playAllTracks( playlist_field.getPlaylist(), queue );
    }
}

// ----------------------------------------------------------------------------
//
void TextUI::stopTrack(void)
{
    m_player->stopTrack();  
}

// ----------------------------------------------------------------------------
//
void TextUI::playTrack(void)
{
    selectTrack( false, false );
}

// ----------------------------------------------------------------------------
//
void TextUI::playTrackSeek(void)
{
    selectTrack( false, true );
}

// ----------------------------------------------------------------------------
//
void TextUI::queueTrack(void)
{
    selectTrack( true, false );
}

// ----------------------------------------------------------------------------
//
void TextUI::playPlaylist(void)
{
    selectPlaylist( false );
}

// ----------------------------------------------------------------------------
//
void TextUI::queuePlaylist(void)
{
    selectPlaylist( true );
}

// ----------------------------------------------------------------------------
//
void TextUI::forwardTrack(void)
{
    m_player->forwardTrack();
}

// ----------------------------------------------------------------------------
//
void TextUI::pauseTrack(void)
{
    bool desired_state = !m_player->isTrackPaused();

    m_player->pauseTrack( desired_state );

    while ( m_player->isTrackPaused() != desired_state ) {
        CString track_link;
        m_player->getPlayingTrack( track_link );

        if ( track_link.GetLength() == 0 )
            break;
    }
}

// ----------------------------------------------------------------------------
//
void TextUI::backTrack(void)
{
    m_player->backTrack();
}

// ----------------------------------------------------------------------------
//
void TextUI::showQueuedTracks(void)
{
    PlayerItems tracks;
    m_player->getQueuedTracks( tracks );
    int index = 1;

    for ( PlayerItems::iterator it=tracks.begin(); it != tracks.end(); it++, index++ ) {
        CString title = m_player->getTrackFullName( (*it) );
        printf( "   %d: %s\n", index, (LPCSTR)title );
    }
}

// ----------------------------------------------------------------------------
//
void TextUI::showPlayedTracks(void)
{
    PlayerItems tracks;
    m_player->getPlayedTracks( tracks );
    int index = 1;

    std::reverse( tracks.begin(), tracks.end() );

    for ( PlayerItems::iterator it=tracks.begin(); it != tracks.end(); it++, index++ ) {
        CString title = m_player->getTrackFullName( (*it) );
        printf( "   %d: %s\n", index, (LPCSTR)title );
    }
}

// ----------------------------------------------------------------------------
//
void TextUI::enableEvents()
{
    bool result = m_player->registerEventListener( this );

    printf( "Events enabled: %s\n", result ? "YES" : "NO" );
}

// ----------------------------------------------------------------------------
//
void TextUI::disableEvents()
{
    bool result = m_player->unregisterEventListener( this );

    printf( "Events disabled: %s\n", result ? "YES" : "NO" );
}

// ----------------------------------------------------------------------------
//
HRESULT TextUI::notify( PlayerEventData* pNotify )
{
    LPCSTR eventName;
    
    switch( pNotify->m_event ) {
        case TRACK_PLAY:            eventName = "PLAY"; break;
        case TRACK_STOP:            eventName = "STOP"; break;
        case TRACK_PAUSE:           eventName = "PAUSE"; break;
        case TRACK_RESUME:          eventName = "RESUME"; break;
        case TRACK_POSITION:        eventName = "POSITION"; break;
        case TRACK_QUEUES:          eventName = "TRACK_QUEUES"; break;
        case PLAYLIST_ADDED:        eventName = "PLAYLIST_ADDED"; break;
        case PLAYLIST_REMOVED:      eventName = "PLAYLIST_REMOVED"; break;
        case PLAYLIST_CHANGED:      eventName = "PLAYLIST_CHANGED"; break;
        default:                    eventName = "UNKNOWN";  break;
    }

    if ( pNotify->m_event != TRACK_QUEUES ) {
        LPCSTR link = pNotify->m_link == NULL ? "?" : pNotify->m_link;

        printf( "EVENT: %s %s (%s)\n", eventName, link, (LPCSTR)track_time(pNotify->m_event_ms) );
    }
    else
        printf( "EVENT: %s played=%lu queued=%lu\n", eventName, pNotify->m_played_size, pNotify->m_queued_size );

    return S_OK;
}

