#ifndef SPEEDRUNNERS_H
#define SPEEDRUNNERS_H

#include <QMainWindow>
#include "apexbot/src/overlay.hpp"

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
    Overlay* apex_overlay;

private slots:
    void handle_bt_speedrunners();

    void handle_bt_apex();

    void update_settings();
};
#endif // SPEEDRUNNERS_H
