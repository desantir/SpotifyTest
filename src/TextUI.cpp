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

static CString track_time( DWORD time );

// ----------------------------------------------------------------------------
//
TextUI::TextUI( MusicPlayer* player ) :
    m_player( player ),
    m_running( false )
{
	function_map[ "q" ] = HandlerInfo( &TextUI::quit,						false,	"Quit" );
	function_map[ "lp" ] = HandlerInfo( &TextUI::listPlaylists,				false,	"List Playlists" );
	function_map[ "lt" ] = HandlerInfo( &TextUI::listTracks,				false,	"List Tracks" );
	function_map[ "pt" ] = HandlerInfo( &TextUI::playTrack,	    			false,	"Play Track" );
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
            label.Format( "Now Playing: %s", m_player->getTrackFullName( track ) );
            label.AppendFormat( " | length %s remaining %s", track_time(length), track_time(remaining) );

            if ( m_player->isTrackPaused() )
                label.Append( " | PAUSED" );
        }

		m_text_io.printf( "\n%s\n> ", (LPCSTR)label );

		m_text_io.clear();

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

		m_text_io.printf( "\n" );

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

            m_player->getTrackInfo( (*it), &title, &artist, NULL, &duration, &starred );
            
            duration /= 1000;
            int minutes = duration/60;
            int seconds = duration%60;



            printf( "%s [%d:%02d] %s\n   %s\n", title, minutes, seconds, (starred) ? "*" : "", artist );
        }
    }
}

// ----------------------------------------------------------------------------
//
void TextUI::selectTrack( bool queue )
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
        m_player->playTrack( tracks_field.getTrack(), queue );
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
    selectTrack( false );
}

// ----------------------------------------------------------------------------
//
void TextUI::queueTrack(void)
{
    selectTrack( true );
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
