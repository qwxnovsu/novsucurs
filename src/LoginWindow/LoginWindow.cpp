#include <QMessageBox>
#include "LoginWindow.h"
#include "MainWindow.h"
#include "Utils/Tracing.h"

LoginWindow::LoginWindow(QWidget *parent)
    : QMainWindow(parent)
{
    TRACE_FUNCTION();
    m_dbManager = new DatabaseManager(this);

    if (!m_dbManager->connectToDatabase("localhost", "curs3_db", "postgres", "postgres")) {
        QMessageBox::critical(this, "Ошибка базы данных", "Не удалось подключиться к базе данных. Проверьте параметры подключения.");
    }

    m_authManager = new AuthManager(m_dbManager, this);

    setupUI();
    setupConnections();

    setWindowTitle("Учебный процесс - Вход");
    resize(400, 300);
}

LoginWindow::~LoginWindow()
{
}

void LoginWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setAlignment(Qt::AlignCenter);


    m_titleLabel = new QLabel("Управление учебным процессом", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);


    m_emailLabel = new QLabel("Email:", this);
    m_emailEdit = new QLineEdit(this);
    m_emailEdit->setPlaceholderText("Введите email");
    m_emailEdit->setMinimumWidth(250);


    m_passwordLabel = new QLabel("Пароль:", this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("Введите пароль");
    m_passwordEdit->setEchoMode(QLineEdit::Password);


    m_emailEdit->setText("admin@university.com");
    m_passwordEdit->setText("admin123");


    m_loginButton = new QPushButton("Войти", this);
    m_loginButton->setMinimumWidth(100);


    m_mainLayout->addWidget(m_titleLabel);
    m_mainLayout->addSpacing(30);
    m_mainLayout->addWidget(m_emailLabel);
    m_mainLayout->addWidget(m_emailEdit);
    m_mainLayout->addWidget(m_passwordLabel);
    m_mainLayout->addWidget(m_passwordEdit);
    m_mainLayout->addSpacing(20);
    m_mainLayout->addWidget(m_loginButton, 0, Qt::AlignCenter);


    m_mainLayout->insertStretch(0, 1);
    m_mainLayout->addStretch(1);
}

void LoginWindow::setupConnections()
{
    connect(m_loginButton, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(m_authManager, &AuthManager::loginSuccess, this, &LoginWindow::onAuthSuccess);
    connect(m_authManager, &AuthManager::loginFailed, this, &LoginWindow::onAuthFailed);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
}

void LoginWindow::onLoginClicked()
{
    QString email = m_emailEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (email.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Ошибка ввода", "Введите email и пароль.");
        return;
    }

    m_loginButton->setEnabled(false);
    m_loginButton->setText("Вход...");

    m_authManager->login(email, password);
}

void LoginWindow::onAuthSuccess()
{
    TRACE_FUNCTION();


    m_mainWindow = new MainWindow(
        m_dbManager,
        m_authManager->getUserId(),
        m_authManager->getUserName(),
        m_authManager->getUserRole(),
        nullptr);

    connect(m_mainWindow, &MainWindow::logoutRequested, this, &LoginWindow::onMainWindowClosed);

    m_mainWindow->show();


    this->hide();

    INFO_MSG(QString("Login successful for user: %1 (ID: %2)")
                 .arg(m_authManager->getUserName())
                 .arg(m_authManager->getUserId()));


    m_emailEdit->clear();
    m_passwordEdit->clear();

    m_loginButton->setEnabled(true);
    m_loginButton->setText("Войти");
}


void LoginWindow::onMainWindowClosed()
{
    TRACE_FUNCTION();


    this->show();


    if (m_mainWindow) {
        m_mainWindow->deleteLater();
        m_mainWindow = nullptr;
    }


    m_authManager->logout();

    INFO_MSG("Returned to login screen");
}

void LoginWindow::onAuthFailed(const QString &error)
{
    QMessageBox::critical(this, "Ошибка входа", error);

    m_loginButton->setEnabled(true);
    m_loginButton->setText("Войти");
    m_passwordEdit->clear();
    m_passwordEdit->setFocus();
}
