/***************************************************************************
 *   Copyright (C) 2012 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include "sevensegment.h"
#include "circuitwidget.h"
#include "itemlibrary.h"
#include "connector.h"
#include "simulator.h"
#include "circuit.h"
#include "pin.h"

#include "doubleprop.h"
#include "stringprop.h"
#include "boolprop.h"
#include "intprop.h"

#define tr(str) simulideTr("SevenSegment",str)

Component* SevenSegment::construct( QString type, QString id )
{ return new SevenSegment( type, id ); }

LibraryItem* SevenSegment::libraryItem()
{
    return new LibraryItem(
        tr("7 Segment"),
        "Leds",
        "seven_segment.png",
        "Seven Segment",
        SevenSegment::construct );
}

SevenSegment::SevenSegment( QString type, QString id )
            : Component( type, id )
            , eElement( id )
{
    m_graphical = true;

    setLabelPos( 20,-44, 0 );

    m_color = QColor(0,0,0);
    m_commonCathode = true;
    m_verticalPins  = false;
    m_miniSize      = false;
    m_numDisplays = 0;
    m_threshold  = 2.4;
    m_maxCurrent = 0.02;
    m_resistance = 1;

    m_ePin.resize(8);
    m_pin.resize(8);

    QString pinid;
    for( int i=0; i<7; ++i ) // Create Pins for 7 segments
    {
        pinid = m_id+"-pin_"+QString( 97+i ); // a..g
        m_ePin[i] = m_pin[i] = new Pin( 180, QPoint(-16-8,-24+i*8 ), pinid, 0, this );
        m_pin[i]->setSpace( 1 );
        m_pin[i]->setLength( 6 );
        m_pin[i]->setFontSize( 4 );
        m_pin[i]->setLabelText( QChar('a'+i) );
    }
    // Pin dot
    m_ePin[7] = m_pin[7] = new Pin( 270, QPoint( -8, 24+8 ), m_id+"-pin_dot", 0, this );
    m_pin[7]->setSpace( 1 );
    m_pin[7]->setLength( 4 );
    m_pin[7]->setFontSize( 4 );
    m_pin[7]->setLabelText( "." );

    setNumDisplays( 1 );

    Simulator::self()->addToUpdateList( this );

    addPropGroup( { tr("Main"), {
        new StrProp <SevenSegment>("Color", tr("Color"), LedBase::getColorList()
                                  , this, &SevenSegment::colorStr, &SevenSegment::setColorStr,0,"enum" ),

        new IntProp <SevenSegment>("NumDisplays"  , tr("Size"), ""
                                  , this, &SevenSegment::numDisplays, &SevenSegment::setNumDisplays, propNoCopy ),

        new BoolProp<SevenSegment>("Vertical_Pins", tr("Vertical Pins"), ""
                                  , this, &SevenSegment::verticalPins, &SevenSegment::setVerticalPins, propNoCopy ),

        new BoolProp<SevenSegment>("Mini_Size", tr("Mini Size"), ""
                                  , this, &SevenSegment::miniSize, &SevenSegment::setMiniSize, propNoCopy )
            }, 0} );

    addPropGroup( { tr("Electric"), {
        new BoolProp<SevenSegment>("CommonCathode", tr("Common Cathode"), ""
                                  , this, &SevenSegment::isComCathode, &SevenSegment::setComCathode, propNoCopy ),

        new DoubProp<SevenSegment>("Threshold" , tr("Forward Voltage"),"V"
                                  , this, &SevenSegment::threshold, &SevenSegment::setThreshold ),

        new DoubProp<SevenSegment>("MaxCurrent", tr("Max Current"),"mA"
                                  , this, &SevenSegment::maxCurrent, &SevenSegment::setMaxCurrent ),

        new DoubProp<SevenSegment>("Resistance", tr("Resistance"),"Ω"
                                  , this, &SevenSegment::resistance, &SevenSegment::setResistance ),
    },0} );
}
SevenSegment::~SevenSegment() { }

void SevenSegment::stamp()
{
    for( int i=0; i<8; ++i )  // Get eNode of pin i
        m_enode[i] = (i < (int)m_ePin.size() && m_ePin[i]) ? m_ePin[i]->getEnode() : nullptr;

    for( int i=0; i<m_numDisplays; ++i )
    {
        if( i >= (int)m_commonPin.size() || !m_commonPin[i] ) continue;
        eNode* commonEnode = m_commonPin[i]->getEnode();     // Get eNode of common

        int pin;
        for( int j=0; j<8; ++j )
        {
            pin = i*8+j;
            if( pin >= (int)m_cathodePin.size() || pin >= (int)m_anodePin.size() ) continue;
            if( !m_cathodePin[pin] || !m_anodePin[pin] ) continue;

            if( m_commonCathode )
            {
                m_cathodePin[pin]->setEnode( commonEnode );
                m_anodePin[pin]->setEnode( m_enode[j] );
            }else{
                m_anodePin[pin]->setEnode( commonEnode );
                m_cathodePin[pin]->setEnode( m_enode[j] );
}   }   }   }

void SevenSegment::updateStep()
{
    // Adjust Z-order: lit segments on top, unlit segments below
    for( LedSmd* segment : m_segment )
    {
        if( segment ) segment->setZValue( segment->brightness() > 0 ? 1 : 0 );
    }
}

QString SevenSegment::colorStr()
{
    if( m_segment[0] ) return m_segment[0]->colorStr();
    else               return "Yellow";
}

void SevenSegment::setColorStr( QString color )
{
    for( LedSmd* segment : m_segment )
        if( segment ) segment->setColorStr( color );
    if( m_showVal && (m_showProperty == "Color") )
        setValLabelText( color );
}

void SevenSegment::setNumDisplays( int displays )
{
    if( displays < 1 ) displays = 1;
    if( displays == m_numDisplays ) return;

    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();

    if( m_miniSize ) m_area = QRect(-18, -16, 32*displays+4, 32 );
    else             m_area = QRect(-18, -24-4, 32*displays+4, 48+8 );

    if( displays > m_numDisplays )
    {
        resizeData( displays );
        for( int i=m_numDisplays; i<displays; ++i ) createDisplay( i );
    }else{
        for( int i=displays; i<m_numDisplays; ++i ) deleteDisplay( i );
        resizeData( displays );
    }
    m_numDisplays = displays;
    setResistance( m_resistance );
    setThreshold( m_threshold );
    setMaxCurrent( m_maxCurrent );

    Circuit::self()->update();
}

void SevenSegment::resizeData( int displays )
{
    m_pin.resize( 8+displays );
    m_commonPin.resize( displays );
    m_cathodePin.resize( displays*8 );
    m_anodePin.resize( displays*8 );
    m_segment.resize( displays*8 );
}

void SevenSegment::setComCathode( bool isCommonCathode )
{
    if( Simulator::self()->isRunning() )  CircuitWidget::self()->powerCircOff();
    m_commonCathode = isCommonCathode;
    QString label = m_commonCathode ? "|" : "+";
    for( int i=0; i<m_numDisplays && i<(int)m_commonPin.size(); ++i )
        if( m_commonPin[i] ) m_commonPin[i]->setLabelText( label );
}

void SevenSegment::setVerticalPins( bool v )
{
    if( m_miniSize && !v ) return; // Mini size requires vertical pins
    if( v == m_verticalPins ) return;
    if( m_pin.size() < 8 ) return;  // Need at least 8 pins
    m_verticalPins = v;

    int topY    = m_miniSize ? -12-8 : -24-8;
    int bottomY = m_miniSize ?  12+8 :  24+8;

    if( v ) {
        for( int i=0; i<5; ++i ){
            if( !m_pin[i] ) continue;
            m_pin[i]->setPos(-16+8*i, topY );
            m_pin[i]->setLength( 4 );
            m_pin[i]->setPinAngle( 90 );
            m_pin[i]->isMoved();
        }
        for( int i=5; i<8; ++i ){
            if( !m_pin[i] ) continue;
            m_pin[i]->setPos(-16+8*(i-5), bottomY );
            m_pin[i]->setLength( 4 );
            m_pin[i]->setPinAngle( 270 );
            m_pin[i]->isMoved();
        }
    }else{
        for( int i=0; i<7; ++i ){
            if( !m_pin[i] ) continue;
            m_pin[i]->setPos(-16-8,-24+i*8 );
            m_pin[i]->setLength( 6 );
            m_pin[i]->setPinAngle( 180 );
            m_pin[i]->isMoved();
        }
        if( m_pin[7] )
        {
            m_pin[7]->setPos(-8, 24+8 );
            m_pin[7]->setRotation( 270 );
            m_pin[7]->isMoved();
        }
    }
    if( m_miniSize ) m_area = QRect(-18, -16, 32*m_numDisplays+4, 32 );
    else             m_area = QRect(-18, -24-4, 32*m_numDisplays+4, 48+8 );

    Circuit::self()->update();
}

void SevenSegment::setMiniSize( bool mini )
{
    if( mini == m_miniSize ) return;

    if( Simulator::self()->isRunning() ) CircuitWidget::self()->powerCircOff();

    m_miniSize = mini;

    // Reposition signal pins for new size
    if( mini && !m_verticalPins ) {
        m_verticalPins = false; // Force setVerticalPins to reposition pins
        setVerticalPins( true );
    }
    else if( m_verticalPins && m_pin.size() >= 8 ) {
        int topY    = mini ? -12-8 : -24-8;
        int bottomY = mini ?  12+8 :  24+8;
        for( int i=0; i<5; ++i ){
            if( !m_pin[i] ) continue;
            m_pin[i]->setPos(-16+8*i, topY );
            m_pin[i]->isMoved();
        }
        for( int i=5; i<8; ++i ){
            if( !m_pin[i] ) continue;
            m_pin[i]->setPos(-16+8*(i-5), bottomY );
            m_pin[i]->isMoved();
        }
    }

    // Update area
    if( mini ) m_area = QRect(-18, -16, 32*m_numDisplays+4, 32 );
    else       m_area = QRect(-18, -24-4, 32*m_numDisplays+4, 48+8 );

    // Recreate all displays with new dimensions
    int numDisp = m_numDisplays;
    for( int i=numDisp-1; i>=0; --i ) deleteDisplay( i );
    resizeData( 0 );
    m_numDisplays = 0;

    for( int i=0; i<numDisp; ++i )
    {
        resizeData( i+1 );
        createDisplay( i );
    }
    m_numDisplays = numDisp;

    setResistance( m_resistance );
    setThreshold( m_threshold );
    setMaxCurrent( m_maxCurrent );

    Circuit::self()->update();
}

void SevenSegment::setResistance( double res )
{
    if( res < 1e-6 ) res = 1;
    m_resistance = res;
    for( uint i=0; i<m_segment.size(); ++i )
        if( m_segment[i] ) m_segment[i]->setRes( res );
}

void SevenSegment::setThreshold( double threshold )
{
    if( threshold < 1e-6 ) threshold = 2.4;
    m_threshold = threshold;
    for( uint i=0; i<m_segment.size(); ++i )
        if( m_segment[i] ) m_segment[i]->setThreshold( threshold );
}

void SevenSegment::setMaxCurrent( double current )
{
    if( current < 1e-6 ) current = 0.02;
    m_maxCurrent = current;
    for( uint i=0; i<m_segment.size(); ++i )
        if( m_segment[i] ) m_segment[i]->setMaxCurrent( current );
}

void SevenSegment::deleteDisplay(int n )
{
    if( n < (int)m_commonPin.size() && m_commonPin[n] )
        deletePin( m_commonPin[n] );
    for( int i=0; i<8; ++i )
    {
        int idx = n*8+i;
        if( idx < (int)m_segment.size() && m_segment[idx] )
        {
            delete m_segment[idx];
            m_segment[idx] = nullptr;
        }
    }
}

void SevenSegment::createDisplay(int n )
{
    // Bounds check
    if( n < 0 || n >= (int)m_commonPin.size() || (8+n) >= (int)m_pin.size() ) return;
    if( (n*8+7) >= (int)m_segment.size() ) return;

    int x = 32*n;

    // Pin common
    QString label = m_commonCathode ? "|" : "+";
    QString pinid = m_id+"-pin_common"+QString( 97+n );
    int commonY = m_miniSize ? 12+8 : 24+8;
    m_commonPin[n] = m_pin[8+n]= new Pin( 270, QPoint( x+8, commonY ), pinid, 0, this );
    m_commonPin[n]->setLength( 4 );
    m_commonPin[n]->setSpace( 1 );
    m_commonPin[n]->setFontSize( 4 );
    m_commonPin[n]->setLabelText( label );

    // Dimensions: standard vs mini
    double segW = m_miniSize ? 7.0  : 13.5;  // Segment width
    double segH = m_miniSize ? 0.7  : 1.5;   // Segment height
    double dotS = m_miniSize ? 0.7  : 1.5;   // Dot size

    for( int i=0; i<8; ++i ) // Create segments
    {
        pinid = m_id+"-led_"+QString( 97+i );
        LedSmd* lsmd;
        if( i<7 ) lsmd = new LedSmd( "LEDSMD", pinid, QRectF(0, 0, segW, segH) ); // Segment
        else      lsmd = new LedSmd( "LEDSMD", pinid, QRectF(0, 0, dotS, dotS) ); // Point

        lsmd->setParentItem(this);
        lsmd->setFlag( QGraphicsItem::ItemIsSelectable, false );
        lsmd->setAcceptedMouseButtons( 0 );
        lsmd->setRes( m_resistance );
        lsmd->setMaxCurrent( m_maxCurrent );
        lsmd->setThreshold( m_threshold );
        lsmd->setColorStr( colorStr() );

        m_anodePin  [n*8+i] = lsmd->getEpin(0);
        m_cathodePin[n*8+i] = lsmd->getEpin(1);
        m_segment   [n*8+i] = lsmd;
    }
    if( m_miniSize )
    {
        // Mini size: 32x32 pixels layout (visible area -12 to +12)
        m_segment[n*8+0]->setPos( x-2, -10 );                   // a: top
        m_segment[n*8+1]->setPos( x+6.5, -8 );                  // b: top-right
        m_segment[n*8+1]->setRotation(96);
        m_segment[n*8+2]->setPos( x+5.5, 1 );                   // c: bottom-right
        m_segment[n*8+2]->setRotation(96);
        m_segment[n*8+3]->setPos( x-4, 10 );                    // d: bottom
        m_segment[n*8+4]->setPos( x-5, 1 );                     // e: bottom-left
        m_segment[n*8+4]->setRotation(96);
        m_segment[n*8+5]->setPos( x-4, -8 );                    // f: top-left
        m_segment[n*8+5]->setRotation(96);
        m_segment[n*8+6]->setPos( x-3, 0 );                     // g: middle
        m_segment[n*8+7]->setPos( x+7, 10 );                    // dp: dot
    }
    else
    {
        // Standard size: 32x48 pixels layout
        m_segment[n*8+0]->setPos( x-5, -20 );
        m_segment[n*8+1]->setPos( x+11.5, -16 );
        m_segment[n*8+1]->setRotation(96);
        m_segment[n*8+2]->setPos( x+10, 3 );
        m_segment[n*8+2]->setRotation(96);
        m_segment[n*8+3]->setPos( x-8, 19 );
        m_segment[n*8+4]->setPos( x-9, 3 );
        m_segment[n*8+4]->setRotation(96);
        m_segment[n*8+5]->setPos( x-7.5, -16 );
        m_segment[n*8+5]->setRotation(96);
        m_segment[n*8+6]->setPos( x-6.5, 0 );
        m_segment[n*8+7]->setPos( x+12, 19 );
    }
}

void SevenSegment::setHidden( bool hid, bool hidArea, bool hidLabel )
{
    Component::setHidden( hid, hidArea, hidLabel );
    if( m_miniSize )
    {
        if( hid ) m_area = QRect(-16, -12, 32*m_numDisplays, 24 );
        else      m_area = QRect(-18, -16, 32*m_numDisplays+4, 32 );
    }else{
        if( hid ) m_area = QRect(-16, -24, 32*m_numDisplays, 48 );
        else      m_area = QRect(-18, -24-4, 32*m_numDisplays+4, 48+8 );
    }
}

void SevenSegment::paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w )
{
    Component::paint( p, o, w );
    p->drawRect( m_area );

    Component::paintSelected( p );
}
