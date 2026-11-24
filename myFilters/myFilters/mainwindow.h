#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>

#include "filters.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onOpenImage();
    void onApply();
    void onSaveResult();

private:
    QImage original;
    QImage result;

    filters filter;

    QLabel *lblOriginal = nullptr;
    QLabel *lblResult   = nullptr;
    QComboBox *cbMethod    = nullptr;
    QSpinBox *sbWindow    = nullptr;
    QDoubleSpinBox *sbK         = nullptr;
    QLabel *lblWindow   = nullptr;
    QLabel *lblK        = nullptr;

    void updateParamVisibility();
};

#endif