#ifndef _OXDNAEXPORTER_H_
#define _OXDNAEXPORTER_H_

#include <Definition.h>

#include <maya/MDagPath.h>
#include <maya/MPxFileTranslator.h>

#define HELIX_OXDNA_FILE_TYPE	"oxDNA " HELIX_OXDNA_TOP_FILE_TYPE ", " HELIX_OXDNA_CONF_FILE_TYPE

namespace Helix {
	class OxDnaTranslator : public MPxFileTranslator {
	public:
		virtual MStatus writer (const MFileObject& file, const MString& optionsString, MPxFileTranslator::FileAccessMode mode);
		virtual MStatus reader (const MFileObject& file, const MString & options, MPxFileTranslator::FileAccessMode mode);
		virtual bool haveWriteMethod () const;
		virtual bool haveReadMethod () const;
		virtual bool canBeOpened () const;
		virtual MString defaultExtension () const;
		virtual MPxFileTranslator::MFileKind identifyFile (	const MFileObject& file, const char *buffer, short size) const;

		static void *creator();
	};
}

#endif /* _OXDNAEXPORTER_H_ */

