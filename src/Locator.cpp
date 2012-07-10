/*
 * HelixLocator.cpp
 *
 *  Created on: 9 jun 2011
 *      Author: johan
 */

#include <opengl.h>

#include <Locator.h>
#include <Tracker.h>
#include <HelixBase.h>
#include <Utility.h>
#include <DNA.h>
#include <ToggleLocatorRender.h>
#include <ToggleCylinderBaseView.h>

#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MFnDagNode.h>
#include <maya/MPxTransform.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MPlugArray.h>
#include <maya/MFnCamera.h>

#include <model/Helix.h>
#include <model/Strand.h>

#include <algorithm>

// I include the OpenGL headers directly for OpenGL 2.0, even though Maya has it's own definition of OpenGL calls

#ifdef MAC_PLUGIN

#include <OpenGL/glext.h>
#include <OpenGL/gl.h>

#else

#include <GL/gl.h>
#include <GL/glext.h>

#endif /* N MAC_PLUGIN */

//
// OpenGL 2.0 extensions
//

#if defined(WIN32) || defined(WIN64)
#define GETPROCADDRESS(str)		wglGetProcAddress((LPCSTR) str)
#else
#define GETPROCADDRESS(str)		glXGetProcAddressARB((const GLubyte *) str)
#endif /* No windows */

namespace Helix {
	const MTypeId HelixLocator::id(HELIX_LOCATOR_ID);
	const MTypeId transform_id(HELIX_TRANSFORM_ID);
	//MObject HelixLocator::aBases;
	bool HelixLocator::s_gl_initialized = false, HelixLocator::s_gl_failed = false;
	GLint HelixLocator::s_program = 0, HelixLocator::s_vertex_shader = 0, HelixLocator::s_fragment_shader = 0, HelixLocator::s_screen_dimensions_uniform = -1, HelixLocator::s_halo_size_attrib_location = -1;
	
	HelixLocator::~HelixLocator() {
		
	}

	void HelixLocator::initializeGL() {

#if defined(WIN32) || defined(WIN64)
		installGLExtensions();
#endif /* WINDOWS */

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

		/*
		 * Setup halo size attribute for shader
		 */

		if ((s_halo_size_attrib_location = glGetAttribLocation(s_program, HALO_DIAMETER_MULTIPLIER_ATTRIB_NAME)) == -1) {
			std::cerr << "Failed to obtain location of " HALO_DIAMETER_MULTIPLIER_ATTRIB_NAME << " attribute" << std::endl;
			s_gl_failed = true;
		}

		s_gl_initialized = true;
	}

	static const unsigned char static_colors[] [4] = {
			SELECTED_HALO_COLOR, SELECTED_NEIGHBOUR_HALO_COLOR, SELECTED_FIVEPRIME_HALO_COLOR, SELECTED_THREEPRIME_HALO_COLOR,
			SELECTED_ADJACENT_HALO_COLOR, SELECTED_ADJACENT_NEIGHBOUR_HALO_COLOR, SELECTED_ADJACENT_FIVEPRIME_HALO_COLOR, SELECTED_ADJACENT_THREEPRIME_HALO_COLOR
	};

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
	MStatus ExtractDataForRendering(MDagPathArray array, unsigned int & vertex_index, unsigned int & line_index, float *vertices, float *line_vertices, unsigned char *colors, unsigned char *line_colors, char *sequence, int coloring, float *halo_diameters, float halo_multiplier = 1.0f) {
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

			MPlug labelPlug(object, HelixBase::aLabel);
			DNA::Name label;

			if (!(status = labelPlug.getValue((int &) label))) {
				status.perror("MPlug::getValue");
				return status;
			}

			if (labelPlug.isDestination(&status))
				label = label.opposite();

			if (!status) {
				status.perror("MPlug::isDestination");
				return status;
			}

			// Set halo size

			halo_diameters[vertex_index] = halo_multiplier;

			// Extract sequence data AND increase the vertex_index

			sequence[vertex_index++] = label.toChar();

			MDagPath opposite;
			if (HelixBase_NextBase(object, HelixBase::aLabel, opposite, &status)) {
				// We have a pair!

				MFnTransform opposite_transform(opposite);
				MVector opposite_vector = opposite_transform.getTranslation(MSpace::kTransform, &status);

				if (!status) {
					status.perror("MFnTransform::getTranslation");
					return status;
				}

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
		bool isOrtho = false;

		// Do not render halos if the view is not the perspective view. It needs to be taken care of differently
		//

		{
			MDagPath camera_dagPath;

			if (!(stat = view.getCamera(camera_dagPath))) {
				stat.perror("M3dView::getCamera");
				return;
			}

			MFnCamera camera(camera_dagPath);

			isOrtho = camera.isOrtho(&stat);

			if (!stat) {
				stat.perror("MFnCamera::isOrtho");
				return;
			}
		}

		/*
		 * New extraction code using the new API. Fixes duplications where nodes had several halos (Selected, prime ends and neighbour)
		 * Might also be a bit faster, and definitely easier to read
		 */

		MObjectArray selectedBases, selectedChildBasesWithNeighbours;
		Model::Helix helix(MFnDagNode(thisMObject()).parent(0, &stat)); // We can assume our node has a parent and that it is only the helix
		
		if (!stat) {
			stat.perror("MFnDagNode::parent");
			return;
		}

		MFnTransform helix_transform(helix.getDagPath(stat));

		if (!stat) {
			stat.perror("Helix::getDagPath");
			return;
		}

		/*if (!(stat = helix.getSelectedChildBases(selectedChildBases))) {
			stat.perror("Helix::getSelectedChildBases");
			return;
		}*/

		if (!(stat = Model::Base::AllSelected(selectedBases))) {
			stat.perror("Base::AllSelected");
			return;
		}

		/*
		 * Setup OpenGL rendering state
		 */

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
		glEnable (GL_LINE_SMOOTH);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		//glDisable(GL_DEPTH_TEST); // I think we want this, at least when we have blending
		glDepthMask(GL_FALSE); // This is better than disabling the depth test, now blending works, but not through solid objects!

		/*
		 * If the user has anything selected, render halos, lines, sequences etc
		 */

		if (selectedBases.length() > 0) {

			/*
			 * Iterate over all bases and iterate over their strands to extract neighbour bases
			 */

			for(unsigned int i = 0; i < selectedBases.length(); ++i) {
				Model::Base base(selectedBases[i]);
				Model::Strand strand(base);

				Model::Strand::ForwardIterator it = strand.forward_begin();
				for(; it != strand.forward_end(); ++it) {

					if (it->getParent(stat) == helix)
						selectedChildBasesWithNeighbours.append(it->getObject(stat));
					

					if (!stat) {
						stat.perror("Base::getParent/Base::getObject 1");
						return;
					}
				}

				if (!it.loop()) {
					for(Model::Strand::BackwardIterator bit = ++strand.reverse_begin(); bit != strand.reverse_end(); ++bit) {
						if (bit->getParent(stat) == helix)
							selectedChildBasesWithNeighbours.append(bit->getObject(stat));

						if (!stat) {
							stat.perror("Base::getParent/Base::getObject 2");
							return;
						}
					}
				}
			}

			/*
			 * Data declaration, same as old code, might need a cleanup
			 */
		
			size_t vertices_count = selectedChildBasesWithNeighbours.length();

			// NOTE; Multiply length by two if adjacent bases are to be rendered. See code above!!
			float *vertices = new float[vertices_count * 3], *line_vertices = new float[vertices_count * 3 * 2], *halo_diameters = new float[vertices_count];
			unsigned char *colors = new unsigned char[vertices_count * 4], *line_colors = new unsigned char[vertices_count * 4 * 2];
			char *sequence = new char[vertices_count];
			unsigned int /*vertex_index = 0, */ line_index = 0;

			/*
			 * Data gathering
			 */

			for(unsigned int i = 0; i < selectedChildBasesWithNeighbours.length(); ++i) {
				Model::Base base(selectedChildBasesWithNeighbours[i]);
				MVector base_translation;

				/*
				 * Translation
				 */

				if (!(stat = base.getTranslation(base_translation, MSpace::kTransform))) {
					stat.perror("Base::getTranslation");
					return;
				}

				for(int j = 0; j < 3; ++j) {
					vertices[i * 3 + j] = (float) base_translation[j];
					line_vertices[line_index * 3 + j] = (float) base_translation[j];
				}

				/*
				 * Halo radius and coloring
				 */

				/* FIXME: Check if this is a selected base, in that case color it red */

				if (std::find(&selectedBases[0], &selectedBases[0] + selectedBases.length(), selectedChildBasesWithNeighbours[i]) != &selectedBases[0] + selectedBases.length()) {
					/*
					 * This is a selected base
					 */

					for(int j = 0; j < 4; ++j) {
						colors[i * 4 + j] = static_colors[0][j];
						line_colors[line_index * 4 + j] = static_colors[0][j];
					}
					halo_diameters[i] = HALO_SELECTED_BASE_DIAMETER_MULTIPLIER;
				}
				else {
					switch(base.type(stat)) {
			
					case Model::Base::FIVE_PRIME_END:
						for(int j = 0; j < 4; ++j) {
							colors[i * 4 + j] = static_colors[2][j];
							line_colors[line_index * 4 + j] = static_colors[2][j];
						}
						halo_diameters[i] = HALO_FIVE_PRIME_BASE_DIAMETER_MULTIPLIER;
						break;
					case Model::Base::THREE_PRIME_END:
						for(int j = 0; j < 4; ++j) {
							colors[i * 4 + j] = static_colors[3][j];
							line_colors[line_index * 4 + j] = static_colors[3][j];
						}
						halo_diameters[i] = HALO_THREE_PRIME_BASE_DIAMETER_MULTIPLIER;
						break;
					default:
						for(int j = 0; j < 4; ++j) {
							colors[i * 4 + j] = static_colors[1][j];
							line_colors[line_index * 4 + j] = static_colors[1][j];
						}
						halo_diameters[i] = 1.0f;
						break;
					}
				}

				if (!stat) {
					stat.perror("Base::type");
					return;
				}

				/*
				 * Label
				 */

				DNA::Name label;

				if (!(stat = base.getLabel(label))) {
					stat.perror("Base::getLabel");
					return;
				}

				sequence[i] = label.toChar();

				/*
				 * Opposite base for line connection
				 */

				Model::Base opposite_base = base.opposite(stat);

				if (stat) {
					++line_index;

					MVector opposite_base_translation;

					if (!(stat = opposite_base.getTranslation(opposite_base_translation, MSpace::kTransform))) {
						stat.perror("Base::getTranslation 2");
						return;
					}

					for(int j = 0; j < 3; ++j)
						line_vertices[line_index * 3 + j] = (float) opposite_base_translation[j];

					/*
					 * Opposite base type
					 */

					switch(opposite_base.type(stat)) {
					case Model::Base::FIVE_PRIME_END:
						for(int j = 0; j < 4; ++j)
							line_colors[line_index * 4 + j] = static_colors[2][j];
						break;
					case Model::Base::THREE_PRIME_END:
						for(int j = 0; j < 4; ++j)
							line_colors[line_index * 4 + j] = static_colors[3][j];
						break;
					default:
						for(int j = 0; j < 4; ++j)
							line_colors[line_index * 4 + j] = static_colors[1][j];
						break;
					}

					++line_index;
				}
				else if (stat != MStatus::kNotFound) {
					stat.perror("Base::opposite");
					return;
				}
			}

			// Now render labels
			//

			glColor3ub(255, 255, 255);

			if (ToggleLocatorRender::CurrentRender & ToggleLocatorRender::kRenderSequence) {
				for(size_t i = 0; i < vertices_count; ++i) {
					if (!(stat = view.drawText(MString(&sequence[i], 1), MPoint(vertices[i * 3], vertices[i * 3 + 1] + DNA::RADIUS * DNA::SEQUENCE_RENDERING_Y_OFFSET, vertices[i * 3 + 2]), M3dView::kCenter))) {
						stat.perror("M3dView::drawText");
						break;
					}
				}
			}

			// Render halos using point sprites
			//

			if (!isOrtho && (ToggleLocatorRender::CurrentRender & ToggleLocatorRender::kRenderHalo)) {
				glEnableVertexAttribArray(s_halo_size_attrib_location);
				glVertexAttribPointer(s_halo_size_attrib_location, 1, GL_FLOAT, GL_FALSE, 0, halo_diameters);

				glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

				glEnable(GL_POINT_SPRITE);
				glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

				glUseProgram(s_program);

				glUniform2f(s_screen_dimensions_uniform, (GLfloat) view.portWidth(), (GLfloat) view.portHeight());

				glVertexPointer(3, GL_FLOAT, 0, vertices);
				glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);

				glDrawArrays(GL_POINTS, 0, GLsizei(vertices_count));

				glUseProgram(0);

				glDisableVertexAttribArray(s_halo_size_attrib_location);
			}

			// Render the lines
			//

			if (ToggleLocatorRender::CurrentRender & ToggleLocatorRender::kRenderPairLines) {
				glLineWidth(BASE_CONNECTIONS_LINE_WIDTH);

				glColorPointer(4, GL_UNSIGNED_BYTE, 0, line_colors);
				glVertexPointer(3, GL_FLOAT, 0, line_vertices);
			
				glDrawArrays(GL_LINES, 0, GLsizei(line_index));
			}

			// Cleanup
			//

			delete vertices;
			delete line_vertices;
			delete line_colors;
			delete colors;
			delete sequence;
			delete halo_diameters;
		}

		// Render cylinder direction arrow
		//

		if (ToggleLocatorRender::CurrentRender & ToggleLocatorRender::kRenderDirectionalArrow) {
			static const float direction_arrow_vertices[] =
			{
					0.0f, -0.05f, -0.75f,
					0.0f, -0.1f, 0.25f,
					0.0f, -0.2f, 0.25f,
					0.0f, 0.0f, 0.75f,
					0.0f, 0.2f, 0.25f,
					0.0f, 0.1f, 0.25f,
					0.0f, 0.05f, -0.75f
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

			static unsigned char direction_arrow_contour_colors[] = {
					0x0, 0x0, 0x0, 0x0,
					0x0, 0x0, 0x0, 0x7F,
					0x0, 0x0, 0x0, 0x7F,
					0x0, 0x0, 0x0, 0xFF,
					0x0, 0x0, 0x0, 0x7F,
					0x0, 0x0, 0x0, 0x7F,
					0x0, 0x0, 0x0, 0x0
			};

			static unsigned char direction_arrow_indices[] = {
					0, 1, 5,
					0, 5, 6,
					2, 3, 4
			};

			bool renderingCylinder = false;

			if (ToggleCylinderBaseView::CurrentView == 1) {
				/*
				 * We're currently rendering cylinders, 
				 * move the arrow outside the cylinder instead of just rendering it at origo
				 */

				double cylinder_origo, cylinder_height;

				if (!(stat = helix.getCylinderRange(cylinder_origo, cylinder_height))) {
					stat.perror("Model::Helix::getCylinderRange");
				}
				else {
					glPushMatrix();
					glTranslatef(0.0f, 0.0f, (GLfloat) cylinder_height / 2.0f + 0.3f + (GLfloat) cylinder_origo);
					glScalef(1.0f, 1.0f, 0.4f);

					renderingCylinder = true;
				}
			}

			glShadeModel(GL_SMOOTH);

			glColorPointer(4, GL_UNSIGNED_BYTE, 0, direction_arrow_colors);
			glVertexPointer(3, GL_FLOAT, 0, direction_arrow_vertices);

			glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_BYTE, direction_arrow_indices);

			/*
			 * Draw border surrounding the arrow
			 */

			glLineWidth(BASE_CONNECTIONS_LINE_WIDTH);

			glColorPointer(4, GL_UNSIGNED_BYTE, 0, direction_arrow_contour_colors);
			glDrawArrays(GL_LINE_LOOP, 0, 7);

			if (renderingCylinder)
				glPopMatrix();
		}

		glPopClientAttrib();
		glPopAttrib();

		view.endGL();
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

