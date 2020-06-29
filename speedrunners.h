#ifndef SPEEDRUNNERS_H
#define SPEEDRUNNERS_H

#include <QMainWindow>
#include "QtOverlay/overlay.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class SpeedRunners; }
QT_END_NAMESPACE

class SpeedRunners : public QMainWindow
{
    Q_OBJECT

public:
    SpeedRunners(QWidget *parent = nullptr);
    ~SpeedRunners();

private:
    Ui::SpeedRunners *ui;
    QtOverlay::Overlay* overlay;

private slots:
    void handle_bt_speedrunners();
    void handle_bt_apex();
    void handle_bt_cod_warzone();
    void handle_bt_civilization_vi();

    void update_settings();
};
#endif // SPEEDRUNNERS_H
