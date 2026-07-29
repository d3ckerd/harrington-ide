#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QsciScintilla;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;


private slots: 
    void openDirectory();
    void keyPressEvent(QKeyEvent *event);
    //void onSidebarFileDoubleClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    QsciScintilla* m_editor = nullptr;
    QString m_currentFilePath;

    QTabWidget* m_tab = nullptr;
    // Qtab widget knows about widgets and tab titles, not about 
    // which file it is or if its been edited, we have to track that 
    // with two sepserate maps
    QMap<QsciScintilla*, QString> m_filePaths;
    QMap<QsciScintilla*, bool> m_modified;
    QsciScintilla* currentEditor() const;
    QsciScintilla* newEditorTab(const QString& title);
    void applyLexerFor(QsciScintilla* editor, const QString& filePath);
    void applyLexerForCurrentFile();
    void closeTab(int index);

    // for file saving/loading
    void openFile();
    bool saveFile(QsciScintilla* editor);
    bool saveFileAs(QsciScintilla* editor);

    // for project side bar
    void loadFileIntoEditor(const QString &filePath);

    QDockWidget *m_projectDock = nullptr;
    //QTreeView *m_projectTreeView = nullptr;
    //QFileSystemModel *m_fileModel = nullptr; 
    QPoint m_dragPosition; // stores offset
};
#endif // MAINWINDOW_H
