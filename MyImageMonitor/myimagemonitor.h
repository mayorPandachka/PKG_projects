#ifndef MYIMAGEMONITOR_H
#define MYIMAGEMONITOR_H

#include <QMainWindow>
#include <QThread>
#include <QFileInfo>

QT_BEGIN_NAMESPACE
namespace Ui { class MyImageMonitor; }
QT_END_NAMESPACE

class MyImageMonitor : public QMainWindow
{
    Q_OBJECT

public:
    explicit MyImageMonitor(QWidget *parent = nullptr);
    ~MyImageMonitor() override;

private slots:
    void on_btnOpenFolder_clicked();
    void on_btnAddFiles_clicked();
    void on_btnClear_clicked();
    void on_btnExportCsv_clicked();
    void on_tableWidget_cellDoubleClicked(int row, int column);

private:
    void addFileToList(const QString &path);
    void scanFolder(const QString &dir);

    Ui::MyImageMonitor *ui;
};

#endif // MYIMAGEMONITOR_H