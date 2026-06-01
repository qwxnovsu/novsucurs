#include "AddUserDialog.h"
#include "Utils/Tracing.h"

AddUserDialog::AddUserDialog(const QVector<QString> &availableRoles, QWidget *parent)
    : QDialog(parent)
{
    TRACE_FUNCTION();

    setWindowTitle("Добавить пользователя");
    setMinimumWidth(400);


    m_emailEdit = new QLineEdit(this);
    m_emailEdit->setPlaceholderText("example@domain.com");

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("Введите пароль");
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Имя");

    m_secondNameEdit = new QLineEdit(this);
    m_secondNameEdit->setPlaceholderText("Фамилия");

    m_thirdNameEdit = new QLineEdit(this);
    m_thirdNameEdit->setPlaceholderText("Отчество (необязательно)");

    m_roleComboBox = new QComboBox(this);
    for (const QString &role : availableRoles) {
        QString displayRole = role;
        if (role == "admin")
            displayRole = "Администратор";
        else if (role == "teacher")
            displayRole = "Преподаватель";
        else if (role == "student")
            displayRole = "Студент";

        m_roleComboBox->addItem(displayRole, role);
    }

    setupUI();


    connect(m_emailEdit, &QLineEdit::textChanged, this, &AddUserDialog::validateInput);
    connect(m_passwordEdit, &QLineEdit::textChanged, this, &AddUserDialog::validateInput);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &AddUserDialog::validateInput);
    connect(m_secondNameEdit, &QLineEdit::textChanged, this, &AddUserDialog::validateInput);


    m_okButton->setEnabled(false);
}

void AddUserDialog::setupUI()
{
    TRACE_FUNCTION();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);


    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    formLayout->addRow("Email*:", m_emailEdit);
    formLayout->addRow("Пароль*:", m_passwordEdit);
    formLayout->addRow("Имя*:", m_nameEdit);
    formLayout->addRow("Фамилия*:", m_secondNameEdit);
    formLayout->addRow("Отчество:", m_thirdNameEdit);
    formLayout->addRow("Роль*:", m_roleComboBox);

    mainLayout->addLayout(formLayout);


    QLabel *noteLabel = new QLabel("* - обязательные поля", this);
    noteLabel->setStyleSheet("color: gray; font-size: 10pt;");
    mainLayout->addWidget(noteLabel);


    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_okButton = new QPushButton("Добавить", this);
    m_cancelButton = new QPushButton("Отмена", this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);


    connect(m_okButton, &QPushButton::clicked, this, &AddUserDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void AddUserDialog::validateInput()
{
    bool isValid = !m_emailEdit->text().trimmed().isEmpty() &&
                   !m_passwordEdit->text().isEmpty() &&
                   !m_nameEdit->text().trimmed().isEmpty() &&
                   !m_secondNameEdit->text().trimmed().isEmpty() &&
                   m_roleComboBox->currentIndex() >= 0;

    m_okButton->setEnabled(isValid);
}

void AddUserDialog::onAccept()
{
    TRACE_FUNCTION();


    QString email = m_emailEdit->text().trimmed();
    if (!email.contains('@') || !email.contains('.')) {
        QMessageBox::warning(this, "Ошибка", "Введите корректный email адрес.");
        m_emailEdit->setFocus();
        return;
    }


    QString password = m_passwordEdit->text();
    if (password.length() < 3) {
        QMessageBox::warning(this, "Ошибка", "Пароль должен содержать минимум 3 символа.");
        m_passwordEdit->setFocus();
        return;
    }


    m_userData.email = email;
    m_userData.password = password;
    m_userData.name = m_nameEdit->text().trimmed();
    m_userData.secondName = m_secondNameEdit->text().trimmed();
    m_userData.thirdName = m_thirdNameEdit->text().trimmed();
    m_userData.role = m_roleComboBox->currentData().toString();

    accept();
}

AddUserDialog::UserData AddUserDialog::getUserData() const
{
    return m_userData;
}
