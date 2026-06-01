#ifndef DEBUGMACROS_H
#define DEBUGMACROS_H

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QThread>


#ifdef QT_DEBUG
#    define TRACE_FUNCTION()                                                           \
        qDebug() << "[TRACE]" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") \
                 << "Thread:" << QThread::currentThreadId()                            \
                 << "Function:" << Q_FUNC_INFO
#else
#    define TRACE_FUNCTION() \
        do {                 \
        } while (0)
#endif


#ifdef QT_DEBUG
#    define DEBUG_MSG(msg)                                                             \
        qDebug() << "[DEBUG]" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") \
                 << "Thread:" << QThread::currentThreadId()                            \
                 << Q_FUNC_INFO << ":" << msg
#else
#    define DEBUG_MSG(msg) \
        do {               \
        } while (0)
#endif


#define INFO_MSG(msg)                                                                       \
    qInfo() << "[INFO]" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") \
            << "Thread:" << QThread::currentThreadId()                                      \
            << Q_FUNC_INFO << ":" << msg


#define WARNING_MSG(msg)                                                                       \
    qWarning() << "[WARN]" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") \
               << "Thread:" << QThread::currentThreadId()                                      \
               << Q_FUNC_INFO << ":" << msg


#define ERROR_MSG(msg)                                                                           \
    qCritical() << "[ERROR]" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") \
                << "Thread:" << QThread::currentThreadId()                                       \
                << Q_FUNC_INFO << ":" << msg


#ifdef QT_DEBUG
#    define TRACE_SCOPE(name) \
        DebugScopeTracer tracer_##__LINE__(name, Q_FUNC_INFO, __FILE__, __LINE__)

class DebugScopeTracer
{
public:
    DebugScopeTracer(const QString &name, const QString &funcInfo, const QString &file, int line)
        : m_name(name),
          m_funcInfo(funcInfo),
          m_file(file),
          m_line(line)
    {
        qDebug() << "[ENTER]" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
                 << "Thread:" << QThread::currentThreadId()
                 << "Scope:" << m_name
                 << "Function:" << m_funcInfo
                 << "File:" << QFileInfo(m_file).fileName() << ":" << m_line;
    }

    ~DebugScopeTracer()
    {
        qDebug() << "[EXIT]" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
                 << "Thread:" << QThread::currentThreadId()
                 << "Scope:" << m_name
                 << "Function:" << m_funcInfo;
    }

private:
    QString m_name;
    QString m_funcInfo;
    QString m_file;
    int m_line;
};
#else
#    define TRACE_SCOPE(name) \
        do {                  \
        } while (0)
#endif

#ifdef QT_DEBUG
#    define TIME_SCOPE(name) \
        TimeScopeTracer timeTracer_##__LINE__(name)

class TimeScopeTracer
{
public:
    TimeScopeTracer(const QString &name)
        : m_name(name),
          m_startTime(QDateTime::currentMSecsSinceEpoch())
    {
        qDebug() << "[TIMER START]" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
                 << "Scope:" << m_name;
    }

    ~TimeScopeTracer()
    {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_startTime;
        qDebug() << "[TIMER END]" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
                 << "Scope:" << m_name
                 << "Elapsed:" << elapsed << "ms";
    }

private:
    QString m_name;
    qint64 m_startTime;
};
#else
#    define TIME_SCOPE(name) \
        do {                 \
        } while (0)
#endif

#endif
