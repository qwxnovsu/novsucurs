#include "TeachersWidget.h"

#include <QLabel>
#include <QLayout>
#include "DatabaseManager/DatabaseManager.h"
#include "TeachersTableManager.h"
#include "Utils/Tracing.h"

TeachersScreen::TeachersScreen(DatabaseManager *dbManager, QWidget *parent)
    : BaseScreen(parent),
      m_dbManager(dbManager)
{
    TRACE_FUNCTION();
    setupUI();
}

void TeachersScreen::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *title = new QLabel("👨‍🏫 Управление преподавателями", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto widget = new TeachersTableManager(m_dbManager, this);
    widget->setTitleVisible(false);

    layout->addWidget(title);
    layout->addSpacing(20);
    layout->addWidget(widget);
    layout->addStretch();
}
