#include <QMouseEvent>

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget* parent = nullptr) : QWidget(parent) {}

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event -> button() == Qt::LeftButton) {
            m_dragPos = event -> globalPosition().toPoint() - window() -> frameGeometry().topLeft();
            event -> accept();
        }
    }
    void mouseMoveEvent(QMouseEvent* event) override {
        if (event -> buttons() & Qt::LeftButton) {
            window() -> move(event -> globalPosition().toPoint() - m_dragPos);
            event -> accept();
        }
    }

private:
    QPoint m_dragPos;
};