#include <TextBasedTranslator.h>

#include <controller/TextBasedImporter.h>

namespace Helix {
	MStatus TextBasedTranslator::reader(const MFileObject& file, const MString & options, MPxFileTranslator::FileAccessMode mode) {
		HPRINT("Reading text based file \"%s\"", file.fullName().asChar());
		MStatus status;
		Controller::TextBasedImporter importer;
		HMEVALUATE_RETURN(status = importer.read(file.fullName().asChar()), status);
		return MStatus::kSuccess;
	}

	bool TextBasedTranslator::haveReadMethod() const {
		return true;
	}

	bool TextBasedTranslator::canBeOpened() const {
		return true;
	}

	MString TextBasedTranslator::defaultExtension() const {
		return HELIX_TEXT_BASED_FILE_TYPE;
	}

	MPxFileTranslator::MFileKind TextBasedTranslator::identifyFile(const MFileObject& file, const char *buffer, short size) const {
		const MString filepath(file.fullName());
		return filepath.rindexW("." HELIX_TEXT_BASED_FILE_TYPE) == int(filepath.length()) - int(strlen(HELIX_TEXT_BASED_FILE_TYPE)) - 1 ? MPxFileTranslator::kIsMyFileType : MPxFileTranslator::kNotMyFileType;
	}

	void *TextBasedTranslator::creator() {
		return new TextBasedTranslator();
	}
}
