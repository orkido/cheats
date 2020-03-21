#include "speedrunners.h"
#include "ui_speedrunners.h"

SpeedRunners::SpeedRunners(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::SpeedRunners) {
    ui->setupUi(this);
}

SpeedRunners::~SpeedRunners() {
    delete ui;
}

