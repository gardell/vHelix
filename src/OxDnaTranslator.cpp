/*
 * OxDnaTranslator.cpp
 *
 *  Created on: Jan 28, 2014
 *      Author: johan
 */

#include <Utility.h>
#include <OxDnaTranslator.h>

#include <model/Base.h>
#include <model/Helix.h>
#include <controller/OxDnaExporter.h>
#include <controller/OxDnaImporter.h>

#include <algorithm>
#include <functional>
#include <limits>

#include <maya/MProgressWindow.h>

namespace Helix {
	/*
	 * Helper method for obtaining the two filenames.
	 */
	void get_filenames(const MFileObject& file, MString & topology_filename, MString & configuration_filename, MString & vhelix_filename) {
		const MString filename(file.fullName());

		int extension = filename.rindexW("." HELIX_OXDNA_CONF_FILE_TYPE);

		if (extension != int(filename.length()) - int(strlen(HELIX_OXDNA_CONF_FILE_TYPE)) - 1) {
			extension = filename.rindexW("." HELIX_OXDNA_TOP_FILE_TYPE);

			if (extension != int(filename.length()) - int(strlen(HELIX_OXDNA_TOP_FILE_TYPE)) - 1)
				extension = -1;
		}

		const MString stripped_filename(filename.asChar(), extension != -1 ? extension : filename.length());
		configuration_filename = stripped_filename + "." HELIX_OXDNA_CONF_FILE_TYPE;
		topology_filename = stripped_filename + "." HELIX_OXDNA_TOP_FILE_TYPE;
		vhelix_filename = stripped_filename + "." HELIX_OXDNA_VHELIX_FILE_TYPE;
	}

	class OxDnaExporterWithAdvanceProgress : public Controller::OxDnaExporter {
	protected:

		MStatus doExecute(Model::Strand & element) {
			MProgressWindow::advanceProgress(1);
			return Controller::OxDnaExporter::doExecute(element);
		}
	};

	class OxDnaImportWithAdvanceProgress : public Controller::OxDnaImporter {
	protected:
		void onProcessStart(int count) {
			if (!MProgressWindow::reserve())
				MGlobal::displayWarning("Failed to reserve the progress window");

			MProgressWindow::setTitle("oxDNA Exporter");
			MProgressWindow::setProgressStatus("Writing strands...");
			MProgressWindow::setProgressRange(0, count);
			MProgressWindow::startProgress();
		}

		void onProcessStep() {
			MProgressWindow::advanceProgress(1);
		}

		void onProcessEnd() {
			MProgressWindow::endProgress();
		}
	};

	MStatus OxDnaTranslator::writer (const MFileObject& file, const MString& optionsString, MPxFileTranslator::FileAccessMode mode) {
		MStatus status;
		MObjectArray helices;
		HMEVALUATE_RETURN(status = Model::Helix::AllSelected(helices), status);

		if (helices.length() == 0) {
			HMEVALUATE_RETURN(status = Model::Helix::All(helices), status);
		}

		if (helices.length() == 0) {
			MGlobal::displayError("Nothing to export. Aborting...");
			return MStatus::kSuccess;
		}

		if (!MProgressWindow::reserve())
			MGlobal::displayWarning("Failed to reserve the progress window");

		MProgressWindow::setTitle("oxDNA Exporter");
		MProgressWindow::setProgressStatus("Identifying strands...");
		MProgressWindow::setProgressRange(0, helices.length());
		MProgressWindow::startProgress();

		MVector minTranslation(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()),
				maxTranslation(-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity());

		// Since a strand is defined by any base along it, the same strand will be obtained multiple times if we don't track them.
		// This is pretty slow but, since it is only run once it should be ok.
		std::list<Model::Strand> strands;

		for (unsigned int i = 0; i < helices.length(); ++i) {
			Model::Helix helix(helices[i]);

			for (Model::Helix::BaseIterator it = helix.begin(); it != helix.end(); ++it) {
				MVector translation;
				HMEVALUATE_RETURN(status = it->getTranslation(translation, MSpace::kWorld), status);

				minTranslation.x = std::min(minTranslation.x, translation.x);
				minTranslation.y = std::min(minTranslation.y, translation.y);
				minTranslation.z = std::min(minTranslation.z, translation.z);

				maxTranslation.x = std::max(maxTranslation.x, translation.x);
				maxTranslation.y = std::max(maxTranslation.y, translation.y);
				maxTranslation.z = std::max(maxTranslation.z, translation.z);

				if (std::find_if(strands.begin(), strands.end(), std::bind2nd(std::ptr_fun(&Model::Strand::Contains_base), *it)) == strands.end())
					strands.push_back(*it);
			}

			MProgressWindow::advanceProgress(1);
		}

		MProgressWindow::endProgress();

		if (!MProgressWindow::reserve())
			MGlobal::displayWarning("Failed to reserve the progress window");

		MProgressWindow::setTitle("oxDNA Exporter");
		MProgressWindow::setProgressStatus("Writing strands...");
		MProgressWindow::setProgressRange(0, helices.length());
		MProgressWindow::startProgress();

		OxDnaExporterWithAdvanceProgress exporter;
		HMEVALUATE(std::for_each(strands.begin(), strands.end(), exporter.execute()), exporter.status());

		MProgressWindow::endProgress();

		if (!exporter.status())
			return exporter.status();

		MString top_filename, conf_filename, vhelix_filename;
		get_filenames(file, top_filename, conf_filename, vhelix_filename);

		HMEVALUATE_RETURN(status = exporter.write(
				top_filename.asChar(), conf_filename.asChar(), vhelix_filename.asChar(), minTranslation, maxTranslation), status);

		return status;
	}

	MStatus OxDnaTranslator::reader (const MFileObject& file, const MString & options, MPxFileTranslator::FileAccessMode mode) {
		MStatus status;

		MString top_filename, conf_filename, vhelix_filename;
		get_filenames(file, top_filename, conf_filename, vhelix_filename);

		OxDnaImportWithAdvanceProgress importer;

		HMEVALUATE(status = importer.read(top_filename.asChar(), conf_filename.asChar(), vhelix_filename.asChar()), status);

		MProgressWindow::endProgress();

		return status;
	}

	bool OxDnaTranslator::haveWriteMethod () const {
		return true;
	}

	bool OxDnaTranslator::haveReadMethod () const {
		return true;
	}

	bool OxDnaTranslator::canBeOpened () const {
		return true;
	}

	MString OxDnaTranslator::defaultExtension () const {
		return HELIX_OXDNA_TOP_FILE_TYPE;
	}

	MPxFileTranslator::MFileKind OxDnaTranslator::identifyFile (const MFileObject& file, const char *buffer, short size) const {
		const MString filename(file.resolvedFullName().toLowerCase());

		if (filename.rindexW("." HELIX_OXDNA_CONF_FILE_TYPE) == int(filename.length()) - int(strlen(HELIX_OXDNA_CONF_FILE_TYPE)) - 1 ||
			filename.rindexW("." HELIX_OXDNA_TOP_FILE_TYPE) == int(filename.length()) - int(strlen(HELIX_OXDNA_TOP_FILE_TYPE)) - 1)
			return MPxFileTranslator::kIsMyFileType;
		else
			return MPxFileTranslator::kNotMyFileType;
	}

	void *OxDnaTranslator::creator() {
		return new OxDnaTranslator();
	}
}


