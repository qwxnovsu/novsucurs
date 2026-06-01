#ifndef ABSTRACTTABLEMANAGER_H
#define ABSTRACTTABLEMANAGER_H

#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHelpEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>

class SqlArrayDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    SqlArrayDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    QString displayText(const QVariant &value, const QLocale &locale) const override
    {
        Q_UNUSED(locale);

        return value.toString().remove("{").remove("}").remove("NULL");
    }
};

class CustomTableView : public QTableView
{
public:
    CustomTableView(QWidget *parent = nullptr)
        : QTableView(parent)
    {
    }

protected:
    bool viewportEvent(QEvent *event) override
    {
        if (event->type() == QEvent::ToolTip) {
            QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
            QModelIndex index = indexAt(helpEvent->pos());

            if (index.isValid()) {
                QString text = index.data(Qt::DisplayRole).toString();
                QToolTip::showText(helpEvent->globalPos(), text, viewport());
                return true;
            }
        }
        return QTableView::viewportEvent(event);
    }
};
class DatabaseManager;

class AbstractTableManager : public QWidget
{
    Q_OBJECT

public:
    explicit AbstractTableManager(DatabaseManager *dbManager, const QString &tableName, QWidget *parent = nullptr);
    virtual ~AbstractTableManager();

    virtual void refreshData();

    void setTitleVisible(bool visible)
    {
        m_titleLabel->setVisible(visible);
    }

protected:
    virtual void setupTableModel() = 0;
    virtual QString getDisplayName() const = 0;
    virtual QString getDescription() const = 0;


    virtual void setupUI();
    virtual void setupButtons();

    virtual bool addRecord() = 0;
    virtual bool editRecord(int row)
    {
        Q_UNUSED(row);
        return false;
    }
    virtual bool deleteRecord(int row) = 0;

    virtual void showError(const QString &message);
    virtual void showSuccess(const QString &message);


    DatabaseManager *m_dbManager;
    QSqlDatabase m_database;
    QString m_tableName;

    QTableView *m_tableView;
    QSqlTableModel *m_tableModel;

    QPushButton *m_addButton;
    QPushButton *m_deleteButton;
    QPushButton *m_refreshButton;

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_buttonLayout;
    QLabel *m_titleLabel;

private slots:
    void onAddClicked();
    void onDeleteClicked();
    void onRefreshClicked();

protected:
    void setupConnections();
};

#endif
