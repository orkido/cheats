#include "overlay.hpp"

#include <QtQuick3D>
#include <QtQml>

namespace QtOverlay {
	Overlay::Overlay() : QQmlApplicationEngine() {
		connect(this, SIGNAL(dataChanged()), SLOT(onDataChanged()));

		//QSurfaceFormat::setDefaultFormat(QQuick3D::idealSurfaceFormat());

		this->load(QUrl(QStringLiteral("qrc:/QtOverlay/overlay.qml")));

		for (auto obj : this->rootObjects()) {
			this->window = qobject_cast<QQuickWindow*>(obj);
			if (this->window) {
				break;
			}
		}
		if (!this->window)
			assert(false);
	}

	QSize Overlay::screen_size() {
		return this->window->screen()->size();
	}

	void Overlay::onDataChanged() {
		this->data_mutex.lock();

		// Create local copy of the data container
		DataContainer data_copy = this->data;

		// Reset change records
		this->data.camera_changed = false;
		this->data.camera_type_changed = false;
		this->data.window_changed = false;
		while (!this->data.change_queue.empty())
			this->data.change_queue.pop();
		
		this->data_mutex.unlock();

		bool dummy;

		while (!data_copy.change_queue.empty()) {
			auto change = std::move(data_copy.change_queue.front());
			data_copy.change_queue.pop();
			auto entity = data_copy.entity_list[change.entity_list_index];

			for (int i = index_map.size(); i < change.entity_list_index + 1; ++i)
				index_map.push_back(-1);

			if (change.type_changed) {
				// Delete if there is already an outdated node
				if (index_map[change.entity_list_index] != -1) {
					int tmp_index = index_map[change.entity_list_index];
					QMetaObject::invokeMethod(this->window, "delete_node",
						Q_RETURN_ARG(bool, dummy),
						Q_ARG(int, tmp_index)
					);

					// Reduce all following indicies of the removed element by one
					for (auto& index : index_map) {
						if (index > tmp_index) {
							--index;
						}
					}
					index_map[change.entity_list_index] = -1;
				}

				// Create a new node
				if ((IS_PropSurvival(entity->type) && entity->type != entity_type::PropSurvival_unneeded)
					|| (IS_Player(entity->type) && (overlay_config.overlay_config_highlight_all || entity->type == entity_type::PlayerEnemy) && entity->type != entity_type::PlayerDead)) {
					int new_index;
					double tmp_radar_radius = overlay_config.radar_unit_radius;
					QMetaObject::invokeMethod(this->window, "create_node",
						Q_RETURN_ARG(int, new_index),
						Q_ARG(double, tmp_radar_radius)
					);
					index_map[change.entity_list_index] = new_index;

					// Force position update
					change.pos_changed = true;
				}
			}

			// TODO: Set the front of all entities towards camera
			if (change.pos_changed && index_map[change.entity_list_index] != -1) {
				int tmp_index = index_map[change.entity_list_index];
				auto tmp_position = entity->position;
				auto tmp_rotation = QVector3D();
				QMetaObject::invokeMethod(this->window, "update_node",
					Q_RETURN_ARG(bool, dummy),
					Q_ARG(int, tmp_index),
					Q_ARG(QVector3D, tmp_position),
					Q_ARG(QVector3D, tmp_rotation)
				);
			}
		}

		if (data_copy.window_changed) {
			data_copy.camera_changed = true;
			this->window->setGeometry(data_copy.window_rectangle);
		}

		if (data_copy.camera_type_changed) {
			QMetaObject::invokeMethod(this->window, "update_camera_type",
				Q_RETURN_ARG(bool, dummy),
				Q_ARG(bool, data_copy.camera_type == QtOverlay::CameraType::PerspectiveProjection)
			);
			data_copy.camera_changed = true;
		}

		if (data_copy.camera_changed) {
			// TODO: support perspective camera
			data_copy.camera_fov = 0.0f;
			auto tmp_scale = QVector3D(data_copy.camera_right - data_copy.camera_left, data_copy.camera_top - data_copy.camera_bottom, 1.0f);
			tmp_scale /= QVector3D(this->window->width(), this->window->height(), 1.0f);

			QMetaObject::invokeMethod(this->window, "update_camera",
				Q_RETURN_ARG(bool, dummy),
				Q_ARG(double, data_copy.camera_fov),
				Q_ARG(QVector3D, data_copy.camera_position),
				Q_ARG(QVector3D, data_copy.camera_rotation),
				Q_ARG(QVector3D, tmp_scale)
			);
		}


	}
}
