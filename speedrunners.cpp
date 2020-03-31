#include "speedrunners.h"
#include "ui_speedrunners.h"

#include <QMessageBox>

bool hack(float boost_factor);
bool deinit_hack();

bool hack_apex();
bool deinit_hack_apex();

SpeedRunners::SpeedRunners(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::SpeedRunners) {
    ui->setupUi(this);
}

SpeedRunners::~SpeedRunners() {
    deinit_hack();
    deinit_hack_apex();
    delete ui;
}

void SpeedRunners::handle_bt_speedrunners() {
    double boost_factor = ui->doubleSpinBox->value();
    hack(boost_factor);
}

void SpeedRunners::handle_bt_apex() {
    hack_apex();
}

void showMessageBox(const char* title, const char* text) {
    QMessageBox msgbox;
    msgbox.setWindowTitle(title);
    msgbox.setText(text);
    msgbox.exec();
}
