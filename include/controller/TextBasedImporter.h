#ifndef _TEXTBASEDCONTROLLER_H_
#define _TEXTBASEDCONTROLLER_H_

#include <DNA.h>
#include <Utility.h>

#include <model/Base.h>
#include <model/Helix.h>

#include <unordered_map>
#include <vector>

#include <maya/MQuaternion.h>

namespace Helix {
	namespace Controller {
		class VHELIXAPI TextBasedImporter {
		public:
			MStatus read(const char *filename);

		private:
			struct Base {
				std::string name, helixName, materialName;
				MVector position;
				DNA::Name label;

				inline Base(const char *name, const MVector & position, const char *materialName, const DNA::Name & label) : name(name), position(position), materialName(materialName), label(label) {}
				inline Base() {}
			};

			struct Helix {
				MVector position;
				MQuaternion orientation;
				std::string name;
				unsigned int bases; // Bases automatically added with the 'hb' command.

				inline Helix(const MVector & position, const MQuaternion & orientation, const char *name, unsigned int bases = 0) : position(position), orientation(orientation), name(name), bases(bases) {}
				inline Helix() : bases(0) {}
				inline bool operator==(const char *name) const { return this->name == name; }
			};

			struct Connection {
				enum Type {
					kForwardThreePrime,
					kForwardFivePrime,
					kBackwardThreePrime,
					kBackwardFivePrime,
					kNamed,
					kInvalid
				} fromType, toType;

				std::string fromHelixName, toHelixName, fromName, toName; // Only used when fromType/toType are kNamed.

				inline Connection(const char *fromHelixName, const char *fromName, const char *toHelixName, const char *toName, Type fromType, Type toType) : fromHelixName(fromHelixName), toHelixName(toHelixName), fromName(fromName), toName(toName), fromType(fromType), toType(toType) {}

				static Type TypeFromString(const char *type);
			};

			std::vector<Helix> helices;
			std::vector<Connection> connections;
			std::vector<Base> explicitBases; // Bases explicitly created with the 'b' command.
			std::unordered_map<std::string, DNA::Name> explicitBaseLabels;

			friend MStatus getBaseFromConnectionType(Model::Helix & helix, Connection::Type type, Model::Base & base);
		};
	}
}

#endif /* N _TEXTBASEDCONTROLLER_H_ */
