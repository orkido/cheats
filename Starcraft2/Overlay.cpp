#include "../QtOverlay/overlay.hpp"
#include "Overlay.hpp"

namespace Starcraft2 {
Overlay::Overlay(void* shared_ptr_overlay1, void* shared_ptr_overlay2)
    : shared_ptr_overlay1(shared_ptr_overlay1), shared_ptr_overlay2(shared_ptr_overlay2)
{
}

#define IF_UPDATE_SET_TRUE(dst, src, out) \
if ((dst) != (src)) { \
    (dst) = (src); \
    (out) = true; \
}

void update_overlay_data(std::shared_ptr<QtOverlay::Overlay>& overlay, struct SC2Data* sc2_data) {
    if (!overlay)
        return;

    overlay->data_mutex.lock();

    // Fill overlay data list with default elements until its greater or equal our element count
    for (uint32_t i = overlay->data.entity_list.size(); i < sc2_data->units_length; ++i) {
        overlay->data.entity_list.push_back(std::shared_ptr<QtOverlay::Entity>(new QtOverlay::Entity()));
    }

    // Update overlay data list and create EntityChange elements if required
    for (uint32_t i = 0; i < sc2_data->units_length; ++i) {
        auto& entity = overlay->data.entity_list[i];
        auto& src_entity = sc2_data->units[i];
        
        bool type_changed = false;
        bool pos_changed = false;

        if (entity->current_health > 0) {
            if (src_entity.team == Team::Self) {
                IF_UPDATE_SET_TRUE(entity->type, QtOverlay::entity_type::Player, type_changed);
            } else if (src_entity.team == Team::Ally) {
                IF_UPDATE_SET_TRUE(entity->type, QtOverlay::entity_type::PlayerTeam, type_changed);
            } else if (src_entity.team == Team::Enemy) {
                IF_UPDATE_SET_TRUE(entity->type, QtOverlay::entity_type::PlayerEnemy, type_changed);
            } else if (src_entity.team == Team::Neutral) {
                IF_UPDATE_SET_TRUE(entity->type, QtOverlay::entity_type::PlayerNPC, type_changed);
            } else {
                IF_UPDATE_SET_TRUE(entity->type, QtOverlay::entity_type::PlayerEnemy, type_changed);
            }
        } else {
            IF_UPDATE_SET_TRUE(entity->type, QtOverlay::entity_type::PlayerDead, type_changed);
        }

        IF_UPDATE_SET_TRUE(entity->current_health, src_entity.health, type_changed);
        // IF_UPDATE_SET_TRUE(entity->max_health, src_entity.max_health, type_changed);
        IF_UPDATE_SET_TRUE(entity->current_shield, src_entity.shields, type_changed);
        // IF_UPDATE_SET_TRUE(entity->max_shield, src_entity.max_shields, type_changed);
        
        auto position = QVector3D(src_entity.position_x, src_entity.position_y, src_entity.position_z);
        IF_UPDATE_SET_TRUE(entity->position, position, pos_changed);

        if (pos_changed || type_changed)
            overlay->data.change_queue.push(QtOverlay::EntityChange(i, type_changed, pos_changed));
    }
    // Set all not (anymore) existing elements to none
    for (uint32_t i = sc2_data->units_length; i < overlay->data.entity_list.size(); ++i) {
        auto& entity = overlay->data.entity_list[i];

        bool pos_changed = false;
        bool type_changed = false;

        IF_UPDATE_SET_TRUE(entity->type, QtOverlay::entity_type::None, type_changed);

        if (pos_changed || type_changed)
            overlay->data.change_queue.push(QtOverlay::EntityChange(i, type_changed, pos_changed));
    }

    // TODO: Following is only valid for radar overlay
    // Update camera
    IF_UPDATE_SET_TRUE(overlay->data.camera_type, QtOverlay::CameraType::OrthographicProjection, overlay->data.camera_type_changed);

    auto playable_mapsize_x_min = sc2_data->playable_mapsize_x_min;
    auto playable_mapsize_y_min = sc2_data->playable_mapsize_y_min;
    auto playable_mapsize_x_max = sc2_data->playable_mapsize_x_max;
    auto playable_mapsize_y_max = sc2_data->playable_mapsize_y_max;
    auto playable_mapsize_x = playable_mapsize_x_max - playable_mapsize_x_min;
    auto playable_mapsize_y = playable_mapsize_y_max - playable_mapsize_y_min;

    constexpr float vector_length = 100.0f; // Use large rotation vectors to mitigate cancellation in add/sub operations in fpu
    auto camera_position = QVector3D(playable_mapsize_x / 2.0f + playable_mapsize_x_min, playable_mapsize_y / 2.0f + playable_mapsize_y_min, 4000.0f);
    auto camera_rotation = QVector3D(0.0f, 0.0f, 0.0f);
    auto camera_fov = QtOverlay::overlay_config.overlay_config_fov;
    // auto camera_up_vector = QVector3D(0.0f, 0.0f, vector_length);
    // auto camera_view_direction = QVector3D(0.0f, vector_length, 0.0f);
    float camera_bottom = -(playable_mapsize_y / 2.0f);
    float camera_top = playable_mapsize_y / 2.0f;
    float camera_right = playable_mapsize_x / 2.0f;
    float camera_left = -(playable_mapsize_x / 2.0f);

    IF_UPDATE_SET_TRUE(overlay->data.camera_position, camera_position, overlay->data.camera_changed);
    IF_UPDATE_SET_TRUE(overlay->data.camera_rotation, camera_rotation, overlay->data.camera_changed);
    IF_UPDATE_SET_TRUE(overlay->data.camera_fov, camera_fov, overlay->data.camera_changed);
    // IF_UPDATE_SET_TRUE(overlay->data.camera_up_vector, camera_up_vector, overlay->data.camera_changed);
    // IF_UPDATE_SET_TRUE(overlay->data.camera_view_direction, camera_view_direction, overlay->data.camera_changed);
    IF_UPDATE_SET_TRUE(overlay->data.camera_bottom, camera_bottom, overlay->data.camera_changed);
    IF_UPDATE_SET_TRUE(overlay->data.camera_top, camera_top, overlay->data.camera_changed);
    IF_UPDATE_SET_TRUE(overlay->data.camera_right, camera_right, overlay->data.camera_changed);
    IF_UPDATE_SET_TRUE(overlay->data.camera_left, camera_left, overlay->data.camera_changed);

    // Create minimap size
    QSize wSize(playable_mapsize_x, playable_mapsize_y);
    wSize.scale(355, 346, Qt::KeepAspectRatio);
    // Center minimap over real minimap
    QRect rect(QPoint(), wSize);
    // overlay_radar->screen()->size().width()
    rect.moveCenter(QPoint(38 + (355 / 2), overlay->screen_size().height() - (18 + (346 / 2))));

    IF_UPDATE_SET_TRUE(overlay->data.window_rectangle, rect, overlay->data.window_changed);

    overlay->data_mutex.unlock();

    emit overlay->dataChanged();
}

void Overlay::update_overlay(struct SC2Data* sc2_data) {
    if (!sc2_data->startup_done)
        return;

    std::shared_ptr<QtOverlay::Overlay>* overlay1_ = (std::shared_ptr<QtOverlay::Overlay>*)shared_ptr_overlay1;
    std::shared_ptr<QtOverlay::Overlay> overlay_third_person = *overlay1_;

    std::shared_ptr<QtOverlay::Overlay>* overlay2_ = (std::shared_ptr<QtOverlay::Overlay>*)shared_ptr_overlay2;
    std::shared_ptr<QtOverlay::Overlay> overlay_radar = *overlay2_;

    update_overlay_data(overlay_radar, sc2_data);
}
}
