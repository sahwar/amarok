Importer.loadQtBinding( "qt.core" );
Importer.load( "debug.js" );

print ( "this is main.js" );

//test engine:

var version = Amarok.Version();
print ( ( "Amarok Version: " + version ));

/*
var Engine;
Engine = Amarok.Engine;
Engine.Stop( true );
Engine.Play();
Engine.Seek ( 60*1000 );
*/

//test WindowAction

function Menu1Clicked()
{
    print ("hey, menu1!");
}

function Menu2Clicked()
{
    print ("hey, menu2!");
}

Amarok.Window.addToolsSeparator();

if ( Amarok.Window.addToolsMenu( "testMenu1" ) )
    Amarok.Window.ToolsMenu.testMenu1.triggered.connect(Menu1Clicked);
else
    print ( "Menu1 already exists!" );

if ( Amarok.Window.addToolsMenu( "testMenu2" ) )
    Amarok.Window.ToolsMenu.testMenu2.triggered.connect(Menu2Clicked);
else
    print ( "Menu2 already exists!" );


var TrackInfo = Amarok.Playlist.TrackInfo( -1 );
var OSD = Amarok.Window.OSD;
var StatusBar = Amarok.Window.Statusbar;

if ( TrackInfo["isValid"] )
{
    OSD.setText( "Hey, this is " + TrackInfo["artist"] );
    OSD.show();

    StatusBar.longMessage( "You are listening to album: " + TrackInfo["album"] );
}

function onConfigure()
{
    Amarok.alert( "This script does not require any configuration." );
}

function TrackChanged()
{
    print( "Track Changed!" );
}

function TrackSeeked()
{
    print( "Track Seeked!" );
}

function TrackFinished()
{
    print( "Track Finished!" );
}

function PlaylistCountChanged( count )
{
    print( count );
}

Amarok.configured.connect( onConfigure );
Amarok.Engine.trackChanged.connect( TrackChanged );
Amarok.Engine.trackSeeked.connect( TrackSeeked );
Amarok.Engine.trackFinished.connect( TrackFinished );
Amarok.Playlist.CountChanged.connect( PlaylistCountChanged );


var time = QTime.currentTime();
print( time.hour() );
print( time.minute() );