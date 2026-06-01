#ifndef ADDUSERDIALOG_H
#define ADDUSERDIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

class AddUserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddUserDialog(const QVector<QString> &availableRoles, QWidget *parent = nullptr);
    ~AddUserDialog() = default;

    struct UserData
    {
        QString email;
        QString password;
        QString name;
        QString secondName;
        QString thirdName;
        QString role;
    };

    UserData getUserData() const;

private slots:
    void onAccept();
    void validateInput();

private:
    void setupUI();

    QLineEdit *m_emailEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_nameEdit;
    QLineEdit *m_secondNameEdit;
    QLineEdit *m_thirdNameEdit;
    QComboBox *m_roleComboBox;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    UserData m_userData;
};

#endif
