#include "mainwindow.h"
#include <QToolButton>
#include <QMessageBox>
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

    auto* newTabButton = new QToolButton(this);
    newTabButton->setText("+");
    newTabButton->setToolTip("New Tab");

    connect(newTabButton, &QToolButton::clicked, this, [this]() {
        newEditorTab("untitled");
    });
    m_tab->setCornerWidget(newTabButton, Qt::TopRightCorner);

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
    if (!editor) {
        return;
    }
    
    QMessageBox::StandardButton result = QMessageBox::Discard;

    if (m_modified.value(editor, false)) {
        auto result = QMessageBox::question(
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
    editor -> deleteLater(); // learn more about this function
}

