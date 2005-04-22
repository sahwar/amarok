
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _HELIX_CONFIG_H_
#define _HELIX_CONFIG_H_

#include "plugin/pluginconfig.h"
#include <qptrlist.h>
#include <qtabwidget.h>
#include <qvbox.h>

class QGridLayout;
class KLineEdit;
class HelixEngine;

// since many preferences can be set in Helix, I'm planning on more config items later
// for now I'll just get the location of the Helix core/plugins for initializing
// the Helix core
class HelixConfigEntry : public QObject
{
Q_OBJECT
public:
    HelixConfigEntry( QWidget *parent, amaroK::PluginConfig*, 
                      int row, const char *description, const char *defaultvalue, const char *tooltip );
    HelixConfigEntry( QWidget *parent, QCString &str, amaroK::PluginConfig*, 
                      int row, const char *description, const char *defaultvalue, const char *tooltip );

    bool isChanged() const { return m_valueChanged; }
    void setUnchanged() { m_valueChanged = false; }
    const QCString& key() const { return m_key; }
    const QCString& stringValue() const { return m_stringValue; }
    int numValue() const { return m_numValue; }

private slots:
    void slotStringChanged( const QString& );

private:
   bool     m_valueChanged;
   int      m_numValue;
   QCString m_key;
   QCString m_stringValue;
   QCString &m_str;
};


class HelixConfigDialog : public amaroK::PluginConfig, public QTabWidget
{
public:
    HelixConfigDialog( HelixEngine *engine, QWidget *parent = 0 );

    virtual QWidget *view() { return this; }
    virtual bool hasChanged() const;
    virtual bool isDefault() const;

    /** Save view state into configuration */
    virtual void save();

private:
    QPtrList<HelixConfigEntry> entries;
    HelixEngine *m_engine;
};

#endif
