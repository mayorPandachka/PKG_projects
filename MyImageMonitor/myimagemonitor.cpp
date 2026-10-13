#include "myimagemonitor.h"
#include "ui_myimagemonitor.h"
#include "imageparser.h"

#include <QFileDialog>
#include <QDirIterator>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QPixmap>
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QDebug>

MyImageMonitor::MyImageMonitor(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MyImageMonitor)
{
    ui->setupUi(this);

    ui->tableWidget->setColumnCount(9);
    ui->tableWidget->setHorizontalHeaderLabels({
        "Имя файла", "Формат", "Размер (px)", "DPI (X×Y)",
        "Глубина цвета (бит)", "Сжатие", "Доп. сведения", "Статус", "Путь"
    });
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setAlternatingRowColors(true);
}

MyImageMonitor::~MyImageMonitor()
{
    delete ui;
}

void MyImageMonitor::scanFolder(const QString &dir)
{
    QStringList filters = {"*.jpg","*.jpeg","*.gif","*.tif","*.tiff",
                           "*.bmp","*.png","*.pcx"};
    QDirIterator it(dir, filters, QDir::Files | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        addFileToList(it.next());
    }
}

void MyImageMonitor::addFileToList(const QString &path)
{
    ImageInfo info = ImageParser::parse(path);

    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    auto setItem = [&](int col, const QString &text) {
        ui->tableWidget->setItem(row, col, new QTableWidgetItem(text));
    };

    setItem(0, info.fileName);
    setItem(1, info.format);
    setItem(2, QString("%1 × %2").arg(info.width).arg(info.height));

    if (info.dpiX > 0 || info.dpiY > 0)
        setItem(3, QString("%1 × %2").arg(info.dpiX, 0, 'f', 1)
                       .arg(info.dpiY, 0, 'f', 1));
    else
        setItem(3, "72.0 × 72.0");

    setItem(4, QString::number(info.bitDepth));
    setItem(5, info.compression);
    setItem(6, info.extra);

    QTableWidgetItem *statusItem = new QTableWidgetItem(info.status);
    if (info.status != "OK") {
        statusItem->setBackground(QBrush(QColor(255, 200, 200)));
    } else {
        statusItem->setBackground(QBrush(QColor(200, 255, 200)));
    }
    ui->tableWidget->setItem(row, 7, statusItem);
    setItem(8, path);
}

void MyImageMonitor::on_btnOpenFolder_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку с изображениями");
    if (dir.isEmpty()) return;

    ui->tableWidget->setRowCount(0);
    scanFolder(dir);
    ui->statusbar->showMessage(QString("Обработано файлов: %1")
                                   .arg(ui->tableWidget->rowCount()));
}

void MyImageMonitor::on_btnAddFiles_clicked()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this, "Выберите изображения", QString(),
        "Images (*.jpg *.jpeg *.gif *.tif *.tiff *.bmp *.png *.pcx);;All files (*.*)");
    for (const QString &f : files)
        addFileToList(f);
    ui->statusbar->showMessage(QString("Добавлено файлов: %1").arg(files.size()));
}

void MyImageMonitor::on_btnClear_clicked()
{
    ui->tableWidget->setRowCount(0);
    ui->statusbar->showMessage("Список очищен");
}

void MyImageMonitor::on_btnExportCsv_clicked()
{
    QString path = QFileDialog::getSaveFileName(this, "Экспорт в CSV",
                                                "report.csv", "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл для записи");
        return;
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "Имя;Формат;Ширина;Высота;DPI X;DPI Y;Глубина;Сжатие;Статус\n";
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        out << ui->tableWidget->item(r,0)->text() << ";"
            << ui->tableWidget->item(r,1)->text() << ";"
            << ui->tableWidget->item(r,2)->text() << ";"
            << ui->tableWidget->item(r,3)->text() << ";"
            << ui->tableWidget->item(r,4)->text() << ";"
            << ui->tableWidget->item(r,5)->text() << ";"
            << ui->tableWidget->item(r,6)->text() << ";"
            << ui->tableWidget->item(r,7)->text() << "\n";
    }
    f.close();
    ui->statusbar->showMessage("Отчёт сохранён: " + path);
}

void MyImageMonitor::on_tableWidget_cellDoubleClicked(int row, int)
{
    QString path = ui->tableWidget->item(row, 8)->text();
    QPixmap pm(path);
    if (pm.isNull()) {
        QMessageBox::information(this, "Просмотр", "Не удалось отобразить изображение");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(path);
    auto *lay = new QVBoxLayout(&dlg);
    auto *lbl = new QLabel;
    lbl->setPixmap(pm.scaled(800, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    lbl->setAlignment(Qt::AlignCenter);
    lay->addWidget(lbl);
    dlg.resize(820, 640);
    dlg.exec();
}