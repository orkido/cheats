#ifndef overlay_data_interface_hpp
#define overlay_data_interface_hpp

#include <queue>
#include <mutex>
#include <vector>
#include <QtGui/qvector3d.h>
#include <Qt3DCore/qentity.h>

namespace QtOverlay {
enum class entity_type : uint32_t {
	None = 0x0,

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

class ApexEntity : public Entity {
	virtual ApexEntity* clone() override { return new ApexEntity(*this); }
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

	bool type_changed = false;
	bool pos_changed = false;
};

// Locked transfer data
extern std::mutex shared_data_mutex;
extern std::vector<std::shared_ptr<Entity>> shared_entity_list;
extern std::queue<EntityChange> shared_change_queue;
extern QVector3D shared_camera_position, shared_camera_angle;
extern int32_t shared_camera_fov;
extern bool shared_camera_changed;


// Configuration
extern QVector3D overlay_config_up_vector;
extern int32_t overlay_config_fov;
// update every N ms
extern uint32_t overlay_config_refresh_rate;
// radius of sphere in overlay
extern float overlay_config_overlay_propsurvival_radius;
extern bool overlay_config_highlight_all;
#endif

}
