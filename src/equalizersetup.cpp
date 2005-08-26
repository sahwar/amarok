/***************************************************************************
 Setup dialog for the equalizer

 (c) 2004 Mark Kretschmann <markey@web.de>
 (c) 2005 Seb Ruiz <me@sebruiz.net>
 (c) 2005 Markus Brueffer <markus@brueffer.de>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"
#include "enginebase.h"
#include "enginecontroller.h"
#include "equalizergraph.h"
#include "equalizerpresetmanager.h"
#include "equalizersetup.h"
#include "sliderwidget.h"

#include <qcheckbox.h>
#include <qdesktopwidget.h>
#include <qdom.h>
#include <qfile.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qstringlist.h>
#include <qtextstream.h>   //presets

#include <kapplication.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kinputdialog.h>  //presets
#include <klocale.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h> //locate()
#include <kwin.h>

EqualizerSetup* EqualizerSetup::s_instance = 0;


EqualizerSetup::EqualizerSetup()
        : QVBox( amaroK::mainWindow(), 0, WType_TopLevel|WDestructiveClose|WStyle_Customize|
                                          WStyle_DialogBorder|WStyle_StaysOnTop )
{
    using amaroK::Slider;

    s_instance = this;

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n( "Equalizer" ) ) );

    // Gives the window a small title bar, and skips a taskbar entry
    KWin::setType( winId(), NET::Utility );
    KWin::setState( winId(), NET::SkipTaskbar );

    setMargin( KDialog::marginHint() );
    setSpacing( KDialog::spacingHint() );

    QVBoxLayout* layout = new QVBoxLayout( this );

    // BEGIN Presets
    QHBox* presetBox = new QHBox( this );
    presetBox->setSpacing( KDialog::spacingHint() );

    new QLabel( i18n("Presets:"), presetBox );

    m_presetCombo = new KComboBox( presetBox );
    m_presetCombo->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred ) );

    QPushButton* presetAdd = new QPushButton( presetBox );
    presetAdd->setIconSet( KGlobal::iconLoader()->loadIconSet("filenew", KIcon::Small) );
    connect( presetAdd, SIGNAL( clicked() ), SLOT( addPreset() ) );

    QPushButton* presetConf = new QPushButton( presetBox );
    presetConf->setIconSet( KGlobal::iconLoader()->loadIconSet("configure", KIcon::Small) );
    connect( presetConf, SIGNAL( clicked() ), SLOT( editPresets() ) );

    layout->addWidget( presetBox );
    loadPresets();
    connect( m_presetCombo, SIGNAL( activated(int) ), SLOT( presetChanged(int) ) );
    // END Presets

    // BEGIN GroupBox
    QGroupBox* groupBoxSliders = new QGroupBox( 1, Qt::Vertical, i18n("Enable Equalizer"), this );
    groupBoxSliders->setCheckable( true );
    groupBoxSliders->setChecked( AmarokConfig::equalizerEnabled() );
    groupBoxSliders->setInsideMargin( KDialog::marginHint() );
    connect( groupBoxSliders, SIGNAL( toggled( bool ) ), SLOT( setEqualizerEnabled( bool ) ) );
    layout->addWidget( groupBoxSliders );

    // Helper widget for layouting inside the groupbox
    QWidget* slidersLayoutWidget = new QWidget( groupBoxSliders );
    slidersLayoutWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QGridLayout* slidersGridLayout = new QGridLayout( slidersLayoutWidget, 1, 1, 0, KDialog::spacingHint() );
    // END GroupBox

    // BEGIN Preamp slider
    m_slider_preamp = new Slider( Qt::Vertical, slidersLayoutWidget, 100 );
    m_slider_preamp->setMinValue( -100 );
    m_slider_preamp->setTickmarks( QSlider::Right );
    m_slider_preamp->setTickInterval( 100 );
    connect( m_slider_preamp, SIGNAL( valueChanged( int ) ), SLOT( setEqualizerParameters() ) );
    slidersGridLayout->addMultiCellWidget(m_slider_preamp, 0, 0, 0, 1, Qt::AlignHCenter );

    QLabel* preampLabel = new QLabel( i18n("Pre-amp"), slidersLayoutWidget );
    slidersGridLayout->addMultiCellWidget(preampLabel, 1, 1 , 0, 1, Qt::AlignHCenter );
    // END

    // BEGIN Band Sliders
    const char *bandLabels[] = { "60", "170", "310", "600", "1k", "3k", "6k", "12k", "14k", "16k" };

    int minWidth = 0;
    QFontMetrics fm = fontMetrics(); //apparently it's an expensive call
    for ( int i = 0; i < 10; i++ ) {
         int w = fm.width( bandLabels[i] );
         if ( w > minWidth )
             minWidth = w;
    }

    for ( int i = 0; i < 10; i++ ) {
        Slider *slider = new Slider( Qt::Vertical, slidersLayoutWidget );
        QLabel *label  = new QLabel( bandLabels[i], slidersLayoutWidget );

        slider->setMinValue( -100 );
        slider->setMaxValue( +100 );
        slider->setMinimumWidth( minWidth );
        slidersGridLayout->addMultiCellWidget(slider, 0, 0, 2 * i + 2, 2 * i + 3, Qt::AlignHCenter );
        slidersGridLayout->addMultiCellWidget(label, 1, 1, 2 * i + 2, 2 * i + 3, Qt::AlignHCenter );
        m_bandSliders.append( slider );

        connect( slider, SIGNAL( valueChanged( int ) ), SLOT( setEqualizerParameters() ) );
        connect( slider, SIGNAL( valueChanged( int ) ), SLOT( sliderChanged() ) );
    }
    // END

    // BEGIN Equalizer Graph Widget
    QGroupBox* graphGBox = new QGroupBox( 2, Qt::Horizontal, 0, this  );
    graphGBox->setInsideMargin( KDialog::marginHint() );
    layout->addWidget( graphGBox );

    QVBox* graphVBox = new QVBox( graphGBox );
    QLabel* graphLabel1 = new QLabel("+20 db", graphVBox);
    QLabel* graphLabel2 = new QLabel("0 db", graphVBox);
    QLabel* graphLabel3 = new QLabel("-20 db", graphVBox);
    graphLabel1->setAlignment( Qt::AlignRight | Qt::AlignTop );
    graphLabel2->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    graphLabel3->setAlignment( Qt::AlignRight | Qt::AlignBottom );

    m_equalizerGraph = new EqualizerGraph( graphGBox );
    m_equalizerGraph->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    // END Graph Widget

    // Fill the combobox
    updatePresets( AmarokConfig::equalizerPreset() );

    // Init sliders
    presetChanged( AmarokConfig::equalizerPreset() );

    // Center the widget on screen
    const QDesktopWidget dw;
    const int w = dw.screenGeometry().width();
    const int h = dw.screenGeometry().height();
    move( w / 2 - width() / 2, h / 2 - height() / 2 );
}


EqualizerSetup::~EqualizerSetup()
{
    savePresets();
    s_instance = 0;
}

void
EqualizerSetup::setBands( int preamp, QValueList<int> gains )
{
    m_slider_preamp->setValue( preamp );

    // Note: As a side effect, this automatically switches the
    //       preset to 'Manual', which is by intention 
    for ( uint i = 0; i < m_bandSliders.count(); i++ )
        m_bandSliders.at(i)->setValue( ( *gains.at(i) ) );

    setEqualizerParameters();
}

void
EqualizerSetup::setPreset( QString name )
{
    // Look for the preset id and verify name
    int i, count = m_presetCombo->count();
    bool found = false;
    for( i = 0; i < count; i++ ) {
        if ( m_presetCombo->text( i ) == name ) {
            found = true;
            break;
        }
    }

    if ( found ) {
        m_presetCombo->setCurrentItem( i );
        presetChanged( name );
    }
}

/////////////////////////////////////////////////////////////////////////////////////
// EQUALIZER PRESETS
/////////////////////////////////////////////////////////////////////////////////////

QString
EqualizerSetup::presetsCache() const
{
    // returns the playlists stats cache file
    return amaroK::saveLocation() + "equalizerpresets_save.xml";
}


void
EqualizerSetup::loadPresets()
{
    // Create predefined presets 'Zero' and 'Manual'
    QValueList<int> zeroGains;
    zeroGains << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0;
    m_presets[ i18n("Manual") ] = zeroGains;
    m_presets[ i18n("Zero") ] = zeroGains;

    QFile file( presetsCache() );
    if ( !file.exists() )
        file.setName( locate( "data", "amarok/data/equalizer_presets.xml" ) );

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );

    QDomDocument d;

    if( !file.open( IO_ReadOnly ) || !d.setContent( stream.read() ) ) {
        // Everything went wrong, so at least provide the two predefined presets
        updatePresets( AmarokConfig::equalizerPreset() );
        return;
    }

    QDomNode n = d.namedItem( "equalizerpresets" ).namedItem("preset");

    for( ; !n.isNull();  n = n.nextSibling() )
    {
        QDomElement e = n.toElement();
        QString title = e.attribute( "name" );

        QValueList<int> gains;
        gains << e.namedItem( "b0" ).toElement().text().toInt();
        gains << e.namedItem( "b1" ).toElement().text().toInt();
        gains << e.namedItem( "b2" ).toElement().text().toInt();
        gains << e.namedItem( "b3" ).toElement().text().toInt();
        gains << e.namedItem( "b4" ).toElement().text().toInt();
        gains << e.namedItem( "b5" ).toElement().text().toInt();
        gains << e.namedItem( "b6" ).toElement().text().toInt();
        gains << e.namedItem( "b7" ).toElement().text().toInt();
        gains << e.namedItem( "b8" ).toElement().text().toInt();
        gains << e.namedItem( "b9" ).toElement().text().toInt();

        m_presets[ title ] = gains;
    }

    file.close();
}


void
EqualizerSetup::savePresets()
{
    QFile file( presetsCache() );

    if( !file.open( IO_WriteOnly ) ) return;

    QDomDocument doc;
    QDomElement e = doc.createElement("equalizerpresets");
    e.setAttribute( "product", "amaroK" );
    e.setAttribute( "version", APP_VERSION );
    e.setAttribute( "formatversion", "1.1" );

    doc.appendChild( e );

    QStringList info;
    info << "b0" << "b1" << "b2" << "b3" << "b4"
         << "b5" << "b6" << "b7" << "b8" << "b9";

    for( uint x = 0; x < m_presets.count(); x++ )
    {
        const QString title = m_presetCombo->text( x );

        // dont save the 'Zero' preset
        if ( title == i18n("Zero") )
            continue;

        QValueList<int> gains = m_presets[ title ];

        QDomElement i = doc.createElement("preset");
        i.setAttribute( "name", title );
        debug() << "Title: " << title << endl;

        QDomElement attr;
        QDomText t;
        for( uint y=0; y < info.count(); y++ )
        {
            attr = doc.createElement( info[y] );
            t    = doc.createTextNode( QString::number( gains.first() ) );
            attr.appendChild( t );
            i.appendChild( attr );
            debug() << "\t" << info[y] << ": " << gains.first() << endl;
            gains.pop_front();
        }
        e.appendChild( i );
    }

    QTextStream stream( &file );
    stream.setEncoding( QTextStream::UnicodeUTF8 );
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    stream << doc.toString();
    file.close();
}

void
EqualizerSetup::editPresets()
{
    EqualizerPresetManager * editor = new EqualizerPresetManager(this);
    editor->setPresets(m_presets);

    if ( editor->exec() ) {
        QMap< QString, QValueList<int> > presets = editor->presets();

        QString currentTitle = m_presetCombo->currentText();
        QValueList<int> currentGains= m_presets[ currentTitle ];

        QString newTitle = currentTitle;

        // Check if the selected item was renamed
        if ( presets.find( currentTitle ) == presets.end() || currentGains != presets[ currentTitle ] ) {

            // Find the new name
            QMap< QString, QValueList<int> >::Iterator end = presets.end();
            for ( QMap< QString, QValueList<int> >::Iterator it = presets.begin(); it != end; ++it ) {
                if ( it.data() == currentGains ) {
                    newTitle = it.key();
                    break;
                }
            }
        }

        m_presets = presets;
        updatePresets( newTitle );
    }

    delete editor;
}

void
EqualizerSetup::addPreset()
{
    bool ok;
    const QString title = KInputDialog::getText( i18n("Add Equalizer Preset"),
                                                 i18n("Enter preset name:"), i18n("Untitled"), &ok, this);

    if (ok) {
        // Check if the new preset title exists
        if ( m_presets.find( title ) != m_presets.end() ) {
            int button = KMessageBox::warningYesNo( this, i18n( "A preset with the name %1 already exists. Overwrite?" ).arg( title ) );

            if ( button != KMessageBox::Yes )
                return;
        }

        // Add the new preset based on the current slider positions
        QValueList <int> gains;
        for ( uint i = 0; i < m_bandSliders.count(); i++ )
            gains += m_bandSliders.at( i )->value();
        m_presets[ title ] = gains;

        // Rebuild the combobox
        updatePresets(title);

        // Save
        setEqualizerParameters();
    }
}

void
EqualizerSetup::updatePresets(QString selectTitle)
{
    // Save the selected item
    if ( selectTitle.isEmpty() )
        selectTitle = m_presetCombo->currentText();

    // Sort titles
    QStringList titles;
    QMap< QString, QValueList<int> >::Iterator end = m_presets.end();
    for ( QMap< QString, QValueList<int> >::Iterator it = m_presets.begin(); it != end; ++it )
        titles << it.key();

    titles.sort();

    // rebuild preset combobox and look for the previously selected title
    int i = 0;
    int newIndex = -1;
    m_presetCombo->clear();
    QStringList::Iterator titlesEnd = titles.end();
    for ( QStringList::Iterator it = titles.begin(); it != titlesEnd; ++it ) {
        m_presetCombo->insertItem( *it );
        if ( *it == selectTitle )
            newIndex = i;
        if ( *it == i18n("Manual") )
            m_manualPos = i;
        i++;
    }

    if ( newIndex == -1 )
        newIndex = m_manualPos;

    m_presetCombo->setCurrentItem( newIndex );
}

/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
/////////////////////////////////////////////////////////////////////////////////////

void
EqualizerSetup::presetChanged( int id ) //SLOT
{
    presetChanged( m_presetCombo->text(id) );
}

void
EqualizerSetup::presetChanged( QString title ) //SLOT
{
    const QValueList<int> gains = m_presets[ title ];

    for ( uint i = 0; i < m_bandSliders.count(); i++ ) {
        // Block signals to prevent unwanted setting to 'Manual'
        m_bandSliders.at(i)->blockSignals(true);
        m_bandSliders.at(i)->setValue( ( *gains.at(i) ) );
        m_bandSliders.at(i)->blockSignals(false);
    }

    setEqualizerParameters();
}

void
EqualizerSetup::setEqualizerEnabled( bool active ) //SLOT
{
    EngineController::engine()->setEqualizerEnabled( active );
    AmarokConfig::setEqualizerEnabled( active );

    if( active )
        //this way the developer of the eq doesn't have to cache the eq values
        setEqualizerParameters();
    else
        // zero the graph
        m_equalizerGraph->update();
}


void
EqualizerSetup::setEqualizerParameters() //SLOT
{
    AmarokConfig::setEqualizerPreamp( m_slider_preamp->value() );
    AmarokConfig::setEqualizerPreset( m_presetCombo->currentText() );
    AmarokConfig::setEqualizerGains ( m_presets[ m_presetCombo->currentText() ] );

    // Transfer values to the engine if the EQ is enabled
    if ( AmarokConfig::equalizerEnabled() )
        EngineController::engine()->setEqualizerParameters( m_slider_preamp->value(), m_presets[ m_presetCombo->currentText() ] );

    m_equalizerGraph->update();
}


void
EqualizerSetup::sliderChanged() //SLOT
{
    m_presetCombo->setCurrentItem( m_manualPos );

    QValueList<int> gains;
    for ( uint i = 0; i < m_bandSliders.count(); i++ )
        gains += m_bandSliders.at( i )->value();

    m_presets[ i18n("Manual") ] = gains;
}

#include "equalizersetup.moc"
