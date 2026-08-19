#include "mainwindow.h"
#include "titlebar.h"
#include <QFileDialog>
#include <QToolButton>
#include <QMessageBox>
#include <QStandardPaths>
#include <QFile>
#include <QFontDatabase>
#include <QTextStream>
#include <QFileInfo>
#include <QTreeView>
#include <QFileSystemModel>
#include <QMouseEvent>
#include <QtGlobal>
#include <QDockWidget>
#include <QHBoxLayout>
#include "./ui_mainwindow.h"
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexercpp.h>

// TODO: 
// with close button added, need to loop through all open editors and make sure no changes needed to be saved
// auto complete of brackets
// fix ui of file menu and make it so when open program no text editor open, some form of homescreen
// add the ability to type something like "harrington ." (or wtv dir) in a linux cli, and have the ide opened up with the specified directory
// make the close scope not active on if/elses (just functions) and make it not white

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupFileTree();
    tabInit();
    plusButton();
    actionLinks();
	setupTitleBar();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// CONSTRUCTOR HELPER FUNCTIONS

void MainWindow::tabInit() {
    m_tab = new QTabWidget(this);
    m_tab -> setTabsClosable(true);
    m_tab -> setMovable(true);
    connect(m_tab, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    setCentralWidget(m_tab);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
}

void MainWindow::plusButton() {
    // + icon for new files
    auto* newTabButton = new QToolButton(this);
    newTabButton->setText("+");
    newTabButton->setToolTip("New Tab");

    connect(newTabButton, &QToolButton::clicked, this, [this]() {
        newEditorTab("untitled");
    });
    m_tab->setCornerWidget(newTabButton, Qt::TopRightCorner);
    newEditorTab("untitled");
}

void MainWindow::actionLinks() {
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

    
    // to open a directory.. not just single file
    QAction *openDirAction = new QAction("Open &Project Directory...", this);
    connect(openDirAction, &QAction::triggered, this, &MainWindow::openDirectory);
    fileMenu -> addAction(openDirAction);


    // file menu save action
    QAction *saveAction = new QAction("&Save...", this);
    saveAction -> setShortcut(QKeySequence::Save); // Ctrl+S
    // need lambda because saveFile takes a QScintillia* param
    connect(saveAction, &QAction::triggered, this, [this]() {
        saveFile(currentEditor());
    });
    fileMenu -> addAction(saveAction);
}


void MainWindow::setupFileTree() {

    // not setting root path off the bat for file tree so full linux directory isnt displayed
    m_fileModel = new QFileSystemModel(this);
    m_fileTree = new QTreeView(this);
    m_fileTree -> setModel(m_fileModel);
    connect(m_fileTree, &QTreeView::doubleClicked, this, &MainWindow::onFileTreeDoubleClicked);

    // hiding other pats of tree (size/type/date)... got clutered
    for (int i = 1; i < m_fileModel -> columnCount(); ++i) {
        m_fileTree -> hideColumn(i);
    }

    // add a block for removing the unwanted status in the file tree, only need to see directories and file names.. wondering if instead of having holy directory displayed should have a button to choose one
    m_fileDock = new QDockWidget("Project View", this);
    m_fileDock -> setWidget(m_fileTree);
    m_fileDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);

    addDockWidget(Qt::LeftDockWidgetArea, m_fileDock);
}


void MainWindow::setupTitleBar() {
	auto* titleBar = new TitleBar(this);
	titleBar -> setFixedHeight(32);
	
	auto* layout = new QHBoxLayout(titleBar);
	layout -> setContentsMargins(0, 0, 0, 0);
	layout -> setSpacing(0);
	
	// moving existing file menu into this layour
	layout -> addWidget(menuBar());
	layout -> addStretch();
	
	auto* minimizeBtn = new QToolButton(this);
	minimizeBtn -> setText("—");
	minimizeBtn -> setFixedSize(40, 32);
	connect(minimizeBtn, &QToolButton::clicked, this, &MainWindow::showMinimized);

	auto* maximizeBtn = new QToolButton(this);
	maximizeBtn -> setText("☐");
	maximizeBtn -> setFixedSize(40, 32);
	connect(maximizeBtn, &QToolButton::clicked, this, [this]() {
		isMaximized() ? showNormal() : showMaximized();
	});

	auto* closeBtn = new QToolButton(this);
	closeBtn -> setText("✕");
	closeBtn -> setFixedSize(40, 32);
	connect(closeBtn, &QToolButton::clicked, this, &QWidget::close);

	layout -> addWidget(minimizeBtn);
	layout -> addWidget(maximizeBtn);
	layout -> addWidget(closeBtn);
	
	setMenuWidget(titleBar);
}


// function to acutally do the opening of the project folder 
void MainWindow::openProjectFolder(const QString& path) {
    if (path.isEmpty()) {
        return;
    }

    m_fileModel -> setRootPath(path);
    m_fileTree -> setRootIndex(m_fileModel -> index(path));
    m_fileDock -> setWindowTitle(QFileInfo(path).fileName());
}

// EDITOR FUNCTIONS

QsciScintilla* MainWindow::currentEditor() const {
    return qobject_cast<QsciScintilla*>(m_tab -> currentWidget());
}

QsciScintilla* MainWindow::newEditorTab(const QString& title) {
    auto* editor = new QsciScintilla(m_tab);

    // setting font
    QFont codeFont("TypeWriter", 11);
    codeFont.setStyleHint(QFont::TypeWriter);
    if (!QFontDatabase::systemFont(QFontDatabase::FixedFont).family().isEmpty()) {
        codeFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        codeFont.setPointSize(11);
    }
    editor->setFont(codeFont);

    QColor bgColor(30, 30, 30);
    QColor textColor(220, 220, 220);

    QsciLexerCPP *lexer = new QsciLexerCPP(editor);

    lexer -> setPaper(bgColor, 0);
    lexer -> setColor(textColor, 0);
    lexer -> setFont(codeFont, 0);

    editor -> setLexer(lexer);
    editor -> SendScintilla(QsciScintilla::SCI_STYLECLEARALL); // clears old states, forces what defined above

    // check actual # for styles used, dont have the enum (making it so background for all diff stysles/QsciLexerCPP enums is the same) 
    for (int style = 0; style <= 135; ++style) {
        lexer->setPaper(bgColor, style);
        lexer->setColor(textColor, style);
        lexer->setFont(codeFont, style);
    }

    // setting lexer colors for different components of c++ code, very open to change
    lexer -> setDefaultFont(codeFont);
    lexer -> setPaper(bgColor);
    lexer -> setDefaultColor(textColor);
	lexer -> setColor(QColor(181, 206, 168), QsciLexerCPP::Number);
    lexer -> setColor(QColor(0, 128, 0), QsciLexerCPP::Comment);
    lexer -> setColor(QColor(0, 128, 0), QsciLexerCPP::CommentLine);
    lexer -> setColor(QColor(0, 128, 0), QsciLexerCPP::CommentDoc);
    lexer -> setColor(QColor(215, 58, 73), QsciLexerCPP::Keyword);
    lexer -> setColor(QColor(163, 61, 61), QsciLexerCPP::DoubleQuotedString);
    lexer -> setColor(QColor(163, 61, 61), QsciLexerCPP::SingleQuotedString);
	lexer -> setColor(QColor(255, 255, 255), QsciLexerCPP::UnclosedString);
    lexer -> setColor(QColor(170, 186, 153), QsciLexerCPP::Identifier);
	lexer -> setColor(QColor(215, 225, 225), QsciLexerCPP::Operator);
	lexer -> setColor(QColor(220, 220, 100), QsciLexerCPP::TaskMarker);
    lexer -> setColor(QColor(78, 201, 176), QsciLexerCPP::PreProcessor);

    // line highlighting
    editor -> setCaretForegroundColor(Qt::white); // Ensures you can see your text cursor on a dark background
    editor -> setCaretLineVisible(true);
    editor -> setCaretLineBackgroundColor(QColor(45, 45, 45)); 

    // margins/geo
    editor -> setMarginsBackgroundColor(QColor(40, 40, 40));
    editor -> setMarginsForegroundColor(QColor(150, 150, 150));
    editor -> setMarginType(0, QsciScintilla::NumberMargin);
    editor -> setMarginWidth(0, "0000");
    editor -> setMarginWidth(1, 10);
    
    editor -> setAutoIndent(true);
    editor -> setTabWidth(4);
    editor -> setFolding(QsciScintilla::BoxedTreeFoldStyle);

    // modification states
    connect(editor, &QsciScintilla::textChanged, this, [this, editor]() {
        m_modified[editor] = true;
    });


    // adding to tab
    m_tab->addTab(editor, title);
    m_tab->setCurrentWidget(editor);

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


// FILE I/O FUNCTIONS

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

    // make sure that if saved doesnt prompt for saving when close
    m_modified[editor] = false;
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

void MainWindow::openDirectory() {
    QString linuxHome = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    // this line grabs the directory user choses.. have to then link to the dock widget and tree
    QString dirPath = QFileDialog::getExistingDirectory(this, "Open Project Directory", linuxHome, QFileDialog::ShowDirsOnly);
    openProjectFolder(dirPath);
}

// OVERRIDEN EVENT HANDLERS
// added for know since no more fullscreen button 
void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event -> key() == Qt::Key_F11) {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    }
}


void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (event -> button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event -> globalPosition().toPoint() - frameGeometry().topLeft();
        event -> accept();
    }
}


void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && (event -> buttons() & Qt::LeftButton)) {
        move(event -> globalPosition().toPoint() - m_dragPosition);
        event -> accept();
    }
}


void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    if (event -> button() == Qt::LeftButton) {
        m_dragging = false; 
        event -> accept();
    }
}


void MainWindow::onFileTreeDoubleClicked(const QModelIndex& index) {

	// let tree expand/collapse - dont try to open them
	if (m_fileModel -> isDir(index)) {
		return;
	}
	
	QString filePath = m_fileModel -> filePath(index);
	
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QMessageBox::warning(this, "Error", "Could not open file" + filePath);
		return;
	}
	
	QTextStream in (&file);
	QString content = in.readAll();
	QsciScintilla* editor = newEditorTab(QFileInfo(filePath).fileName());
	editor -> setText(content);
	m_filePaths[editor] = filePath;
	m_modified[editor] = false;
}