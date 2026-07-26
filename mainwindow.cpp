#include "mainwindow.h"
#include <QFileDialog>
#include <QToolButton>
#include <QMessageBox>
#include <QStandardPaths>
#include <QFile>
#include <QFontDatabase>
#include <QTextStream>
#include <QFileInfo>
#include "./ui_mainwindow.h"
#include <Qsci/qsciscintilla.h>

// init and connects all different functionalites of the ide in constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // tab widgets
    m_tab = new QTabWidget(this);
    m_tab -> setTabsClosable(true);
    m_tab -> setMovable(true);
    connect(m_tab, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    setCentralWidget(m_tab);

    // + icon for new files
    auto* newTabButton = new QToolButton(this);
    newTabButton->setText("+");
    newTabButton->setToolTip("New Tab");

    connect(newTabButton, &QToolButton::clicked, this, [this]() {
        newEditorTab("untitled");
    });
    m_tab->setCornerWidget(newTabButton, Qt::TopRightCorner);
    newEditorTab("untitled");

    // file menu + open file action
    // looks for a file menu, if there link to exists otherwise create new
    QMenu *fileMenu = nullptr;
    for (QAction *action : menuBar() -> actions()) {
        if (action -> menu() && action -> text().contains("File")) {
            fileMenu = action -> menu();
        }
    } 
    if (!fileMenu) {
        fileMenu = menuBar() -> addMenu(("&File"));
    }

    QAction *openAction = new QAction("&Open...", this);
    openAction -> setShortcut(QKeySequence::Open); // Ctrl+O
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    fileMenu -> addAction(openAction);

    // file menu save action
    QAction *saveAction = new QAction("&Save...", this);
    saveAction -> setShortcut(QKeySequence::Save); // Ctrl+S
    // need lambda because saveFile takes a QScintillia* param
    connect(saveAction, &QAction::triggered, this, [this]() {
        saveFile(currentEditor());
    });
    fileMenu -> addAction(saveAction);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QsciScintilla* MainWindow::currentEditor() const {
    return qobject_cast<QsciScintilla*>(m_tab -> currentWidget());
}

QsciScintilla* MainWindow::newEditorTab(const QString& title) {
    auto* editor = new QsciScintilla(m_tab);
    editor -> setMarginType(0, QsciScintilla::NumberMargin);
    editor -> setMarginWidth(0, "0000");
    editor -> setAutoIndent(true);
    editor -> setTabWidth(4);

    connect(editor, &QsciScintilla::textChanged, this, [this, editor]() {
        m_modified[editor] = true;
    });

    m_tab -> addTab(editor, title);
    m_tab -> setCurrentWidget(editor);

    // adding font to different editors...
    QFont codeFont("TypeWriter", 11);
    codeFont.setStyleHint(QFont::TypeWriter);
    // making sure font exists in the database (dynamic memory cache)
    if (!QFontDatabase::systemFont(QFontDatabase::FixedFont).family().isEmpty()) {
        codeFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        codeFont.setPointSize(11);
    }
    editor -> setFont(codeFont);

    // adding some whtespace 
    editor -> setMarginWidth(1, 10);

    // a very subtle line highlight showing where typing
    editor -> setCaretLineVisible(true);
    editor -> setCaretLineBackgroundColor(QColor(200, 200, 200));
    return editor;
}

void MainWindow::closeTab(int index) {
    // sets editor to tab we want to close
    auto* editor = qobject_cast<QsciScintilla*>(m_tab -> widget(index));
    if (!editor) {
        return;
    }
    
    QMessageBox::StandardButton result = QMessageBox::Discard;

    if (m_modified.value(editor, false)) {
        result = QMessageBox::question(
            this, "Unsaved changes",
            "This file has unsaved changes. Save before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );
    }

    // if user doesn't want to close the tab - abort beofre removing below
    if (result == QMessageBox::Cancel) {
        return;
    }

    if (result == QMessageBox::Save) {
        if (!saveFile(editor)) {
            return; // something went wrong, not closing tab
        }
    }
    m_filePaths.remove(editor);
    m_modified.remove(editor);

    if (m_tab->count() <= 1) {
        newEditorTab("untitled"); // replace the last tab with a fresh blank one, might be some form of home tab in the future
    }
    m_tab -> removeTab(index);
    editor -> deleteLater(); // delete later waits for all ui actions/events before deleting memory
}

void MainWindow::openFile() {
    
    // path to WSL directory
    QString linuxHome = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    
    // config dialog object
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Open WSL File"));
    dialog.setDirectory(linuxHome); // starting folder
    dialog.setFileMode(QFileDialog::ExistingFile); // only allowing files that exist
    dialog.setNameFilter(tr("All Files (*)")); // filter for file extensions

    // forces qt to render own ui instead of relying on linux packages on my windows machine
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    // open dialog -> check if clicked open/ok
    if (dialog.exec()) {

        // retrive selected file
        QString fileName = dialog.selectedFiles().at(0);
        
        // here is where file input logic should be, reading existing into editor
        qDebug() << "successfully opened file at: " << fileName;

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Error", "Could not open file" + fileName);
            return;
        }

        QTextStream in (&file);
        QString content = in.readAll();
        QsciScintilla* editor = newEditorTab(QFileInfo(fileName).fileName());
        editor -> setText(content);
        m_filePaths[editor] = fileName;
        m_modified[editor] = false;
    }

    else {
        qDebug() << "user canceled file selection";
    }

}

bool MainWindow::saveFile(QsciScintilla* editor) {
    if (!editor) {
        return false;
    }

    // if there is no path yet.. it is a new tab and redirect to save as
    if (!m_filePaths.contains(editor) || m_filePaths[editor].isEmpty()) {
        return saveFileAs(editor); // need to implement this funtion
    }

    QFile saveFile(m_filePaths[editor]);
    // writing to file through QIO, overwrites what's already in filePath if exists, else
    // creates a new file, QIODevice::Text handles all '\r\n' and utf-8 encoding
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Could not save the file";
        QMessageBox::warning(this, "Error", "Could not save file: " + m_filePaths[editor]);
        return false;
    }

    // using text stream to write to the verified filepath
    QTextStream out(&saveFile);
    out << editor -> text();

    // update tab title
    int index = m_tab -> indexOf(editor);
    if (index != -1) {
        QString title = QFileInfo(m_filePaths[editor]).fileName();
        m_tab -> setTabText(index, title);
    }

    return true;
}

// heavier lifiting is done in saveFile, here just adding filepath to the map
bool MainWindow::saveFileAs(QsciScintilla* editor) {
    
    if (!editor) {
        return false;
    }

    // prompting for save file name
    QString path = QFileDialog::getSaveFileName(this, "Save File As");
    if (path.isEmpty()) {
        return false;
    }
    
    m_filePaths[editor] = path;
    return saveFile(editor);
}

//TODO: 
// Advanced text and sytax like autocompletion of lines.. 
// cleanup ui.. maybe add a home screen if no file is open..
// add sidebar to track files in directory that project is in
// if writing () or {}, want autofill in for those