/*
 * RoutedMeshTranslator.cpp
 *
 *  Created on: Feb 4, 2014
 *      Author: johan
 */

#include <RoutedMeshTranslator.h>
#include <controller/RoutedMeshImporter.h>

#include <cstring>

#include <maya/MPxFileTranslator.h>

namespace Helix {
	MStatus RoutedMeshTranslator::reader (const MFileObject& file, const MString & options, MPxFileTranslator::FileAccessMode mode) {
		const MString filepath(file.fullName());
		HPRINT("Parsing file \"%s\"", filepath.asChar());

		Controller::RoutedMeshImporter importer;
		MStatus status;
		HMEVALUATE_RETURN(status = importer.read(filepath.asChar()), status);
		return MStatus::kSuccess;
	}

	bool RoutedMeshTranslator::haveReadMethod () const {
		return true;
	}

	bool RoutedMeshTranslator::canBeOpened () const {
		return true;
	}

	MString RoutedMeshTranslator::defaultExtension () const {
		return HELIX_ROUTED_MESH_FILE_TYPE;
	}

	MPxFileTranslator::MFileKind RoutedMeshTranslator::identifyFile(const MFileObject& file, const char *buffer, short size) const {
		const MString filepath(file.fullName());
		std::cerr << "Is my filetype: " << (filepath.rindexW("." HELIX_ROUTED_MESH_FILE_TYPE) == int(filepath.length()) - int(strlen(HELIX_ROUTED_MESH_FILE_TYPE)) - 1) << std::endl;

		return filepath.rindexW("." HELIX_ROUTED_MESH_FILE_TYPE) == int(filepath.length()) - int(strlen(HELIX_ROUTED_MESH_FILE_TYPE)) - 1 ? MPxFileTranslator::kIsMyFileType : MPxFileTranslator::kNotMyFileType;
	}

	void *RoutedMeshTranslator::creator() {
		return new RoutedMeshTranslator();
	}
}


