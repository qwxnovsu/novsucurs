#include <QDate>
#include "AddGroupDialog.h"
#include "Utils/Tracing.h"

AddGroupDialog::AddGroupDialog(QWidget *parent)
    : QDialog(parent)
{
    TRACE_FUNCTION();

    setWindowTitle("Добавить группу");
    setMinimumWidth(400);


    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Например: 3093");

    m_startYearSpinBox = new QSpinBox(this);
    m_startYearSpinBox->setRange(2000, QDate::currentDate().year());
    m_startYearSpinBox->setValue(QDate::currentDate().year());

    m_durationComboBox = new QComboBox(this);
    m_durationComboBox->addItem("4 года", 4);
    m_durationComboBox->addItem("5 лет", 5);
    m_durationComboBox->addItem("2 года (магистратура)", 2);
    m_durationComboBox->setCurrentIndex(0);

    setupUI();


    connect(m_nameEdit, &QLineEdit::textChanged, this, &AddGroupDialog::validateInput);


    m_okButton->setEnabled(false);
}

void AddGroupDialog::setupUI()
{
    TRACE_FUNCTION();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);


    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    formLayout->addRow("Название группы*:", m_nameEdit);
    formLayout->addRow("Год начала обучения*:", m_startYearSpinBox);
    formLayout->addRow("Длительность обучения*:", m_durationComboBox);

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


    connect(m_okButton, &QPushButton::clicked, this, &AddGroupDialog::onAccept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void AddGroupDialog::validateInput()
{
    bool isValid = !m_nameEdit->text().trimmed().isEmpty();
    m_okButton->setEnabled(isValid);
}

void AddGroupDialog::onAccept()
{
    TRACE_FUNCTION();


    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите название группы.");
        m_nameEdit->setFocus();
        return;
    }


    m_groupData.name = name;
    m_groupData.startYear = m_startYearSpinBox->value();
    m_groupData.durationYears = m_durationComboBox->currentData().toInt();

    accept();
}

AddGroupDialog::GroupData AddGroupDialog::getGroupData() const
{
    return m_groupData;
}
