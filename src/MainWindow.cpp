#include "MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include "Ui/Attendance/AttendanceScreen.h"
#include "Ui/Grades/GradesScreen.h"
#include "Ui/Reports/ReportsScreen.h"
#include "Ui/Schedule/ScheduleScreen.h"
#include "Ui/SettingsWidget/SettingsWidget.h"
#include "Ui/StudentsWidget/StudentsWidget.h"
#include "Ui/TeachersWidget/TeachersWidget.h"
#include "Utils/Tracing.h"


MainWindow::MainWindow(DatabaseManager *dbManager, int userId, const QString &userName, AuthManager::UserRole userRole, QWidget *parent)
    : QMainWindow(parent),
      m_userId(userId),
      m_userName(userName),
      m_userRole(userRole),
      m_dbManager(dbManager)
{
    TRACE_FUNCTION();
    m_dbManager->setCurrentUser(m_userId);

    setWindowTitle("Учебный процесс");
    setMinimumSize(1024, 768);

    setupUI();
    setupMenuBar();
    setupNavigation();
    setupScreens();

    INFO_MSG(QString("MainWindow created for user: %1 (ID: %2, Role: %3)")
                 .arg(userName)
                 .arg(userId)
                 .arg(static_cast<int>(userRole)));
}

MainWindow::~MainWindow()
{
    TRACE_FUNCTION();
}

void MainWindow::setupUI()
{
    TRACE_FUNCTION();

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_mainSplitter = new QSplitter(Qt::Horizontal, centralWidget);
    m_mainSplitter->setHandleWidth(1);
    m_mainSplitter->setChildrenCollapsible(false);

    QWidget *navWidget = new QWidget(m_mainSplitter);
    navWidget->setMinimumWidth(200);
    navWidget->setMaximumWidth(300);
    QVBoxLayout *navLayout = new QVBoxLayout(navWidget);
    navLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *navTitle = new QLabel("Меню", navWidget);
    QFont titleFont = navTitle->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    navTitle->setFont(titleFont);
    navTitle->setAlignment(Qt::AlignCenter);
    navTitle->setStyleSheet("background-color: #2b579a; color: white; padding: 10px;");
    navLayout->addWidget(navTitle);

    m_navigationList = new QListWidget(navWidget);
    m_navigationList->setSpacing(2);
    m_navigationList->setFrameShape(QFrame::NoFrame);
    m_navigationList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_navigationList, &QListWidget::itemClicked, this, &MainWindow::onNavigationItemClicked);
    navLayout->addWidget(m_navigationList);

    QGroupBox *userInfoBox = new QGroupBox("Пользователь", navWidget);
    userInfoBox->setAlignment(Qt::AlignCenter);
    userInfoBox->setStyleSheet("QGroupBox { font-weight: bold; }");
    QVBoxLayout *userLayout = new QVBoxLayout(userInfoBox);
    userLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    QLabel *userNameLabel = new QLabel(m_userName, userInfoBox);
    userNameLabel->setWordWrap(true);

    QString roleText;
    switch (m_userRole) {
    case AuthManager::Admin: roleText = "Администратор"; break;
    case AuthManager::Teacher: roleText = "Преподаватель"; break;
    case AuthManager::Student: roleText = "Студент"; break;
    default: roleText = "Неизвестно";
    }

    QLabel *userRoleLabel = new QLabel(roleText, userInfoBox);
    userRoleLabel->setStyleSheet("color: #666; font-size: 10pt;");

    userLayout->addWidget(userNameLabel);
    userLayout->addWidget(userRoleLabel);
    navLayout->addWidget(userInfoBox);

    m_screenStack = new QStackedWidget(m_mainSplitter);

    m_mainSplitter->addWidget(navWidget);
    m_mainSplitter->addWidget(m_screenStack);

    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(m_mainSplitter);
}

void MainWindow::setupMenuBar()
{
    TRACE_FUNCTION();


    QMenu *fileMenu = menuBar()->addMenu("&Файл");

    m_logoutAction = new QAction("&Выйти из системы", this);
    m_logoutAction->setShortcut(QKeySequence("Ctrl+L"));
    m_logoutAction->setStatusTip("Выйти из текущей учетной записи");
    connect(m_logoutAction, &QAction::triggered, this, &MainWindow::onLogout);
    fileMenu->addAction(m_logoutAction);

    fileMenu->addSeparator();

    m_exitAction = new QAction("&Выход", this);
    m_exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    m_exitAction->setStatusTip("Выйти из приложения");
    connect(m_exitAction, &QAction::triggered, qApp, &QApplication::quit);
    fileMenu->addAction(m_exitAction);


    QMenu *viewMenu = menuBar()->addMenu("&Вид");

    QAction *toggleNavAction = new QAction("&Показать/скрыть панель навигации", this);
    toggleNavAction->setShortcut(QKeySequence("Ctrl+N"));
    toggleNavAction->setCheckable(true);
    toggleNavAction->setChecked(true);
    connect(toggleNavAction, &QAction::toggled, [this](bool checked) {
        m_mainSplitter->widget(0)->setVisible(checked);
    });
    viewMenu->addAction(toggleNavAction);

    QMenu *helpMenu = menuBar()->addMenu("&Справка");

    QAction *aboutAction = new QAction("&О программе", this);
    connect(aboutAction, &QAction::triggered, []() {
        QMessageBox::about(nullptr, "О программе", "Информационная система управления учебным процессом\n"
                                                   "Версия 1.0.0\n\n"
                                                   "Разработано для учета расписания, посещаемости,\n"
                                                   "оценок, отчетности и автоматической проверки зачета.");
    });
    helpMenu->addAction(aboutAction);
}

void MainWindow::setupNavigation()
{
    TRACE_FUNCTION();

    m_navigationList->clear();

    QStringList commonItems;

    switch (m_userRole) {
    case AuthManager::Admin:
        commonItems << "schedule:📅 Расписание"
                    << "attendance:📝 Посещаемость"
                    << "grades:✅ Оценки"
                    << "students:👨‍🎓 Студенты"
                    << "teachers:👨‍🏫 Преподаватели"
                    << "reports:📈 Отчеты"
                    << "settings:⚙ Настройки";
        break;

    case AuthManager::Teacher:
        commonItems << "schedule:📅 Расписание"
                    << "attendance:📝 Посещаемость"
                    << "grades:✅ Оценки"
                    << "students:👨‍🎓 Студенты";
        break;

    case AuthManager::Student:
        commonItems << "schedule:📅 Расписание"
                    << "attendance:📝 Моя посещаемость"
                    << "grades:✅ Мои оценки";
        break;

    default:
        break;
    }

    for (const QString &item : commonItems) {
        QStringList parts = item.split(':');
        if (parts.size() == 2) {
            QString screenId = parts[0];
            QString displayName = parts[1];

            QListWidgetItem *listItem = new QListWidgetItem(displayName, m_navigationList);
            listItem->setData(Qt::UserRole, screenId);
            listItem->setSizeHint(QSize(0, 40));
            listItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);


            QString style = QString(
                "QListWidget::item {"
                "    padding: 10px;"
                "    border-bottom: 1px solid #eee;"
                "}"
                "QListWidget::item:selected {"
                "    background-color: #2b579a;"
                "    color: white;"
                "}"
                "QListWidget::item:hover:!selected {"
                "    background-color: #f0f0f0;"
                "}");
            listItem->setData(Qt::UserRole + 1, style);
        }
    }

    m_navigationList->setStyleSheet(
        "QListWidget {"
        "    background-color: white;"
        "    border: none;"
        "}"
        "QListWidget::item {"
        "    padding: 10px;"
        "    border-bottom: 1px solid #eee;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #2b579a;"
        "    color: white;"
        "}"
        "QListWidget::item:hover:!selected {"
        "    background-color: #f0f0f0;"
        "}");
}

void MainWindow::setupScreens()
{
    TRACE_FUNCTION();

    switch (m_userRole) {
    case AuthManager::Admin:
        addScreen("schedule", new ScheduleScreen(m_dbManager, m_userId, true, this));
        addScreen("attendance", new AttendanceScreen(m_dbManager, m_userId, true, this));
        addScreen("grades", new GradesScreen(m_dbManager, m_userId, true, this));
        addScreen("students", new StudentsScreen(m_dbManager, m_userId, this));
        addScreen("teachers", new TeachersScreen(m_dbManager, this));
        addScreen("reports", new ReportsScreen(m_dbManager, this));
        addScreen("settings", new SettingsWidget(m_dbManager, this));
        break;

    case AuthManager::Teacher:
        addScreen("schedule", new ScheduleScreen(m_dbManager, m_userId, false, this));
        addScreen("attendance", new AttendanceScreen(m_dbManager, m_userId, true, this));
        addScreen("grades", new GradesScreen(m_dbManager, m_userId, true, this));
        addScreen("students", new StudentsScreen(m_dbManager, m_userId, this));
        break;

    case AuthManager::Student:

        addScreen("schedule", new ScheduleScreen(m_dbManager, m_userId, false, this));
        addScreen("attendance", new AttendanceScreen(m_dbManager, m_userId, false, this));
        addScreen("grades", new GradesScreen(m_dbManager, m_userId, false, this));
        break;

    default:
        break;
    }
}

void MainWindow::addScreen(const QString &screenId, BaseScreen *screen)
{
    TRACE_FUNCTION();

    if (m_screens.contains(screenId)) {
        WARNING_MSG(QString("Screen %1 already exists").arg(screenId));
        return;
    }

    m_screens[screenId] = screen;
    m_screenStack->addWidget(screen);

    DEBUG_MSG(QString("Screen added: %1").arg(screenId));
}

void MainWindow::showScreen(const QString &screenId)
{
    TRACE_FUNCTION();

    if (!m_screens.contains(screenId)) {
        ERROR_MSG(QString("Screen not found: %1").arg(screenId));
        return;
    }

    BaseScreen *screen = m_screens[screenId];
    m_screenStack->setCurrentWidget(screen);


    for (int i = 0; i < m_navigationList->count(); ++i) {
        QListWidgetItem *item = m_navigationList->item(i);
        if (item->data(Qt::UserRole).toString() == screenId) {
            m_navigationList->setCurrentItem(item);
            break;
        }
    }


    setWindowTitle(QString("Учебный процесс - %1").arg(screen->screenName()));


    screen->refreshData();

    INFO_MSG(QString("Switched to screen: %1").arg(screenId));
}

void MainWindow::onNavigationItemClicked(QListWidgetItem *item)
{
    TRACE_FUNCTION();

    QString screenId = item->data(Qt::UserRole).toString();
    showScreen(screenId);
}

void MainWindow::onLogout()
{
    TRACE_FUNCTION();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Выход из системы",
        "Вы уверены, что хотите выйти из системы?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        INFO_MSG("User logging out");
        emit logoutRequested();
        this->close();
    }
}
