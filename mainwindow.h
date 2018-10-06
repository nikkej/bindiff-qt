//*****************************************************************************
//
//     mainwindow.h
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>


namespace Ui {
class MainWindow;
}
class QFile;
class BinFileView;

struct FileModel
{
    FileModel( QFile* file, uchar* mmap ) : _file( file ), _mmap( mmap ) {}
    QFile* _file;
    uchar* _mmap;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow( QWidget* parent = nullptr );
    ~MainWindow();

private slots:
    void open( BinFileView* );
    void open( const QString&, BinFileView* view = nullptr );
    void updateDiff( BinFileView* );
    void on_actionE_xit_triggered();

private: // No copying
    MainWindow( const MainWindow& );
    MainWindow& operator=( const MainWindow& );

private: // Data
    Ui::MainWindow* ui;
    QMap<BinFileView* , struct FileModel> _files;
    uchar* _diffMap;
    qint64 _diffMapSize;
};

#endif // MAINWINDOW_H
