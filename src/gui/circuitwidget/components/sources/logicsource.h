/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#ifndef LOGICSOURCE_H
#define LOGICSOURCE_H

#include <QToolButton>
#include <QGraphicsProxyWidget>
#include <QColor>

#include "component.h"
#include "e-element.h"

class IoPin;
class LibraryItem;
class CustomButton;

class LogicSource : public Component, public eElement
{
    public:
        LogicSource( QString type, QString id );
        ~LogicSource();

 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();

        virtual void stamp() override;
        virtual void updateStep() override;

        bool out();
        virtual void setOut( bool out );

        double volt() { return m_voltage; }
        void setVolt( double v );

        QString colors() { return m_colorsStr; }
        void setColors( QString colors );

        virtual QRectF boundingRect() const override;
        virtual QPainterPath shape() const override;
        virtual void paint( QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget ) override;

    public slots:
        virtual void onbuttonclicked();

    protected:
        void updateOutput();

        double m_voltage;
        bool   m_outState;

        QString m_colorsStr;
        QColor  m_colorHigh;
        QColor  m_colorLow;

        IoPin* m_outpin;

        CustomButton* m_button;
        QGraphicsProxyWidget* m_proxy;
};

#endif
