#include "speedrunners.h"
#include "ui_speedrunners.h"

int hack();

SpeedRunners::SpeedRunners(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::SpeedRunners) {
    ui->setupUi(this);

    hack();
}

SpeedRunners::~SpeedRunners() {
    delete ui;
}

