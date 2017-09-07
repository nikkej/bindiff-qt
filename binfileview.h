//*****************************************************************************
//
//     binfileview.h
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

#ifndef BINFILEVIEW_H
#define BINFILEVIEW_H

#include <QAbstractScrollArea>

class QMenu;
class QAction;

class BinFileView : public QAbstractScrollArea
{
    Q_OBJECT

public:
    enum Constants {
        minimumOfByteGroups = 2,
        bytesPerGroup = 4
    };

    BinFileView( QWidget* parent = 0 );
    virtual ~BinFileView();

    virtual void setFont( QFont );
    const uchar* data();
    void setData( const uchar*, const qint64 );
    void setColoringData( uchar* );
    inline int capacity() { return _linesOnViewPort * _bytesPerLine; }
    inline int addressCharacters() { return _addressChars; }
    void setAddressCharacters( const int );
    qint64 addressAddend();

signals:
    void fileDropped( QString, BinFileView* );
    void fileOpenRequested( BinFileView* );
    void fileViewContentChanged( BinFileView* );
    void defaultVisualsChanged( BinFileView* );

public slots:
    void showContextMenu( const QPoint& );
    void synchronizeVisuals( BinFileView* );

protected:
    virtual void dragEnterEvent( QDragEnterEvent* );
    virtual void dropEvent( QDropEvent* );
    virtual void resizeEvent( QResizeEvent* );
    virtual void paintEvent( QPaintEvent* );
    virtual void scrollContentsBy( int, int );

private: // Methods
    void calculateNumOfByteGroups();
    void adjust( const int newByteGroups = 0 );
    int preFitWholeLine( const int );
    void drawEmptyViewInstructions( QPainter& );

private: // No copying
    BinFileView( const BinFileView& );
    BinFileView& operator=( const BinFileView& );

private: // Data
    QFont         _font;
    QMenu*        _contextMenu;
    QAction*      _contextAction;
    const uchar*  _data;               // ptr to raw binary data, not owned
    const uchar*  _colorData;
    qint64        _size;               // accessible file size
    qint64        _upperMask;          // masks for address area, address is drawn
    qint64        _lowerMask;          // like %0nX:%0nX where n is _addressChars / 2
    qint64        _addend;
    int           _addressChars;       // # of characters on address field
    int           _lineCount;          // == division of size per # of bytes on one line
    int           _linesOnViewPort;    // # of lines viewport is capable to draw
    int           _byteGroups;
    int           _bytesPerLine;

    // Pixel measures
    int     _leftMargin;
    int     _rightMargin;
    int     _bottomMargin;
    int     _byteWidth;
    int     _groupWidth;
    int     _addressAreaWidth;
    int     _hexAreaWidth;
    int     _asciiAreaWidth;
    int     _groupGap;
};

#endif // BINFILEVIEW_H
