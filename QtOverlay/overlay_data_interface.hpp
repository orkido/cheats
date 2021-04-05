#ifndef overlay_data_interface_hpp
#define overlay_data_interface_hpp

#include <queue>
#include <vector>
#include <memory>
#include <QtGui/qvector3d.h>
#include <Qt3DCore/qentity.h>

namespace QtOverlay {
enum class entity_type : uint32_t {
    None = 0x0, // Not existing element

    // Genernal items
    PropSurvival = 0x100,

    PropSurvival_weapon,
    PropSurvival_weapon_extension,
    PropSurvival_weapon_ammo,
    PropSurvival_throwable_ammo,
    PropSurvival_recharge, // Armor and health packs
    PropSurvival_unneeded,

    Player = 0x400, // all players if local player is invalid
    PlayerTeam,
    PlayerEnemy,
    PlayerNPC,
    PlayerDead, // all dead players, no difference by team
};
#define IS_PropSurvival(x) (static_cast<uint32_t>(x) & static_cast<uint32_t>(entity_type::PropSurvival))
#define IS_Player(x) (static_cast<uint32_t>(x) & static_cast<uint32_t>(entity_type::Player))

class Entity {
public:
    Entity() {
        type = entity_type::None;
        position = QVector3D();

        current_health = 0;
        max_health = 0;
        current_shield = 0;
        max_shield = 0;
    }

    virtual Entity* clone() = 0;
    virtual Qt3DCore::QEntity* to_qentity() = 0;

    entity_type type;
    QVector3D position;

    uint32_t current_health;
    uint32_t max_health;
    uint32_t current_shield;
    uint32_t max_shield;
};

class FirstPersonEntity : public Entity {
    virtual FirstPersonEntity* clone() override { return new FirstPersonEntity(*this); }
    virtual Qt3DCore::QEntity* to_qentity() override;
};

class RadarEntity : public Entity {
    virtual RadarEntity* clone() override { return new RadarEntity(*this); }
    virtual Qt3DCore::QEntity* to_qentity() override;
};

class EntityChange {
public:
    explicit EntityChange(uint32_t shared_entity_list_index, bool type_changed, bool pos_changed) {
        this->shared_entity_list_index = shared_entity_list_index;
        this->type_changed = type_changed;
        this->pos_changed = pos_changed;
    }

    uint32_t shared_entity_list_index;

    bool type_changed;
    bool pos_changed;
};

enum CameraType {
    PerspectiveProjection,
    OrthographicProjection,

};

class DataContainer {
public:
    // 3D first person overlay data
    std::vector<std::unique_ptr<Entity>> entity_list;
    std::queue<EntityChange> change_queue;
    QVector3D camera_position, camera_angle;
    QVector3D camera_up_vector;
    QVector3D camera_view_direction;
    int32_t camera_fov = 1; // Start with valid value
    bool camera_changed = false;
    CameraType camera_type = CameraType::PerspectiveProjection;
    bool camera_type_changed = false;

    // Orthographic projection mode only values
    float camera_bottom;
    float camera_top;
    float camera_right;
    float camera_left;
};

struct OverlayConfig {
    // Configuration first person
    int32_t overlay_config_fov;
    // update every N ms
    uint32_t overlay_config_refresh_rate;
    // radius of sphere in overlay
    float overlay_config_overlay_propsurvival_radius;
    bool overlay_config_highlight_all;

    // Configuration radar
    float radar_unit_radius;
};

extern struct OverlayConfig overlay_config;

}
#endif
