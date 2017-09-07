//*****************************************************************************
//
//     mainwindow.cpp
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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QScrollBar>
#include <sys/mman.h>
//#include <ctype.h>

MainWindow::MainWindow( QWidget *parent ) :
    QMainWindow( parent ),
    ui( new Ui::MainWindow ),
    _diffMap( NULL ),
    _diffMapSize( 0 )
{
    ui->setupUi( this );

    setWindowTitle( QCoreApplication::applicationName() );

//    ui->binFileView1->setAcceptDrops( true );
//    ui->binFileView2->setAcceptDrops( true );

    ui->binFileView1->setFont( QFont( "Monospace", 10 ) );
    ui->binFileView2->setFont( QFont( "Monospace", 10 ) );

    // Cross-connect vertical & horizontal scroll bars
    connect( ui->binFileView1->verticalScrollBar(), \
             SIGNAL( valueChanged( int ) ), \
             ui->binFileView2->verticalScrollBar(), \
             SLOT( setValue( int ) ) );
    connect( ui->binFileView2->verticalScrollBar(), \
             SIGNAL( valueChanged( int ) ), \
             ui->binFileView1->verticalScrollBar(), \
             SLOT( setValue( int ) ) );

    connect( ui->binFileView1->horizontalScrollBar(), \
             SIGNAL( valueChanged( int ) ), \
             ui->binFileView2->horizontalScrollBar(), \
             SLOT( setValue( int ) ) );
    connect( ui->binFileView2->horizontalScrollBar(), \
             SIGNAL( valueChanged( int ) ), \
             ui->binFileView1->horizontalScrollBar(), \
             SLOT( setValue( int ) ) );

    // to maintain consistent width for both views
    connect( ui->binFileView1, SIGNAL( defaultVisualsChanged( BinFileView* ) ), \
             ui->binFileView2, SLOT( synchronizeVisuals( BinFileView* ) ) );
    connect( ui->binFileView2, SIGNAL( defaultVisualsChanged( BinFileView* ) ), \
             ui->binFileView1, SLOT( synchronizeVisuals( BinFileView* ) ) );

    // File drag & drop signals
    connect( ui->binFileView1, SIGNAL( fileDropped( QString, BinFileView* ) ), \
             this, SLOT( open( QString, BinFileView* ) ) );
    connect( ui->binFileView2, SIGNAL( fileDropped( QString, BinFileView* ) ), \
             this, SLOT( open( QString, BinFileView* ) ) );

    // And view's file ctx menu signals
    connect( ui->binFileView1, SIGNAL( fileOpenRequested( BinFileView* ) ), \
             this, SLOT( open( BinFileView* ) ) );
    connect( ui->binFileView2, SIGNAL( fileOpenRequested( BinFileView* ) ), \
             this, SLOT( open( BinFileView* ) ) );

    // On demand (i.e. via changed content of view's data) requested diffing
    connect( ui->binFileView1, SIGNAL( fileViewContentChanged( BinFileView* ) ), \
             this, SLOT( updateDiff( BinFileView* ) ), Qt::DirectConnection );
    connect( ui->binFileView2, SIGNAL( fileViewContentChanged( BinFileView* ) ), \
             this, SLOT( updateDiff( BinFileView* ) ), Qt::DirectConnection );

    // Get the arguments
    if( QCoreApplication::arguments().size() == 3 ) {
        open( QCoreApplication::arguments().at( 1 ), ui->binFileView1 );
        open( QCoreApplication::arguments().at( 2 ), ui->binFileView2 );
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    if( !_files.empty() ) {
        for( const auto f : _files ) {
            f._file->unmap( f._mmap );
            f._file->close();
            delete f._file;
        }
    }
    if( _diffMap )
        ::munmap( _diffMap, _diffMapSize );
}

void MainWindow::open( BinFileView* view )
{
    open( QFileDialog::getOpenFileName( this ), view );
}

void MainWindow::open( const QString& fileName , BinFileView* view )
{
    if ( !fileName.isEmpty() ) {
        QFile *file = new QFile( fileName );

        if( !file->open( QIODevice::ReadOnly ) ) {
            qWarning() << "File open failed!";
            delete file;
            return;
        }

        uchar *mmap = file->map( 0, file->size() );

        if( !mmap ) {
            qWarning() << "File mmap failed!";
            delete file;
            return;
        }

        if( view ) {
            const auto f = _files.find( view );
            if( f != _files.end() ) {
                (*f)._file->unmap( (*f)._mmap );
                (*f)._file->close();
                delete (*f)._file;
            }
            _files.insert( view, FileModel( file, mmap ) );
            view->setData( mmap, file->size() );
            view->setToolTip( fileName );
        }
        if( _files.size() == 2 ) {
            if( _diffMap )
                ::munmap( _diffMap, _diffMapSize );

            auto f = _files.begin();
            qint64 size1 = (*f)._file->size();
            qint64 size2 = (*++f)._file->size();
            _diffMapSize = qMax( size1, size2 );

            // Qt's QFileDevice::map doesn't eat original mmap flags, --> use 'real stuff' here!
            _diffMap = (uchar*)::mmap( 0, _diffMapSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );

            if( !_diffMap ) {
                qWarning() << "Mapping failed!!!!";
            }
            else {
                updateDiff( NULL );
                QList<BinFileView*> views = _files.keys();
                for( auto v : views )
                    v->setColoringData( _diffMap );
            }
        }
    }
}

void MainWindow::updateDiff( BinFileView* )
{
    if( _files.size() != 2 || !_diffMap )
        return;

    qint64 addend1 = ui->binFileView1->addressAddend();
    qint64 addend2 = ui->binFileView2->addressAddend();
    int viewCapacity = ui->binFileView1->capacity();

    auto f = _files.begin();
    qint64 size1 = (*f)._file->size();
    uchar* file1 = (*f)._mmap;
    qint64 size2 = (*++f)._file->size();
    uchar* file2 = (*f)._mmap;

    if( _diffMap ) {
        // Define compare window begin offset
        qint64 cwBegin;
        // If view's data position beyond smaller file's size
        if( qMax( addend1, addend2 ) - qMin( addend1, addend2 ) > viewCapacity )
            cwBegin = qMax( addend1, addend2 );
        else
            cwBegin = qMin( addend1, addend2 );

        for( qint64 c( cwBegin ); c < qMax( addend1 + viewCapacity, addend2 + viewCapacity ); c++ ) {
            if( c < qMin( size1, size2 ) ) {
                // Compare data
                if( *( file1 + c ) != *( file2 + c ) )
                    *( _diffMap + c ) = (uchar)Qt::red;
                else
                    *( _diffMap + c ) = (uchar)Qt::black;
            }
            else if( c < qMax( size1, size2 ) ) {
                // Address offset beyond smaller file size
                *( _diffMap + c ) = (uchar)Qt::red;
            }
        }
    }
}

void MainWindow::on_actionE_xit_triggered()
{
    this->close();
}
