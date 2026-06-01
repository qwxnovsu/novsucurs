#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include "AuthManager/AuthManager.h"
#include "DatabaseManager/DatabaseManager.h"

class MainWindow;

class LoginWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

signals:
    void loginSuccessful(int userId, const QString &userName, AuthManager::UserRole role);

private slots:
    void onLoginClicked();
    void onAuthSuccess();
    void onMainWindowClosed();
    void onAuthFailed(const QString &error);

private:
    void setupUI();
    void setupConnections();

    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;

    QLabel *m_titleLabel;
    QLabel *m_emailLabel;
    QLineEdit *m_emailEdit;
    QLabel *m_passwordLabel;
    QLineEdit *m_passwordEdit;
    QPushButton *m_loginButton;

    DatabaseManager *m_dbManager;
    AuthManager *m_authManager;
    MainWindow *m_mainWindow;
};

#endif
