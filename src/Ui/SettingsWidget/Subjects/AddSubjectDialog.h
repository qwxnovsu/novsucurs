#ifndef ADDSUBJECTDIALOG_H
#define ADDSUBJECTDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>

class AddSubjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddSubjectDialog(QWidget *parent = nullptr);
    ~AddSubjectDialog() = default;

    struct SubjectData
    {
        QString name;
    };

    SubjectData getSubjectData() const;

private slots:
    void onAccept();
    void validateInput();

private:
    void setupUI();

    QLineEdit *m_nameEdit;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    SubjectData m_subjectData;
};

#endif
