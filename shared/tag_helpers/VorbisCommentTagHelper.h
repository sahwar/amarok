/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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


#ifndef VORBISCOMMENTTAGHELPER_H
#define VORBISCOMMENTTAGHELPER_H

#include "TagHelper.h"

#include <taglib/xiphcomment.h>
#include <taglib/flacfile.h>

class QImage;

namespace Meta
{
    namespace Tag
    {
        class AMAROK_EXPORT VorbisCommentTagHelper : public TagHelper
        {
            public:
                VorbisCommentTagHelper( TagLib::Tag *tag, TagLib::Ogg::XiphComment *commentsTag, Amarok::FileType fileType );
                VorbisCommentTagHelper( TagLib::Tag *tag, TagLib::Ogg::XiphComment *commentsTag, TagLib::FLAC::File *file, Amarok::FileType fileType );

                virtual Meta::FieldHash tags() const;
                virtual bool setTags( const Meta::FieldHash &changes );

                virtual TagLib::ByteVector render() const;

#ifndef UTILITIES_BUILD
                virtual bool hasEmbeddedCover() const;
                virtual QImage embeddedCover() const;
                virtual bool setEmbeddedCover( const QImage &cover );
#endif  //UTILITIES_BUILD

            private:
                static bool parsePictureBlock( const TagLib::StringList& block, QImage* result = 0 );
                TagLib::Ogg::XiphComment *m_tag;
                TagLib::FLAC::File *m_flacFile;
        };
    }
}

#endif // VORBISCOMMENTTAGHELPER_H
