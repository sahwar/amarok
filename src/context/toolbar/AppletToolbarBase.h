/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef APPLETTOOLBARBASE_H
#define APPLETTOOLBARBASE_H

#include <QGraphicsWidget>

namespace Context
{

class AppletToolbarBase : public QGraphicsWidget
{
public:
    explicit AppletToolbarBase(QGraphicsItem* parent = nullptr, Qt::WindowFlags wFlags = {});
    ~AppletToolbarBase();
    
    void paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr ) override;

};

} // namespace

#endif // APPLETTOOLBARBASE_H
