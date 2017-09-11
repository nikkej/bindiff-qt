//*****************************************************************************
//
//     binfileview.cpp
//     Copyright(c) 2016 Juha T Nikkanen <nikkej@gmail.com>
//
// --- Legal stuff ---
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//*****************************************************************************

#include "binfileview.h"

#include <QDebug>
#include <QObject>
#include <QPainter>
#include <QPaintEvent>
#include <QScrollBar>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QMenu>
#include <QAction>
#include <ctype.h>
#include <climits>

BinFileView::BinFileView( QWidget* parent )
    : QAbstractScrollArea( parent ),
      _contextMenu( new QMenu( this ) ),
      _contextAction( new QAction( tr( "&Open" ), this ) ),
      _data( NULL ),
      _colorData( NULL ),
      _size( 0 ),
      _upperMask( 0xffff0000LL ),
      _lowerMask( 0x0000ffffLL ),
      _addend( 0 ),
      _addressChars( 8 ),
      _lineCount( 0 ),
      _linesOnViewPort( 0 ),
      _byteGroups( 2 ),
      _bytesPerLine( _byteGroups * BinFileView::bytesPerGroup ),
      _leftMargin( 0 ),
      _rightMargin( 0 ),
      _bottomMargin( 0 ),
      _byteWidth( 0 ),
      _groupWidth( 0 ),
      _addressAreaWidth( 0 ),
      _hexAreaWidth( 0 ),
      _asciiAreaWidth( 0 ),
      _groupGap( 0 )
{
    _contextMenu->addAction( _contextAction );
    connect( _contextAction, &QAction::triggered, [=](){ emit fileOpenRequested( this ); } );
    connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ), \
             this, SLOT( showContextMenu( const QPoint& ) ) );
}

BinFileView::~BinFileView()
{
}

void BinFileView::setFont( QFont font )
{
    if( !font.fixedPitch() ) {
        qWarning() << "Not a fixed pitch font!";
        font.setFixedPitch( true );
    }

    QWidget::setFont( font );

    adjust();
    viewport()->update();
}

const uchar* BinFileView::data()
{
    return _data;
}

void BinFileView::setData( const uchar* data, const qint64 size )
{
    _data = data;
    _size = size;

    // Give others, possibly connected views a change to adjust
    // parameters affecting view port width & data layout...
    int oldAddressChars = _addressChars;

    if( _size > INT_MAX )
        setAddressCharacters( 16 );
    else
        setAddressCharacters( 8 );

    // ...and signal those views
    if( oldAddressChars < _addressChars )
        emit defaultVisualsChanged( this );

    _lineCount = (int)( _size / _bytesPerLine );
    if( _size % _bytesPerLine )
        _lineCount += 1;

    _linesOnViewPort = viewport()->height() / fontMetrics().height();
    verticalScrollBar()->setRange( 0, _lineCount - _linesOnViewPort );
    verticalScrollBar()->setPageStep( _linesOnViewPort );

    viewport()->update();
}

void BinFileView::setColoringData( uchar* data )
{
    _colorData = data;
    viewport()->update();
}

void BinFileView::setAddressCharacters( const int chars )
{
    _addressChars = chars;
    if( _addressChars & 1 )
        _addressChars++;

    // Calculate upper and lower half masks
    _lowerMask = 0xf;
    for( int t( 1 ); t < _addressChars / 2; t++ ) {
        _lowerMask |= (qint64)0xf << (4 * t);
    }
    _upperMask = _lowerMask << (4 * _addressChars / 2);

    adjust();
}

qint64 BinFileView::addressAddend()
{
    return (qint64)verticalScrollBar()->sliderPosition() * _bytesPerLine;
}

void BinFileView::showContextMenu( const QPoint& pos )
{
    _contextMenu->exec( mapToGlobal( pos ) );
}

void BinFileView::synchronizeVisuals( BinFileView* view )
{
    setAddressCharacters( view->addressCharacters() );

    calculateNumOfByteGroups();

    horizontalScrollBar()->setRange( 0, preFitWholeLine( _byteGroups ) - viewport()->width() );
    horizontalScrollBar()->setPageStep( viewport()->width() );

    viewport()->update();
}

void BinFileView::dragEnterEvent( QDragEnterEvent* event )
{
    if( event->mimeData()->hasUrls() ) {
        event->acceptProposedAction();
    }
}

void BinFileView::dropEvent( QDropEvent* event )
{
    QList<QUrl> urls = event->mimeData()->urls();
    if( urls.count() > 0 ) {
        emit fileDropped( urls.at( 0 ).toLocalFile(), this );
    }
    // In view instance, we can ignore the event data because we
    // have passed it to controller via signal
    event->setDropAction( Qt::IgnoreAction );
    event->accept();
}

void BinFileView::resizeEvent( QResizeEvent* )
{
    _linesOnViewPort = ( viewport()->height() - _bottomMargin ) / fontMetrics().height();

    calculateNumOfByteGroups();

    horizontalScrollBar()->setRange( 0, preFitWholeLine( _byteGroups ) - viewport()->width() );
    horizontalScrollBar()->setPageStep( viewport()->width() );

    verticalScrollBar()->setRange( 0, _lineCount - _linesOnViewPort );
    verticalScrollBar()->setPageStep( _linesOnViewPort );
    verticalScrollBar()->setSliderPosition( _addend / _bytesPerLine );

    emit fileViewContentChanged( this );
}

void BinFileView::paintEvent( QPaintEvent* event )
{
    QPainter painter( viewport() );

    int xOffset = horizontalScrollBar()->value();

    painter.fillRect( QRect( -xOffset, event->rect().top(), _addressAreaWidth, height() ), viewport()->palette().color( QPalette::Button ) );

    painter.setPen( viewport()->palette().color( QPalette::WindowText ) );
    painter.drawLine( _addressAreaWidth + _hexAreaWidth - xOffset, event->rect().top(), _addressAreaWidth + _hexAreaWidth -xOffset, event->rect().height() );

    if( _lineCount ) {
        _addend = addressAddend();

        int yIncr = fontMetrics().height();
        int yPos = yIncr;
        for( qint64 row( 0 ); row < qMin( _linesOnViewPort, _lineCount ); row++, yPos += yIncr ) {
            qint64 addr = row * (qint64)_bytesPerLine + _addend;
            QString addrString = QString( "%1:%2" ).arg( ( addr & _upperMask ) >> (4 * _addressChars / 2), _addressChars / 2, 16, QChar( '0' ) ).toUpper() \
                                                   .arg( addr & _lowerMask, _addressChars / 2, 16, QChar( '0' ) ).toUpper();

            painter.setPen( viewport()->palette().color( QPalette::ButtonText ) );
            painter.drawText( _leftMargin - xOffset, yPos, addrString );

            painter.setPen( viewport()->palette().color( QPalette::WindowText ) );

            int xPos = _addressAreaWidth + _leftMargin - xOffset;
            for( qint64 b( 0 ); b < qMin( (qint64)_bytesPerLine, _size - addr ); b++, xPos += _byteWidth ) {
                uchar binByte = *( _data + b + addr );
                QString byte = QString( "%1" ).arg( binByte, 2, 16, QChar( '0' ) ).toUpper();
                if( b > 0 && b % BinFileView::bytesPerGroup == 0 )
                    xPos += _groupGap;
                if( _colorData )
                    painter.setPen( (Qt::GlobalColor)*( _colorData + b + addr ) );
                painter.drawText( xPos, yPos, byte );
            }

            xPos = _addressAreaWidth + _hexAreaWidth + _leftMargin - xOffset;
            int charWidth = fontMetrics().averageCharWidth();
            for( qint64 c( 0 ); c < qMin( (qint64)_bytesPerLine, _size - addr ); c++, xPos += charWidth ) {
                uchar binByte = *( _data + c + addr );
                if( _colorData )
                    painter.setPen( (Qt::GlobalColor)*( _colorData + c + addr ) );
                painter.drawText( xPos, yPos, isprint( binByte ) ? QString( binByte ) : QString( '.' ) );
            }
        }
        painter.setPen( viewport()->palette().color( QPalette::WindowText ) );
    } else {
        drawEmptyViewInstructions( painter );
    }
}

void BinFileView::scrollContentsBy( int, int )
{
    emit fileViewContentChanged( this );
    viewport()->update();
}

void BinFileView::calculateNumOfByteGroups()
{
    // Calculate new # of byte groups
    while( viewport()->width() > preFitWholeLine( _byteGroups + 1 ) || viewport()->width() < preFitWholeLine( _byteGroups ) ) {
        if( viewport()->width() > preFitWholeLine( _byteGroups + 1 ) )
            adjust( ++_byteGroups );
        else if( viewport()->width() < preFitWholeLine( _byteGroups ) && _byteGroups > BinFileView::minimumOfByteGroups )
            adjust( --_byteGroups );
        else {
            adjust( BinFileView::minimumOfByteGroups );
            _byteGroups = BinFileView::minimumOfByteGroups;
            break;
        }
    }
}

void BinFileView::adjust( const int newbyteGroups )
{
    if( newbyteGroups ) {
        _byteGroups = newbyteGroups;
        _bytesPerLine = _byteGroups * BinFileView::bytesPerGroup;
    }

    _lineCount = (int)( _size / _bytesPerLine );
    if( _size % _bytesPerLine )
        _lineCount += 1;

    QString address = QString( "%1:%2" ).arg( 0, _addressChars / 2, 16, QChar( '0' ) ) \
                                        .arg( 0, _addressChars / 2, 16, QChar( '0' ) );
    QString byte = QString( "%1" ).arg( 0, 2, 16, QChar( '0' ) );

    // Set all margins to half of a character width
    _leftMargin = fontMetrics().averageCharWidth() / 2;
    _rightMargin = _leftMargin;
    _bottomMargin = _leftMargin;
    _byteWidth = fontMetrics().width( byte ) + _leftMargin;
    _addressAreaWidth = fontMetrics().width( address ) + _leftMargin + _rightMargin;
    _groupGap = fontMetrics().averageCharWidth();
    _groupWidth = BinFileView::bytesPerGroup * _byteWidth;
    _hexAreaWidth = _byteGroups * _groupWidth + (_byteGroups - 1) * _groupGap + _rightMargin;
    _asciiAreaWidth = _bytesPerLine * fontMetrics().averageCharWidth() + _leftMargin + _rightMargin;
}

int BinFileView::preFitWholeLine( const int byteGroups )
{
    int newHexAreaWidth = byteGroups * _groupWidth + (byteGroups - 1) * _groupGap + _rightMargin;
    int newAsciiAreaWidth = byteGroups * BinFileView::bytesPerGroup * fontMetrics().averageCharWidth() + _leftMargin + _rightMargin;
    return( _addressAreaWidth + newHexAreaWidth + newAsciiAreaWidth );
}

void BinFileView::drawEmptyViewInstructions( QPainter& painter )
{
    QString help1( tr( "< Drag & drop or >" ) );
    QString help2( tr( "< use ctx menu >" ) );
    QString help3( tr( "< to open a file >" ) ) ;

    int xPos = _addressAreaWidth + _hexAreaWidth / 2 - fontMetrics().width( help1 ) / 2;
    int yIncr = fontMetrics().height();
    int yPos = ( viewport()->height() + yIncr / 2 - 3 * yIncr + yIncr ) / 2;
    painter.drawText( xPos, yPos, help1 );
    yPos += yIncr;
    int xPos2 = _addressAreaWidth + _hexAreaWidth / 2 - fontMetrics().width( help2 ) / 2;
    painter.drawText( xPos2, yPos, help2 );
    yPos += yIncr;
    painter.drawText( xPos, yPos, help3 );
}
