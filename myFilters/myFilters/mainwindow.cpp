#include "mainwindow.h"
#include "imagebuffer.h"
#include "filters.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);

    auto *ctrl = new QHBoxLayout();

    auto *btnOpen  = new QPushButton("Адкрыць відарыс", this);
    auto *btnApply = new QPushButton("Ужыць", this);
    auto *btnSave  = new QPushButton("Захаваць вынік", this);

    connect(btnOpen,  &QPushButton::clicked, this, &MainWindow::onOpenImage);
    connect(btnApply, &QPushButton::clicked, this, &MainWindow::onApply);
    connect(btnSave,  &QPushButton::clicked, this, &MainWindow::onSaveResult);

    cbMethod = new QComboBox(this);
    cbMethod->addItem("Лакальная: Niblack",              0);
    cbMethod->addItem("Лакальная: Sauvola",              1);
    cbMethod->addItem("Адаптыўная: Bradley (SAT)",       2);
    cbMethod->addItem("Нелінейны: медиянны фільтр",    3);
    cbMethod->addItem("Нелінейны: min (эрозія)",        4);
    cbMethod->addItem("Нелінейны: max (дылатацыя)",     5);

    sbWindow = new QSpinBox(this);
    sbWindow->setRange(3, 99);
    sbWindow->setSingleStep(2);
    sbWindow->setValue(15);

    sbK = new QDoubleSpinBox(this);
    sbK->setRange(-2.0, 2.0);
    sbK->setDecimals(3);
    sbK->setSingleStep(0.05);
    sbK->setValue(-0.2);

    lblWindow = new QLabel("Акно:", this);
    lblK      = new QLabel("k:", this);

    ctrl->addWidget(btnOpen);
    ctrl->addSpacing(20);
    ctrl->addWidget(new QLabel("Метад:", this));
    ctrl->addWidget(cbMethod);
    ctrl->addWidget(lblWindow);
    ctrl->addWidget(sbWindow);
    ctrl->addWidget(lblK);
    ctrl->addWidget(sbK);
    ctrl->addWidget(btnApply);
    ctrl->addWidget(btnSave);
    ctrl->addStretch();

    root->addLayout(ctrl);

    auto *images = new QHBoxLayout();

    auto makePane = [&](const QString &title, QLabel *&lbl) {
        auto *box = new QGroupBox(title, this);
        auto *lay = new QVBoxLayout(box);
        lbl = new QLabel(box);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setMinimumSize(400, 400);
        lbl->setStyleSheet("background:#202020; color:#cccccc;");
        lbl->setText("(няма відарыса)");
        lbl->setScaledContents(false);
        lay->addWidget(lbl);
        return box;
    };

    images->addWidget(makePane("Арыгінал",  lblOriginal));
    images->addWidget(makePane("Вынік", lblResult));
    root->addLayout(images);

    setCentralWidget(central);
    resize(1000, 600);

    connect(cbMethod, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ updateParamVisibility(); });

    updateParamVisibility();
}

void MainWindow::updateParamVisibility()
{
    int id = cbMethod->currentData().toInt();

    // Niblack (0), Sauvola (1), Bradley (2) — трэба задаць параметр k/t
    // Медыянны (3), min (4), max (5)       — трэба задаць радыус
    bool needsK = (id == 0 || id == 1 || id == 2);

    lblK->setVisible(needsK);
    sbK->setVisible(needsK);
    lblWindow->setVisible(true);
    sbWindow->setVisible(true);

    switch (id) {
    case 0: // Niblack
        lblWindow->setText("Окно:");
        sbWindow->setRange(3, 99);
        sbWindow->setSingleStep(2);
        if (sbWindow->value() % 2 == 0) sbWindow->setValue(sbWindow->value() + 1);

        lblK->setText("k:");
        sbK->setRange(-2.0, 2.0);
        sbK->setDecimals(3);
        sbK->setSingleStep(0.05);
        sbK->setValue(-0.2);
        break;

    case 1: // Sauvola
        lblWindow->setText("Акно:");
        sbWindow->setRange(3, 99);
        sbWindow->setSingleStep(2);
        if (sbWindow->value() % 2 == 0) sbWindow->setValue(sbWindow->value() + 1);

        lblK->setText("k:");
        sbK->setRange(0.0, 2.0);
        sbK->setDecimals(3);
        sbK->setSingleStep(0.05);
        sbK->setValue(0.5);
        break;

    case 2: // Bradley
        lblWindow->setText("Акно:");
        sbWindow->setRange(3, 99);
        sbWindow->setSingleStep(2);
        if (sbWindow->value() % 2 == 0) sbWindow->setValue(sbWindow->value() + 1);

        lblK->setText("t:");
        sbK->setRange(0.0, 1.0);
        sbK->setDecimals(3);
        sbK->setSingleStep(0.05);
        sbK->setValue(0.15);
        break;

    case 3: // Медыяны
    case 4: // min
    case 5: // max
        lblWindow->setText("Радыус:");
        sbWindow->setRange(1, 10);
        sbWindow->setSingleStep(1);
        if (sbWindow->value() < 1) sbWindow->setValue(1);
        break;
    }
}

void MainWindow::onOpenImage()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Адкрыць вiдарыс", QString(),
        "Вiдарысы (*.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty())
        return;

    QImage img(path);
    if (img.isNull()) {
        QMessageBox::warning(this, "Памылка", "Не атрымалася адкрыць файл.");
        return;
    }
    original = img;
    result   = QImage();

    lblOriginal->setPixmap(QPixmap::fromImage(original).scaled(lblOriginal->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    lblResult->setText("(вынiк з'явiцца пасля апрацоўкi)");
    lblResult->setPixmap(QPixmap());
}

void MainWindow::onApply()
{
    if (original.isNull()) {
        QMessageBox::information(this, "Няма вiдарыса", "Адкройце вiдарыс.");
        return;
    }

    ImageBuffer src(original);
    int id = cbMethod->currentData().toInt();

    ImageBuffer dst;
    switch (id) {
    case 0: { // Niblack
        int w = sbWindow->value(); if (w % 2 == 0) ++w;
        dst = filter.niblack(src, w, sbK->value());
        break;
    }
    case 1: { // Sauvola
        int w = sbWindow->value(); if (w % 2 == 0) ++w;
        dst = filter.sauvola(src, w, sbK->value(), 128.0);
        break;
    }
    case 2: { // Bradley
        int w = sbWindow->value(); if (w % 2 == 0) ++w;
        dst = filter.adaptiveBradley(src, w, sbK->value());
        break;
    }
    case 3: // Медыянны
        dst = filter.medianFilter(src, sbWindow->value());
        break;
    case 4: // min
        dst = filter.minFilter(src, sbWindow->value());
        break;
    case 5: // max
        dst = filter.maxFilter(src, sbWindow->value());
        break;
    default:
        QMessageBox::warning(this, "Памылка", "Невядомы метад.");
        return;
    }
    // - парогавыя метады (0,1,2) вяртаюць бiнарную маску —> ужываем да колера
    // - парадкавыя фiльтры (3,4,5) —> ужываем да кожнага канала RGB

    bool isBinary = (id == 0 || id == 1 || id == 2);

    if (isBinary) {
        ImageBuffer masked = ImageBuffer::applyBinaryMask(src, dst);
        result = masked.toQImageColor();
    } else if (id >= 3 && id <= 5) {
        int radius = sbWindow->value();
        ImageBuffer colored(src.width, src.height, src.hasColor);
        colored.gray = dst.gray;
        if (src.hasColor) {
            // фiльры апрацоўваюць каналы асобна праз часовы ImageBuffer
            ImageBuffer tmp;
            auto processChannel = [&](const std::vector<uint8_t> &ch,
                                      std::vector<uint8_t> &outCh) {
                tmp.width = src.width; tmp.height = src.height;
                tmp.gray = ch;
                ImageBuffer filtered;
                if (id == 3)      filtered = filter.medianFilter(tmp, radius);
                else if (id == 4) filtered = filter.minFilter(tmp, radius);
                else              filtered = filter.maxFilter(tmp, radius);
                outCh = filtered.gray;
            };
            processChannel(src.r, colored.r);
            processChannel(src.g, colored.g);
            processChannel(src.b, colored.b);
        }
        result = colored.toQImageColor();
    } else {
        result = dst.toQImage();
    }
    lblResult->setPixmap(QPixmap::fromImage(result)
                             .scaled(lblResult->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::onSaveResult()
{
    if (result.isNull()) {
        QMessageBox::information(this, "Няма вынiка",
                                 "Спачатку апрацуйце вiдарыс.");
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this, "Захаваць вынiк", QString(),
        "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp)");
    if (path.isEmpty())
        return;

    if (!result.save(path))
        QMessageBox::warning(this, "Памылка", "Не атрымалася захаваць файл.");
}