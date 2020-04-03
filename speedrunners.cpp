#include "speedrunners.h"
#include "ui_speedrunners.h"

#include <QMessageBox>

bool hack(float boost_factor);
bool deinit_hack();

bool hack_apex();
bool deinit_hack_apex();

SpeedRunners::SpeedRunners(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::SpeedRunners), apex_overlay(nullptr) {
    ui->setupUi(this);
}

SpeedRunners::~SpeedRunners() {
    if (apex_overlay) {
        delete apex_overlay;
        apex_overlay = nullptr;
    }

    deinit_hack();
    deinit_hack_apex();
    delete ui;
}

void SpeedRunners::handle_bt_speedrunners() {
    double boost_factor = ui->doubleSpinBox->value();
    hack(boost_factor);
}

void SpeedRunners::handle_bt_apex() {
    update_settings();
    if (hack_apex()) {
        if (apex_overlay) {
            delete apex_overlay;
            apex_overlay = nullptr;
        }
        apex_overlay = new Overlay();
    }
}

#include "hack_apex.hpp"

int32_t config_fov;
uint32_t config_refresh_rate;
float config_overlay_propsurvival_radius;

void SpeedRunners::update_settings() {
    config_fov = ui->spinBox_FOV->value();
    config_overlay_propsurvival_radius = ui->doubleSpinBox_overlay_propsurvival_radius->value();
    config_refresh_rate = ui->spinBox_refresh_rate->value();
}

void showMessageBox(const char* title, const char* text) {
    QMessageBox msgbox;
    msgbox.setWindowTitle(title);
    msgbox.setText(text);
    msgbox.exec();
}
