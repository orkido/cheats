#include "speedrunners.h"
#include "ui_speedrunners.h"

#include <QMessageBox>

bool hack(float boost_factor);

SpeedRunners::SpeedRunners(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::SpeedRunners) {
    ui->setupUi(this);
}

SpeedRunners::~SpeedRunners() {
    delete ui;
}

void SpeedRunners::handleButton() {
    double boost_factor = ui->doubleSpinBox->value();
    hack(boost_factor);
}

void showMessageBox(const char* title, const char* text) {
    QMessageBox msgbox;
    msgbox.setWindowTitle(title);
    msgbox.setText(text);
    msgbox.exec();
}
