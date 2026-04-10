/***************************************************************************
 *   Copyright (C) 2010 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <QPainter>
#include <math.h>

#include "sevensegment_bcd.h"
#include "itemlibrary.h"
#include "simulator.h"
#include "iopin.h"
#include "ledbase.h"

#include "stringprop.h"
#include "boolprop.h"

#define tr(str) simulideTr("SevenSegmentBCD",str)

Component* SevenSegmentBCD::construct( QString type, QString id )
{ return new SevenSegmentBCD( type, id ); }

LibraryItem* SevenSegmentBCD::libraryItem()
{
    return new LibraryItem(
        tr("7 Seg BCD"),
        "Other Logic",
        "7segbcd.png",
        "7-Seg BCD", /// FIXME: only alphanumeric in Component type
        SevenSegmentBCD::construct );
}

SevenSegmentBCD::SevenSegmentBCD( QString type, QString id )
               : BcdBase( type, id )
{
    m_graphical = true;

    m_width  = 4;
    m_height = 4;
    m_color  = Qt::black;

    QStringList pinList;

    pinList // Inputs:
            << "ID041"
            << "ID032"
            << "ID024"
            << "ID018"
            ;
    init( pinList );
    for( uint i=0; i<m_inPin.size(); ++i )
    {
        if( !m_inPin[i] ) continue;
        m_inPin[i]->setX( m_inPin[i]->x()-4);
        m_inPin[i]->setSpace( 1 );
        m_inPin[i]->setFontSize( 4 );
        m_inPin[i]->setLabelColor( QColor( 250, 250, 200 ) );
    }


    m_miniSize = true; 
    m_segColorStr  = "Yellow";
    m_backColorStr = "Black";
    m_segColor  = QColor( m_segColorStr );
    m_backColor = QColor( m_backColorStr );

    setLabelPos(-16,-40, 0);

    addPropGroup( { tr("Main"), {
        new StrProp<SevenSegmentBCD>("Segment_Color", tr("Segment Color"),"", this
                              , &SevenSegmentBCD::segmentColor, &SevenSegmentBCD::setSegmentColor ),

        new StrProp<SevenSegmentBCD>("Background_Color", tr("Background Color"),"", this
                              , &SevenSegmentBCD::backgroundColor, &SevenSegmentBCD::setBackgroundColor )
    },groupNoCopy} );

    Simulator::self()->addToUpdateList( this );

    m_digit = 0;
}
SevenSegmentBCD::~SevenSegmentBCD(){}

void SevenSegmentBCD::updateStep()
{
    if( !m_changed ) return;
    m_changed = false;

    m_digit = 0;
    if( !Simulator::self()->isRunning() ) m_digit = 0;
    else if( !m_linkedTo ){
        BcdBase::voltChanged();
    }
    update();
}

void SevenSegmentBCD::voltChanged() { m_changed = true; }

void SevenSegmentBCD::setOrientation()
{
    // Force mini size for horizontal orientation (±90°)
    int rot = (int)rotation() % 360;
    if( rot < 0 ) rot += 360;

    m_height = 4;
    m_area = QRect(-16, -16, 32, 32 );

    // Reposition pins Y position only (width stays the same)
    int yDown = m_area.y() + m_height*8 + 8;
    int yUp   = m_area.y() - 8;

    for( uint i=0; i<4 && i<m_inPin.size(); ++i )  // Down pins (0-3): 1, 2, 4, 8
    {
        if( !m_inPin[i] ) continue;
        m_inPin[i]->setY( yDown );
        m_inPin[i]->isMoved();
    }
    update();
}

void SevenSegmentBCD::setAngle( double angle )
{
    Component::setAngle( angle );
    setOrientation();
}

void SevenSegmentBCD::rotateAngle( double a )
{
    Component::rotateAngle( a );
    setOrientation();
}


void SevenSegmentBCD::setSegmentColor( QString color )
{
    QColor c( color );
    if( !c.isValid() ) return;

    m_segColorStr = color;
    m_segColor = c;
    update();
}

void SevenSegmentBCD::setBackgroundColor( QString color )
{
    QColor c( color );
    if( !c.isValid() ) return;

    m_backColorStr = color;
    m_backColor = c;
    m_color = c;  // Update component background
    update();
}

bool SevenSegmentBCD::setLinkedTo( Linker* li )
{
    bool linked = Component::setLinkedTo( li );
    if( li && linked )
        for( uint i=0; i<m_inPin.size(); ++i )
            if( m_inPin[i] ) m_inPin[i]->removeConnector();

    setHidden( (li && linked), false, false );

    return linked;
}

void SevenSegmentBCD::setLinkedValue( double v, int i )
{
    int vInt = v;
    vInt &= 0xFF;
    if( i == 0 ) m_digit = m_values[vInt]; // Display value
    else         m_digit = vInt;           // 1 bit for each segment
    m_changed = true;
}

void SevenSegmentBCD::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );

    // Draw background
    p->setBrush( m_backColor );
    p->drawRect( m_area );

    //----------------- Segments -----------------
    // Apply counter-transformation to keep segments always readable
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

    // Offset segments offset pixel away from pins for aesthetics
    int offset = 2; // 
    int offsetX = 0, offsetY = 0;
    if( rot == 0 )        offsetY = -1*offset;  // pins at bottom, shift up
    else if( rot == 90 )  offsetX =  1*offset;  // pins on left, shift right
    else if( rot == 180 ) offsetY =  1*offset;  // pins at top, shift down
    else if( rot == 270 ) offsetX = -1*offset;  // pins on right, shift left
    if( h == -1 ) { offsetX = -offsetX; } // hflip
    if( v == -1 ) { offsetY = -offsetY; } // vflip

    // Dimensions: adapt to mini or standard size
    const int mgX = 8;  // Horizontal margin
    const int mgY = 4;  // Vertical margin (same for both)
    const int ds  = 0;   // "Slope"
    const int tk  = 4;   // Line thickness
    const int tko = 3;  // tk-1
    const int x1  =  m_area.x()+mgX + offsetX;
    const int x2  = -m_area.x()-mgX + offsetX;
    const int y1  =  m_area.y()+mgY + offsetY;
    const int y2  = -m_area.y()-mgY + offsetY;

    QPen pen;
    pen.setWidth( tk );
    pen.setColor( m_segColor );
    pen.setCapStyle( Qt::RoundCap );
    p->setPen( pen );

    if( m_digit & 1<<0 ) p->drawLine( x1+tko+ds, y1,    x2-tko+ds, y1    );
    if( m_digit & 1<<1 ) p->drawLine( x2+ds,    y1+tko, x2,      offsetY-tko    );
    if( m_digit & 1<<2 ) p->drawLine( x2,       offsetY+tko,    x2-ds,    y2-tko );
    if( m_digit & 1<<3 ) p->drawLine( x1+tko+ds, y2,    x2-tko+ds, y2    );
    if( m_digit & 1<<4 ) p->drawLine( x1-ds,    y2-tko, x1,       offsetY+tko    );
    if( m_digit & 1<<5 ) p->drawLine( x1,      offsetY-tko,    x1+ds,    y1+tko );
    if( m_digit & 1<<6 ) p->drawLine( x1+tko+ds,    offsetY,     x2-tko+ds,    offsetY     );

    // end of segment space rotation and flip
    p->restore();

    //------------- Indicators ----------------
    // Draw bit state indicators (red=1, green=0) for BCD pins
    const int indicatorR = 3;       // Radius
    // Pin positions (after -4 offset): pin0=12, pin1=4, pin2=-4, pin3=-12
    const int pinPos[] = { 12, 4, -4, -12 };  // X positions for pins 1, 2, 4, 8

    p->setPen( Qt::NoPen );

    if( true )
    {
        const int x1  =  m_area.x();
        const int x2  = -m_area.x();
        const int y1  =  m_area.y();
        const int y2  = -m_area.y();

        // Horizontal orientation: pins on side
        // rot=90: pins on left in screen space, rot=270: pins on right
        int indicatorX = (rot == 90) ? x1 - 3 : x2 + 3;

        int offsetY = -1; // try put round on label

        for( size_t i = 0; i < 4 && i < m_inPin.size(); ++i )
        {
            if( !m_inPin[i] ) continue;
            bool state = m_inPin[i]->getInpState();
            p->setBrush( state ? QColor(255, 0, 0) : QColor(0, 200, 0) );
            // Y maps to screen X after rotation
            //int indicatorY = (rot == 90) ? -pinPos[i] : pinPos[i];
            int indicatorY = y2 + offsetY;
            indicatorX =  pinPos[i];
            p->drawEllipse( QPointF(indicatorX, indicatorY), indicatorR, indicatorR );
        }
    }



    Component::paintSelected( p );
}
