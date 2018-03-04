/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@asbest-online.de>               *
 *   Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "TestTrackLoader.h"

#include "amarokconfig.h"
#include "config-amarok-test.h"
#include "core/meta/Meta.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/meta/proxy/MetaProxy.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core-impl/support/TrackLoader.h"

#include <QSignalSpy>
#include <QTest>

#include <ThreadWeaver/Queue>

QTEST_MAIN( TestTrackLoader )

void
TestTrackLoader::initTestCase()
{
    AmarokConfig::instance("amarokrc");

    qRegisterMetaType<Meta::TrackPtr>();
    qRegisterMetaType<Meta::TrackList>();
    CollectionManager::instance(); // create in the main thread
}

void
TestTrackLoader::cleanupTestCase()
{
    // Wait for other jobs, like MetaProxys fetching meta data, to finish
    ThreadWeaver::Queue::instance()->finish();
}

void
TestTrackLoader::testFullMetadataInit()
{
    typedef QPair<QString, int> StringIntPair;
    QList<StringIntPair> pathsCounts;
    pathsCounts << qMakePair( dataPath( "data/audio/album" ), 3 )
                << qMakePair( dataPath( "data/audio/album2" ), 2 )
                << qMakePair( dataPath( "data/playlists/test.asx" ), 1 )
                << qMakePair( dataPath( "data/playlists/test.m3u" ), 10 )
                << qMakePair( dataPath( "data/playlists/test.pls" ), 4 )
                << qMakePair( dataPath( "data/playlists/test.xspf" ), 23 );

    // it is more probable to get unresolved MetaProxy::Track for small runs:
    for( const StringIntPair &pair : pathsCounts )
    {
        TrackLoader *loader = new TrackLoader( TrackLoader::FullMetadataRequired );
        QSignalSpy spy( loader, &TrackLoader::finished );
        loader->init( QUrl::fromLocalFile( pair.first ) );

        QVERIFY2( spy.wait( 15000 ), "loader did not finish within timeout" );

        Meta::TrackList found = spy.first().first().value<Meta::TrackList>();
        QCOMPARE( found.count(), pair.second );
        for( const Meta::TrackPtr &track : found )
        {
            MetaProxy::TrackPtr proxyTrack = MetaProxy::TrackPtr::dynamicCast( track );
            if( !proxyTrack )
            {
                qDebug() << track->prettyUrl() << "is not a MetaProxy::Track. Strange and we cannot test it";
                continue;
            }
            QVERIFY2( proxyTrack->isResolved(), proxyTrack->prettyUrl().toLocal8Bit().data() );
        }
        delete loader;
    }
}

void
TestTrackLoader::testInit()
{
    TrackLoader *loader1 = new TrackLoader();
    QSignalSpy spy1( loader1, &TrackLoader::finished );
    loader1->init( QUrl::fromLocalFile( dataPath( "data/audio" ) ) ); // test the convenience overload

    QVERIFY2( spy1.wait( 5000 ), "loader1 did not finish within timeout" );

    Meta::TrackList found = spy1.first().first().value<Meta::TrackList>();
    QCOMPARE( found.count(), 15 );
    QVERIFY2( found.at( 0 )->uidUrl().endsWith( "audio/album/Track01.ogg" ), found.at( 0 )->uidUrl().toLocal8Bit().data() );
    QVERIFY2( found.at( 1 )->uidUrl().endsWith( "audio/album/Track02.ogg" ), found.at( 1 )->uidUrl().toLocal8Bit().data() );
    QVERIFY2( found.at( 2 )->uidUrl().endsWith( "audio/album/Track03.ogg" ), found.at( 2 )->uidUrl().toLocal8Bit().data() );
    QVERIFY2( found.at( 3 )->uidUrl().endsWith( "audio/album2/Track01.ogg" ), found.at( 3 )->uidUrl().toLocal8Bit().data() );
    QVERIFY2( found.at( 4 )->uidUrl().endsWith( "audio/album2/Track02.ogg" ), found.at( 4 )->uidUrl().toLocal8Bit().data() );
    QVERIFY2( found.at( 5 )->uidUrl().endsWith( "audio/Platz 01.mp3" ), found.at( 5 )->uidUrl().toLocal8Bit().data() );
    QVERIFY2( found.at( 10 )->uidUrl().endsWith( "audio/Platz 06.mp3" ), found.at( 10 )->uidUrl().toLocal8Bit().data() );
    QVERIFY2( found.at( 14 )->uidUrl().endsWith( "audio/Platz 10.mp3" ), found.at( 14 )->uidUrl().toLocal8Bit().data() );

    TrackLoader *loader2 = new TrackLoader();
    QSignalSpy spy2( loader2, &TrackLoader::finished );
    loader2->init( QList<QUrl>() << QUrl::fromLocalFile( dataPath( "data/audio/album2" ) ) );

    QVERIFY2( spy2.wait( 5000 ), "loader2 did not finish within timeout" );

    found = spy2.first().first().value<Meta::TrackList>();
    QCOMPARE( found.count(), 2 );
    QVERIFY2( found.at( 0 )->uidUrl().endsWith( "audio/album2/Track01.ogg" ), found.at( 0 )->uidUrl().toLocal8Bit().data() );
    QVERIFY2( found.at( 1 )->uidUrl().endsWith( "audio/album2/Track02.ogg" ), found.at( 1 )->uidUrl().toLocal8Bit().data() );
}

void
TestTrackLoader::testInitWithPlaylists()
{
    TrackLoader *loader = new TrackLoader();
    QSignalSpy spy( loader, &TrackLoader::finished );
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile( dataPath( "data/playlists/test.asx" ) )
         << QUrl::fromLocalFile( dataPath( "data/audio/album" ) )
         << QUrl::fromLocalFile( dataPath( "data/playlists/test.xspf" ) );
    loader->init( urls );

    QVERIFY2( spy.wait( 5000 ), "loader did not finish within timeout" );

    Meta::TrackList found = spy.first().first().value<Meta::TrackList>();
    QCOMPARE( found.count(), 1 + 3 + 23 );
    QVERIFY( found.at( 0 )->uidUrl().endsWith( "/audio/album/Track01.ogg" ) ); // "audio/album" folder
    QVERIFY( found.at( 1 )->uidUrl().endsWith( "/audio/album/Track02.ogg" ) );
    QVERIFY( found.at( 2 )->uidUrl().endsWith( "/audio/album/Track03.ogg" ) );
    QCOMPARE( found.at( 3 )->uidUrl(), QString( "http://85.214.44.27:8000" ) ); // test.asx playlist
    QCOMPARE( found.at( 4 )->uidUrl(), QString( "http://he3.magnatune.com/all/01-Sunset-Ammonite.ogg" ) ); // start of test.xspf playlist
    QCOMPARE( found.at( 5 )->uidUrl(), QString( "http://he3.magnatune.com/all/02-Heaven-Ammonite.ogg" ) );
}

void
TestTrackLoader::testDirectlyPassingPlaylists()
{
    using namespace Playlists;
    TrackLoader *loader = new TrackLoader();
    QSignalSpy spy( loader, &TrackLoader::finished );
    PlaylistList playlists;
    playlists << PlaylistPtr::staticCast( loadPlaylistFile( QUrl::fromLocalFile( dataPath( "data/playlists/test.asx" ) ) ) )
              << PlaylistPtr::staticCast( loadPlaylistFile( QUrl::fromLocalFile( dataPath( "data/playlists/test.xspf" ) ) ) );
    loader->init( playlists );

    QVERIFY2( spy.wait( 5000 ), "loader did not finish within timeout" );

    Meta::TrackList found = spy.first().first().value<Meta::TrackList>();
    QCOMPARE( found.count(), 1 + 23 );
    QCOMPARE( found.at( 0 )->uidUrl(), QString( "http://85.214.44.27:8000" ) ); // test.asx playlist
    QCOMPARE( found.at( 1 )->uidUrl(), QString( "http://he3.magnatune.com/all/01-Sunset-Ammonite.ogg" ) ); // start of test.xspf playlist
    QCOMPARE( found.at( 2 )->uidUrl(), QString( "http://he3.magnatune.com/all/02-Heaven-Ammonite.ogg" ) );
}

QString
TestTrackLoader::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}
