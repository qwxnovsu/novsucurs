#ifndef ADDGROUPDIALOG_H
#define ADDGROUPDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

class AddGroupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddGroupDialog(QWidget *parent = nullptr);
    ~AddGroupDialog() = default;

    struct GroupData
    {
        QString name;
        int startYear;
        int durationYears;
    };

    GroupData getGroupData() const;

private slots:
    void onAccept();
    void validateInput();

private:
    void setupUI();

    QLineEdit *m_nameEdit;
    QSpinBox *m_startYearSpinBox;
    QComboBox *m_durationComboBox;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    GroupData m_groupData;
};

#endif
