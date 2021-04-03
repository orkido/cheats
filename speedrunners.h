#ifndef SPEEDRUNNERS_H
#define SPEEDRUNNERS_H

#include <QMainWindow>
#include "QtOverlay/overlay.hpp"
#include <memory>

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
    std::shared_ptr<QtOverlay::Overlay> overlay_apex;
    std::shared_ptr<QtOverlay::Overlay> overlay_starcraft2_one;
    std::shared_ptr<QtOverlay::Overlay> overlay_starcraft2_two;

private slots:
    void handle_bt_speedrunners();
    void handle_bt_apex();
    void handle_bt_cod_warzone();
    void handle_bt_civilization_vi();
    void handle_bt_starcraft2();

    void update_settings();
};
#endif // SPEEDRUNNERS_H
