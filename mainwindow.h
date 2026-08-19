#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QsciScintilla;
class QTreeView;
class QFileSystemModel;
class QDockWidget;

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
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots: 
    void openDirectory();
    void keyPressEvent(QKeyEvent *event);
    void onFileTreeDoubleClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    QsciScintilla* m_editor = nullptr;
    QString m_currentFilePath;

    // helper functions for constructor
    void tabInit();
    void plusButton();
    void actionLinks();
	void setupTitleBar();

    // editor functions
    QsciScintilla* currentEditor() const;
    QsciScintilla* newEditorTab(const QString& title);
    void applyLexerFor(QsciScintilla* editor, const QString& filePath);
    void applyLexerForCurrentFile();
    void closeTab(int index);
    void setupFileTree();
    void openProjectFolder(const QString& path);

    // for file saving/loading
    void openFile();
    bool saveFile(QsciScintilla* editor);
    bool saveFileAs(QsciScintilla* editor);

    // for project side bar
    void loadFileIntoEditor(const QString &filePath);

    QDockWidget *m_fileDock = nullptr;
    QTreeView *m_fileTree = nullptr;
    QFileSystemModel *m_fileModel = nullptr; 

    QTabWidget* m_tab = nullptr;
    // Qtab widget knows about widgets and tab titles, not about 
    // which file it is or if its been edited, we have to track that 
    // with two sepserate maps
    QMap<QsciScintilla*, QString> m_filePaths;
    QMap<QsciScintilla*, bool> m_modified;

    bool m_dragging = false;
    QPoint m_dragPosition;
};
#endif // MAINWINDOW_H