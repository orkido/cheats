#include "speedrunners.h"
#include "ui_speedrunners.h"

#include <QMessageBox>

int hack();

SpeedRunners::SpeedRunners(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::SpeedRunners) {
    ui->setupUi(this);
}

SpeedRunners::~SpeedRunners() {
    delete ui;
}

void showMessageBox(const char* title, const char* text) {
    QMessageBox msgbox;
    msgbox.setWindowTitle(title);
    msgbox.setText(text);
    msgbox.exec();
}