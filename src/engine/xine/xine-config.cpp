/***************************************************************************
 * copyright            : (C) 2005 Ian Monroe <ian@monroe.nu> 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#include "debug.h"
#include "xine-config.h"
#include "xinecfg.h"

#include <kcombobox.h>
#include <klocale.h>
#include <knuminput.h>
#include <kstandarddirs.h>

#include <q3groupbox.h>
#include <QLabel>
#include <QPixmap>
//Added by qt3to4:
#include <QByteArray>

////////////////////
/// XineGeneralEntry
////////////////////
XineGeneralEntry::XineGeneralEntry(const QString& key, xine_t *xine, XineConfigDialog* xcf)
    : m_valueChanged(false)
    , m_key(key)
    , m_xine(xine)
{
    debug() << "new entry " << m_key;
    connect(this, SIGNAL(viewChanged()), xcf, SIGNAL(viewChanged() ) );
}


void XineGeneralEntry::entryChanged()
{
    m_valueChanged = true;
    emit viewChanged();
}

/////////////////
/// Function saveXineEntry
/////////////////
template<class T, class Functor>
void saveXineEntry(Functor& storeEntry, T val, const QString& key, xine_t *xine)
{
    if(xine) debug() << "its not null " << key << ' ' << val;
    xine_cfg_entry_t ent;
    if(xine_config_lookup_entry(xine, key.toAscii(), &ent))
    {
        storeEntry(&ent, val);
        xine_config_update_entry(xine, &ent);
    }
    else
        debug()<<"Error saving " << val << " with key " << key;

}
//////////////////
/// Functors
//////////////////
void
XineIntFunctor::operator()( xine_cfg_entry_t* ent, int val )
{
    ent->num_value = val;
}


void
XineStrFunctor::operator()( xine_cfg_entry_t* ent, const QString& val )
{
    ent->str_value = val.toAscii().data();
}

////////////////////
/// XineStrEntry
////////////////////
XineStrEntry::XineStrEntry(QLineEdit* input, const QByteArray & key, xine_t *xine, XineConfigDialog* xcf)
    : XineGeneralEntry(key,xine,xcf)
{
    xine_cfg_entry_t ent;
    if( xine_config_lookup_entry( m_xine, m_key.toAscii(), &ent ) )
    {
        input->setText(ent.str_value);
        m_val = ent.str_value;
    }
     connect( input, SIGNAL( textChanged( const QString & ) ), this, SLOT( entryChanged(const QString &) ) );
}


void
XineStrEntry::save()
{
    XineStrFunctor func;
    saveXineEntry(func, m_val, m_key, m_xine);
    m_valueChanged = false;
}


void
XineStrEntry::entryChanged(const QString & val)
{
    m_val = val;
    XineGeneralEntry::entryChanged();
}

////////////////////
/// XineIntEntry
////////////////////
XineIntEntry::XineIntEntry(QSpinBox* input, const QByteArray & key, xine_t *xine, XineConfigDialog* xcf)
  : XineGeneralEntry(key,xine,xcf)
{
    xine_cfg_entry_t ent;
    if(xine_config_lookup_entry(m_xine,  m_key.toAscii(), &ent))
        {
            input->setValue(ent.num_value);
            m_val = ent.num_value;
        }
     connect( input,  SIGNAL( valueChanged( int ) ), this, SLOT( entryChanged( int ) ) );
}


XineIntEntry::XineIntEntry(const QString& key, xine_t *xine, XineConfigDialog* xcf)
    : XineGeneralEntry(key,xine,xcf)
{ }


void
XineIntEntry::save()
{
    XineIntFunctor func;
    saveXineEntry(func, m_val, m_key, m_xine);
    m_valueChanged = false;
}


void
XineIntEntry::entryChanged(int val)
{
    m_val = val;
    XineGeneralEntry::entryChanged();
}

////////////////////
/// XineEnumEntry
////////////////////
XineEnumEntry::XineEnumEntry(QComboBox* input, const QByteArray & key, xine_t *xine, XineConfigDialog* xcf)
    : XineIntEntry(key,xine,xcf)
{
    input->clear();
    xine_cfg_entry_t ent;
    if(xine_config_lookup_entry( m_xine, m_key.toAscii(), &ent) )
    {
        for( int i = 0; ent.enum_values[i]; ++i )
        {
            input->insertItem( i, QString::fromLocal8Bit( ent.enum_values[i] ) );
            input->setCurrentIndex( i );
            m_val = ent.num_value;
        }
    }
     connect( input,  SIGNAL( activated( int ) ), this, SLOT( entryChanged( int ) ) );
}

///////////////////////
/// XineConfigDialog
///////////////////////

XineConfigDialog::XineConfigDialog( const xine_t* const xine)
        : PluginConfig()
        , m_xine (const_cast<xine_t*>(xine))
{
    m_view = new XineConfigBase(0);
    m_view->xineLogo->setPixmap( QPixmap( KStandardDirs::locate( "data", "amarok/images/xine_logo.png" ) ) );
    //sound output combo box
    m_view->deviceComboBox->insertItem( 0, i18n("Autodetect") );
    const char* const* drivers = xine_list_audio_output_plugins(m_xine);
    for(int i=0; drivers[i]; ++i)
    {
        if(qstrcmp(drivers[i],"none") != 0) //returns 0 if equal
            m_view->deviceComboBox->insertItem( i+1, drivers[i] ); // i+1 to be after the Autodetect entry
    }

    connect( m_view->deviceComboBox, SIGNAL( activated( int ) ), SIGNAL( viewChanged() ) );
    m_view->deviceComboBox->setCurrentItem( (XineCfg::outputPlugin() == "auto" ) ? "Autodetect" : XineCfg::outputPlugin() );
    init();
    showHidePluginConfigs();
}


XineConfigDialog::~XineConfigDialog()
{
    XineCfg::self()->writeConfig();
    delete m_view;
}


void
XineConfigDialog::init()
{
    #define add(X) m_entries.append(X)
    add(new XineStrEntry(m_view->hostLineEdit, "media.network.http_proxy_host", m_xine, this));
    add(new XineIntEntry(m_view->portIntBox,"media.network.http_proxy_port", m_xine, this));
    add(new XineStrEntry(m_view->userLineEdit, "media.network.http_proxy_user", m_xine, this));
    add(new XineStrEntry(m_view->passLineEdit, "media.network.http_proxy_password", m_xine, this));
    //alsaGroupBox
    add(new XineStrEntry(m_view->monoLineEdit, "audio.device.alsa_default_device", m_xine,this));
    add(new XineStrEntry(m_view->stereoLineEdit, "audio.device.alsa_front_device", m_xine,this));
    add(new XineStrEntry(m_view->chan4LineEdit, "audio.device.alsa_surround40_device", m_xine, this));
    add(new XineStrEntry(m_view->chan5LineEdit, "audio.device.alsa_surround51_device", m_xine, this));
    //ossGroupBox
    add(new XineEnumEntry(m_view->ossDeviceComboBox, "audio.device.oss_device_name", m_xine,this));
    add(new XineEnumEntry(m_view->speakerComboBox, "audio.output.speaker_arrangement", m_xine, this));
    // audiocdGroupBox
    add(new XineStrEntry(m_view->audiocd_device, "media.audio_cd.device", m_xine, this));
    add(new XineStrEntry(m_view->cddb_server, "media.audio_cd.cddb_server", m_xine, this));
    add(new XineIntEntry(m_view->cddb_port, "media.audio_cd.cddb_port", m_xine, this));
    add(new XineStrEntry(m_view->cddb_cache_dir, "media.audio_cd.cddb_cachedir", m_xine, this));
    #undef add
}


void
XineConfigDialog::showHidePluginConfigs() const
{
    if(m_view->deviceComboBox->currentText() == "alsa")
    {
        m_view->alsaGroupBox->show();
        m_view->ossGroupBox->hide();
        if(XineCfg::outputPlugin() == "alsa")
            m_view->alsaGroupBox->setEnabled(true);
        else
            m_view->alsaGroupBox->setEnabled(false);
    }
    else if(m_view->deviceComboBox->currentText() == "oss")
    {
        m_view->alsaGroupBox->hide();
        m_view->ossGroupBox->show();
       if(XineCfg::outputPlugin() == "oss")
            m_view->ossGroupBox->setEnabled(true);
        else
            m_view->ossGroupBox->setEnabled(false);
    }
    else
    {
        m_view->alsaGroupBox->hide();
        m_view->ossGroupBox->hide();
        m_view->alsaGroupBox->setEnabled(false);
        m_view->ossGroupBox->setEnabled(false);
    }
}


bool
XineConfigDialog::hasChanged() const
{
    showHidePluginConfigs();
    if(XineCfg::outputPlugin() != ((m_view->deviceComboBox->currentIndex() == 0) ? "auto" : m_view->deviceComboBox->currentText()))
        return true;

    QListIterator<XineGeneralEntry *> it( m_entries );
    XineGeneralEntry* entry;
    while( it.hasNext() )
    {
        it.next();
        if(entry->hasChanged())
            return true;
    }
    return false;
}


bool
XineConfigDialog::isDefault() const
{
    return false;
}


void
XineConfigDialog::reset(xine_t* xine) //SLOT
{
    debug() << &m_xine << " " << &xine;
    while (!m_entries.isEmpty())
      delete m_entries.takeFirst();
    m_xine = xine;
    debug() << "m_entries now empty " << m_entries.isEmpty();
    init();
}


void
XineConfigDialog::save()//SLOT
{
    if(hasChanged())
    {
        //its not Autodetect really, its just auto
        XineCfg::setOutputPlugin((m_view->deviceComboBox->currentIndex() == 0)
            ? "auto" : m_view->deviceComboBox->currentText());
        QListIterator<XineGeneralEntry*> it(m_entries);
        while(it.hasNext())
        {
            if(it.next()->hasChanged())
                it.next()->save();
        }
        emit settingsSaved();
    }
}

#include "xine-config.moc"
