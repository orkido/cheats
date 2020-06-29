#include "overlay_data_interface.hpp"
#include <Qt3DExtras>

namespace QtOverlay {

std::mutex shared_data_mutex;
std::vector<std::shared_ptr<Entity>> shared_entity_list;
std::queue<EntityChange> shared_change_queue;
QVector3D shared_camera_position, shared_camera_angle;
int32_t shared_camera_fov;
bool shared_camera_changed;

// Configuration
QVector3D overlay_config_up_vector;
int32_t overlay_config_fov;
uint32_t overlay_config_refresh_rate;
float overlay_config_overlay_propsurvival_radius;
bool overlay_config_highlight_all;

enum OverlayItemType {
    CubeMiddle,

    BoxLeft,
    BoxRight,
    BoxTop,
    BoxBottom,

    BarHealth,
    BarShield,
};

Qt3DCore::QEntity* ApexEntity::to_qentity()
{
    auto overlay_entities = std::vector<std::pair<OverlayItemType, Qt3DCore::QEntity*>>();

    if (IS_PropSurvival(this->type)) {
        auto sphere_mesh = new Qt3DExtras::QSphereMesh();
        sphere_mesh->setRadius(overlay_config_overlay_propsurvival_radius);
        sphere_mesh->setRings(100);
        sphere_mesh->setSlices(20);
        
        // transform
        Qt3DCore::QTransform* transform = new Qt3DCore::QTransform();

        // pack into entity object
        auto overlay_entity = new Qt3DCore::QEntity();
        overlay_entity->addComponent(sphere_mesh);
        overlay_entity->addComponent(transform);
        overlay_entities.push_back(std::make_pair(OverlayItemType::CubeMiddle, overlay_entity));
    }

    if (IS_Player(this->type)) {
        float health_percentage = static_cast<float>(this->current_health) / static_cast<float>(this->max_health);
        float shield_percentage = static_cast<float>(this->current_shield) / static_cast<float>(this->max_shield);

        health_percentage = std::fmaxf(health_percentage, 0.001f);
        shield_percentage = std::fmaxf(shield_percentage, 0.001f);


        // create mesh
        /*auto cuboid_mesh = new Qt3DExtras::QCuboidMesh();
        cuboid_mesh->setXExtent(overlay_config_overlay_propsurvival_radius * 1.0f);
        cuboid_mesh->setYExtent(overlay_config_overlay_propsurvival_radius * 1.0f);
        cuboid_mesh->setZExtent(overlay_config_overlay_propsurvival_radius * 1.0f);

        // transform
        Qt3DCore::QTransform* transform = new Qt3DCore::QTransform();

        // pack into entity object
        auto overlay_entity = new Qt3DCore::QEntity();
        overlay_entity->addComponent(cuboid_mesh);
        overlay_entity->addComponent(transform);
        overlay_entities.push_back(overlay_entity);*/


        // Box
        /*auto plane_mesh_left = new Qt3DExtras::QPlaneMesh();
        plane_mesh_left->setHeight(74.0f);
        plane_mesh_left->setWidth(3.0f);
        auto plane_mesh_right = new Qt3DExtras::QPlaneMesh();
        plane_mesh_right->setHeight(74.0f);
        plane_mesh_right->setWidth(3.0f);
        auto plane_mesh_top = new Qt3DExtras::QPlaneMesh();
        plane_mesh_top->setHeight(3.0f);
        plane_mesh_top->setWidth(32.0f);
        auto plane_mesh_bottom = new Qt3DExtras::QPlaneMesh();
        plane_mesh_bottom->setHeight(3.0f);
        plane_mesh_bottom->setWidth(32.0f);*/
        auto plane_mesh_left = new Qt3DExtras::QCuboidMesh();
        plane_mesh_left->setXExtent(3.0f);
        plane_mesh_left->setYExtent(74.0f);
        plane_mesh_left->setZExtent(3.0f);
        auto plane_mesh_right = new Qt3DExtras::QCuboidMesh();
        plane_mesh_right->setXExtent(3.0f);
        plane_mesh_right->setYExtent(74.0f);
        plane_mesh_right->setZExtent(3.0f);
        auto plane_mesh_top = new Qt3DExtras::QCuboidMesh();
        plane_mesh_top->setXExtent(32.0f);
        plane_mesh_top->setYExtent(3.0f);
        plane_mesh_top->setZExtent(3.0f);
        auto plane_mesh_bottom = new Qt3DExtras::QCuboidMesh();
        plane_mesh_bottom->setXExtent(32.0f);
        plane_mesh_bottom->setYExtent(3.0f);
        plane_mesh_bottom->setZExtent(3.0f);
        auto plane_mesh_health = new Qt3DExtras::QCuboidMesh();
        plane_mesh_health->setXExtent(2.0f);
        plane_mesh_health->setYExtent(74.0f * health_percentage);
        plane_mesh_health->setZExtent(2.0f);
        auto plane_mesh_shield = new Qt3DExtras::QCuboidMesh();
        plane_mesh_shield->setXExtent(2.0f);
        plane_mesh_shield->setYExtent(74.0f * shield_percentage);
        plane_mesh_shield->setZExtent(2.0f);

        // transform
        Qt3DCore::QTransform* transform_left = new Qt3DCore::QTransform();
        Qt3DCore::QTransform* transform_right = new Qt3DCore::QTransform();
        Qt3DCore::QTransform* transform_top = new Qt3DCore::QTransform();
        Qt3DCore::QTransform* transform_bottom = new Qt3DCore::QTransform();
        Qt3DCore::QTransform* transform_health = new Qt3DCore::QTransform();
        Qt3DCore::QTransform* transform_shield = new Qt3DCore::QTransform();
        transform_left->setTranslation(QVector3D(-16.0f, 37.0f, 0.0f));
        transform_right->setTranslation(QVector3D(16.0f, 37.0f, 0.0f));
        transform_top->setTranslation(QVector3D(0.0f, 74.0f, 0.0f));
        transform_bottom->setTranslation(QVector3D(0.0f, 0.0f, 0.0f));
        transform_health->setTranslation(QVector3D(19.5f, plane_mesh_health->yExtent() / 2.0f, 0.0f));
        transform_shield->setTranslation(QVector3D(22.5f, plane_mesh_shield->yExtent() / 2.0f, 0.0f));

        // pack into entity object
        auto overlay_entity_left = new Qt3DCore::QEntity();
        overlay_entity_left->addComponent(plane_mesh_left);
        overlay_entity_left->addComponent(transform_left);
        overlay_entities.push_back(std::make_pair(OverlayItemType::BoxLeft, overlay_entity_left));
        auto overlay_entity_right = new Qt3DCore::QEntity();
        overlay_entity_right->addComponent(plane_mesh_right);
        overlay_entity_right->addComponent(transform_right);
        overlay_entities.push_back(std::make_pair(OverlayItemType::BoxRight, overlay_entity_right));
        auto overlay_entity_top = new Qt3DCore::QEntity();
        overlay_entity_top->addComponent(plane_mesh_top);
        overlay_entity_top->addComponent(transform_top);
        overlay_entities.push_back(std::make_pair(OverlayItemType::BoxTop, overlay_entity_top));
        auto overlay_entity_bottom = new Qt3DCore::QEntity();
        overlay_entity_bottom->addComponent(plane_mesh_bottom);
        overlay_entity_bottom->addComponent(transform_bottom);
        overlay_entities.push_back(std::make_pair(OverlayItemType::BoxBottom, overlay_entity_bottom));
        auto overlay_entity_health = new Qt3DCore::QEntity();
        overlay_entity_health->addComponent(plane_mesh_health);
        overlay_entity_health->addComponent(transform_health);
        overlay_entities.push_back(std::make_pair(OverlayItemType::BarHealth, overlay_entity_health));
        auto overlay_entity_shield = new Qt3DCore::QEntity();
        overlay_entity_shield->addComponent(plane_mesh_shield);
        overlay_entity_shield->addComponent(transform_shield);
        overlay_entities.push_back(std::make_pair(OverlayItemType::BarShield, overlay_entity_shield));
    }

    for (auto [entity_type, overlay_entity] : overlay_entities) {
        Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial();
        switch (this->type) {
        // Items
        case entity_type::PropSurvival_recharge:
            material->setDiffuse(QColor(QRgb(0x663300)));
            break;
        case entity_type::PropSurvival_throwable_ammo:
            material->setDiffuse(QColor(QRgb(0x2a37eb)));
            break;
        case entity_type::PropSurvival_weapon:
            material->setDiffuse(QColor(QRgb(0xfc03ec)));
            break;
        case entity_type::PropSurvival_weapon_ammo:
            material->setDiffuse(QColor(QRgb(0x104a10)));
            break;
        case entity_type::PropSurvival_weapon_extension:
            material->setDiffuse(QColor(QRgb(0x5effbc)));
            break;

        // Players
        case entity_type::PlayerEnemy:
            material->setDiffuse(QColor(QRgb(0xff0000))); // red
            break;
        case entity_type::PlayerTeam:
            material->setDiffuse(QColor(QRgb(0x00ff00))); // green
            break;

        case entity_type::PlayerNPC:
        case entity_type::PropSurvival:
        case entity_type::PropSurvival_unneeded:
        case entity_type::Player:
        case entity_type::PlayerDead:
        default:
            if (auto sphere_mesh = overlay_entity->componentsOfType<Qt3DExtras::QSphereMesh>(); sphere_mesh.size() == 1)
                sphere_mesh[0]->setRadius(overlay_config_overlay_propsurvival_radius * 2.0);

            material->setDiffuse(QColor(QRgb(0xbeb32b))); // yellow
            break;
        }

        switch (entity_type) {
        case OverlayItemType::BarHealth:
            material->setDiffuse(Qt::red);
            break;
        case OverlayItemType::BarShield:
            material->setDiffuse(Qt::blue);
            break;
        default:
            break;
        }

        // Torus
        overlay_entity->addComponent(material);
        overlay_entity->setEnabled(true);
    }

    Qt3DCore::QEntity* combined_entity = nullptr;
    if (overlay_entities.size() >= 1) {
        combined_entity = new Qt3DCore::QEntity();

        // Create transform object to set entity position later
        Qt3DCore::QTransform* transform = new Qt3DCore::QTransform();
        combined_entity->addComponent(transform);
        combined_entity->setEnabled(true);

        for (auto [entity_type, overlay_entity] : overlay_entities)
            overlay_entity->setParent(combined_entity);

        overlay_entities.clear();
    } else {
        assert(false);
    }

    return combined_entity;
}
}