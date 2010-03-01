from PyQt4 import QtCore, QtGui, uic
from PyQt4.QtCore import *
from PyQt4.QtGui import *

import data

class Mod:
    def __init__(self, path):
        import os
        self.path = path
        self.size = None
        self.name = os.path.basename(path)

def isZipFile(path):
    import zipfile
    return zipfile.is_zipfile(path)

def showError(error):
    print error

class MyTable(QtGui.QTableWidget):
    def __init__(self, parent):
        QtGui.QTableWidget.__init__(self, parent)
        self.setAcceptDrops(True)
        self.mods = []

    def initialize(self):
        def maybeAdd(path):
            import os
            #print "Check path %s" % path
            if os.path.exists("%s/%s.txt" % (path, os.path.basename(path))):
                self.addMod(path)
        
        self.setColumnCount(3)
        
        import os
        for path in os.listdir(data.modPath()):
            maybeAdd('%s/%s' % (data.modPath(), path))

    def addMod(self, path):
        column_name = 0
        column_enabled = 1
        column_size = 2

        mod = Mod(path)
        self.mods.append(mod)

        row = self.rowCount()
        self.setRowCount(self.rowCount() + 1)

        self.setItem(row, column_name, QtGui.QTableWidgetItem(mod.name))
        # self.setItem(1, 1, QtGui.QTableWidgetItem('foobar'))
        #for c in range(0, self.columnCount()):
        #    for r in range(0, self.rowCount()):
        #        self.setItem(r, c, QtGui.QTableWidgetItem('foobar'))

    def dragEnterEvent(self, event):
        # print "Has urls? %s" % event.mimeData().hasUrls()
        if event.mimeData().hasUrls():
            # print "Accept event"
            event.accept()
        else:
            event.ignore()
        # print "Drag event: mime %s" % event.mimeData().urls()

    def dragMoveEvent(self, event):
        event.accept()
        # print "Drag move event"

    def doFile(self, path):
        import zipfile, os
        print "Installing mod '%s'" % path
        zip = zipfile.ZipFile(path, 'r')
        toplevel = os.path.dirname(zip.namelist()[0])
        zip.extractall(data.modPath())
        self.addMod('%s/%s' % (data.modPath(), toplevel))

    def dropEvent(self, event):
        for file in event.mimeData().urls():
            path = str(file.path())
            if isZipFile(path):
                self.doFile(path)
            else:
                showError("'%s' is not a zip file. Please drag and drop .zip files" % path)
