#include <controller/Connect.h>

namespace Helix {
	namespace Controller {
		MStatus Connect::connect(Model::Base source, Model::Base target) {
			MStatus status;

			/*
			 * Find and save all current connections for future undo/redo functionality
			 */

			for(int i = 0; i < 2; ++i)
				m_previous_connections[i].clear();

			Model::Base previous_target = source.forward(status);

			if (!status && status != MStatus::kNotFound) {
				status.perror("Base::forward");
				return status;
			}

			m_previous_connections[0].reserve(status ? 2 : 1);
			m_previous_connections[0].push_back(source);

			if (status)
				m_previous_connections[0].push_back(previous_target);
			
			Model::Base previous_source = target.backward(status);

			if (!status && status != MStatus::kNotFound) {
				status.perror("Base::backward");
				return status;
			}

			m_previous_connections[1].reserve(status ? 2 : 1);
			m_previous_connections[0].push_back(target);

			if (status)
				m_previous_connections[0].push_back(previous_source);

			return redo();
		}

		MStatus Connect::undo() {
			MStatus status;

			/*
			 * Remove the newly created connection
			 */

			if (!(status = m_previous_connections[0][0].disconnect_forward())) {
				status.perror("Base::disconnect_forward");
				return status;
			}

			/*
			 * Make the old connections. Note that there is no connect_backward method, thus the indices have been switched
			 */

			if (!(status = m_previous_connections[0][0].connect_forward(m_previous_connections[0][1]))) {
				status.perror("Base::connect_forward 1");
				return status;
			}

			if (!(status = m_previous_connections[1][1].connect_forward(m_previous_connections[1][0]))) {
				status.perror("Base::connect_forward 2");
				return status;
			}

			return MStatus::kSuccess;
		}

		MStatus Connect::redo() {
			MStatus status;

			/*
			 * Disconnect the 'old' connections (or if restored by undo) then make the new connections
			 */

			if (m_previous_connections[0].size() > 1) {
				/*
				 * The base is connected, acting as the source
				 */

				if (!(status = m_previous_connections[0][0].disconnect_forward())) {
					status.perror("Base::disconnect_forward");
					return status;
				}
			}

			if (m_previous_connections[1].size() > 1) {
				/*
				 * This base too, is already connected, acting as the target
				 */

				if (!(status = m_previous_connections[1][0].disconnect_backward())) {
					status.perror("Base::disconnect_backward");
					return status;
				}
			}

			/*
			 * Connect the source to its new destination
			 */

			if (!(status = m_previous_connections[0][0].connect_forward(m_previous_connections[1][0]))) {
				status.perror("Base::connect_forward");
				return status;
			}

			return MStatus::kSuccess;
		}
	}
}
