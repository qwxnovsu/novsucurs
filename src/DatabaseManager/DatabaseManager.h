#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <memory>
#include <QDateTime>
#include <QDebug>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool connectToDatabase(const QString &host,
                           const QString &databaseName,
                           const QString &username,
                           const QString &password,
                           int port = 5432);

    bool isConnected() const;
    QSqlDatabase getDatabase() const;


    QSqlQuery executeQuery(const QString &query);
    bool executeTransaction(const QStringList &queries);


    bool authenticateUser(const QString &email, const QString &password, int &userId, QString &userName, QString &role);

    void disconnect();


    struct SubjectInfo
    {
        QString name;
    };

    struct SubjectRef
    {
        qint64 id;
        QString name;
    };

    struct TeacherRef
    {
        qint64 id;
        QString fullName;
    };

    QVector<SubjectInfo> getAllSubjects();
    QVector<SubjectRef> getSubjectRefs();
    bool addSubject(const QString &name);
    bool deleteSubject(const QString &subjectName);


    struct GroupInfo
    {
        QString name;
        int startYear;
        int durationYears;
    };

    QVector<GroupInfo> getAllGroups();
    QVector<GroupInfo> getAllGroupsForUser(int userId);
    bool addGroup(const QString &name, int startYear, int durationYears);
    bool deleteGroup(const QString &groupName);


    struct UserInfo
    {
        static UserInfo fromQuery(QSqlQuery const &);
        int id;
        QString email;
        QString name;
        QString secondName;
        QString thirdName;
        QString role;
    };

    QVector<UserInfo> getAllUsers();
    bool addUser(const QString &email, const QString &password, const QString &name, const QString &secondName, const QString &thirdName, const QString &role);
    bool deleteUser(int userId);
    QVector<QString> getAvailableRoles();
    bool setCurrentUser(const int &userId);

    struct TeacherInfo
    {
        static TeacherInfo fromQuery(QSqlQuery const &);
        UserInfo user;
        QStringList subjects;
        QVector<int> subject_ids;
    };
    QVector<TeacherInfo> getAllTeachers();

    struct StudentInfo
    {
        static StudentInfo fromQuery(QSqlQuery const &);
        UserInfo user;
        QString group_name;
        int duration_years;
        int start_year;
    };
    QVector<StudentInfo> getAllStudents();


    struct LessonInfo
    {
        qint64 id;
        qint64 subjectId;
        qint64 teacherId;
        QString groupName;
        QString subjectName;
        QDateTime startTime;
        int durationHours;
        QString teacherName;
    };

    struct StudentShortInfo
    {
        qint64 id;
        qint64 userId;
        QString name;
        QString secondName;
    };

    QVector<LessonInfo> getLessonsForGroupAndDate(const QString &groupName, const QDate &date);
    bool addLesson(qint64 subjectId, qint64 teacherId, const QString &groupName, const QDateTime &startTime, int durationHours);
    bool updateLesson(qint64 lessonId, qint64 subjectId, qint64 teacherId, const QString &groupName, const QDateTime &startTime, int durationHours);
    bool deleteLesson(qint64 lessonId);

    QVector<StudentShortInfo> getStudentsForGroup(const QString &groupName);
    QVector<TeacherRef> getTeacherRefs();

    QMap<qint64, QString> getAttendanceForLesson(qint64 lessonId);

    bool updateAttendance(qint64 lessonId, qint64 studentId, const QString &status);

    struct GradeInfo
    {
        qint64 studentId;
        qint64 studentUserId;
        QString studentName;
        bool hasGrade;
        int grade;
        QString comment;
    };

    QVector<GradeInfo> getGradesForLesson(qint64 lessonId);
    bool saveGrade(qint64 lessonId, qint64 studentId, int grade, const QString &comment);
    bool deleteGrade(qint64 lessonId, qint64 studentId);

    struct SemesterInfo
    {
        qint64 id;
        QString groupName;
        QDate startDate;
        QDate endDate;
        int year;
        int number;
    };

    struct CreditStatusItem
    {
        qint64 studentUserId;
        QString studentName;
        QString groupName;
        QString subjectName;
        double attendancePercentage;
        double averageGrade;
        double minAttendancePercentage;
        double minAverageGrade;
        bool creditPassed;
    };

    QVector<CreditStatusItem> getCreditStatus(const QString &groupName, qint64 subjectId, const QDate &start, const QDate &end);
    QVector<SemesterInfo> getSemestersForGroup(const QString &groupName);
    bool addGradeCriteria(qint64 subjectId, double minAttendancePercentage, double minAverageGrade);
    bool deleteGradeCriteria(qint64 criteriaId);
    bool addSemester(const QString &groupName, const QDate &startDate, const QDate &endDate, int year, int number);
    bool deleteSemester(qint64 semesterId);


    struct StudentReportItem
    {
        QString studentName;
        QString groupName;
        qint64 totalLessons;
        qint64 absentCount;
        double attendancePercentage;
    };

    struct AggregatedReportItem
    {
        QString name;
        double averagePercentage;
        qint64 totalAbsences;
    };

    struct StudentGradeReportItem
    {
        QString studentName;
        QString groupName;
        QString subjectName;
        qint64 gradedLessons;
        double averageGrade;
    };

    struct GradeAggregateReportItem
    {
        QString name;
        double averageGrade;
        qint64 gradedLessons;
    };

    QVector<StudentReportItem> getStudentAttendanceReport(const QDate &start, const QDate &end, const QString &groupName = "");
    QVector<AggregatedReportItem> getGroupAttendanceStats(const QDate &start, const QDate &end);
    QVector<AggregatedReportItem> getCourseAttendanceStats(const QDate &start, const QDate &end);
    QVector<StudentGradeReportItem> getStudentGradeReport(const QDate &start, const QDate &end, const QString &groupName = "", qint64 subjectId = 0);
    QVector<GradeAggregateReportItem> getGroupGradeStats(const QDate &start, const QDate &end);
    QVector<GradeAggregateReportItem> getSemesterGradeStats(const QDate &start, const QDate &end);


private:
    QSqlDatabase m_database;
    bool m_connected;

    void seedMockData();
    bool tableIsEmpty(const QString &tableName);
    bool loadDataFromSqlFile(const QString &filePath);
};

#endif
