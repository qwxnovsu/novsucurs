#ifndef BASESCREEN_H
#define BASESCREEN_H

#include <QWidget>

class BaseScreen : public QWidget
{
    Q_OBJECT

public:
    explicit BaseScreen(QWidget *parent = nullptr);

    virtual ~BaseScreen() = default;

    virtual QString screenName() const = 0;
    virtual void refreshData()
    {}

signals:
    void navigationRequested(const QString &screenName);
};

#endif
