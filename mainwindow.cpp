#include "mainwindow.h"
#include <QFileDialog>
#include <QToolButton>
#include <QMessageBox>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>
#include "./ui_mainwindow.h"
#include <Qsci/qsciscintilla.h>

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
    newEditorTab("Untitled");

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
        // TODO: create a saveEditor(editor) later 
    }
    m_filePaths.remove(editor);
    m_modified.remove(editor);

    if (m_tab->count() <= 1) {
        newEditorTab("untitled"); // replace the last tab with a fresh blank one, might be some form of home tab in the future
    }
    m_tab -> removeTab(index);
    // delete later waits for all ui actions/events before deleting memory
    editor -> deleteLater(); 
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
    }

    else {
        qDebug() << "user canceled file selection";
    }

}

//TODO: Add file saving and loading
// Advanced text and sytax like autocompletion of lines.. 
// cleanup ui.. maybe add a home screen if no file is open..
// add sidebar to track files in directory that project is in