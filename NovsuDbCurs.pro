QT += core gui sql widgets

TARGET = StudyProcessSystem
TEMPLATE = app

INCLUDEPATH += src
CONFIG -= debug_and_release debug_and_release_target
CONFIG += debug
QMAKE_CXXFLAGS_DEBUG += -g

DESTDIR = $$PWD/build
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc
RCC_DIR = $$PWD/build/rcc
UI_DIR = $$PWD/build/ui

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/DatabaseManager/DatabaseManager.cpp \
    src/AuthManager/AuthManager.cpp \
    src/LoginWindow/LoginWindow.cpp \
    src/Ui/BaseScreen.cpp \
    src/Ui/SettingsWidget/SettingsWidget.cpp \
    src/Ui/Widgets/AbstractTableManager.cpp \
    src/Ui/SettingsWidget/Users/UsersTableManager.cpp \
    src/Ui/SettingsWidget/Users/AddUserDialog.cpp \
    src/Ui/SettingsWidget/Groups/GroupsTableManager.cpp \
    src/Ui/SettingsWidget/Groups/AddGroupDialog.cpp \
    src/Ui/SettingsWidget/Subjects/SubjectsTableManager.cpp \
    src/Ui/SettingsWidget/Subjects/AddSubjectDialog.cpp \
    src/Ui/SettingsWidget/Criteria/CriteriaTableManager.cpp \
    src/Ui/SettingsWidget/Semesters/SemestersTableManager.cpp \
    src/Ui/TeachersWidget/TeachersWidget.cpp \
    src/Ui/TeachersWidget/TeachersTableManager.cpp \
    src/Ui/StudentsWidget/StudentsWidget.cpp \
    src/Ui/StudentsWidget/StudentsTableManager.cpp \
    src/Ui/Attendance/AttendanceScreen.cpp \
    src/Ui/Schedule/ScheduleScreen.cpp \
    src/Ui/Grades/GradesScreen.cpp \
    src/Ui/Reports/ReportsScreen.cpp

HEADERS += \
    src/MainWindow.h \
    src/Utils/Tracing.h \
    src/DatabaseManager/DatabaseManager.h \
    src/AuthManager/AuthManager.h \
    src/LoginWindow/LoginWindow.h \
    src/Ui/BaseScreen.h \
    src/Ui/SettingsWidget/SettingsWidget.h \
    src/Ui/Widgets/AbstractTableManager.h \
    src/Ui/SettingsWidget/Users/UsersTableManager.h \
    src/Ui/SettingsWidget/Users/AddUserDialog.h \
    src/Ui/SettingsWidget/Groups/GroupsTableManager.h \
    src/Ui/SettingsWidget/Groups/AddGroupDialog.h \
    src/Ui/SettingsWidget/Subjects/SubjectsTableManager.h \
    src/Ui/SettingsWidget/Subjects/AddSubjectDialog.h \
    src/Ui/SettingsWidget/Criteria/CriteriaTableManager.h \
    src/Ui/SettingsWidget/Semesters/SemestersTableManager.h \
    src/Ui/TeachersWidget/TeachersWidget.h \
    src/Ui/TeachersWidget/TeachersTableManager.h \
    src/Ui/StudentsWidget/StudentsWidget.h \
    src/Ui/StudentsWidget/StudentsTableManager.h \
    src/Ui/Attendance/AttendanceScreen.h \
    src/Ui/Schedule/ScheduleScreen.h \
    src/Ui/Grades/GradesScreen.h \
    src/Ui/Reports/ReportsScreen.h
