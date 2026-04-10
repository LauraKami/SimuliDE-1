/***************************************************************************
 *   Copyright (C) 2010 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#ifndef SEVENSEGMENTBCD_H
#define SEVENSEGMENTBCD_H

#include <QColor>

#include "bcdbase.h"

class LibraryItem;

class SevenSegmentBCD : public BcdBase
{
    public:
        SevenSegmentBCD( QString type, QString id );
        ~SevenSegmentBCD();

 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

        virtual void updateStep() override;
        virtual void voltChanged() override;

        bool miniSize() { return m_miniSize; }
        void setOrientation();

        QString segmentColor() { return m_segColorStr; }
        void setSegmentColor( QString color );

        QString backgroundColor() { return m_backColorStr; }
        void setBackgroundColor( QString color );

        virtual bool setLinkedTo( Linker* li ) override;
        virtual void setLinkedValue( double v, int i=0  ) override;

        void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

        virtual void setAngle( double angle ) override;
        virtual void rotateAngle( double a ) override;

    private:
        void checkHorizontalMiniSize();
        bool m_miniSize; 

        QString m_segColorStr;
        QString m_backColorStr;
        QColor  m_segColor;
        QColor  m_backColor;

};

#endif
