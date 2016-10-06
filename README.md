# bindiff-qt
An application for comparing two binary files

This small application is made for visually diffing two binary files. Currently, differing bytes are shown on red while equal bytes are black. Files can be opened by dropping them onto respective view area, upper or lower. Alternatively, file can be opened via view's context menu.

The layout and look&feel for view widget is heavily inspired by [Okteta](https://utils.kde.org/projects/okteta/) and also some influences are from [qhexedit2](https://github.com/Simsys/qhexedit2) & [binview](https://github.com/vurdalakov/abandoned). However, since none of the view widgets of those projects were not suitable mutual diffing of two files, this application and it's components are complete rewrite.

File model is simple mmap'ped files and also buffer for data containing difference coloring is anonymous (not backed by real file) mmap in virtual address space. Diffing is done 'on demand', i.e. only when user scrolls files and content of the views change. This makes it possibe to diff >2GB files efficiently.

Enjoy ;-)
