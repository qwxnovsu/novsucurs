#include <QDate>
#include "AddSubjectDialog.h"
#include "Utils/Tracing.h"

AddSubjectDialog::AddSubjectDialog(QWidget *parent)
    : QDialog(parent)
{
    TRACE_FUNCTION();

    setWindowTitle("Добавить предмет");
    setMinimumWidth(400);


    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Например: Прикладная математика");

    setupUI();
    connect(m_nameEdit, &QLineEdit::textChanged, this, &AddSubjectDialog::validateInput);

    m_okButton->setEnabled(false);
}

void AddSubjectDialog::setupUI()
{
    TRACE_FUNCTION();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    formLayout->addRow("Название предмета*:", m_nameEdit);

    mainLayout->addLayout(formLayout);

    QLabel *noteLabel = new QLabel(
        "* - обязательные поля",
        this);
    noteLabel->setStyleSheet("color: gray; font-size: 9pt; padding: 5px; border: 1px solid #eee; border-radius: 3px;");
    noteLabel->setWordWrap(true);
    mainLayout->addWidget(noteLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_okButton = new QPushButton("Добавить", this);
    m_cancelButton = new QPushButton("Отмена", this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(m_okButton, &QPushButton::clicked, this, &AddSubjectDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void AddSubjectDialog::validateInput()
{
    bool isValid = !m_nameEdit->text().trimmed().isEmpty();
    m_okButton->setEnabled(isValid);
}

void AddSubjectDialog::onAccept()
{
    TRACE_FUNCTION();

    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите название предмета.");
        m_nameEdit->setFocus();
        return;
    }

    m_subjectData.name = name;

    accept();
}

AddSubjectDialog::SubjectData AddSubjectDialog::getSubjectData() const
{
    return m_subjectData;
}
