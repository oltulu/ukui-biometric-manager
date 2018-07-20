#ifndef MESSAGEDIALOG_H
#define MESSAGEDIALOG_H

#include <QDialog>

namespace Ui {
class MessageDialog;
}

class MessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MessageDialog(int type = Normal, QWidget *parent = 0);
    ~MessageDialog();
    void setTitle(const QString &text);
    void setMessage(const QString &text);
    enum{Normal, Error};

private:
    Ui::MessageDialog *ui;
};

#endif // MESSAGEDIALOG_H