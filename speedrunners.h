#ifndef SPEEDRUNNERS_H
#define SPEEDRUNNERS_H

#include <QMainWindow>

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
};
#endif // SPEEDRUNNERS_H
