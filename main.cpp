//*****************************************************************************
//
//     main.cpp
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
#include <QApplication>
#include <QFileInfo>

int main( int argc, char* argv[] )
{
    QFileInfo execFile( argv[0] );
    QCoreApplication::setApplicationName( execFile.fileName() );
    QApplication a( argc, argv );
    MainWindow w;
    w.show();

    return a.exec();
}
