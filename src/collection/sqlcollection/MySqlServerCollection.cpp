/*
 *  Copyright (c) 2008 Edward Toroshchin <edward.hades@gmail.com>
 *  Copyright (c) 2009 Jeff Mitchell <mitchell@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "MySqlServerCollection.h"

#include "Amarok.h"
#include "Debug.h"
#include "amarokconfig.h"

#include <QMutexLocker>
#include <QThreadStorage>
#include <QVarLengthArray>

#include <mysql.h>

MySqlServerCollection::MySqlServerCollection( const QString &id, const QString &prettyName )
    : MySqlCollection( id, prettyName )
{
    DEBUG_BLOCK
    if( mysql_library_init( 0, NULL, NULL ) )
    {
        error() << "MySQL library initialization failed!";
        return;
    }

    m_db = mysql_init( NULL );

    if( !m_db )
    {
        error() << "MySQL initialization failed";
        return;
    }

    if( !mysql_real_connect( m_db,
                Amarok::config( "MySQL" ).readEntry( "host" ).toUtf8(),
                Amarok::config( "MySQL" ).readEntry( "user" ).toUtf8(),
                Amarok::config( "MySQL" ).readEntry( "password" ).toUtf8(),
                NULL,
                Amarok::config( "MySQL" ).readEntry( "port" ).toInt(),
                NULL,
                CLIENT_COMPRESS )
        )
    {
        debug() << "connection to mysql failed";
        error() << "Could not connect to mysql!";
        reportError( "na" );
        mysql_close( m_db );
        m_db = 0;
    }
    else
    {
        QString databaseName = Amarok::config( "MySQL" ).readEntry( "database", "amarok" );
        if( mysql_query( m_db, "SET NAMES 'utf8'" ) )
            debug() << "set names died";
        if( mysql_query( m_db, QString( "CREATE DATABASE IF NOT EXISTS %1 DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_unicode_ci" ).arg( databaseName ).toUtf8() ) )
            debug() << "died creating database";
        if( mysql_query( m_db, QString( "ALTER DATABASE %1 DEFAULT CHARACTER SET utf8 DEFAULT COLLATE utf8_unicode_ci" ).arg( databaseName ).toUtf8() ) )
            debug() << "alter database died";
        if( mysql_query( m_db, QString( "USE %1" ).arg( databaseName ).toUtf8() ) )
            debug() << "use database died";

        debug() << "Connected to MySQL server" << mysql_get_server_info( m_db );
    }

    MySqlCollection::initThreadInitializer();
    init();
}

MySqlServerCollection::~MySqlServerCollection()
{
    DEBUG_BLOCK
}

QString
MySqlServerCollection::type() const
{
    return "MySQL";
}

#include "MySqlServerCollection.moc"

