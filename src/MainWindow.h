#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMainWindow>
#include <QMap>
#include <QMenuBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include "AuthManager/AuthManager.h"
#include "Ui/BaseScreen.h"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(DatabaseManager *dbManager, int userId, const QString &userName, AuthManager::UserRole userRole, QWidget *parent = nullptr);
    ~MainWindow();

    void addScreen(const QString &screenId, BaseScreen *screen);
    void showScreen(const QString &screenId);

signals:
    void logoutRequested();

private slots:
    void onNavigationItemClicked(QListWidgetItem *item);
    void onLogout();

private:
    void setupUI();
    void setupMenuBar();
    void setupNavigation();
    void setupScreens();


    QSplitter *m_mainSplitter;
    QListWidget *m_navigationList;
    QStackedWidget *m_screenStack;


    int m_userId;
    QString m_userName;
    AuthManager::UserRole m_userRole;


    QMap<QString, BaseScreen *> m_screens;
    QMap<QString, QString> m_screenTitles;


    QAction *m_logoutAction;
    QAction *m_exitAction;

    DatabaseManager *m_dbManager;

    friend class AttendanceScreen;
    friend class StudentsScreen;
    friend class TeachersScreen;
    friend class ReportsScreen;
    friend class SettingsScreen;
};


#endif
