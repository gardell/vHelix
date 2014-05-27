#include <TextBasedTranslator.h>

#include <controller/TextBasedImporter.h>

namespace Helix {
	MStatus TextBasedTranslator::reader(const MFileObject& file, const MString & options, MPxFileTranslator::FileAccessMode mode) {
		MStatus status;
		Controller::TextBasedImporter importer;

		int nicking_min_length = 0, nicking_max_length = 0;
		MStringArray options_array;
		options.split(';', options_array);
		for (unsigned int i = 0; i < options_array.length(); ++i) {
			const char *line = options_array[i].asChar(); // for debugging only!
			sscanf(options_array[i].asChar(), "nicking_min_length=%u", &nicking_min_length);
			sscanf(options_array[i].asChar(), "nicking_max_length=%u", &nicking_max_length);
		}

		HPRINT("Reading text based file \"%s\". Options: %s parsed to min_length: %u, max_length: %u", file.fullName().asChar(), options.asChar(), nicking_min_length, nicking_max_length);
		HMEVALUATE_RETURN(status = importer.read(file.fullName().asChar(), nicking_min_length, nicking_max_length), status);
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
