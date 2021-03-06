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

#pragma once

#include "stdafx.h"
#include "TextIO.h"
#include "Form.h"
#include "MusicPlayer.h"

class TextUI : private IPlayerEventCallback
{
	bool	        m_running;
	TextIO	        m_text_io;
    MusicPlayer*    m_player;
	
public:
	TextUI( MusicPlayer* player );
	~TextUI(void);

	void run(void);

private:
    bool spotify_login();
    bool isUserSure();
    void selectTrack( bool queue, bool cache=false );
    void selectPlaylist( bool queue );

	void quit(void);
    void help(void);
	void listPlaylists(void);
	void listTracks(void);
    void playTrack(void);
	void searchTrack(void);
    void stopTrack(void);
    void pauseTrack(void);
    void playTrackSeek(void);
    void playPlaylist(void);
    void queueTrack(void);
    void forwardTrack(void);
    void backTrack(void);
    void queuePlaylist(void);
    void showQueuedTracks(void);
    void showPlayedTracks(void);
    void showTrackAudioInfo(void);
    void showTrackAnalysis(void);
    void enableEvents(void);
    void disableEvents(void);

    HRESULT STDMETHODCALLTYPE notify( PlayerEventData* pNotify );
};

typedef void (TextUI::*HandlerFunc)();

struct HandlerInfo
{
	HandlerFunc		m_funcptr;
	bool			m_running;
	const char*		m_desc;

	HandlerInfo( HandlerFunc funcptr, bool running, const char* desc ) :
		m_funcptr( funcptr ),
		m_running( running ),
		m_desc( desc)
	{}

	HandlerInfo( ) {}
};

typedef std::map<CString, HandlerInfo> HandlerMap;

class ConfirmForm : public Form
{
	BooleanField m_sure_field;

public:
	ConfirmForm( TextIO* input_stream, LPCSTR title=NULL, bool auto_delete=false ) :
		  Form( input_stream, title, auto_delete ),
		  m_sure_field( "Are you sure", false )
	{}

	bool play( void ) {
		if ( size() == 0 || getField<Field>(size()-1) != & m_sure_field )
			add( m_sure_field );
		return Form::play() && m_sure_field.isSet();
	}
};

class PlaylistField : public NumberedListField
{
    MusicPlayer*    m_player;
    PlayerItems     m_playlists;

    static int last_playlist_selection;

public:
    PlaylistField( LPCSTR name, MusicPlayer* player ) :
        NumberedListField( name ),
        m_player( player )
    {
        m_player->getPlaylists( m_playlists );

        int key = 1;

        for ( PlayerItems::iterator it=m_playlists.begin(); it != m_playlists.end(); it++ ) {
			PlaylistInfo playlist_info;
			if ( m_player->getPlaylistInfo((*it), &playlist_info) ) {
				CString name( playlist_info.playlist_name );
				name.Remove( '\n' );
				addKeyValue( key++, name );
			}
        }
        
        setDefaultListValue( last_playlist_selection );      
    }

    LPCSTR getPlaylist() {
        last_playlist_selection = getListValue();
        return m_playlists[ last_playlist_selection-1 ];
    }
};

class TrackListField : public NumberedListField
{
    MusicPlayer*    m_player;
    PlayerItems     m_playlists;
    PlayerItems     m_tracks;

public:
    TrackListField( LPCSTR name, MusicPlayer* player ) :
        NumberedListField( name ),
        m_player( player )
    {
    }

    void setPlaylist( LPCSTR playlist ) {
        m_player->getTracks( playlist, m_tracks );

        clear();

        int key = 1;

        for ( PlayerItems::iterator it=m_tracks.begin(); it != m_tracks.end(); it++ ) {
            CString title = m_player->getTrackFullName( (*it) );
            addKeyValue( key++, title );
        }
        
        setDefaultListValue( 1 );  
    }

	void searchTracks( LPCSTR search_query ) {
		m_player->searchTracks( search_query, m_tracks );

		clear();

		int key = 1;

		for ( PlayerItems::iterator it=m_tracks.begin(); it != m_tracks.end(); it++ ) {
			CString title = m_player->getTrackFullName( (*it) );
			addKeyValue( key++, title );
		}

		setDefaultListValue( 1 );  
	}

    LPCSTR getTrack() {
        return m_tracks.size() == 0 ? NULL : m_tracks[getListValue()-1 ];
    }
};
