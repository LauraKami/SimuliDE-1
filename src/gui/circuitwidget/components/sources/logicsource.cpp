/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>

#include "logicsource.h"
#include "circuit.h"
#include "itemlibrary.h"
#include "simulator.h"
#include "iopin.h"
#include "custombutton.h"

#include "doubleprop.h"
#include "stringprop.h"
#include "boolprop.h"

#define tr(str) simulideTr("LogicSource",str)

Component* LogicSource::construct( QString type, QString id )
{ return new LogicSource( type, id ); }

LibraryItem* LogicSource::libraryItem()
{
    return new LibraryItem(
        tr("Logic Source"),
        "Sources",
        "logicsource.png",
        "LogicSource",
        LogicSource::construct );
}

LogicSource::LogicSource( QString type, QString id )
          : Component( type, id )
          , eElement( id )
{
    m_area = QRect( -4, -4, 8, 8 );

    m_graphical = true;
    m_changed   = false;
    m_outState  = false;

    // Default colors: High=Red, Low=Green
    m_colorsStr = "Red,Green";
    m_colorHigh = QColor("Red");
    m_colorLow  = QColor("Green");

    m_pin.resize(1);
    m_pin[0] = m_outpin = new IoPin( 0, QPoint(12, 0), id+"-outnod", 0, this, source );
    m_outpin->setLength( 8 );

    setLabelPos( 12, -12, 0 );

    m_button = new CustomButton();
    m_button->setMaximumSize( 10, 10 );
    m_button->setGeometry(-14, -10, 10, 10);
    m_button->setCheckable( true );

    m_proxy = Circuit::self()->addWidget( m_button );
    m_proxy->setParentItem( this );
    m_proxy->setPos( QPoint(-14, -5) );

    setVolt( 5.0 );

    Simulator::self()->addToUpdateList( this );

    QObject::connect( m_button, &CustomButton::clicked, [=](){ onbuttonclicked(); } );

    addPropGroup( { tr("Main"), {
        new DoubProp<LogicSource>("Voltage", tr("Voltage"), "V"
                               , this, &LogicSource::volt, &LogicSource::setVolt ),

        new StrProp<LogicSource>("Colors", tr("Colors"), ""
                               , this, &LogicSource::colors, &LogicSource::setColors )
    }, 0} );

    addPropGroup( { "Hidden", {
new BoolProp<LogicSource>( "Out", "","", this, &LogicSource::out, &LogicSource::setOut ),
    }, groupHidden} );
}
LogicSource::~LogicSource(){}

void LogicSource::stamp()
{
    if( !Simulator::self()->isPaused() ) m_changed = true;
}

void LogicSource::updateStep()
{
    if( !m_changed ) return;
    m_changed = false;
    m_outState = m_button->isChecked();
    m_outpin->setOutState( m_outState );
    update();
}

void LogicSource::onbuttonclicked()
{
    if( Simulator::self()->isRunning() ) m_changed = true;
    else{
        m_outState = m_button->isChecked();
        m_outpin->setOutState( m_outState );
        update();
    }
}

void LogicSource::updateOutput()
{
    m_outpin->setOutHighV( m_voltage );
    m_changed = true;
}

bool LogicSource::out() { return m_button->isChecked(); }

void LogicSource::setOut( bool out )
{
    m_button->setChecked( out );
    onbuttonclicked();
}

void LogicSource::setVolt( double v )
{
    m_voltage = v;
    m_outpin->setOutHighV( m_voltage );
    m_changed = true;
}

void LogicSource::setColors( QString colors )
{
    QStringList list = colors.split(",");
    if( list.size() < 2 ) return;

    QColor high( list[0].trimmed() );
    QColor low ( list[1].trimmed() );

    if( !high.isValid() || !low.isValid() ) return;

    m_colorsStr  = colors;
    m_colorHigh  = high;
    m_colorLow   = low;
    update();
}

QRectF LogicSource::boundingRect() const
{
    // 26 ->14
    return QRectF( -14, -5, 20, 10 );  // Include button and pin
}

QPainterPath LogicSource::shape() const
{
    QPainterPath path;
    path.addRect( QRectF( -14, -5, 20, 10 ) );
    return path;
}

void LogicSource::paint( QPainter* p, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    if( m_hidden ) return;

    Component::paint( p, option, widget );

    QColor bgColor;
    QString text;

    if( m_outState ) {
        bgColor = m_colorHigh;
        text = "1";
    } else {
        bgColor = m_colorLow;
        text = "0";
    }

    p->setBrush( bgColor );
    p->drawRect( QRect(-4, -4, 8, 8) );

    // Draw text centered in the square, always upright and readable
    p->save();

    int rot = (int)rotation() % 360;
    if( rot < 0 ) rot += 360;

    int h = hflip();
    int v = vflip();

    // When rotation is ±90°, swap the flip axes
    if( rot == 90 || rot == 270 )
    {
        int tmp = h;
        h = v;
        v = tmp;
    }

    QTransform transform;
    transform.scale( h, v );          // Counter the flip
    transform.rotate( -rotation() );  // Counter the rotation
    p->setTransform( transform, true );

    p->setPen( Qt::black );
    QFont font = p->font();
    font.setPixelSize( 7 );
    font.setBold( true );
    p->setFont( font );
    p->drawText( QRect(-4, -4, 8, 8), Qt::AlignCenter, text );
    p->restore();

    Component::paintSelected( p );
}
