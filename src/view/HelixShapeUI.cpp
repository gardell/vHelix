#include <opengl.h>

#include <view/HelixShapeUI.h>
#include <view/HelixShape.h>
#include <model/Helix.h>
#include <model/Base.h>
#include <model/Color.h>

#include <maya/MDrawData.h>
#include <maya/MDrawRequest.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnTransform.h>
#include <maya/MQuaternion.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MSelectionMask.h>

#include <memory>
#include <list>
#include <algorithm>

#include <Utility.h>

/*
 * Only render bases if we're in fast_view
 */

#include <ToggleCylinderBaseView.h>

namespace Helix {
	namespace View {

		void HelixShapeUI::getDrawRequests( const MDrawInfo & info, bool objectAndActiveOnly, MDrawRequestQueue & requests ) {
			MDrawData data;

			HelixShape *shape = (HelixShape *) surfaceShape();
			MDrawRequest request = info.getPrototype(*this);

			getDrawData(shape, data);
			request.setDrawData(data);
	
			requests.add(request);
		}

		// Main draw routine. Gets called by maya with draw requests.
		//

		void HelixShapeUI::draw( const MDrawRequest & request, M3dView & view ) const {
			/*if (ToggleCylinderBaseView::CurrentView != 1)
				return;*/

			MStatus status;

			MDrawData data = request.drawData();
			HelixShape *shape = static_cast<HelixShape *> (data.geometry()); // Not really necessary, we can just as well call surfaceShape

			Model::Helix helix(MFnDagNode(shape->thisMObject()).parent(0, &status));

			if (!status) {
				status.perror("MFnDagNode::parent");
				return;
			}
			
			double origo, height;

			if (!(status = helix.getCylinderRange(origo, height))) {
				status.perror("Helix::getCylinderRange");
				return;
			}

			if (int(height / DNA::STEP) <= 0)
				return;

			/*
			 * For first time usage, if there are no shaders, no VBO's and no primitive data downloaded
			 */

			if (s_drawData.failure || m_drawData.failure) {
				std::cerr << "GL broke" << std::endl;
				return;
			}

			
			view.beginGL();

			if (!s_drawData.initialized)
				initializeDraw();

			if (!m_drawData.initialized)
				const_cast<HelixShapeUI *>(this)->initializeLocalDraw();

			/*
			 * Set selection color
			 */

			MColor borderColor;

			switch (request.displayStatus())
            {
            case M3dView::kLead :
				borderColor = view.colorAtIndex( LEAD_COLOR);
				break;
            case M3dView::kActive :
                borderColor = view.colorAtIndex( ACTIVE_COLOR);
                break;
            case M3dView::kActiveAffected :
                borderColor = view.colorAtIndex( ACTIVE_AFFECTED_COLOR);
                break;
            case M3dView::kDormant :
				borderColor = view.colorAtIndex( DORMANT_COLOR, M3dView::kDormantColors);
                break;
            case M3dView::kHilite :
                borderColor = view.colorAtIndex( HILITE_COLOR);
                break;
            default:
            	std::cerr << "Unknown displayStatus for helix. Probably nothing to worry about." << std::endl;
            	break;
            }

			/*
			 * Collect color data from our bases and paint our texture
			 */

			GLsizei texture_height = (GLsizei) ceilf(float(height / DNA::STEP)) + 1;

			GLfloat *colors = new GLfloat[texture_height * 2  * 4];

			/*
			 * Clear the colors by setting their alpha value to zero
			 * The GLSL shader is set to discard any fragment with an alpha value less than 0.5
			 */

			for(int i = 0; i < texture_height; ++i) {
				colors[i * 2 * 4 + 3] = GLfloat(0);

				// Keep this
				colors[i * 2 * 4 + 7] = GLfloat(0);
			}

			for(Model::Helix::BaseIterator it = helix.begin(); it != helix.end(); ++it) {
				Model::Base base(*it);
				MVector base_translation;

				if (!(status = base.getTranslation(base_translation, MSpace::kTransform))) {
					status.perror("Model::Base::getTranslation");
					return;
				}

				/*
				 * Translate the coordinates of the base into coordinates on the texture
				 */

				int y = (int) ((base_translation.z + height / 2.0 - origo) / DNA::STEP + DNA::Z_SHIFT),
					x = (int) ceil(sin(atan2(base_translation.y, base_translation.x) - y * toRadians(DNA::PITCH)));

				/*
				 * If the base has been moved outside of the boundaries, ignore it
				 */

				if (y < 0 || y >= texture_height) {
					//std::cerr << "Base: " << base.getDagPath(status).fullPathName().asChar() << " is out of bounds: " << y << std::endl;
					continue;
				}

				if (!(status = base.getMaterialColor(colors[(y * 2 + x) * 4], colors[(y * 2 + x) * 4 + 1], colors[(y * 2 + x) * 4 + 2], colors[(y * 2 + x) * 4 + 3]))) {
					status.perror("Base::getMaterialColor");
					return;
				}

				colors[(y * 2 + x) * 4 + 3] = 1.0f;
			}

			/*
			 * Notice that downloading textures to the graphics card is pretty expensive, thus, iterating over the whole texture here
			 * to make sure it really changed could save us a lot of GPU processing time
			 */

			bool textureUpdate = true;

			if (m_drawData.last_colors != NULL) {
				if (m_drawData.texture_height == texture_height) {
					if (std::equal(colors, colors +  texture_height * 2, m_drawData.last_colors))
						textureUpdate = false;
				}

				delete[] m_drawData.last_colors;
			}

			const_cast<HelixShapeUI *>(this)->m_drawData.last_colors = colors;

			GLCALL(glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT));
			GLCALL(glBindTexture(GL_TEXTURE_2D, m_drawData.texture));

			if (textureUpdate) {
				GLCALL(glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT));
				GLCALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
				if (m_drawData.texture_height != texture_height) {
					GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, texture_height, 0, GL_RGBA, GL_FLOAT, colors));
					const_cast<HelixShapeUI *>(this)->m_drawData.texture_height = texture_height;
				}
				else {
					GLCALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2, texture_height, GL_RGBA, GL_FLOAT, colors));
				}
				glPopClientAttrib();
			}

			GLCALL(glUseProgram(s_drawData.program));
			//GLCALL(glUniform2f(s_drawData.range_uniform, (GLfloat) origo, (GLfloat) height));
			//GLCALL(glUniform3f(s_drawData.borderColor_uniform, borderColor.r, borderColor.g, borderColor.b));
			s_drawData.updateRangeUniform((GLfloat) origo, (GLfloat) height);
			s_drawData.updateBorderColorUniform(borderColor.r, borderColor.g, borderColor.b);

			GLCALL(glCallList(s_drawData.draw_display_list));

			// Note: No glPopAttrib, cause it's compiled into the display list!

			view.endGL();

			/*
			 * Don't delete the color array, it is saved for the next rendering
			 */
			//delete[] colors;
		}

		// Main selection routine
		//

		bool HelixShapeUI::select( MSelectInfo &selectInfo, MSelectionList &selectionList, MPointArray &worldSpaceSelectPts ) const {
			if (ToggleCylinderBaseView::CurrentView != 1)
				return false;

			HelixShape *shape = (HelixShape *) surfaceShape();
			MStatus status;

			// Should never happen
			//
			if (!s_drawData.initialized) {
				std::cerr << "Can't do selection, OpenGL context not initialized!" << std::endl;
				return false;
			}

			/*
			 * Get cylinder data
			 */

			Model::Helix helix(MFnDagNode(shape->thisMObject()).parent(0, &status));

			if (!status) {
				status.perror("MFnDagNode::parent");
				return false;
			}

			double origo, height;

			if (!(status = helix.getCylinderRange(origo, height))) {
				status.perror("Helix::getCylinderRange");
				return false;
			}

			/*
			 * All code here is from the apiSimpleShapeUI and quadricShape but modified for our needs
			 */

			M3dView view = selectInfo.view();
			const MDagPath & path = selectInfo.multiPath();

			view.beginSelect();

			glPushMatrix();
			glScalef(1.0f, 1.0f, (GLfloat) height);
			glTranslatef(0.0f, 0.0f, (GLfloat) origo - 0.5f);
			glCallList(s_drawData.select_display_list);

			// The display list contains a glPopMatrix

			if (view.endSelect() > 0) {
				MSelectionMask priorityMask( MSelectionMask::kSelectObjectsMask );
				MSelectionList item;
				item.add( selectInfo.selectPath() );
				MPoint xformedPt;
				xformedPt *= path.inclusiveMatrix();
				selectInfo.addSelection( item, xformedPt, selectionList, worldSpaceSelectPts, priorityMask, false );
				return true;
			}

			return false;

			/*bool selected = false;
			M3dView view = selectInfo.view();

			MPoint          xformedPoint;
			MPoint          currentPoint;
			MPoint          selectionPoint;
			double          z,previousZ = 0.0;
			int                     closestPointVertexIndex = -1;

			const MDagPath & path = selectInfo.multiPath();

			// if the user did a single mouse click and we find > 1 selection
			// we will use the alignmentMatrix to find out which is the closest
			//
			MMatrix alignmentMatrix;
			MPoint singlePoint; 
			bool singleSelection = selectInfo.singleSelection();
			if( singleSelection ) {
					alignmentMatrix = selectInfo.getAlignmentMatrix();
			}

			// Get the geometry information
			//
			Model::Helix helix(MFnDagNode(shape->thisMObject()).parent(0, &status));

			if (!status) {
				status.perror("MFnDagNode::parent");
				return false;
			}


			// Loop through all vertices of the mesh and
			// see if they lie withing the selection area
			//
			/*int numVertices = geom.length();
			for ( vertexIndex=0; vertexIndex<numVertices; vertexIndex++ )*
			//int vertexIndex = 0;

			std::list<Model::Helix::BaseIterator> selections;
			Model::Helix::BaseIterator closestPointBaseIterator = helix.end();

			for(Model::Helix::BaseIterator it = helix.begin(); it != helix.end(); ++it)
			{
				MTransformationMatrix matrix;
					
				if (!(status = it->getTransform(matrix))) {//geom[ vertexIndex ];
					status.perror("Base::getTransform");
					return false;
				}

				MMatrix m = matrix.asMatrix();

				// Sets OpenGL's render mode to select and stores
				// selected items in a pick buffer
				//
				view.beginSelect();

				glPushMatrix();
				glMultMatrixd((GLdouble *) m.matrix);

				glBindBuffer(GL_ARRAY_BUFFER, s_drawData.vertex_array_buffer);

				glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, (GLvoid *) s_drawData.vertex_array_offset);

				glDrawArrays(GL_TRIANGLES, 0, Data::BackboneArrowNumVerts);

				glPopClientAttrib();

				glBindBuffer(GL_ARRAY_BUFFER, 0);

				glPopMatrix();

				if ( view.endSelect() > 0 )     // Hit count > 0
				{
					selected = true;

					if ( singleSelection ) {
							xformedPoint = currentPoint;
							xformedPoint.homogenize();
							xformedPoint*= alignmentMatrix;
							z = xformedPoint.z;

							if ( closestPointVertexIndex < 0 || z > previousZ ) {
								closestPointBaseIterator = it;
								singlePoint = currentPoint;
								previousZ = z;
							}
					} else {
						// multiple selection, store all elements
						//
						selections.push_back(it);
					}
				}
			}

			// If single selection, insert the closest point into the array
			//
			if ( selected && selectInfo.singleSelection() ) {
					selections.push_back(closestPointBaseIterator);

					// need to get world space position for this vertex
					//
					selectionPoint = singlePoint;
					selectionPoint *= path.inclusiveMatrix();
			}

			// Add the selected component to the selection list
			//
			if ( selected ) {
					MSelectionList selectionItem;
					
					for(std::list<Model::Helix::BaseIterator>::iterator it = selections.begin(); it != selections.end(); ++it) {
						selectionItem.add((*it)->getDagPath(status));

						if (!status) {
							status.perror("Base::getObject");
							return false;
						}
					}

					MSelectionMask mask( MSelectionMask::kSelectObjectsMask );
					selectInfo.addSelection(selectionItem, selectionPoint, selectionList, worldSpaceSelectPts, mask, false);
			}

			return selected;*/
		}

		void *HelixShapeUI::creator() {
			return new HelixShapeUI();
		}

		void HelixShapeUI::initializeDraw() {
			/*
			 * Now, setup shaders, program, VBO's, uniforms and download the models to the VBO's
			 */

#if defined(WIN32) || defined(WIN64)
			installGLExtensions();
#endif /* WINDOWS */

			// Setup the shaders and programs

			if ((s_drawData.program = glCreateProgram()) == 0) {
				std::cerr << "Fatal, failed to create an OpenGL program object" << std::endl;
				s_drawData.failure = true;
			}

			if ((s_drawData.vertex_shader = glCreateShader(GL_VERTEX_SHADER)) == 0) {
				std::cerr << "Fatal, failed to create an OpenGL shader object" << std::endl;
				s_drawData.failure = true;
			}

			if ((s_drawData.fragment_shader = glCreateShader(GL_FRAGMENT_SHADER)) == 0) {
				std::cerr << "Fatal, failed to create an OpenGL shader object" << std::endl;
				s_drawData.failure = true;
			}

			static const char *vertex_shader[] = { HELIXSHAPE_GLSL_VERTEX_SHADER, NULL }, *fragment_shader[] = { HELIXSHAPE_GLSL_FRAGMENT_SHADER, NULL };
			
			glShaderSource(s_drawData.vertex_shader, HELIXSHAPE_GLSL_VERTEX_SHADER_COUNT, vertex_shader, NULL);
			glShaderSource(s_drawData.fragment_shader, HELIXSHAPE_GLSL_FRAGMENT_SHADER_COUNT, fragment_shader, NULL);

			glCompileShader(s_drawData.vertex_shader);
			glCompileShader(s_drawData.fragment_shader);

			GLint status;

			{
				GLuint shaders[] = { s_drawData.vertex_shader, s_drawData.fragment_shader };
				
				for(int i = 0; i < 2; ++i) {
					

					glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &status);

					if (status != GL_TRUE) {
						GLint logLength;
						glGetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &logLength);
						GLchar *log = new GLchar[logLength];
						glGetShaderInfoLog(shaders[i], logLength, NULL, log);

						std::cerr << "OpenGL shader info log (" << (i + 1) << "): " << std::endl << log << std::endl << std::endl;

						s_drawData.failure = true;
					}

					glAttachShader(s_drawData.program, shaders[i]);
				}
			}

			glLinkProgram(s_drawData.program);

			glGetProgramiv(s_drawData.program, GL_LINK_STATUS, &status);

			if (status != GL_TRUE) {
				GLint logLength;
				glGetProgramiv(s_drawData.program, GL_INFO_LOG_LENGTH, &logLength);
				GLchar *log = new GLchar[logLength];
				glGetProgramInfoLog(s_drawData.program, logLength, NULL, log);

				std::cerr << "OpenGL program info log: " << std::endl << log << std::endl << std::endl;

				s_drawData.failure = true;
			}

			/*
			 * Setup uniforms
			 */

			if ((s_drawData.texture_uniform = glGetUniformLocation(s_drawData.program, "texture")) == -1) {
				std::cerr << "Couldn't find uniform texture" << std::endl;
				//s_drawData.failure = true;
			}

			if ((s_drawData.range_uniform = glGetUniformLocation(s_drawData.program, "range")) == -1) {
				std::cerr << "Couldn't find uniform vec2 range" << std::endl;
				//s_drawData.failure = true;
			}

			if ((s_drawData.borderColor_uniform = glGetUniformLocation(s_drawData.program, "borderColor")) == -1) {
				std::cerr << "Couldn't find uniform vec3 borderColor" << std::endl;
				//s_drawData.failure = true;
			}

			/*
			 * Generate a cylinder
			 */

			GLfloat vertices[(HELIXSHAPE_CYLINDER_SLICES + 2) * 3 * 2];
			GLubyte indices[HELIXSHAPE_CYLINDER_SLICES * 3 * 2];

			/*
			 * First index in the vertices array is the center of the first cap
			 * after all the caps vertices, the next caps center vertex and then followed by it's surrounding vertices
			 * this makes it easier to issue a glDrawArrays to render the caps after the cylinder
			 */

			for(int i = 0; i < 3; ++i) {
				vertices[i] = 0.0f;
				vertices[(HELIXSHAPE_CYLINDER_SLICES + 1) * 3 + i] = 0.0f;
			}

			for(int i = 0; i < HELIXSHAPE_CYLINDER_SLICES + 1; ++i) {
				vertices[(i + 1) * 3] = cosf(float(i) / HELIXSHAPE_CYLINDER_SLICES * 2.0f * float(M_PI));
				vertices[(i + 1) * 3 + 1] = sinf(float(i) / HELIXSHAPE_CYLINDER_SLICES * 2.0f * float(M_PI));
				vertices[(i + 1) * 3 + 2] = 0.0f;

				vertices[(i + 2 + HELIXSHAPE_CYLINDER_SLICES) * 3] = vertices[(i + 1) * 3];
				vertices[(i + 2 + HELIXSHAPE_CYLINDER_SLICES) * 3 + 1] = vertices[(i + 1) * 3 + 1];
				vertices[(i + 2 + HELIXSHAPE_CYLINDER_SLICES) * 3 + 2] = 1.0f;
			}

			for(int i = 0; i < HELIXSHAPE_CYLINDER_SLICES; ++i) {
				int next_i = (i + 1) % HELIXSHAPE_CYLINDER_SLICES;

				indices[i * 6] = i + 1;
				indices[i * 6 + 1] = next_i + 3 + HELIXSHAPE_CYLINDER_SLICES;
				indices[i * 6 + 2] = i + 3 + HELIXSHAPE_CYLINDER_SLICES;

				indices[i * 6 + 3] = i + 1;
				indices[i * 6 + 4] = next_i + 1;
				indices[i * 6 + 5] = next_i + 3 + HELIXSHAPE_CYLINDER_SLICES;
			}
			
			/*
			 * Display list with data
			 */

			s_drawData.draw_display_list = glGenLists(2);
			s_drawData.select_display_list = s_drawData.draw_display_list + 1;

			GLCALL(glNewList(s_drawData.draw_display_list, GL_COMPILE));

				GLCALL(glUniform1i(s_drawData.texture_uniform, 0));

				GLCALL(glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT));

				GLCALL(glEnableClientState(GL_VERTEX_ARRAY));
				GLCALL(glVertexPointer(3, GL_FLOAT, 0, vertices));

				GLCALL(glDrawElements(GL_TRIANGLES, HELIXSHAPE_CYLINDER_SLICES * 2 * 3, GL_UNSIGNED_BYTE, indices));
				GLCALL(glDrawArrays(GL_TRIANGLE_FAN, 0, HELIXSHAPE_CYLINDER_SLICES + 2));
				GLCALL(glDrawArrays(GL_TRIANGLE_FAN, HELIXSHAPE_CYLINDER_SLICES + 2, HELIXSHAPE_CYLINDER_SLICES + 2));

				GLCALL(glPopClientAttrib());

				GLCALL(glPopAttrib());

				GLCALL(glUseProgram(0));

			GLCALL(glEndList());

			GLCALL(glNewList(s_drawData.select_display_list, GL_COMPILE));

				GLCALL(glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT));

				GLCALL(glEnableClientState(GL_VERTEX_ARRAY));
				GLCALL(glVertexPointer(3, GL_FLOAT, 0, vertices));

				GLCALL(glDrawElements(GL_TRIANGLES, HELIXSHAPE_CYLINDER_SLICES * 2 * 3, GL_UNSIGNED_BYTE, indices));
				GLCALL(glDrawArrays(GL_TRIANGLE_FAN, 0, HELIXSHAPE_CYLINDER_SLICES + 2));
				GLCALL(glDrawArrays(GL_TRIANGLE_FAN, HELIXSHAPE_CYLINDER_SLICES + 2, HELIXSHAPE_CYLINDER_SLICES + 2));

				GLCALL(glPopClientAttrib());

				GLCALL(glPopMatrix());

			GLCALL(glEndList());

			s_drawData.initialized = true;
		}

		void HelixShapeUI::initializeLocalDraw() {
			/*
			 * Textures
			 */
			
			GLCALL(glPushAttrib(GL_TEXTURE_BIT));

			GLCALL(glGenTextures(1, &m_drawData.texture));
			GLCALL(glBindTexture(GL_TEXTURE_2D, m_drawData.texture));
			GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
			GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
			GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
			GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

			GLCALL(glPopAttrib());

			m_drawData.initialized = true;
		}

		GLfloat g_HelixShapeUI_previous_range_uniform_values[2] = { std::numeric_limits<GLfloat>::infinity(), std::numeric_limits<GLfloat>::infinity() };

		void HelixShapeUI::DrawData::updateRangeUniform(GLfloat origo, GLfloat height) const {
			if (g_HelixShapeUI_previous_range_uniform_values[0] != origo || g_HelixShapeUI_previous_range_uniform_values[1] != height) {
				GLCALL(glUniform2f(range_uniform, origo, height));

				g_HelixShapeUI_previous_range_uniform_values[0] = origo;
				g_HelixShapeUI_previous_range_uniform_values[1] = height;
			}
		}

		GLfloat g_HelixShapeUI_previous_borderColor_uniform_values[3] = { -1.0f, -1.0f, -1.0f };

		void HelixShapeUI::DrawData::updateBorderColorUniform(GLfloat r, GLfloat g, GLfloat b) const {
			if (g_HelixShapeUI_previous_borderColor_uniform_values[0] != r || g_HelixShapeUI_previous_borderColor_uniform_values[1] != g || g_HelixShapeUI_previous_borderColor_uniform_values[2] != b) {
				GLCALL(glUniform3f(borderColor_uniform, r, g, b));

				g_HelixShapeUI_previous_borderColor_uniform_values[0] = r;
				g_HelixShapeUI_previous_borderColor_uniform_values[1] = g;
				g_HelixShapeUI_previous_borderColor_uniform_values[2] = b;
			}
		}

		HelixShapeUI::DrawData HelixShapeUI::s_drawData;
	}
}
