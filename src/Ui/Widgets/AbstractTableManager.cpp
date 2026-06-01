#include "AbstractTableManager.h"

#include <QGroupBox>
#include "DatabaseManager/DatabaseManager.h"
#include "Utils/Tracing.h"

AbstractTableManager::AbstractTableManager(DatabaseManager *dbManager, const QString &tableName, QWidget *parent)
    : QWidget(parent),
      m_dbManager(dbManager),
      m_tableName(tableName),
      m_tableView(nullptr),
      m_tableModel(nullptr)
{
    TRACE_FUNCTION();

    if (m_dbManager && m_dbManager->isConnected()) {
        m_database = m_dbManager->getDatabase();
    }
}

AbstractTableManager::~AbstractTableManager()
{
    TRACE_FUNCTION();

    if (m_tableModel) {
        m_tableModel->deleteLater();
    }
}

void AbstractTableManager::setupUI()
{
    TRACE_FUNCTION();

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);


    m_titleLabel = new QLabel(getDisplayName(), this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setStyleSheet("color: #2b579a; padding: 5px;");

    m_mainLayout->addWidget(m_titleLabel);

    m_tableView = new CustomTableView(this);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_mainLayout->addWidget(m_tableView, 1);


    QLabel *descriptionLabel = new QLabel(getDescription(), this);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setStyleSheet("color: #666; font-style: italic; padding: 10px; border: 1px solid #ddd; border-radius: 5px;");

    m_mainLayout->addWidget(descriptionLabel);


    setupButtons();
}


void AbstractTableManager::setupButtons()
{
    TRACE_FUNCTION();

    m_buttonLayout = new QHBoxLayout();

    m_addButton = new QPushButton("➕ Добавить", this);
    m_deleteButton = new QPushButton("🗑 Удалить", this);
    m_refreshButton = new QPushButton("🔄 Обновить", this);


    auto setupButton = [](QPushButton *button, const QString &color, const QString &hoverColor) {
        button->setMinimumHeight(35);
        button->setStyleSheet(
            QString(
                "QPushButton {"
                "    background-color: %1;"
                "    color: white;"
                "    border: none;"
                "    border-radius: 5px;"
                "    padding: 6px 12px;"
                "    font-weight: bold;"
                "}"
                "QPushButton:hover {"
                "    background-color: %2;"
                "}"
                "QPushButton:disabled {"
                "    background-color: #cccccc;"
                "}")
                .arg(color)
                .arg(hoverColor));
    };

    setupButton(m_addButton, "#4CAF50", "#45a049");
    setupButton(m_deleteButton, "#f44336", "#d32f2f");
    setupButton(m_refreshButton, "#2196F3", "#1976D2");

    m_deleteButton->setEnabled(false);

    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_addButton);
    m_buttonLayout->addWidget(m_deleteButton);
    m_buttonLayout->addWidget(m_refreshButton);

    m_mainLayout->addLayout(m_buttonLayout);
}

void AbstractTableManager::setupConnections()
{
    TRACE_FUNCTION();

    connect(m_addButton, &QPushButton::clicked, this, &AbstractTableManager::onAddClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &AbstractTableManager::onDeleteClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &AbstractTableManager::onRefreshClicked);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection &selected, const QItemSelection &deselected) {
        Q_UNUSED(deselected);
        bool hasSelection = !selected.isEmpty();
        m_deleteButton->setEnabled(hasSelection);
    });
}

void AbstractTableManager::refreshData()
{
    TRACE_FUNCTION();

    if (m_tableModel) {
        m_tableModel->select();
        INFO_MSG(QString("Table data refreshed: %1 rows").arg(m_tableModel->rowCount()));
    }
}

void AbstractTableManager::onAddClicked()
{
    TRACE_FUNCTION();

    if (!m_dbManager || !m_dbManager->isConnected()) {
        showError("Нет подключения к базе данных");
        return;
    }

    bool success = addRecord();

    if (success) {
        refreshData();
    }
}

void AbstractTableManager::onDeleteClicked()
{
    TRACE_FUNCTION();

    if (!m_dbManager || !m_dbManager->isConnected()) {
        showError("Нет подключения к базе данных");
        return;
    }

    QModelIndexList selectedIndexes = m_tableView->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) {
        showError("Не выбрана запись для удаления");
        return;
    }

    int row = selectedIndexes.first().row();
    bool success = deleteRecord(row);

    if (success) {
        refreshData();
    }
}

void AbstractTableManager::onRefreshClicked()
{
    TRACE_FUNCTION();
    refreshData();
}

void AbstractTableManager::showError(const QString &message)
{
    QMessageBox::critical(this, "Ошибка", message);
    ERROR_MSG(message);
}

void AbstractTableManager::showSuccess(const QString &message)
{
    QMessageBox::information(this, "Успех", message);
    INFO_MSG(message);
}
