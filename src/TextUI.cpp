/* 
Copyright (C) 2011,2012 Robert DeSantis
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

// ----------------------------------------------------------------------------
//
TextUI::TextUI( MusicPlayer* player ) :
    m_player( player ),
    m_running( false ),
    m_cached_track( NULL ),
    m_caching( false )
{
	function_map[ "q" ] = HandlerInfo( &TextUI::quit,						false,	"Quit" );
	function_map[ "lp" ] = HandlerInfo( &TextUI::listPlaylists,				false,	"List Playlists" );
	function_map[ "lt" ] = HandlerInfo( &TextUI::listTracks,				false,	"List Tracks" );
	function_map[ "at" ] = HandlerInfo( &TextUI::analyzeTrack,				false,	"Analyze Track" );
	function_map[ "pt" ] = HandlerInfo( &TextUI::playTrack,	    			false,	"Play Track" );
    function_map[ "ct" ] = HandlerInfo( &TextUI::cacheTrack,	    		false,	"Cache Track" );
    function_map[ "pct" ] = HandlerInfo( &TextUI::playCachedTrack,	  		false,	"Play Cached Track" );
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
}

// ----------------------------------------------------------------------------
//
TextUI::~TextUI(void)
{
    clearCachedTrack();
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
        DWORD track = m_player->getPlayingTrack( &length, &remaining );

        if ( track ) {
            label.Format( "Now %s: %s", ( m_caching ) ? "Caching" : "Playing", m_player->getTrackFullName( track ) );
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

        if ( m_caching ) {      // See if cached is done
            if ( m_player->getCachedTrack( &m_cached_track ) ) {
                m_text_io.printf( "STATUS: Track cache complete %.02lf GBs frames %u length %s\n", 
                    m_cached_track->data_size/(1024.0*1024.0*1024.0), 
                    m_cached_track->frames,
                    track_time(m_cached_track->length_ms) );
                m_caching = false;
            }
        }

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

        printf( "%s\n", m_player->getLastPlayerError() );

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
        printf( "%s (%d)\n", name, tracks.size() );
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

            m_player->getTrackInfo( (*it), &title, &artist, NULL, &duration, &starred );
            
            duration /= 1000;
            int minutes = duration/60;
            int seconds = duration%60;

            printf( "%s [%d:%02d] %s by %s {%s}\n", title, minutes, seconds, (starred) ? "*" : "", artist );
        }
    }
}

// ----------------------------------------------------------------------------
//
void TextUI::selectTrack( bool queue, bool cache )
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
        if ( !cache )
            m_player->playTrack( tracks_field.getTrack(), queue );
        else {
            clearCachedTrack();
            m_caching = true;
            m_player->cacheTrack( tracks_field.getTrack() );
        }
    }
}

// ----------------------------------------------------------------------------
//
void TextUI::analyzeTrack( )
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
        CString link;
        AudioInfo audio_info;

        m_player->playTrack( tracks_field.getTrack(), false );

        m_player->getTrackInfo( tracks_field.getTrack(), &title, &artist, NULL, &duration, &starred, &link );
        bool got_info = m_player->getTrackAudioInfo( tracks_field.getTrack(), &audio_info );
         
        duration /= 1000;
        int minutes = duration/60;
        int seconds = duration%60;

        printf( "%s [%d:%02d] %s by %s\n\n", title, minutes, seconds, (starred) ? "*" : "", artist );

        printf( "               link = %s\n", link );
        
        if ( got_info ) {
            printf( "                 id = %s\n", audio_info.id );
            printf( "           duration = %f seconds\n", audio_info.duration );
            printf( "          song type = %s\n", audio_info.song_type );
            printf( "                key = %s\n", key[audio_info.key] );
            printf( "               mode = %s\n", audio_info.mode ? "major" : "minor" );
            printf( "     time_signature = %d\n", audio_info.time_signature );
            printf( "             energy = %f\n", audio_info.energy );
            printf( "           liveness = %f\n", audio_info.liveness );
            printf( "              tempo = %f BPM\n", audio_info.tempo );
            printf( "        speechiness = %f\n", audio_info.speechiness );
            printf( "       acousticness = %f\n", audio_info.acousticness );
            printf( "   instrumentalness = %f\n", audio_info.instrumentalness );
            printf( "           loudness = %f dB\n", audio_info.loudness );
            printf( "            valence = %f\n", audio_info.valence );
            printf( "       danceability = %f\n", audio_info.danceability );
        }

        printf( "\n" );
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
void TextUI::cacheTrack(void)
{
    selectTrack( false, true );
}

// ----------------------------------------------------------------------------
//
void TextUI::playCachedTrack(void)
{
    if ( m_caching || m_cached_track == NULL ) {
        log_status( "Cached track in not available" );
        return;
    }

    printf( "playing cached track (cannot be interrupted)\n" );

    UINT frame = 0;
    const UINT SAMPLE_BUFFER= 44100 * 3;
    UINT frames = m_cached_track->frames;

    while ( frames > 0 ) {
        if ( audio_out->getCachedSamples() > SAMPLE_BUFFER ) {
            ::Sleep( 1000 );
            continue;
        }

        UINT load = std::min<UINT>( SAMPLE_BUFFER, frames );
        printf( "yo playing %u frames\n", load );

        if ( !audio_out->addSamples( load, 2, 44100, &m_cached_track->data[ frame * m_cached_track->getFrameSize() ] ) ) {
            printf( "error\n" );
            continue;
        }

        frame += load;
        frames -= load;
    }
    // m_player->playCachedTrack( );
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
    m_player->pauseTrack( !m_player->isTrackPaused() );
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
        printf( "   %d: %s\n", index, title );
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
        printf( "   %d: %s\n", index, title );
    }
}
