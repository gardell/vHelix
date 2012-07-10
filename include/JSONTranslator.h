/*
 * JSONTranslator.h
 *
 *  Created on: Jul 25, 2011
 *      Author: bjorn
 */

#ifndef JSONTRANSLATOR_H_
#define JSONTRANSLATOR_H_

#include <Definition.h>

#include <controller/JSONImporter.h>

#include <maya/MDagPath.h>
#include <maya/MPxFileTranslator.h>

#define HELIX_CADNANO_JSON_FILE_TYPE	"caDNAno json"

namespace Helix {
	class JSONTranslator : public MPxFileTranslator {
	public:
		virtual MStatus writer (const MFileObject& file, const MString& optionsString, MPxFileTranslator::FileAccessMode mode);
		virtual MStatus reader (const MFileObject& file, const MString & options, MPxFileTranslator::FileAccessMode mode);
		virtual bool haveWriteMethod () const;
		virtual bool haveReadMethod () const;
		virtual bool canBeOpened () const;
		virtual MString defaultExtension () const;
		virtual MPxFileTranslator::MFileKind identifyFile (	const MFileObject& file, const char* buffer, short size) const;

		static void *creator();

	private:
		Controller::JSONImporter m_operator;
	};
}

#endif /* JSONTRANSLATOR_H_ */
