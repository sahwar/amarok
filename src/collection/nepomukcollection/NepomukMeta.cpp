/* 
   Copyright (C) 2008 Daniel Winter <dw@danielwinter.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#define DEBUG_PREFIX "NepomukMeta"

#include "NepomukMeta.h"

#include "Debug.h"
#include "ScriptManager.h"

#include <QDateTime>
#include <QFile>
#include <QMutexLocker>
#include <QThread>

#include <Nepomuk/Variant>
#include <Soprano/BindingSet>
#include <Soprano/Model>

using namespace Meta;

NepomukArtist::NepomukArtist( const QString &name )
    : Artist()
    , m_name( name )
{

}

QString
NepomukArtist::name() const
{
    return m_name;
}

QString
NepomukArtist::prettyName() const
{
    return m_name;
}

QString
NepomukArtist::sortableName() const
{
    if ( m_sortName.isEmpty() && !m_name.isEmpty() ) {
        if ( m_name.startsWith( "the ", Qt::CaseInsensitive ) ) {
            QString begin = m_name.left( 3 );
            m_sortName = QString( "%1, %2" ).arg( m_name, begin );
            m_sortName = m_sortName.mid( 4 );
        }
        else {
            m_sortName = m_name;
        }
    }
    return m_sortName;
}

TrackList
NepomukArtist::tracks()
{
    return TrackList();
}

AlbumList
NepomukArtist::albums()
{
    return AlbumList();
}

// --- ALBUM ----

NepomukAlbum::NepomukAlbum( const QString &name, const QString &artist )
    : Album()
    , m_name( name )
    , m_artist( artist )
{

}

QString
NepomukAlbum::name() const
{
    return m_name;
}

QString
NepomukAlbum::prettyName() const
{
    return m_name;
}

TrackList
NepomukAlbum::tracks()
{
    return TrackList();
}

bool
NepomukAlbum::isCompilation() const
{
    return false;
}

bool
NepomukAlbum::hasAlbumArtist() const
{
    return true;
}

ArtistPtr
NepomukAlbum::albumArtist() const
{
    return ArtistPtr( new NepomukArtist( m_artist ) );
}


// -- TRACK --

class Meta::WriteStatisticsThread : public QThread
{
    public:        
        WriteStatisticsThread( NepomukTrack *track )
            : QThread()
            , m_track( track )
        {
        }

        void run()
        {
            m_track->writeStatistics();
        }
    private:
        NepomukTrack *m_track;
};

NepomukTrack::NepomukTrack( NepomukCollection *collection, const Soprano::BindingSet data )
    : Track()
    , m_collection ( collection )
{
    statsThread =  new WriteStatisticsThread( this );

    m_nepores = Nepomuk::Resource( data[ "r"].uri() ) ;
    m_title = data[ collection->getNameForValue( QueryMaker::valTitle ) ].toString();
    m_url = KUrl( data[ collection->getNameForValue( QueryMaker::valUrl ) ].toString() );
    m_artist = data[ collection->getNameForValue( QueryMaker::valArtist ) ].toString();
    m_album = data[ collection->getNameForValue( QueryMaker::valAlbum ) ].toString();
    m_genre = data[ collection->getNameForValue( QueryMaker::valGenre ) ].toString();
    m_type = data[ collection->getNameForValue( QueryMaker::valFormat ) ].toString();
    m_comment = data[ collection->getNameForValue( QueryMaker::valComment ) ].toString();
    m_composer = data[ collection->getNameForValue( QueryMaker::valComposer ) ].toString();
    m_trackNumber = data[ collection->getNameForValue( QueryMaker::valTrackNr ) ]
                          .literal().toInt();
    m_length = data[ collection->getNameForValue( QueryMaker::valLength ) ]
                     .literal().toInt();
    m_rating = data[ collection->getNameForValue( QueryMaker::valRating ) ]
                         .literal().toInt();
    m_bitrate = data[ collection->getNameForValue( QueryMaker::valBitrate ) ]
                    .literal().toInt();
    m_discNumber = data[ collection->getNameForValue( QueryMaker::valDiscNr ) ]
                             .literal().toInt();
    m_filesize = data[ collection->getNameForValue( QueryMaker::valFilesize ) ]
                       .literal().toInt();
    m_playCount = data[ collection->getNameForValue( QueryMaker::valPlaycount ) ]
                         .literal().toInt();
    m_sampleRate = data[ collection->getNameForValue( QueryMaker::valSamplerate ) ]
                          .literal().toInt();
    m_score = data[ collection->getNameForValue( QueryMaker::valScore ) ]
                        .literal().toInt();
    
    // Soprano gives a warning when they are empty
    Soprano::LiteralValue litval;
    
    litval = data[ collection->getNameForValue( QueryMaker::valFirstPlayed ) ]
                   .literal();
    if ( litval.isDateTime() )
        m_firstPlayed = litval.toDateTime();

    litval = data[ collection->getNameForValue( QueryMaker::valLastPlayed ) ]
                   .literal();
    if ( litval.isDateTime() )
        m_lastPlayed = litval.toDateTime();
    
    litval = data[ collection->getNameForValue( QueryMaker::valCreateDate ) ]
                   .literal();
    if ( litval.isDateTime() )
        m_createDate = litval.toDateTime();
 
    // assuming that Xesam content created is a DateTime, we only want the year
    litval = data[ collection->getNameForValue( QueryMaker::valYear ) ].literal();
    if ( litval.isDateTime() )
        m_year = litval.toDateTime().toString( "yyyy");
}

NepomukTrack::~NepomukTrack()
{
    statsThread->wait();
    delete statsThread;
}

QString
NepomukTrack::name() const
{
    return m_title;
}

QString
NepomukTrack::prettyName() const
{
    if (!m_title.isEmpty())
        return m_title;
    else
        return this->m_url.fileName();
}

KUrl
NepomukTrack::playableUrl() const
{
    return m_url;
}

QString
NepomukTrack::url() const
{
    return m_url.url();
}

QString
NepomukTrack::prettyUrl() const
{
    return m_url.prettyUrl();
}

bool
NepomukTrack::isPlayable() const
{
    //a song is not playable anymore if the collection was removed
    return m_collection && QFile::exists( m_url.path() );
}

bool
NepomukTrack::inCollection() const
{
    return true;
}

AlbumPtr
NepomukTrack::album() const
{
    return AlbumPtr( new NepomukAlbum( m_album, m_artist ) );
}

ArtistPtr
NepomukTrack::artist() const
{
    return ArtistPtr( new NepomukArtist( m_artist ) );
}

GenrePtr
NepomukTrack::genre() const
{
    return GenrePtr( new NepomukGenre( m_genre ) );
}

ComposerPtr
NepomukTrack::composer() const
{
    return ComposerPtr( new NepomukComposer( m_composer ) );
}

YearPtr
NepomukTrack::year() const
{
    return YearPtr( new NepomukYear( m_year ) );
}

QString
NepomukTrack::comment() const
{
    return m_comment;
}

double
NepomukTrack::score() const
{
    // TODO: why double? From 0 to 1?
    return double( m_score );
}

void
NepomukTrack::setScore( double newScore )
{
    // scores are between 0 and 1?  Xesam wants them to be int so lets
    // multiply them by 100 (hope that is enough)
    QMutexLocker locker( &statsMutex );
    int tmpScore =  int( newScore*100 );
    m_nepores.setProperty( QUrl( m_collection->getUrlForValue( QueryMaker::valScore ) ), Nepomuk::Variant( tmpScore ) );
    m_score = newScore;
    notifyObservers();
}

int
NepomukTrack::rating() const
{
    return m_rating;
}

void
NepomukTrack::setRating( int newRating )
{
    m_nepores.setRating( newRating );
    m_rating = newRating;
    notifyObservers();
}

int
NepomukTrack::length() const
{
    return m_length;
}

int
NepomukTrack::filesize() const
{
    return m_filesize;
}

int
NepomukTrack::sampleRate() const
{
    return m_sampleRate;
}

int
NepomukTrack::bitrate() const
{
    return m_bitrate;
}

int
NepomukTrack::trackNumber() const
{
    return m_trackNumber;
}

int
NepomukTrack::discNumber() const
{
    return m_discNumber;
}

uint
NepomukTrack::lastPlayed() const
{
    return m_lastPlayed.toTime_t();
}

int
NepomukTrack::playCount() const
{
    return m_playCount;
}

QString
NepomukTrack::type() const
{
    return m_type;
}

void 
NepomukTrack::finishedPlaying( double playedFraction )
{
    QMutexLocker locker( &statsMutex );
    m_lastPlayed = QDateTime::currentDateTime();
    if( m_playCount == 0 )
    {
        m_firstPlayed = m_lastPlayed;
    }
    m_playCount++;
    ScriptManager::instance()->requestNewScore( url(), score(), playCount(), length(), playedFraction * 100 /*scripts expect it as a percent, not a fraction*/, QString() );
    notifyObservers();
    statsThread->start( QThread::LowPriority );
}

void
NepomukTrack::writeStatistics()
{
    QMutexLocker locker( &statsMutex );
    m_nepores.setProperty( QUrl( m_collection->getUrlForValue( QueryMaker::valLastPlayed) ), Nepomuk::Variant( m_lastPlayed ) );
    m_nepores.setProperty( QUrl( m_collection->getUrlForValue( QueryMaker::valPlaycount) ), Nepomuk::Variant( m_playCount ) );
    m_nepores.setProperty( QUrl( m_collection->getUrlForValue( QueryMaker::valFirstPlayed) ), Nepomuk::Variant( m_firstPlayed ) );
}

// -- GENRE --

NepomukGenre::NepomukGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
{

}

QString
NepomukGenre::name() const
{
    return m_name;
}

QString
NepomukGenre::prettyName() const
{
    return m_name;
}

TrackList
NepomukGenre::tracks()
{
    return TrackList();
}

// -- COMPOSER --

NepomukComposer::NepomukComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
{

}

QString
NepomukComposer::name() const
{
    return m_name;
}

QString
NepomukComposer::prettyName() const
{
    return m_name;
}

TrackList
NepomukComposer::tracks()
{
    return TrackList();
}

// -- YEAR --

NepomukYear::NepomukYear( const QString &name )
    : Meta::Year()
    , m_name( name )
{

}

QString
NepomukYear::name() const
{
    return m_name;
}

QString
NepomukYear::prettyName() const
{
    return m_name;
}

TrackList
NepomukYear::tracks()
{
    return TrackList();
}




