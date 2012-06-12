/*
 * Object.cpp
 *
 *  Created on: 9 feb 2012
 *      Author: johan
 */

#include <model/Object.h>

namespace Helix {
	namespace Model {
		MObject & Object::getObject(MStatus & status) {
			status = MStatus::kSuccess;						
			
			if (m_object.isNull()) {
				if (!m_dagPath.isValid())
					status = MStatus::kInvalidParameter;
				else if (!(status = LoadObjectFromDagPath()))
					status.perror("Object::LoadObjectFromDagPath");
			}

			return m_object;
		}

		MDagPath & Object::getDagPath(MStatus & status) {
			bool isValid = m_dagPath.isValid(&status);

			if (!status) {
				status.perror("MDagPath::isValid");
				return m_dagPath;
			}

			if (!isValid) {
				if (m_object.isNull())
					status = MStatus::kInvalidParameter;
				else if (!(status = LoadAnyDagPathFromObject()))
					status.perror("Object::LoadAnyDagPathFromObject");
			}

			return m_dagPath;
		}

		MStatus Object::LoadObjectFromDagPath() {
			MStatus status;
			m_object = m_dagPath.node(&status);

			return status;
		}

		MStatus Object::LoadAnyDagPathFromObject() {
			return MDagPath::getAPathTo(m_object, m_dagPath);
		}
	}
}
