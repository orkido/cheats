#include "speedrunners.h"
#include "ui_speedrunners.h"

#include <QMessageBox>
#include <QtDebug>

bool hack(float boost_factor);
bool deinit_hack();

#if APEX_ENABLED
bool hack_apex();
bool deinit_hack_apex();
#endif

void showMessageBox(const char* title, const char* text);

SpeedRunners::SpeedRunners(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::SpeedRunners), overlay(nullptr) {
    ui->setupUi(this);
}

#if CIVILIZATION_ENABLED
#include "hack_civilization.hpp"
#endif
SpeedRunners::~SpeedRunners() {
    if (overlay) {
        delete overlay;
        overlay = nullptr;
    }

    deinit_hack();
#if APEX_ENABLED
    deinit_hack_apex();
#endif
#if CIVILIZATION_ENABLED
    CivilizationVI::deinit_hack();
#endif
    delete ui;
}

void SpeedRunners::handle_bt_speedrunners() {
    double boost_factor = ui->doubleSpinBox->value();
    hack(boost_factor);
}

#if APEX_ENABLED
#include "hack_apex.hpp"
#include "QtOverlay/overlay_data_interface.hpp"

void SpeedRunners::handle_bt_apex() {
    update_settings();
    if (hack_apex()) {
        if (overlay) {
            delete overlay;
            overlay = nullptr;
        }
        if (config_display_overlay)
            overlay = new QtOverlay::Overlay();
    }
}
#else
void SpeedRunners::handle_bt_apex() {
    showMessageBox("Error", "Not Implemented!");
}
#endif

#if CoD_WARZONE_ENABLED
#include "hack_cod_warzone.hpp"

void SpeedRunners::handle_bt_cod_warzone() {
    update_settings();
    if (hack_cod_warzone()) {
        if (overlay) {
            delete overlay;
            overlay = nullptr;
        }
        if (config_display_overlay)
            overlay = new Overlay();
    }
}
#else
void SpeedRunners::handle_bt_cod_warzone() {
    showMessageBox("Error", "Not Implemented!");
}
#endif

#if CIVILIZATION_ENABLED
void SpeedRunners::handle_bt_civilization_vi() {
    std::vector<std::pair<uint64_t, uint32_t>> pointers;
    CivilizationVI::hack(pointers, OverwriteMode::Init, NULL);
    update_settings();
}
#else
void SpeedRunners::handle_bt_civilization_vi() {
    showMessageBox("Error", "Not Implemented!");
}
#endif

#if STARCRAFT2_ENABLED
#include "hack_starcraft2.hpp"

void SpeedRunners::handle_bt_starcraft2() {
    update_settings();
    if (hack_starcraft2()) {
        /*if (overlay) {
            delete overlay;
            overlay = nullptr;
        }*/
        /*if (config_display_overlay)
            overlay = new Overlay();*/
    }
}
#else
void SpeedRunners::handle_bt_starcraft2() {
    showMessageBox("Error", "Not Implemented!");
}
#endif

void SpeedRunners::update_settings() {
    // Configure apexbot
#if APEX_ENABLED
    config_overlay_propsurvival_radius = ui->doubleSpinBox_overlay_propsurvival_radius->value();
    config_refresh_rate = ui->spinBox_refresh_rate->value();
    config_unload_driver = ui->cb_unload_driver->isChecked();
    config_aimbot = ui->cb_aimbot->isChecked();
    config_aimbot_teammates = ui->cb_aimbot_teammates->isChecked();
    config_highlight = ui->cb_highlight->isChecked();
    config_highlight_teammates = ui->cb_highlight_teammates->isChecked();
    config_display_overlay = ui->cb_display_overlay->isChecked();
#endif

    // Configure CoD Warzone
#if CoD_WARZONE_ENABLED
    CoD_Warzone::cod_warzone_config_refresh_rate = ui->spinBox_refresh_rate->value();
    config_display_overlay = ui->cb_display_overlay->isChecked(); // TODO: this uses Apex variables
#endif

    // Configure Civilization VI
#if CIVILIZATION_ENABLED
    auto selected_items = ui->table_civilization_vi->selectedItems();
    uint64_t selected_address = 0;
    // if multiple items are selected, pick last one
    for (auto item : selected_items) {
        if (item->column() == 0)
            selected_address = item->text().toULongLong();
    }
    std::vector<std::pair<uint64_t, uint32_t>> pointers;
    CivilizationVI::hack(pointers, ui->rb_civilization_add->isChecked() ? OverwriteMode::AddValue : (ui->rb_civilization_set->isChecked() ? OverwriteMode::SetValue : OverwriteMode::None), selected_address);
    ui->table_civilization_vi->setRowCount(pointers.size());
    int i = 0;
    for (const auto & item : pointers) {
        // ui->table_civilization_vi->insertRow(ui->table_civilization_vi->rowCount());
        ui->table_civilization_vi->setItem(i, 0, new QTableWidgetItem(std::to_string(item.first).c_str()));
        ui->table_civilization_vi->setItem(i, 1, new QTableWidgetItem(std::to_string(item.second).c_str()));
        ui->table_civilization_vi->setItem(i, 2, new QTableWidgetItem(std::to_string(item.second >> 8).c_str()));
        ++i;
    }
#endif

    // Configure overlay
#if QT_OVERLAY_ENABLED
    QtOverlay::overlay_config_fov = ui->spinBox_FOV->value();
    QtOverlay::overlay_config_highlight_all = config_highlight_teammates;
    QtOverlay::overlay_config_overlay_propsurvival_radius = ui->doubleSpinBox_overlay_propsurvival_radius->value();
    QtOverlay::overlay_config_up_vector = QVector3D(0.0f, 0.0f, 1000.0f);
    QtOverlay::overlay_config_refresh_rate = ui->spinBox_refresh_rate->value();
#endif
}

void showMessageBox(const char* title, const char* text) {
    QMessageBox msgbox;
    msgbox.setWindowTitle(title);
    msgbox.setText(text);
    msgbox.exec();
}

void print_debug(const char* msg) {
    qDebug(msg);
}
