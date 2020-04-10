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

#include "hack_apex.hpp"

void SpeedRunners::handle_bt_apex() {
    update_settings();
    if (hack_apex()) {
        if (apex_overlay) {
            delete apex_overlay;
            apex_overlay = nullptr;
        }
        if (config_display_overlay)
            apex_overlay = new Overlay();
    }
}

void SpeedRunners::update_settings() {
    config_fov = ui->spinBox_FOV->value();
    config_overlay_propsurvival_radius = ui->doubleSpinBox_overlay_propsurvival_radius->value();
    config_refresh_rate = ui->spinBox_refresh_rate->value();
    config_unload_driver = ui->cb_unload_driver->isChecked();
    config_aimbot = ui->cb_aimbot->isChecked();
    config_aimbot_teammates = ui->cb_aimbot_teammates->isChecked();
    config_highlight = ui->cb_highlight->isChecked();
    config_highlight_teammates = ui->cb_highlight_teammates->isChecked();
    config_display_overlay = ui->cb_display_overlay->isChecked();
}

void showMessageBox(const char* title, const char* text) {
    QMessageBox msgbox;
    msgbox.setWindowTitle(title);
    msgbox.setText(text);
    msgbox.exec();
}
