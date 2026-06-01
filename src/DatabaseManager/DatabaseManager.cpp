#include "DatabaseManager.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDate>
#include <QFile>
#include <QRandomGenerator>
#include <QStringList>
#include <QTextStream>
#include "Utils/Tracing.h"

namespace
{
const QString PASSWORD_HASH_PREFIX = "sha256";

QString generatePasswordSalt()
{
    QByteArray salt;
    salt.reserve(16);
    for (int i = 0; i < 4; ++i) {
        const quint32 value = QRandomGenerator::global()->generate();
        salt.append(static_cast<char>((value >> 24) & 0xff));
        salt.append(static_cast<char>((value >> 16) & 0xff));
        salt.append(static_cast<char>((value >> 8) & 0xff));
        salt.append(static_cast<char>(value & 0xff));
    }
    return QString::fromLatin1(salt.toHex());
}

QString calculatePasswordHash(const QString &password, const QString &salt)
{
    QByteArray payload = salt.toUtf8();
    payload.append(':');
    payload.append(password.toUtf8());
    return QString::fromLatin1(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex());
}

QString makePasswordHash(const QString &password)
{
    const QString salt = generatePasswordSalt();
    return PASSWORD_HASH_PREFIX + "$" + salt + "$" + calculatePasswordHash(password, salt);
}

bool verifyPasswordHash(const QString &password, const QString &storedPassword)
{
    const QStringList parts = storedPassword.split('$');
    if (parts.size() != 3 || parts.at(0) != PASSWORD_HASH_PREFIX)
        return false;

    return calculatePasswordHash(password, parts.at(1)) == parts.at(2);
}

bool isPasswordHash(const QString &storedPassword)
{
    return storedPassword.startsWith(PASSWORD_HASH_PREFIX + "$");
}
}

QList<int> parseIntArray(const QString &pgArray)
{
    QList<int> result;


    QString clean = pgArray;
    clean = clean.remove('{').remove('}').trimmed();

    if (!clean.isEmpty()) {
        QStringList parts = clean.split(',');
        for (const QString &part : parts) {
            bool ok;
            int value = part.trimmed().toInt(&ok);
            if (ok) {
                result.append(value);
            }
        }
    }

    return result;
}

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent),
      m_connected(false)
{
}

DatabaseManager::~DatabaseManager()
{
    disconnect();
}

bool DatabaseManager::connectToDatabase(const QString &host,
                                        const QString &databaseName,
                                        const QString &username,
                                        const QString &password,
                                        int port)
{
    m_database = QSqlDatabase::addDatabase("QPSQL");
    m_database.setHostName(host);
    m_database.setDatabaseName(databaseName);
    m_database.setUserName(username);
    m_database.setPassword(password);
    m_database.setPort(port);

    if (!m_database.open()) {
        qDebug() << "Database connection error:" << m_database.lastError().text();
        m_connected = false;
        return false;
    }

    m_connected = true;
    qDebug() << "Database connected successfully";


    return true;
}


bool DatabaseManager::loadDataFromSqlFile(const QString &filePath)
{
    TRACE_FUNCTION();

    QFile file(filePath);

    if (!file.exists()) {
        qWarning() << "SQL file not found:" << filePath;
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open SQL file:" << file.errorString();
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    QString sqlContent = stream.readAll();
    file.close();


    QStringList sqlQueries;
    QString currentQuery;
    bool inString = false;
    bool inComment = false;
    bool inFunction = false;
    char stringChar = '\0';

    for (int i = 0; i < sqlContent.length(); ++i) {
        QChar ch = sqlContent[i];
        QChar nextCh = (i + 1 < sqlContent.length()) ? sqlContent[i + 1] : QChar();


        if (!inString && !inFunction && ch == '-' && nextCh == '-') {
            inComment = true;
            currentQuery += ch;
            i++;
            continue;
        }

        if (inComment) {
            currentQuery += ch;
            if (ch == '\n') {
                inComment = false;
            }
            continue;
        }


        if (!inComment && (ch == '\'' || ch == '"')) {
            if (!inString) {
                inString = true;
                stringChar = ch.toLatin1();
            }
            else if (ch == stringChar) {
                if (i > 0 && sqlContent[i - 1] != '\\') {
                    inString = false;
                }
            }
        }


        if (!inString && !inComment &&
            sqlContent.mid(i, 8).compare("$$ LANGUAGE", Qt::CaseInsensitive) == 0) {
            inFunction = false;
        }

        if (!inString && !inComment &&
            sqlContent.mid(i, 18).compare("CREATE OR REPLACE", Qt::CaseInsensitive) == 0) {
            int j = i;
            while (j < sqlContent.length() && sqlContent[j] != 'A' &&
                   sqlContent.mid(j, 3).compare("AS ", Qt::CaseInsensitive) != 0) {
                j++;
            }
            if (j < sqlContent.length() && sqlContent[j] == 'A') {
                j += 3;

                while (j < sqlContent.length() && sqlContent[j] != '$')
                    j++;
                if (j < sqlContent.length()) {
                    inFunction = true;
                }
            }
        }

        currentQuery += ch;


        if (!inString && !inComment && !inFunction && ch == ';') {
            sqlQueries.append(currentQuery.trimmed());
            currentQuery.clear();
        }
    }


    if (!currentQuery.trimmed().isEmpty()) {
        sqlQueries.append(currentQuery.trimmed());
    }

    QSqlDatabase::database().transaction();

    try {
        for (const QString &queryStr : sqlQueries) {
            QString trimmedQuery = queryStr.trimmed();
            if (trimmedQuery.isEmpty()) {
                continue;
            }

            QSqlQuery query(m_database);
            if (!query.exec(trimmedQuery)) {
                qWarning() << "Failed to execute SQL query:";
                qWarning() << "Query:" << trimmedQuery.left(200) << "...";
                qWarning() << "Error:" << query.lastError().text();
                QSqlDatabase::database().rollback();
                return false;
            }
        }

        QSqlDatabase::database().commit();
        qDebug() << "Data loaded successfully from SQL file:" << filePath;
        return true;
    }
    catch (...) {
        QSqlDatabase::database().rollback();
        ERROR_MSG("Transaction failed while loading SQL file");
        return false;
    }
}

bool DatabaseManager::isConnected() const
{
    return m_connected;
}

QSqlDatabase DatabaseManager::getDatabase() const
{
    return m_database;
}

QSqlQuery DatabaseManager::executeQuery(const QString &query)
{
    QSqlQuery sqlQuery(m_database);
    if (!sqlQuery.exec(query)) {
        qDebug() << "Query execution error:" << sqlQuery.lastError().text();
        qDebug() << "Query:" << query;
    }
    return sqlQuery;
}

bool DatabaseManager::executeTransaction(const QStringList &queries)
{
    m_database.transaction();

    for (const QString &query : queries) {
        QSqlQuery sqlQuery(m_database);
        if (!sqlQuery.exec(query)) {
            m_database.rollback();
            qDebug() << "Transaction error:" << sqlQuery.lastError().text();
            return false;
        }
    }

    return m_database.commit();
}

bool DatabaseManager::authenticateUser(const QString &email, const QString &password, int &userId, QString &userName, QString &role)
{
    QSqlQuery query(m_database);
    query.prepare(
        "SELECT u.id, u.name, u.second_name, r.type, u.password "
        "FROM users u "
        "JOIN roles r ON u.role_id = r.id "
        "WHERE u.email = :email");
    query.bindValue(":email", email);

    if (!query.exec()) {
        qDebug() << "Authentication query error:" << query.lastError().text();
        return false;
    }

    if (!query.next())
        return false;

    const QString storedPassword = query.value(4).toString();
    bool passwordMatches = false;

    if (isPasswordHash(storedPassword)) {
        passwordMatches = verifyPasswordHash(password, storedPassword);
    }
    else {
        passwordMatches = (storedPassword == password);
    }

    if (!passwordMatches)
        return false;

    userId = query.value(0).toInt();
    userName = query.value(1).toString() + " " + query.value(2).toString();
    role = query.value(3).toString();

    if (!isPasswordHash(storedPassword)) {
        QSqlQuery updateQuery(m_database);
        updateQuery.prepare("UPDATE users SET password = :password WHERE id = :id");
        updateQuery.bindValue(":password", makePasswordHash(password));
        updateQuery.bindValue(":id", userId);
        if (!updateQuery.exec()) {
            qDebug() << "Password hash migration error:" << updateQuery.lastError().text();
        }
    }

    return true;
}

void DatabaseManager::disconnect()
{
    if (m_database.isOpen()) {
        m_database.close();
        m_connected = false;
    }
}

bool DatabaseManager::tableIsEmpty(const QString &tableName)
{
    QSqlQuery query(QString("SELECT COUNT(*) FROM %1").arg(tableName));
    if (query.exec() && query.next()) {
        return query.value(0).toInt() == 0;
    }
    return true;
}

DatabaseManager::UserInfo DatabaseManager::UserInfo::fromQuery(QSqlQuery const &query)
{
    UserInfo user;
    user.id = query.value("id").toLongLong();
    user.email = query.value("email").toString();
    user.name = query.value("name").toString();
    user.secondName = query.value("second_name").toString();
    user.thirdName = query.value("third_name").toString();
    user.role = query.value("type").toString();

    return user;
}

QVector<DatabaseManager::UserInfo> DatabaseManager::getAllUsers()
{
    TRACE_FUNCTION();

    QVector<UserInfo> users;

    QString queryStr =
        "SELECT u.id, u.email, u.name, u.second_name, u.third_name, r.type "
        "FROM users u "
        "JOIN roles r ON u.role_id = r.id "
        "ORDER BY r.id, u.second_name, u.name";

    QSqlQuery query = executeQuery(queryStr);

    while (query.next()) {
        users.append(UserInfo::fromQuery(query));
    }

    DEBUG_MSG(QString("Retrieved %1 users").arg(users.size()));
    return users;
}

bool DatabaseManager::addUser(const QString &email, const QString &password, const QString &name, const QString &secondName, const QString &thirdName, const QString &role)
{
    TRACE_FUNCTION();

    QSqlDatabase::database().transaction();

    try {
        QSqlQuery emailCheck(m_database);
        emailCheck.prepare("SELECT COUNT(*) FROM users WHERE email = :email");
        emailCheck.bindValue(":email", email);

        if (!emailCheck.exec() || !emailCheck.next()) {
            ERROR_MSG("Failed to check email uniqueness");
            QSqlDatabase::database().rollback();
            return false;
        }

        if (emailCheck.value(0).toInt() > 0) {
            ERROR_MSG(QString("Email already exists: %1").arg(email));
            QSqlDatabase::database().rollback();
            return false;
        }


        QSqlQuery roleQuery(m_database);
        roleQuery.prepare("SELECT id FROM roles WHERE type = :role");
        roleQuery.bindValue(":role", role.toLower());

        if (!roleQuery.exec() || !roleQuery.next()) {
            ERROR_MSG(QString("Role not found: %1").arg(role));
            QSqlDatabase::database().rollback();
            return false;
        }

        qint64 roleId = roleQuery.value(0).toLongLong();

        QSqlQuery insertQuery(m_database);
        insertQuery.prepare(
            "INSERT INTO users (id,role_id, email, password, name, second_name, third_name) "
            "VALUES (DEFAULT, :role_id, :email, :password, :name, :second_name, :third_name) "
            "RETURNING id");

        insertQuery.bindValue(":role_id", roleId);
        insertQuery.bindValue(":email", email);
        insertQuery.bindValue(":password", makePasswordHash(password));
        insertQuery.bindValue(":name", name);
        insertQuery.bindValue(":second_name", secondName);
        insertQuery.bindValue(":third_name", thirdName);

        if (!insertQuery.exec() || !insertQuery.next()) {
            ERROR_MSG(QString("Failed to add user: %1").arg(insertQuery.lastError().text()));
            QSqlDatabase::database().rollback();
            return false;
        }

        qint64 newUserId = insertQuery.value(0).toLongLong();


        QSqlDatabase::database().commit();
        INFO_MSG(QString("User added successfully: %1 %2 (%3), ID: %4")
                     .arg(name)
                     .arg(secondName)
                     .arg(email)
                     .arg(newUserId));
        return true;
    }
    catch (...) {
        QSqlDatabase::database().rollback();
        ERROR_MSG("Transaction failed");
        return false;
    }
}

bool DatabaseManager::deleteUser(int userId)
{
    TRACE_FUNCTION();

    QSqlDatabase::database().transaction();

    try {
        QSqlQuery checkQuery(m_database);
        checkQuery.prepare("SELECT id, role_id FROM users WHERE id = :id");
        checkQuery.bindValue(":id", userId);

        if (!checkQuery.exec() || !checkQuery.next()) {
            ERROR_MSG(QString("User not found: %1").arg(userId));
            QSqlDatabase::database().rollback();
            return false;
        }

        QSqlQuery deleteQuery(m_database);
        deleteQuery.prepare("DELETE FROM users WHERE id = :id");
        deleteQuery.bindValue(":id", userId);

        if (!deleteQuery.exec()) {
            ERROR_MSG(QString("Failed to delete user: %1").arg(deleteQuery.lastError().text()));
            QSqlDatabase::database().rollback();
            return false;
        }

        QSqlDatabase::database().commit();
        INFO_MSG(QString("User deleted successfully: ID %1").arg(userId));
        return true;
    }
    catch (...) {
        QSqlDatabase::database().rollback();
        ERROR_MSG("Transaction failed");
        return false;
    }
}

QVector<QString> DatabaseManager::getAvailableRoles()
{
    TRACE_FUNCTION();

    QVector<QString> roles;

    QSqlQuery query("SELECT type FROM roles ORDER BY id");
    while (query.next()) {
        roles.append(query.value(0).toString());
    }

    return roles;
}

QVector<DatabaseManager::GroupInfo> DatabaseManager::getAllGroups()
{
    TRACE_FUNCTION();

    QVector<GroupInfo> groups;

    QString queryStr = "SELECT name, start_year, duration_years FROM groups ORDER BY name";
    QSqlQuery query = executeQuery(queryStr);

    while (query.next()) {
        GroupInfo group;
        group.name = query.value(0).toString();
        group.startYear = query.value(1).toInt();
        group.durationYears = query.value(2).toInt();
        groups.append(group);
    }

    DEBUG_MSG(QString("Retrieved %1 groups").arg(groups.size()));
    return groups;
}

QVector<DatabaseManager::GroupInfo> DatabaseManager::getAllGroupsForUser(int userId)
{
    TRACE_FUNCTION();

    QVector<GroupInfo> groups;

    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM get_available_groups_for_user(:user_id);");
    query.bindValue(":user_id", userId);


    if (!query.exec()) {
        ERROR_MSG(query.lastError());
        return groups;
    }

    while (query.next()) {
        GroupInfo group;
        group.name = query.value(0).toString();
        group.startYear = query.value(1).toInt();
        group.durationYears = query.value(2).toInt();
        groups.append(group);
    }

    DEBUG_MSG(QString("Retrieved %1 groups").arg(groups.size()));
    return groups;
}
bool DatabaseManager::addGroup(const QString &name, int startYear, int durationYears)
{
    TRACE_FUNCTION();

    QSqlDatabase::database().transaction();

    try {
        QSqlQuery nameCheck(m_database);
        nameCheck.prepare("SELECT COUNT(*) FROM groups WHERE name = :name");
        nameCheck.bindValue(":name", name);

        if (!nameCheck.exec() || !nameCheck.next()) {
            ERROR_MSG("Failed to check group name uniqueness");
            QSqlDatabase::database().rollback();
            return false;
        }

        if (nameCheck.value(0).toInt() > 0) {
            ERROR_MSG(QString("Group already exists: %1").arg(name));
            QSqlDatabase::database().rollback();
            return false;
        }

        QSqlQuery insertQuery(m_database);
        insertQuery.prepare(
            "INSERT INTO groups (name, start_year, duration_years) "
            "VALUES (:name, :start_year, :duration_years)");

        insertQuery.bindValue(":name", name);
        insertQuery.bindValue(":start_year", startYear);
        insertQuery.bindValue(":duration_years", durationYears);

        if (!insertQuery.exec()) {
            ERROR_MSG(QString("Failed to add group: %1").arg(insertQuery.lastError().text()));
            QSqlDatabase::database().rollback();
            return false;
        }

        QSqlDatabase::database().commit();
        INFO_MSG(QString("Group added successfully: %1 (start: %2, duration: %3)")
                     .arg(name)
                     .arg(startYear)
                     .arg(durationYears));
        return true;
    }
    catch (...) {
        QSqlDatabase::database().rollback();
        ERROR_MSG("Transaction failed");
        return false;
    }
}

bool DatabaseManager::deleteGroup(const QString &groupName)
{
    TRACE_FUNCTION();

    QSqlDatabase::database().transaction();

    try {
        QSqlQuery deleteQuery(m_database);
        deleteQuery.prepare("DELETE FROM groups WHERE name = :name");
        deleteQuery.bindValue(":name", groupName);

        if (!deleteQuery.exec()) {
            ERROR_MSG(QString("Failed to delete group: %1").arg(deleteQuery.lastError().text()));
            QSqlDatabase::database().rollback();
            return false;
        }

        QSqlDatabase::database().commit();
        INFO_MSG(QString("Group deleted successfully: %1").arg(groupName));
        return true;
    }
    catch (...) {
        QSqlDatabase::database().rollback();
        ERROR_MSG("Transaction failed");
        return false;
    }
}

QVector<DatabaseManager::SubjectInfo> DatabaseManager::getAllSubjects()
{
    TRACE_FUNCTION();

    QVector<SubjectInfo> subjects;

    QString queryStr = "SELECT name FROM subjects ORDER BY name";
    QSqlQuery query = executeQuery(queryStr);

    while (query.next()) {
        SubjectInfo subject;
        subject.name = query.value(0).toString();
        subjects.append(subject);
    }

    DEBUG_MSG(QString("Retrieved %1 subjects").arg(subjects.size()));
    return subjects;
}

QVector<DatabaseManager::SubjectRef> DatabaseManager::getSubjectRefs()
{
    TRACE_FUNCTION();

    QVector<SubjectRef> subjects;

    QString queryStr = "SELECT id, name FROM subjects ORDER BY name";
    QSqlQuery query = executeQuery(queryStr);

    while (query.next()) {
        SubjectRef subject;
        subject.id = query.value(0).toLongLong();
        subject.name = query.value(1).toString();
        subjects.append(subject);
    }

    return subjects;
}

bool DatabaseManager::addSubject(const QString &name)
{
    TRACE_FUNCTION();

    QSqlDatabase::database().transaction();

    try {
        QSqlQuery insertQuery(m_database);
        insertQuery.prepare(
            "INSERT INTO subjects (name) "
            "VALUES (:name)");

        insertQuery.bindValue(":name", name);

        if (!insertQuery.exec()) {
            ERROR_MSG(QString("Failed to add subject: %1").arg(insertQuery.lastError().text()));
            QSqlDatabase::database().rollback();
            return false;
        }

        QSqlDatabase::database().commit();
        INFO_MSG(QString("Subject added successfully: %1 ")
                     .arg(name));
        return true;
    }
    catch (...) {
        QSqlDatabase::database().rollback();
        ERROR_MSG("Transaction failed");
        return false;
    }
}

bool DatabaseManager::deleteSubject(const QString &subjectName)
{
    TRACE_FUNCTION();

    QSqlDatabase::database().transaction();

    try {
        QSqlQuery deleteQuery(m_database);
        deleteQuery.prepare("DELETE FROM subjects WHERE name = :name");
        deleteQuery.bindValue(":name", subjectName);

        if (!deleteQuery.exec()) {
            ERROR_MSG(QString("Failed to delete subject: %1").arg(deleteQuery.lastError().text()));
            QSqlDatabase::database().rollback();
            return false;
        }

        QSqlDatabase::database().commit();
        INFO_MSG(QString("Subject deleted successfully: %1").arg(subjectName));
        return true;
    }
    catch (...) {
        QSqlDatabase::database().rollback();
        ERROR_MSG("Transaction failed");
        return false;
    }
}

QVector<DatabaseManager::TeacherInfo> DatabaseManager::getAllTeachers()
{
    TRACE_FUNCTION();

    QVector<TeacherInfo> teachers;

    QString queryStr = "SELECT * FROM teacher_view ORDER BY second_name, name";
    QSqlQuery query = executeQuery(queryStr);

    while (query.next()) {
        teachers.append(TeacherInfo::fromQuery(query));
    }

    DEBUG_MSG(QString("Retrieved %1 teachers from view").arg(teachers.size()));
    return teachers;
}

DatabaseManager::TeacherInfo
DatabaseManager::TeacherInfo::fromQuery(QSqlQuery const &query)
{
    TeacherInfo teacher;
    teacher.user = UserInfo::fromQuery(query);
    teacher.subject_ids = parseIntArray(query.value("subjects_ids").toString()).toVector();
    teacher.subjects = query.value("subject_names").toStringList();
    return teacher;
}

QVector<DatabaseManager::StudentInfo> DatabaseManager::getAllStudents()
{
    TRACE_FUNCTION();

    QVector<StudentInfo> students;

    QString queryStr = "SELECT * FROM student_view ORDER BY second_name, name";
    QSqlQuery query = executeQuery(queryStr);

    while (query.next()) {
        students.append(StudentInfo::fromQuery(query));
    }

    DEBUG_MSG(QString("Retrieved %1 students from view").arg(students.size()));
    return students;
}

DatabaseManager::StudentInfo
DatabaseManager::StudentInfo::fromQuery(QSqlQuery const &query)
{
    StudentInfo student;
    student.user = UserInfo::fromQuery(query);
    student.group_name = query.value("group_name").toString();
    student.duration_years = query.value("duration_years").toInt();
    student.start_year = query.value("start_year").toInt();
    return student;
}

QVector<DatabaseManager::LessonInfo> DatabaseManager::getLessonsForGroupAndDate(const QString &groupName, const QDate &date)
{
    TRACE_FUNCTION();
    QVector<LessonInfo> lessons;

    QString queryStr =
        "SELECT "
        "    l.id AS lesson_id, "
        "    l.subject_id, "
        "    COALESCE(l.teacher_id, 0) AS teacher_id, "
        "    l.group_name, "
        "    s.name AS subject_name, "
        "    l.lesson_date_start AS start_time, "
        "    l.lesson_duration_hours AS duration_hours, "
        "    COALESCE(u.second_name || ' ' || u.name, 'Не назначен') AS teacher_name "
        "FROM lessons l "
        "JOIN subjects s ON l.subject_id = s.id "
        "LEFT JOIN users u ON l.teacher_id = u.id "
        "WHERE l.group_name = :group_name "
        "  AND l.lesson_date_start::date = :date "
        "ORDER BY l.lesson_date_start ASC";

    QSqlQuery query(m_database);
    query.prepare(queryStr);
    query.bindValue(":group_name", groupName);
    query.bindValue(":date", date);

    if (query.exec()) {
        while (query.next()) {
            LessonInfo info;

            info.id = query.value("lesson_id").toLongLong();
            info.subjectId = query.value("subject_id").toLongLong();
            info.teacherId = query.value("teacher_id").toLongLong();
            info.groupName = query.value("group_name").toString();
            info.subjectName = query.value("subject_name").toString();
            info.startTime = query.value("start_time").toDateTime();
            info.durationHours = query.value("duration_hours").toInt();
            info.teacherName = query.value("teacher_name").toString();

            lessons.append(info);
        }
    }
    else {
        ERROR_MSG(QString("Failed to get lessons via function: %1").arg(query.lastError().text()));
    }

    return lessons;
}

bool DatabaseManager::addLesson(qint64 subjectId, qint64 teacherId, const QString &groupName, const QDateTime &startTime, int durationHours)
{
    TRACE_FUNCTION();

    QSqlQuery query(m_database);
    query.prepare(
        "INSERT INTO lessons (subject_id, teacher_id, group_name, lesson_date_start, lesson_duration_hours) "
        "VALUES (:subject_id, :teacher_id, :group_name, :start_time, :duration_hours)");
    query.bindValue(":subject_id", subjectId);
    query.bindValue(":teacher_id", teacherId > 0 ? QVariant(teacherId) : QVariant(QVariant::LongLong));
    query.bindValue(":group_name", groupName);
    query.bindValue(":start_time", startTime);
    query.bindValue(":duration_hours", durationHours);

    if (!query.exec()) {
        ERROR_MSG(QString("Failed to add lesson: %1").arg(query.lastError().text()));
        return false;
    }

    return true;
}

bool DatabaseManager::updateLesson(qint64 lessonId, qint64 subjectId, qint64 teacherId, const QString &groupName, const QDateTime &startTime, int durationHours)
{
    TRACE_FUNCTION();

    QSqlQuery query(m_database);
    query.prepare(
        "UPDATE lessons "
        "SET subject_id = :subject_id, "
        "    teacher_id = :teacher_id, "
        "    group_name = :group_name, "
        "    lesson_date_start = :start_time, "
        "    lesson_duration_hours = :duration_hours "
        "WHERE id = :lesson_id");
    query.bindValue(":lesson_id", lessonId);
    query.bindValue(":subject_id", subjectId);
    query.bindValue(":teacher_id", teacherId > 0 ? QVariant(teacherId) : QVariant(QVariant::LongLong));
    query.bindValue(":group_name", groupName);
    query.bindValue(":start_time", startTime);
    query.bindValue(":duration_hours", durationHours);

    if (!query.exec()) {
        ERROR_MSG(QString("Failed to update lesson: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool DatabaseManager::deleteLesson(qint64 lessonId)
{
    TRACE_FUNCTION();

    QSqlQuery query(m_database);
    query.prepare("DELETE FROM lessons WHERE id = :lesson_id");
    query.bindValue(":lesson_id", lessonId);

    if (!query.exec()) {
        ERROR_MSG(QString("Failed to delete lesson: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

QVector<DatabaseManager::StudentShortInfo> DatabaseManager::getStudentsForGroup(const QString &groupName)
{
    TRACE_FUNCTION();
    QVector<StudentShortInfo> students;

    QString queryStr =
        "SELECT s.id, s.user_id, u.name, u.second_name "
        "FROM students s "
        "JOIN users u ON s.user_id = u.id "
        "WHERE s.group_name = :group_name "
        "ORDER BY u.second_name, u.name";

    QSqlQuery query(m_database);
    query.prepare(queryStr);
    query.bindValue(":group_name", groupName);

    if (query.exec()) {
        while (query.next()) {
            StudentShortInfo info;
            info.id = query.value(0).toLongLong();
            info.userId = query.value(1).toLongLong();
            info.name = query.value(2).toString();
            info.secondName = query.value(3).toString();
            students.append(info);
        }
    }
    else {
        ERROR_MSG(QString("Failed to get students: %1").arg(query.lastError().text()));
    }
    return students;
}

QVector<DatabaseManager::TeacherRef> DatabaseManager::getTeacherRefs()
{
    TRACE_FUNCTION();
    QVector<TeacherRef> teachers;

    QString queryStr =
        "SELECT u.id, u.second_name || ' ' || u.name AS full_name "
        "FROM users u "
        "JOIN roles r ON u.role_id = r.id "
        "WHERE r.type = 'teacher' "
        "ORDER BY u.second_name, u.name";

    QSqlQuery query = executeQuery(queryStr);
    while (query.next()) {
        TeacherRef teacher;
        teacher.id = query.value(0).toLongLong();
        teacher.fullName = query.value(1).toString();
        teachers.append(teacher);
    }

    return teachers;
}

QMap<qint64, QString> DatabaseManager::getAttendanceForLesson(qint64 lessonId)
{
    TRACE_FUNCTION();
    QMap<qint64, QString> attendance;

    QString queryStr = "SELECT student_id, attendance_status FROM lesson_attendance WHERE lesson_id = :lesson_id";
    QSqlQuery query(m_database);
    query.prepare(queryStr);
    query.bindValue(":lesson_id", lessonId);

    if (query.exec()) {
        while (query.next()) {
            attendance.insert(query.value(0).toLongLong(), query.value(1).toString());
        }
    }
    return attendance;
}

bool DatabaseManager::updateAttendance(qint64 lessonId, qint64 studentId, const QString &status)
{
    QString queryStr =
        "INSERT INTO lesson_attendance (lesson_id, student_id, attendance_status) "
        "VALUES (:lesson_id, :student_id, :status) "
        "ON CONFLICT (lesson_id, student_id) "
        "DO UPDATE SET attendance_status = :status";

    QSqlQuery query(m_database);
    query.prepare(queryStr);
    query.bindValue(":lesson_id", lessonId);
    query.bindValue(":student_id", studentId);
    query.bindValue(":status", status);

    if (!query.exec()) {
        ERROR_MSG(QString("Failed to update attendance: %1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

QVector<DatabaseManager::GradeInfo> DatabaseManager::getGradesForLesson(qint64 lessonId)
{
    TRACE_FUNCTION();
    QVector<GradeInfo> grades;

    QString queryStr =
        "SELECT "
        "    st.id AS student_id, "
        "    u.id AS student_user_id, "
        "    u.second_name || ' ' || u.name AS student_name, "
        "    gr.grade, "
        "    gr.comment "
        "FROM students st "
        "JOIN users u ON u.id = st.user_id "
        "JOIN lessons l ON l.group_name = st.group_name "
        "LEFT JOIN grades gr ON gr.lesson_id = l.id AND gr.student_id = st.id "
        "WHERE l.id = :lesson_id "
        "ORDER BY u.second_name, u.name";

    QSqlQuery query(m_database);
    query.prepare(queryStr);
    query.bindValue(":lesson_id", lessonId);

    if (query.exec()) {
        while (query.next()) {
            GradeInfo info;
            info.studentId = query.value("student_id").toLongLong();
            info.studentUserId = query.value("student_user_id").toLongLong();
            info.studentName = query.value("student_name").toString();
            info.hasGrade = !query.value("grade").isNull();
            info.grade = info.hasGrade ? query.value("grade").toInt() : 0;
            info.comment = query.value("comment").toString();
            grades.append(info);
        }
    }
    else {
        ERROR_MSG(QString("Failed to get grades: %1").arg(query.lastError().text()));
    }

    return grades;
}

bool DatabaseManager::saveGrade(qint64 lessonId, qint64 studentId, int grade, const QString &comment)
{
    TRACE_FUNCTION();

    QSqlQuery query(m_database);
    query.prepare(
        "INSERT INTO grades (student_id, lesson_id, grade, comment) "
        "VALUES (:student_id, :lesson_id, :grade, :comment) "
        "ON CONFLICT (student_id, lesson_id) "
        "DO UPDATE SET grade = :grade, comment = :comment");
    query.bindValue(":student_id", studentId);
    query.bindValue(":lesson_id", lessonId);
    query.bindValue(":grade", grade);
    query.bindValue(":comment", comment);

    if (!query.exec()) {
        ERROR_MSG(QString("Failed to save grade: %1").arg(query.lastError().text()));
        return false;
    }

    return true;
}

bool DatabaseManager::deleteGrade(qint64 lessonId, qint64 studentId)
{
    TRACE_FUNCTION();

    QSqlQuery query(m_database);
    query.prepare("DELETE FROM grades WHERE lesson_id = :lesson_id AND student_id = :student_id");
    query.bindValue(":lesson_id", lessonId);
    query.bindValue(":student_id", studentId);

    if (!query.exec()) {
        ERROR_MSG(QString("Failed to delete grade: %1").arg(query.lastError().text()));
        return false;
    }

    return true;
}

QVector<DatabaseManager::CreditStatusItem> DatabaseManager::getCreditStatus(const QString &groupName, qint64 subjectId, const QDate &start, const QDate &end)
{
    TRACE_FUNCTION();
    QVector<CreditStatusItem> result;

    QSqlQuery query(m_database);
    query.prepare("SELECT * FROM get_credit_status(:group_name, :subject_id, CAST(:start AS date), CAST(:end AS date))");
    query.bindValue(":group_name", groupName);
    query.bindValue(":subject_id", subjectId);
    query.bindValue(":start", start);
    query.bindValue(":end", end);

    if (query.exec()) {
        while (query.next()) {
            CreditStatusItem item;
            item.studentUserId = query.value("student_user_id").toLongLong();
            item.studentName = query.value("student_name").toString();
            item.groupName = query.value("group_name").toString();
            item.subjectName = query.value("subject_name").toString();
            item.attendancePercentage = query.value("attendance_percentage").toDouble();
            item.averageGrade = query.value("average_grade").toDouble();
            item.minAttendancePercentage = query.value("min_attendance_pct").toDouble();
            item.minAverageGrade = query.value("min_avg_grade").toDouble();
            item.creditPassed = query.value("credit_passed").toBool();
            result.append(item);
        }
    }
    else {
        ERROR_MSG(QString("Failed to get credit status: %1").arg(query.lastError().text()));
    }

    return result;
}

QVector<DatabaseManager::SemesterInfo> DatabaseManager::getSemestersForGroup(const QString &groupName)
{
    TRACE_FUNCTION();
    QVector<SemesterInfo> semesters;

    QSqlQuery query(m_database);
    query.prepare(
        "SELECT id, group_name, start_date, end_date, year, number "
        "FROM semesters "
        "WHERE group_name = :group_name "
        "ORDER BY year DESC, number DESC");
    query.bindValue(":group_name", groupName);

    if (query.exec()) {
        while (query.next()) {
            SemesterInfo semester;
            semester.id = query.value("id").toLongLong();
            semester.groupName = query.value("group_name").toString();
            semester.startDate = query.value("start_date").toDate();
            semester.endDate = query.value("end_date").toDate();
            semester.year = query.value("year").toInt();
            semester.number = query.value("number").toInt();
            semesters.append(semester);
        }
    }
    else {
        ERROR_MSG(QString("Failed to get semesters: %1").arg(query.lastError().text()));
    }

    return semesters;
}

bool DatabaseManager::addGradeCriteria(qint64 subjectId, double minAttendancePercentage, double minAverageGrade)
{
    TRACE_FUNCTION();

    QSqlQuery query(m_database);
    query.prepare(
        "INSERT INTO grade_criteria (subject_id, min_attendance_pct, min_avg_grade) "
        "VALUES (:subject_id, :min_attendance_pct, :min_avg_grade) "
        "ON CONFLICT (subject_id) "
        "DO UPDATE SET min_attendance_pct = :min_attendance_pct, min_avg_grade = :min_avg_grade");
    query.bindValue(":subject_id", subjectId);
    query.bindValue(":min_attendance_pct", minAttendancePercentage);
    query.bindValue(":min_avg_grade", minAverageGrade);

    if (!query.exec()) {
        ERROR_MSG(QString("Failed to save grade criteria: %1").arg(query.lastError().text()));
        return false;
    }

    return true;
}

bool DatabaseManager::deleteGradeCriteria(qint64 criteriaId)
{
    TRACE_FUNCTION();

    QSqlQuery query(m_database);
    query.prepare("DELETE FROM grade_criteria WHERE id = :id");
    query.bindValue(":id", criteriaId);

    if (!query.exec()) {
        ERROR_MSG(QString("Failed to delete grade criteria: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool DatabaseManager::addSemester(const QString &groupName, const QDate &startDate, const QDate &endDate, int year, int number)
{
    TRACE_FUNCTION();

    QSqlQuery query(m_database);
    query.prepare(
        "INSERT INTO semesters (group_name, start_date, end_date, year, number) "
        "VALUES (:group_name, :start_date, :end_date, :year, :number) "
        "ON CONFLICT (group_name, year, number) "
        "DO UPDATE SET start_date = :start_date, end_date = :end_date");
    query.bindValue(":group_name", groupName);
    query.bindValue(":start_date", startDate);
    query.bindValue(":end_date", endDate);
    query.bindValue(":year", year);
    query.bindValue(":number", number);

    if (!query.exec()) {
        ERROR_MSG(QString("Failed to save semester: %1").arg(query.lastError().text()));
        return false;
    }

    return true;
}

bool DatabaseManager::deleteSemester(qint64 semesterId)
{
    TRACE_FUNCTION();

    QSqlQuery query(m_database);
    query.prepare("DELETE FROM semesters WHERE id = :id");
    query.bindValue(":id", semesterId);

    if (!query.exec()) {
        ERROR_MSG(QString("Failed to delete semester: %1").arg(query.lastError().text()));
        return false;
    }

    return query.numRowsAffected() > 0;
}

QVector<DatabaseManager::StudentReportItem> DatabaseManager::getStudentAttendanceReport(const QDate &start, const QDate &end, const QString &groupName)
{
    TRACE_FUNCTION();
    QVector<StudentReportItem> report;

    QString queryStr = "SELECT * FROM get_student_attendance_report(:group, CAST(:start AS date), CAST(:end AS date))";
    QSqlQuery query(m_database);
    query.prepare(queryStr);

    if (groupName.isEmpty()) {
        query.bindValue(":group", QVariant(QVariant::String));
    }
    else {
        query.bindValue(":group", groupName);
    }
    query.bindValue(":start", start);
    query.bindValue(":end", end);

    if (query.exec()) {
        while (query.next()) {
            StudentReportItem item;
            item.studentName = query.value("student_name").toString();
            item.groupName = query.value("group_name").toString();
            item.totalLessons = query.value("total_lessons").toLongLong();
            item.absentCount = query.value("absent_count").toLongLong();
            item.attendancePercentage = query.value("attendance_percentage").toDouble();
            report.append(item);
        }
    }
    else {
        ERROR_MSG(QString("Failed to get student report: %1").arg(query.lastError().text()));
    }
    return report;
}

QVector<DatabaseManager::AggregatedReportItem> DatabaseManager::getGroupAttendanceStats(const QDate &start, const QDate &end)
{
    TRACE_FUNCTION();
    QVector<AggregatedReportItem> report;

    QString queryStr = "SELECT * FROM get_group_attendance_stats(CAST(:start AS date), CAST(:end AS date))";
    QSqlQuery query(m_database);
    query.prepare(queryStr);
    query.bindValue(":start", start);
    query.bindValue(":end", end);

    if (query.exec()) {
        while (query.next()) {
            AggregatedReportItem item;
            item.name = query.value("group_name").toString();
            item.averagePercentage = query.value("avg_attendance_percentage").toDouble();
            item.totalAbsences = query.value("total_absences").toLongLong();
            report.append(item);
        }
    }
    else {
        ERROR_MSG(QString("Failed to get group stats: %1").arg(query.lastError().text()));
    }
    return report;
}

QVector<DatabaseManager::AggregatedReportItem> DatabaseManager::getCourseAttendanceStats(const QDate &start, const QDate &end)
{
    TRACE_FUNCTION();
    QVector<AggregatedReportItem> report;

    QString queryStr = "SELECT * FROM get_course_attendance_stats(CAST(:start AS date), CAST(:end AS date))";
    QSqlQuery query(m_database);
    query.prepare(queryStr);
    query.bindValue(":start", start);
    query.bindValue(":end", end);

    if (query.exec()) {
        while (query.next()) {
            AggregatedReportItem item;
            int courseNum = query.value("course_number").toInt();
            item.name = QString("%1 курс").arg(courseNum);
            item.averagePercentage = query.value("avg_attendance_percentage").toDouble();
            item.totalAbsences = query.value("total_absences").toLongLong();
            report.append(item);
        }
    }
    else {
        ERROR_MSG(QString("Failed to get course stats: %1").arg(query.lastError().text()));
    }
    return report;
}

QVector<DatabaseManager::StudentGradeReportItem> DatabaseManager::getStudentGradeReport(const QDate &start, const QDate &end, const QString &groupName, qint64 subjectId)
{
    TRACE_FUNCTION();
    QVector<StudentGradeReportItem> report;

    QString queryStr =
        "SELECT "
        "    u.second_name || ' ' || u.name AS student_name, "
        "    st.group_name, "
        "    subj.name AS subject_name, "
        "    COUNT(gr.id) AS graded_lessons, "
        "    ROUND(AVG(gr.grade)::numeric, 2) AS average_grade "
        "FROM grades gr "
        "JOIN students st ON st.id = gr.student_id "
        "JOIN users u ON u.id = st.user_id "
        "JOIN lessons l ON l.id = gr.lesson_id "
        "JOIN subjects subj ON subj.id = l.subject_id "
        "WHERE l.lesson_date_start::date BETWEEN :start AND :end "
        "  AND (:group_name = '' OR st.group_name = :group_name) "
        "  AND (:subject_id = 0 OR subj.id = :subject_id) "
        "GROUP BY u.second_name, u.name, st.group_name, subj.name "
        "ORDER BY st.group_name, subj.name, u.second_name, u.name";

    QSqlQuery query(m_database);
    query.prepare(queryStr);
    query.bindValue(":start", start);
    query.bindValue(":end", end);
    query.bindValue(":group_name", groupName);
    query.bindValue(":subject_id", subjectId);

    if (query.exec()) {
        while (query.next()) {
            StudentGradeReportItem item;
            item.studentName = query.value("student_name").toString();
            item.groupName = query.value("group_name").toString();
            item.subjectName = query.value("subject_name").toString();
            item.gradedLessons = query.value("graded_lessons").toLongLong();
            item.averageGrade = query.value("average_grade").toDouble();
            report.append(item);
        }
    }
    else {
        ERROR_MSG(QString("Failed to get student grade report: %1").arg(query.lastError().text()));
    }

    return report;
}

QVector<DatabaseManager::GradeAggregateReportItem> DatabaseManager::getGroupGradeStats(const QDate &start, const QDate &end)
{
    TRACE_FUNCTION();
    QVector<GradeAggregateReportItem> report;

    QString queryStr =
        "SELECT "
        "    st.group_name, "
        "    ROUND(AVG(gr.grade)::numeric, 2) AS average_grade, "
        "    COUNT(gr.id) AS graded_lessons "
        "FROM grades gr "
        "JOIN students st ON st.id = gr.student_id "
        "JOIN lessons l ON l.id = gr.lesson_id "
        "WHERE l.lesson_date_start::date BETWEEN :start AND :end "
        "GROUP BY st.group_name "
        "ORDER BY average_grade DESC, st.group_name";

    QSqlQuery query(m_database);
    query.prepare(queryStr);
    query.bindValue(":start", start);
    query.bindValue(":end", end);

    if (query.exec()) {
        while (query.next()) {
            GradeAggregateReportItem item;
            item.name = query.value("group_name").toString();
            item.averageGrade = query.value("average_grade").toDouble();
            item.gradedLessons = query.value("graded_lessons").toLongLong();
            report.append(item);
        }
    }
    else {
        ERROR_MSG(QString("Failed to get group grade stats: %1").arg(query.lastError().text()));
    }

    return report;
}

QVector<DatabaseManager::GradeAggregateReportItem> DatabaseManager::getSemesterGradeStats(const QDate &start, const QDate &end)
{
    TRACE_FUNCTION();
    QVector<GradeAggregateReportItem> report;

    QString queryStr =
        "SELECT "
        "    sem.group_name || ', ' || sem.year || ' год, ' || sem.number || ' семестр' AS semester_name, "
        "    ROUND(AVG(gr.grade)::numeric, 2) AS average_grade, "
        "    COUNT(gr.id) AS graded_lessons "
        "FROM semesters sem "
        "JOIN students st ON st.group_name = sem.group_name "
        "JOIN grades gr ON gr.student_id = st.id "
        "JOIN lessons l ON l.id = gr.lesson_id "
        "WHERE l.group_name = sem.group_name "
        "  AND l.lesson_date_start::date BETWEEN sem.start_date AND sem.end_date "
        "  AND l.lesson_date_start::date BETWEEN :start AND :end "
        "GROUP BY sem.group_name, sem.year, sem.number "
        "ORDER BY sem.year DESC, sem.number DESC, sem.group_name";

    QSqlQuery query(m_database);
    query.prepare(queryStr);
    query.bindValue(":start", start);
    query.bindValue(":end", end);

    if (query.exec()) {
        while (query.next()) {
            GradeAggregateReportItem item;
            item.name = query.value("semester_name").toString();
            item.averageGrade = query.value("average_grade").toDouble();
            item.gradedLessons = query.value("graded_lessons").toLongLong();
            report.append(item);
        }
    }
    else {
        ERROR_MSG(QString("Failed to get semester grade stats: %1").arg(query.lastError().text()));
    }

    return report;
}

bool DatabaseManager::setCurrentUser(const int &userId)
{
    QSqlQuery query;
    return query.exec(QString("SET app.user_id = '%1'").arg(userId));
}
