/*
 * HelixLocator.cpp
 *
 *  Created on: 9 jun 2011
 *      Author: johan
 */

#include <Locator.h>
#include <Tracker.h>
#include <HelixBase.h>
#include <Utility.h>
#include <DNA.h>
#include <ToggleLocatorRender.h>

#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MFnDagNode.h>
#include <maya/MPxTransform.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MPlugArray.h>
#include <maya/MFnCamera.h>

// I include the OpenGL headers directly for OpenGL 2.0, even though Maya has it's own definition of OpenGL calls

#ifdef MAC_PLUGIN

#include <OpenGL/glext.h>
#include <OpenGL/gl.h>

#else

#include <GL/gl.h>
#include <GL/glext.h>

#endif /* N MAC_PLUGIN */

// The Python command to get the nodes we're interested in

#define PYTHON_COMMAND_RENDERABLE_DATA	\
	"import maya.cmds as cmds\n" \
	"selectedChildBases = set(pm.ls(selection = True, type = 'HelixBase')).intersection(pm.listRelatives('vHelix1', c = True))\n" \
	"return_value = []\n" \
	"for base in selectedChildBases:\n" \
	"    return_value.extend(base.getTranslation())\n" \
	"    return_value.extend([ 1, pm.getAttr(base + '.label'), int(pm.connectionInfo(base + '.label', isDestination = True)) ])\n" \
	"return return_value\n"

//
// OpenGL 2.0 extensions
//

#if defined(WIN32) || defined(WIN64)
#define GETPROCADDRESS(str)		wglGetProcAddress((LPCSTR) str)
#else
#define GETPROCADDRESS(str)		glXGetProcAddressARB((const GLubyte *) str)
#endif /* No windows, FIXME: Mac OS */

namespace Helix {
	const MTypeId HelixLocator::id(HELIX_LOCATOR_ID);
	const MTypeId transform_id(HELIX_TRANSFORM_ID);
	//MObject HelixLocator::aBases;
	bool HelixLocator::s_gl_initialized = false, HelixLocator::s_gl_failed = false;
	GLint HelixLocator::s_program = 0, HelixLocator::s_vertex_shader = 0, HelixLocator::s_fragment_shader = 0, HelixLocator::s_screen_dimensions_uniform = -1;
	
#ifndef MAC_PLUGIN
	PFNGLCREATEPROGRAMPROC glCreateProgram;
	PFNGLCREATESHADERPROC glCreateShader;
	PFNGLATTACHSHADERPROC glAttachShader;
	PFNGLCOMPILESHADERPROC glCompileShader;
	PFNGLLINKPROGRAMPROC glLinkProgram;
	PFNGLSHADERSOURCEPROC glShaderSource;
	PFNGLDELETESHADERPROC glDeleteShader;
	PFNGLDELETEPROGRAMPROC glDeleteProgram;
	PFNGLUSEPROGRAMPROC glUseProgram;
	PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
	PFNGLGETSHADERIVPROC glGetShaderiv;
	PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
	PFNGLGETPROGRAMIVPROC glGetProgramiv;
	PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
	PFNGLUNIFORM2FPROC glUniform2f;
#endif /* N MAC_PLUGIN */
	
	HelixLocator::~HelixLocator() {
		/*if (m_polygon)
			delete m_polygon;*/
	}

	/*MStatus HelixLocator::compute(const MPlug & plug, MDataBlock & data) {
		return MS::kUnknownParameter;
	}*/

	void HelixLocator::initializeGL() {

		//
		// New OpenGL 2.0 based renderer. Initialize the two shaders and the program
		//

		// Load the OpenGL 2.0 procedures
		// FIXME: Signal the errors in a better way

#if defined(WIN32) || defined(WIN64)
		if (wglGetProcAddress == NULL) {
			std::cerr << "Fatal, wglGetProcAddress is not available" << std::endl;
			s_gl_failed = true;
			return;
		}
#endif /* WIN* */

#ifndef MAC_PLUGIN
		if ((glCreateProgram = (PFNGLCREATEPROGRAMPROC) GETPROCADDRESS("glCreateProgram")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

		if ((glCreateShader = (PFNGLCREATESHADERPROC) GETPROCADDRESS("glCreateShader")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}
		if ((glAttachShader = (PFNGLATTACHSHADERPROC) GETPROCADDRESS("glAttachShader")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

		if ((glCompileShader = (PFNGLCOMPILESHADERPROC) GETPROCADDRESS("glCompileShader")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

		if ((glLinkProgram = (PFNGLLINKPROGRAMPROC) GETPROCADDRESS("glLinkProgram")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

		if ((glShaderSource = (PFNGLSHADERSOURCEPROC) GETPROCADDRESS("glShaderSource")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

		if ((glDeleteShader = (PFNGLDELETESHADERPROC) GETPROCADDRESS("glDeleteShader")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

		if ((glDeleteProgram = (PFNGLDELETEPROGRAMPROC) GETPROCADDRESS("glDeleteProgram")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

		if ((glUseProgram = (PFNGLUSEPROGRAMPROC) GETPROCADDRESS("glUseProgram")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

		if ((glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) GETPROCADDRESS("glGetShaderInfoLog")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

		if ((glGetShaderiv = (PFNGLGETSHADERIVPROC) GETPROCADDRESS("glGetShaderiv")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

		if ((glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) GETPROCADDRESS("glGetProgramInfoLog")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

		if ((glGetProgramiv = (PFNGLGETPROGRAMIVPROC) GETPROCADDRESS("glGetProgramiv")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

		if ((glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) GETPROCADDRESS("glGetUniformLocation")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

		if ((glUniform2f = (PFNGLUNIFORM2FPROC) GETPROCADDRESS("glUniform2f")) == NULL) {
			std::cerr << "Fatal, Failed to load OpenGL shader procedures" << std::endl;
			s_gl_failed = true;
		}

#endif /* N MAC_PLUGIN */
		
		// Setup the shaders and programs

		if ((s_program = glCreateProgram()) == 0) {
			std::cerr << "Fatal, failed to create an OpenGL program object" << std::endl;
			s_gl_failed = true;
		}

		if ((s_vertex_shader = glCreateShader(GL_VERTEX_SHADER)) == 0) {
			std::cerr << "Fatal, failed to create an OpenGL shader object" << std::endl;
			s_gl_failed = true;
		}

		if ((s_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER)) == 0) {
			std::cerr << "Fatal, failed to create an OpenGL shader object" << std::endl;
			s_gl_failed = true;
		}

		static const char *vertex_shader_source[] = { GLSL_VERTEX_SHADER },
						  *fragment_shader_source[] = { GLSL_FRAGMENT_SHADER };

		glShaderSource(s_vertex_shader, GLSL_VERTEX_SHADER_COUNT, vertex_shader_source, NULL);
		glShaderSource(s_fragment_shader, GLSL_FRAGMENT_SHADER_COUNT, fragment_shader_source, NULL);

		glCompileShader(s_vertex_shader);
		glCompileShader(s_fragment_shader);

		GLint status;

		{
			GLint shaders[] = { s_vertex_shader, s_fragment_shader };

			for(int i = 0; i < 2; ++i) {
				glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &status);

				if (status != GL_TRUE) {
					GLint logLength;
					glGetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &logLength);
					GLchar *log = new GLchar[logLength];
					glGetShaderInfoLog(shaders[i], logLength, NULL, log);

					std::cerr << "OpenGL shader info log (" << (i + 1) << "): " << std::endl << log << std::endl << std::endl;

					s_gl_failed = true;
				}

				glAttachShader(s_program, shaders[i]);
			}
		}

		glLinkProgram(s_program);

		glGetProgramiv(s_program, GL_LINK_STATUS, &status);

		if (status != GL_TRUE) {
			GLint logLength;
			glGetProgramiv(s_program, GL_INFO_LOG_LENGTH, &logLength);
			GLchar *log = new GLchar[logLength];
			glGetProgramInfoLog(s_program, logLength, NULL, log);

			std::cerr << "OpenGL program info log: " << std::endl << log << std::endl << std::endl;

			s_gl_failed = true;
		}

		/*
		 * Set up screen height uniform required to scale the point sprites
		 */

		if ((s_screen_dimensions_uniform = glGetUniformLocation(s_program, "screen")) == -1) {
			std::cerr << "Failed to obtain location to screenHeight uniform variable" << std::endl;
			s_gl_failed = true;
		}

		s_gl_initialized = true;
	}

	static const unsigned char static_colors[] [4] = {
			SELECTED_HALO_COLOR, SELECTED_NEIGHBOUR_HALO_COLOR, SELECTED_FIVEPRIME_HALO_COLOR, SELECTED_THREEPRIME_HALO_COLOR,
			SELECTED_ADJACENT_HALO_COLOR, SELECTED_ADJACENT_NEIGHBOUR_HALO_COLOR, SELECTED_ADJACENT_FIVEPRIME_HALO_COLOR, SELECTED_ADJACENT_THREEPRIME_HALO_COLOR
	};

	// Helper method
	/*MStatus FromDagPathArrayOfTransformToFloatArray(MDagPathArray & dagPathArray, float *vertices, unsigned char *colors, size_t index, size_t color_index, char *sequence) {
		MStatus stat;

		for(size_t i = 0; i < dagPathArray.length(); ++i) {
			MFnTransform transform(dagPathArray[i], &stat);
			MVector worldTranslation;

			if (!stat) {
				stat.perror("MFnTransform::#ctor");
				return stat;
			}

			worldTranslation = transform.getTranslation(MSpace::kWorld, &stat);

			if (!stat) {
				stat.perror("MFnTransform::getTranslation");
				return stat;
			}

			for(size_t j = 0; j < 3; ++j) {
				vertices[(index + i) * 3 + j] = worldTranslation[j];
				colors[(index + i) * 4 + j] = static_colors[color_index][j];
			}
			colors[(index + i) * 4 + 3] = static_colors[color_index][3];
		}

		return MStatus::kSuccess;
	}*/

	// Helper method
	MStatus recursiveSearchForNeighbourBases(MObjectArray children, MDagPath dagPath, MObject forward_attribute, MObject backward_attribute, MDagPathArray & selectedNeighbourBases, MDagPathArray & endBases, bool force = false) {
		MStatus status;

		size_t selectedNeighbourBases_length = selectedNeighbourBases.length();

		if (!force) {
			for (unsigned int i = 0; i < selectedNeighbourBases_length; ++i) {
				if (selectedNeighbourBases[i] == dagPath) {
					// Already exists, we're not going to work anymore with this node
					return MStatus::kSuccess;
				}
			}
		}

		// Make sure there is a connection here
		//

		bool found = false;

		for(unsigned int i = 0; i < children.length(); ++i) {
			if (children[i] == dagPath.node(&status)) {
				found = true;
				break;
			}
		}

		if (!found)
			return MStatus::kSuccess;


		selectedNeighbourBases.append(dagPath);

		MObject node = dagPath.node(&status);

		if (!status) {
			status.perror("MDagPath::node");
			return status;
		}

		MPlug iteratorPlug(dagPath.node(), forward_attribute);

		bool hasConnections = false;

		if (iteratorPlug.isConnected(&status)) {
			MPlugArray plugArray;

			if (iteratorPlug.connectedTo(plugArray, true, true, &status)) {
				size_t plugArray_length = plugArray.length();

				for(unsigned int i = 0; i < plugArray_length; ++i) {
					MObject targetObject = plugArray[i].node(&status);

					if (!status) {
						status.perror("MPlugArray[i]::node");
						return status;
					}

					MFnDagNode target_dagNode(targetObject);

					if (plugArray[i] == backward_attribute && target_dagNode.typeId(&status) == HelixBase::id) {
						MDagPath target_dagPath;

						if (!(status = target_dagNode.getPath(target_dagPath))) {
							status.perror("MFnDagNode::getPath");
							return status;
						}

						// This is a valid HelixBase target, lets iterate over it

						hasConnections = true;
						recursiveSearchForNeighbourBases(children, target_dagPath, forward_attribute, backward_attribute, selectedNeighbourBases, endBases);
					}

					if (!status) {
						status.perror("MFnDagNode::typeName");
						return status;
					}
				}
			}
		}

		if (!hasConnections) {
			endBases.append(dagPath);
		}

		if (!status) {
			status.perror("MPlug::isConnected");
			return status;
		}

		return MStatus::kSuccess;
	}

	// Helper method. MUST increase line_index and vertex_index!
	MStatus ExtractDataForRendering(MDagPathArray array, unsigned int & vertex_index, unsigned int & line_index, float *vertices, float *line_vertices, unsigned char *colors, unsigned char *line_colors, char *sequence, int coloring) {
		MStatus status;

		for(unsigned int i = 0; i < array.length(); ++i) {
			MFnDagNode dagNode(array[i]);
			MObject object;

			object = dagNode.object(&status);

			if (!status) {
				status.perror("MFnDagNode::object");
				return status;
			}

			MFnTransform transform(array[i]);

			for (unsigned int j = 0; j < 4; ++j)
				colors [vertex_index * 4 + j] = static_colors[coloring][j];

			MVector vector = transform.getTranslation(MSpace::kTransform, &status);

			for (unsigned int j = 0; j < 3; ++j)
				vertices[vertex_index * 3 + j] = (GLfloat) vector[j];

			// Extract label information
			//

			MPlug labelPlug(dagNode.object(), HelixBase::aLabel);
			DNA::Names label;

			if (!(status = labelPlug.getValue((int &) label))) {
				status.perror("MPlug::getValue");
				return status;
			}

			if (labelPlug.isDestination(&status))
				label = DNA::OppositeBase(label);

			if (!status) {
				status.perror("MPlug::isDestination");
				return status;
			}

			sequence[vertex_index++] = DNA::ToChar(label);

			MDagPath opposite;
			if (HelixBase_NextBase(object, HelixBase::aLabel, opposite, &status)) {
				// We have a pair!

				MFnTransform opposite_transform(opposite);
				MVector opposite_vector = opposite_transform.getTranslation(MSpace::kTransform, &status);

				if (!status) {
					status.perror("MFnTransform::getTranslation");
					return status;
				}

				// Render a halo and label around the opposite base too
				//

				/*
				 * NOT INTERESTING ANYMORE; NOTE: To bring this back, you must increase the amount of allocated RAM, below
				 * or it will CRASH!
				 *
				for (size_t j = 0; j < 3; ++j)
					vertices[vertex_index * 3 + j] = (GLfloat) opposite_vector[j];

				for (size_t j = 0; j < 4; ++j)
					colors [vertex_index * 4 + j] = (GLubyte) static_colors[coloring + 4][j];

				sequence[vertex_index++] = DNA::ToChar(DNA::OppositeBase(label));*/

				// Add a connecting line between the bases
				//

				for(unsigned int j = 0; j < 3; ++j) {
					line_vertices[line_index * 3 + j] = (GLfloat) vector[j];
					line_vertices[(line_index + 1) * 3 + j] = (GLfloat) opposite_vector[j];
				}

				for(unsigned int j = 0; j < 4; ++j) {
					line_colors[line_index * 4 + j] = static_colors[coloring][j];
					line_colors[(line_index + 1) * 4 + j] = static_colors[coloring][j];
				}

				line_index += 2;
			}
		}

		return status;
	}

	void HelixLocator::draw(M3dView &view, const MDagPath &path, M3dView::DisplayStyle style, M3dView::DisplayStatus status) {
		/* We assume we're grouped under a vHelix type.
		 * - Figure out what the vHelix is called,
		 * - get the current list of selected HelixBase's,
		 * - filter out only the ones that are children of the vHelix,
		 * - find out what connected helices these have and add them to the rendering (additional)
		 */

		MStatus stat;

		// Do not render if the view is not the perspective view. It needs to be taken care of differently
		//

		{
			MDagPath camera_dagPath;

			if (!(stat = view.getCamera(camera_dagPath))) {
				stat.perror("M3dView::getCamera");
				return;
			}

			MFnCamera camera(camera_dagPath);

			if (camera.isOrtho(&stat))
				return;

			if (!stat) {
				stat.perror("MFnCamera::isOrtho");
				return;
			}
		}

		//
		// Obtain the required information for rendering
		//

		// Find all selected HelixBases

		MObjectArray selectedBases, selectedNeighbourBases;
		MDagPathArray neighbourBases, fivePrimeBases, threePrimeBases, selectedChildBases;

		if (!(stat = SelectedStrands(selectedNeighbourBases, selectedBases))) {
			stat.perror("SelectedBases");
			return;
		}

		// Find all children of type HelixBase and match them with the selectedBases array

		MFnDagNode this_dagNode(thisMObject());
		MFnDagNode parent_dagNode(this_dagNode.parent(0));

		size_t childCount = parent_dagNode.childCount(&stat);

		if (!stat) {
			stat.perror("MFnDagNode::childCount");
			return;
		}

		for(unsigned int i = 0; i < childCount; ++i) {
			MObject child = parent_dagNode.child(i, &stat);

			if (!stat) {
				stat.perror("MFnDagNode::child");
				return;
			}

			// If this base is in the selected bases, add it to the selectedChildBases
			//

			for(unsigned int j = 0; j < selectedBases.length(); ++j) {
				if (selectedBases[j] == child) {
					MFnDagNode dagNode(selectedBases[j]);
					MDagPath dagPath;

					if (!(stat = dagNode.getPath(dagPath))) {
						stat.perror("MFnDagNode::getPath");
						return;
					}

					selectedChildBases.append(dagPath);
				}

				if (!stat) {
					stat.perror("MFnDagNode::child");
					return;
				}
			}

			// If this base is in the selectedNeighbourBases, add it to the neighbourBases

			for(unsigned int j = 0; j < selectedNeighbourBases.length(); ++j) {
				if (selectedNeighbourBases[j] == child) {
					MFnDagNode dagNode(selectedNeighbourBases[j]);
					MDagPath dagPath;

					if (!(stat = dagNode.getPath(dagPath))) {
						stat.perror("MFnDagNode::getPath");
						return;
					}

					neighbourBases.append(dagPath);

					unsigned int endType = HelixBase_endType(child, &stat);

					if (!stat) {
						stat.perror("HelixBase_endType");
						continue;
					}

					if (endType & FivePrime)
						fivePrimeBases.append(dagPath);
					else if (endType & ThreePrime)
						threePrimeBases.append(dagPath);
				}

				if (!stat) {
					stat.perror("MFnDagNode::child");
					return;
				}
			}
		}

		// Ok, we have gathered all the nodes required, now extract the information required for rendering
		//

		size_t vertices_count = selectedChildBases.length() + neighbourBases.length() + fivePrimeBases.length() + threePrimeBases.length();
		// NOTE; Multiply length by two if adjacent bases are to be rendered. See code above!!
		float *vertices = new float[vertices_count * 3], *line_vertices = new float[vertices_count * 3 * 2];
		unsigned char *colors = new unsigned char[vertices_count * 4], *line_colors = new unsigned char[vertices_count * 4 * 2];
		char *sequence = new char[vertices_count * 2];
		unsigned int vertex_index = 0, line_index = 0;

		if (!(stat = ExtractDataForRendering(neighbourBases, vertex_index, line_index, vertices, line_vertices, colors, line_colors, sequence, 1))) {
			stat.perror("ExtractDataForRendering");
			return;
		}

		if (!(stat = ExtractDataForRendering(fivePrimeBases, vertex_index, line_index, vertices, line_vertices, colors, line_colors, sequence, 2))) {
			stat.perror("ExtractDataForRendering");
			return;
		}

		if (!(stat = ExtractDataForRendering(threePrimeBases, vertex_index, line_index, vertices, line_vertices, colors, line_colors, sequence, 3))) {
			stat.perror("ExtractDataForRendering");
			return;
		}

		if (!(stat = ExtractDataForRendering(selectedChildBases, vertex_index, line_index, vertices, line_vertices, colors, line_colors, sequence, 0))) {
			stat.perror("ExtractDataForRendering");
			return;
		}

		// The actual rendering using OpenGL
		//

		view.beginGL();

		if (!s_gl_initialized) {
			if (s_gl_failed) {
				view.endGL();
				return;
			}

			initializeGL();

			if (s_gl_failed) {
				view.endGL();
				return;
			}
		}

		glPushAttrib(GL_POINT_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT | GL_LIGHTING_BIT | GL_LINE_BIT);
		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		//glDisable(GL_DEPTH_TEST); // I think we want this, at least when we have blending
		glDepthMask(GL_FALSE); // This is better than disabling the depth test, now blending works, but not through solid objects!

		// Render halos using point sprites
		//

		if (ToggleLocatorRender::CurrentRender & ToggleLocatorRender::kRenderHalo) {
			glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

			glEnable(GL_POINT_SPRITE);
			glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

			glUseProgram(s_program);

			glUniform2f(s_screen_dimensions_uniform, (GLfloat) view.portWidth(), (GLfloat) view.portHeight());

			glVertexPointer(3, GL_FLOAT, 0, vertices);
			glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);

			glDrawArrays(GL_POINTS, 0, GLsizei(vertex_index));

			glUseProgram(0);
		}

		// Render the lines
		//

		if (ToggleLocatorRender::CurrentRender & ToggleLocatorRender::kRenderPairLines) {
			glLineWidth(BASE_CONNECTIONS_LINE_WIDTH);

			glColorPointer(4, GL_UNSIGNED_BYTE, 0, line_colors);
			glVertexPointer(3, GL_FLOAT, 0, line_vertices);
			
			glDrawArrays(GL_LINES, 0, GLsizei(line_index));
		}

		// Render cylinder direction arrow
		//

		if (ToggleLocatorRender::CurrentRender & ToggleLocatorRender::kRenderDirectionalArrow) {
			static const float direction_arrow_vertices[] =
			{
					0.0f, -0.05f, 0.0f,
					0.0f, -0.1f, 1.0f,
					0.0f, -0.2f, 1.0f,
					0.0f, 0.0f, 1.5f,
					0.0f, 0.2f, 1.0f,
					0.0f, 0.1f, 1.0f,
					0.0f, 0.05f, 0.0f
			};
			static unsigned char direction_arrow_colors[] = {
					0x0, 0x0, 0x0, 0x0,
					0x0, 0x7F, 0x0, 0x7F,
					0x0, 0x7F, 0x0, 0x7F,
					0x0, 0xFF, 0x0, 0xFF,
					0x0, 0x7F, 0x0, 0x7F,
					0x0, 0x7F, 0x0, 0x7F,
					0x0, 0x0, 0x0, 0x0
			};

			static unsigned char direction_arrow_indices[] = {
					0, 1, 5,
					0, 5, 6,
					2, 3, 4
			};

			glShadeModel(GL_SMOOTH);

			glColorPointer(4, GL_UNSIGNED_BYTE, 0, direction_arrow_colors);
			glVertexPointer(3, GL_FLOAT, 0, direction_arrow_vertices);

			//glDrawArrays(GL_LINE_LOOP, 0, 7);
			glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_BYTE, direction_arrow_indices);
		}

		glPopClientAttrib();
		glPopAttrib();

		// Now render labels
		//

		if (ToggleLocatorRender::CurrentRender & ToggleLocatorRender::kRenderSequence) {
			for(size_t i = 0; i < vertex_index; ++i) {
				if (!(stat = view.drawText(MString(&sequence[i], 1), MPoint(vertices[i * 3], vertices[i * 3 + 1] + DNA::RADIUS * DNA::SEQUENCE_RENDERING_Y_OFFSET, vertices[i * 3 + 2]), M3dView::kCenter))) {
					stat.perror("M3dView::drawText");
					break;
				}
			}
		}

		view.endGL();

		// Cleanup
		//

		delete vertices;
		delete line_vertices;
		delete line_colors;
		delete colors;
		delete sequence;
	}

	bool HelixLocator::isBounded() const {
		return false;
	}

	bool HelixLocator::isTransparent() const {
		return true;
	}

	void * HelixLocator::creator() {
		return new HelixLocator();
	}

	MStatus HelixLocator::initialize() {
		return MStatus::kSuccess;
	}
}

