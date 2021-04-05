#pragma once

#include "overlay_data_interface.hpp"

#include <QObject>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <Qt3DExtras/Qt3DExtras>
#include <QWindow>

#include <mutex>

namespace QtOverlay {
    class Overlay : public Qt3DExtras::Qt3DWindow
    {
        Q_OBJECT
    public:
        explicit Overlay(QWidget* parent = nullptr);
        void updateOverlay();

        std::mutex data_mutex;
        DataContainer data;

    public slots:
        void onDataChanged();
        void onGeometryChanged(QRect);

    signals:
        void dataChanged();
        void geometryChanged(QRect);

    private:
        QTimer draw_timer;

        Qt3DCore::QEntity* rootEntity;
        Qt3DRender::QDirectionalLight* light;
        std::vector<Qt3DCore::QEntity*> overlay_entity_list;
        //QWidget* container;
    };
}
