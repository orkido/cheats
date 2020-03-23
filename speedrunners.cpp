#include "speedrunners.h"
#include "ui_speedrunners.h"

#include <QMessageBox>

bool hack();

SpeedRunners::SpeedRunners(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::SpeedRunners) {
    ui->setupUi(this);
}

SpeedRunners::~SpeedRunners() {
    delete ui;
}

void SpeedRunners::handleButton() {
    hack();
}

void showMessageBox(const char* title, const char* text) {
    QMessageBox msgbox;
    msgbox.setWindowTitle(title);
    msgbox.setText(text);
    msgbox.exec();
}
