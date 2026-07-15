#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <Qsci/qsciscintilla.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_tab = new QTabWidget(this);
    m_tab -> setTabsClosable(true);
    m_tab -> setMovable(true);
    connect(m_tab, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    setCentralWidget(m_tab);

    newEditorTab("Untitled");
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

    m_filePaths.remove(editor);
    m_modified.remove(editor);
    m_tab -> removeTab(index);
    editor -> deleteLater(); // learn more about this function
}

