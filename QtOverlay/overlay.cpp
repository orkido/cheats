#include "overlay.hpp"

#include <Qt3DCore/Qt3DCore>
#include <Qt3DExtras/Qt3DExtras>
#include <Qt3DRender/Qt3DRender>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QCommandLinkButton>
#include <QtGui/QScreen>

#include "overlay_data_interface.hpp"

namespace QtOverlay {
Overlay::Overlay(QWidget* parent) : Qt3DExtras::Qt3DWindow()/*, container(QWidget::createWindowContainer(this, parent))*/, draw_timer(), rootEntity(nullptr), light(nullptr) {
    connect(this, SIGNAL(dataChanged()), SLOT(onDataChanged()));
    connect(this, SIGNAL(geometryChanged(QRect)), SLOT(onGeometryChanged(QRect)));

    /*container->setWindowFlags(container->windowFlags() | Qt::Window | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowStaysOnTopHint);
    container->setAttribute(Qt::WA_NoSystemBackground);
    container->setAttribute(Qt::WA_TranslucentBackground);
    container->showFullScreen();*/

    /*QPainter painter(container);
    painter.fillRect(10, 100, 100, 100, Qt::red);*/

    this->setFlags(this->flags() | Qt::Window | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowStaysOnTopHint);
    QSurfaceFormat surfaceFormat;
    surfaceFormat.setAlphaBufferSize(8);
    this->setFormat(surfaceFormat);
    this->defaultFrameGraph()->setClearColor(Qt::transparent);
    this->showFullScreen();

    draw_timer.setInterval(overlay_config.overlay_config_refresh_rate);
    connect(&draw_timer, &QTimer::timeout, [this]() { updateOverlay(); });
    draw_timer.start();

    // Root entity
    rootEntity = new Qt3DCore::QEntity();

    // Camera
    Qt3DRender::QCamera* cameraEntity = this->camera();

    float near_plane = 100.0f, far_plane = 6000.0f;
    cameraEntity->lens()->setPerspectiveProjection(data.camera_fov, static_cast<float>(this->screen()->size().width()) / static_cast<float>(this->screen()->size().height()), near_plane, far_plane);

    Qt3DCore::QEntity* lightEntity = new Qt3DCore::QEntity(rootEntity);
    light = new Qt3DRender::QDirectionalLight(lightEntity);
    light->setColor(Qt::white);
    light->setIntensity(1);
    light->setWorldDirection(cameraEntity->viewVector());
    lightEntity->addComponent(light);

    // Set root object of the scene
    this->setRootEntity(rootEntity);
}

void Overlay::updateOverlay() {
    // Cache shared data for fast mutex unlock
    bool camera_changed, camera_type_changed;
    CameraType camera_type;
    QVector3D camera_position, camera_angle;
    int32_t camera_fov;
    QVector3D camera_up_vector;
    QVector3D camera_view_direction;
    float camera_bottom, camera_top, camera_left, camera_right;
    std::vector<std::pair<EntityChange, std::shared_ptr<Entity>>> entity_change_list;

    data_mutex.lock();

    camera_changed = data.camera_changed;
    if (camera_changed) {
        data.camera_changed = false;
        camera_position = data.camera_position;
        camera_angle = data.camera_angle;
        camera_fov = data.camera_fov;
        camera_up_vector = data.camera_up_vector;
        camera_view_direction = data.camera_view_direction;
        camera_bottom = data.camera_bottom;
        camera_top = data.camera_top;
        camera_left = data.camera_left;
        camera_right = data.camera_right;
    }

    camera_type_changed = data.camera_type_changed;
    if (camera_type_changed) {
        data.camera_type_changed = false;
        camera_type = data.camera_type;
    }

    // Copy shared objects list into local list and clear the shared list
    while (!data.change_queue.empty()) {
        auto shared_change = data.change_queue.front();
        data.change_queue.pop();
        auto& shared_entity = data.entity_list[shared_change.shared_entity_list_index];
        entity_change_list.push_back(std::make_pair(EntityChange(shared_change), std::shared_ptr<Entity>(shared_entity->clone())));
    }
    data_mutex.unlock();

    // Update camera type
    if (camera_type_changed) {
        Qt3DRender::QCamera* cameraEntity = this->camera();
        if (camera_type == CameraType::PerspectiveProjection) {
            cameraEntity->setProjectionType(Qt3DRender::QCameraLens::PerspectiveProjection);
        } else if (camera_type == CameraType::OrthographicProjection) {
            cameraEntity->setProjectionType(Qt3DRender::QCameraLens::OrthographicProjection);
        }
    }

    // Update camera and light
    if (camera_changed) {
        constexpr float vector_length = 1000.0f; // Use large rotation vectors to mitigate cancellation in add/sub operations in fpu
        Qt3DRender::QCamera* cameraEntity = this->camera();
        cameraEntity->setFieldOfView(data.camera_fov);
        cameraEntity->setPosition(camera_position);

        cameraEntity->setUpVector(camera_up_vector);
        cameraEntity->setViewCenter(camera_position + camera_view_direction);
        cameraEntity->rotate(QQuaternion::fromAxisAndAngle(QVector3D(vector_length, 0, 0), camera_angle.x()));
        cameraEntity->rotate(QQuaternion::fromAxisAndAngle(QVector3D(0, vector_length, 0), camera_angle.y()));
        cameraEntity->rotate(QQuaternion::fromAxisAndAngle(QVector3D(0, 0, vector_length), camera_angle.z()));

        // Update orthographic projection mode only values
        cameraEntity->setBottom(camera_bottom);
        cameraEntity->setTop(camera_top);
        cameraEntity->setRight(camera_right);
        cameraEntity->setLeft(camera_left);

        light->setWorldDirection(cameraEntity->viewVector());
    }

    // Update entities
    for (auto& [entityChange, pEntity] : entity_change_list) {
        auto& entity = *pEntity;
        // Create overlay qentity entry for the index if it does not exist
        for (int i = overlay_entity_list.size(); i < entityChange.shared_entity_list_index + 1; ++i)
            overlay_entity_list.push_back(nullptr);

        // 1. Set entity type
        if (entityChange.type_changed) {
            auto old_entity = overlay_entity_list[entityChange.shared_entity_list_index];
            // Delete old 3D object
            if (old_entity) {
                old_entity->deleteLater();
                overlay_entity_list[entityChange.shared_entity_list_index] = old_entity = nullptr;
            }

            // Create new 3D object
            if ((IS_PropSurvival(entity.type) && entity.type != entity_type::PropSurvival_unneeded)
                || (IS_Player(entity.type) && (overlay_config.overlay_config_highlight_all || entity.type == entity_type::PlayerEnemy) && entity.type != entity_type::PlayerDead)) {
                auto entity_new = entity.to_qentity();
                assert(entity_new);
                overlay_entity_list[entityChange.shared_entity_list_index] = entity_new;

                entity_new->setParent(rootEntity);
                entityChange.pos_changed = true;
            }
        }

        // 2. Set entity position
        if (entityChange.pos_changed && overlay_entity_list[entityChange.shared_entity_list_index]) {
            auto overlay_entity = overlay_entity_list[entityChange.shared_entity_list_index];
            // Transform
            auto transform = overlay_entity->componentsOfType<Qt3DCore::QTransform>();
            assert(transform.size() == 1);
            transform[0]->setTranslation(entity.position);
        }
    }
    // Set the front of all entities towards camera
    for (auto overlay_entity : overlay_entity_list) {
        if (overlay_entity) {

            auto transform = overlay_entity->componentsOfType<Qt3DCore::QTransform>();
            assert(transform.size() == 1);

            QQuaternion rotation = QQuaternion::fromDirection(this->camera()->position() - transform[0]->translation(), camera_up_vector);
            transform[0]->setRotation(rotation);
        }
    }
}

void Overlay::onDataChanged() {
    // If this function is called we assume that it will always called when data changes therefore we can disable the timer
    this->draw_timer.stop();

    updateOverlay();
}

void Overlay::onGeometryChanged(QRect rect) {
    this->setGeometry(rect);
}
}
