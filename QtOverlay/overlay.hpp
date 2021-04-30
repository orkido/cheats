#ifndef overlay_hpp
#define overlay_hpp

#include "overlay_data_interface.hpp"

#include <QtQuick3D>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QWidget>

#include <mutex>
#include <vector>

namespace QtOverlay {
    class Overlay : QQmlApplicationEngine
    {
        Q_OBJECT
    public:
        explicit Overlay();

        QSize screen_size();

        std::mutex data_mutex;
        DataContainer data;

    private:
        QQuickWindow* window;
        std::vector<int> index_map;

    public slots:
        void onDataChanged();

    signals:
        void dataChanged();
    };
}
#endif
