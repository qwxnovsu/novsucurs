#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>
#include "AttendanceScreen.h"
#include "Utils/Tracing.h"

AttendanceScreen::AttendanceScreen(DatabaseManager *dbManager, int _currentUserId, bool canEdit, QWidget *parent)
    : BaseScreen(parent),
      m_dbManager(dbManager),
      currentUserId(_currentUserId),
      m_canEdit(canEdit)
{
    TRACE_FUNCTION();
    setupUI();
    refreshData();
}

void AttendanceScreen::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    QLabel *title = new QLabel(m_canEdit ? "📝 Учет посещаемости" : "📝 Моя посещаемость", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    mainLayout->addWidget(title);

    QHBoxLayout *controlsLayout = new QHBoxLayout();

    QLabel *groupLabel = new QLabel("Группа:", this);
    m_groupCombo = new QComboBox(this);
    m_groupCombo->setMinimumWidth(150);

    QLabel *dateLabel = new QLabel("Дата:", this);
    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("dd.MM.yyyy");

    m_loadButton = new QPushButton("🔄 Загрузить занятия", this);

    controlsLayout->addWidget(groupLabel);
    controlsLayout->addWidget(m_groupCombo);
    controlsLayout->addSpacing(20);
    controlsLayout->addWidget(dateLabel);
    controlsLayout->addWidget(m_dateEdit);
    controlsLayout->addSpacing(20);
    controlsLayout->addWidget(m_loadButton);
    controlsLayout->addStretch();

    mainLayout->addLayout(controlsLayout);

    m_attendanceTable = new QTableWidget(this);
    m_attendanceTable->setAlternatingRowColors(true);
    m_attendanceTable->setSelectionMode(QAbstractItemView::NoSelection);
    mainLayout->addWidget(m_attendanceTable);

    QHBoxLayout *footerLayout = new QHBoxLayout();
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: green; font-weight: bold;");

    m_saveButton = new QPushButton("💾 Сохранить изменения", this);
    m_saveButton->setStyleSheet("background-color: #4CAF50; color: white; padding: 8px 16px; font-weight: bold;");
    m_saveButton->setVisible(m_canEdit);

    footerLayout->addWidget(m_statusLabel);
    footerLayout->addStretch();
    footerLayout->addWidget(m_saveButton);
    mainLayout->addLayout(footerLayout);

    connect(m_loadButton, &QPushButton::clicked, this, &AttendanceScreen::onLoadClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &AttendanceScreen::onSaveClicked);
}

void AttendanceScreen::refreshData()
{
    loadGroups();
}

void AttendanceScreen::loadGroups()
{
    m_groupCombo->clear();


    auto groups = m_dbManager->getAllGroupsForUser(currentUserId);
    for (const auto &group : groups) {
        m_groupCombo->addItem(group.name);
    }
}

void AttendanceScreen::onLoadClicked()
{
    TRACE_FUNCTION();
    QString groupName = m_groupCombo->currentText();
    QDate date = m_dateEdit->date();

    if (groupName.isEmpty())
        return;

    m_statusLabel->clear();
    m_attendanceTable->clear();


    m_currentLessons = m_dbManager->getLessonsForGroupAndDate(groupName, date);
    m_currentStudents = m_dbManager->getStudentsForGroup(groupName);

    if (!m_canEdit) {
        QVector<DatabaseManager::StudentShortInfo> currentStudentOnly;
        for (const auto &student : m_currentStudents) {
            if (student.userId == currentUserId) {
                currentStudentOnly.append(student);
                break;
            }
        }
        m_currentStudents = currentStudentOnly;
    }

    if (m_currentLessons.isEmpty()) {
        QMessageBox::information(this, "Информация", "На выбранную дату занятий не найдено.");
        m_attendanceTable->setRowCount(0);
        m_attendanceTable->setColumnCount(0);
        return;
    }

    if (m_currentStudents.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "В выбранной группе нет студентов.");
        return;
    }

    m_attendanceTable->setColumnCount(1 + m_currentLessons.size());
    m_attendanceTable->setRowCount(m_currentStudents.size());

    QStringList headers;
    headers << "Студент";

    for (const auto &lesson : m_currentLessons) {
        QString header = QString("%1\n%2")
                             .arg(lesson.subjectName)
                             .arg(lesson.startTime.toString("HH:mm"));
        headers << header;
    }
    m_attendanceTable->setHorizontalHeaderLabels(headers);

    for (int i = 0; i < headers.size(); ++i) {
        m_attendanceTable->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }

    for (int row = 0; row < m_currentStudents.size(); ++row) {
        const auto &student = m_currentStudents[row];

        QTableWidgetItem *nameItem = new QTableWidgetItem(
            QString("%1 %2").arg(student.secondName).arg(student.name));
        nameItem->setFlags(nameItem->flags() ^ Qt::ItemIsEditable);
        nameItem->setData(Qt::UserRole, QVariant::fromValue(student.id));
        m_attendanceTable->setItem(row, 0, nameItem);
    }

    for (int col = 0; col < m_currentLessons.size(); ++col) {
        const auto &lesson = m_currentLessons[col];
        auto attendanceMap = m_dbManager->getAttendanceForLesson(lesson.id);

        for (int row = 0; row < m_currentStudents.size(); ++row) {
            const auto &student = m_currentStudents[row];

            QWidget *cellWidget = new QWidget();
            QHBoxLayout *layout = new QHBoxLayout(cellWidget);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setAlignment(Qt::AlignCenter);

            QCheckBox *checkBox = new QCheckBox();

            bool isPresent = false;
            if (attendanceMap.contains(student.id)) {
                if (attendanceMap[student.id] == "present")
                    isPresent = true;
            }

            checkBox->setChecked(isPresent);
            checkBox->setEnabled(m_canEdit);

            layout->addWidget(checkBox);
            m_attendanceTable->setCellWidget(row, col + 1, cellWidget);
        }
    }
}

void AttendanceScreen::onSaveClicked()
{
    TRACE_FUNCTION();

    if (!m_canEdit) {
        QMessageBox::warning(this, "Доступ запрещен", "Текущий пользователь может только просматривать посещаемость.");
        return;
    }

    if (m_attendanceTable->rowCount() == 0)
        return;

    int updatedCount = 0;


    for (int col = 0; col < m_currentLessons.size(); ++col) {
        qint64 lessonId = m_currentLessons[col].id;


        for (int row = 0; row < m_currentStudents.size(); ++row) {
            qint64 studentId = m_attendanceTable->item(row, 0)->data(Qt::UserRole).toLongLong();

            QWidget *cellWidget = m_attendanceTable->cellWidget(row, col + 1);
            if (!cellWidget)
                continue;


            QCheckBox *checkBox = cellWidget->findChild<QCheckBox *>();
            if (checkBox) {
                QString status = checkBox->isChecked() ? "present" : "absent";
                if (m_dbManager->updateAttendance(lessonId, studentId, status)) {
                    updatedCount++;
                }
            }
        }
    }

    m_statusLabel->setText(QString("Сохранено записей: %1").arg(updatedCount));
    QMessageBox::information(this, "Успех", "Данные о посещаемости сохранены.");
}
