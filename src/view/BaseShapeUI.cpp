#include <opengl.h>

#include <view/BaseShapeUI.h>
#include <view/BaseShape.h>
#include <model/Base.h>

#include <maya/MSelectionMask.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MDrawData.h>
#include <maya/MMaterial.h>
#include <maya/MMatrix.h>

namespace Helix {
	namespace Data {

#include <data/BackboneArrow.h>

	}

	namespace View {
		BaseShapeUI::DrawData BaseShapeUI::s_drawData;

		void BaseShapeUI::getDrawRequests( const MDrawInfo & info, bool objectAndActiveOnly, MDrawRequestQueue & requests ) {
			MStatus status;

			MDrawData data;

			BaseShape *shape = (BaseShape *) surfaceShape();
			MDrawRequest request = info.getPrototype(*this);
			M3dView view = info.view();

			getDrawData(shape, data);

			request.setDrawData(data);

			MDagPath path = request.multiPath();
			MMaterial material = MPxSurfaceShapeUI::material(path);

			if (!(status = material.evaluateMaterial(view, path)))
				status.perror("MMaterial::evaluateMaterial");

			if (!(status = material.evaluateDiffuse()))
				status.perror("MMaterial::evaluateDiffuse");

			request.setMaterial(material);
			request.setToken(info.displayStyle());
	
			requests.add(request);
		}
		
		void BaseShapeUI::draw( const MDrawRequest & request, M3dView & view ) const {
			if (s_drawData.failure)
				return;

			MStatus status;

			view.beginGL();

			if (!s_drawData.initialized)
				initializeDraw();

			BaseShape *shape = (BaseShape *) surfaceShape();

			Model::Base base(MFnDagNode(shape->thisMObject()).parent(0, &status));

			if (!status) {
				status.perror("MFnDagNode::parent");
				return;
			}

			//MDagPath path = request.multiPath();
			MMaterial material = request.material();
			MColor color, borderColor;

			if (!(status = material.getDiffuse(color))) {
				status.perror("MMaterial::getDiffuse");
				return;
			}

			bool wireframe = (M3dView::DisplayStyle) request.token() == M3dView::kWireFrame;

			if (wireframe) {
				glPushAttrib(GL_POLYGON_BIT);

				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}

			glUseProgram(s_drawData.program);
			
			switch (request.displayStatus())
            {
            case M3dView::kLead :
				borderColor = view.colorAtIndex( LEAD_COLOR);
				glUniform1f(s_drawData.border_uniform, 1.0f);
				glUniform3f(s_drawData.borderColor_uniform, borderColor.r, borderColor.g, borderColor.b);
				break;
            case M3dView::kActive :
                borderColor = view.colorAtIndex( ACTIVE_COLOR);
				glUniform1f(s_drawData.border_uniform, 1.0f);
				glUniform3f(s_drawData.borderColor_uniform, borderColor.r, borderColor.g, borderColor.b);
                break;
            case M3dView::kActiveAffected :
                borderColor = view.colorAtIndex( ACTIVE_AFFECTED_COLOR);
				glUniform1f(s_drawData.border_uniform, 0.0f);
                break;
            case M3dView::kDormant :
				borderColor = view.colorAtIndex( DORMANT_COLOR, M3dView::kDormantColors);
				glUniform1f(s_drawData.border_uniform, 0.0f);
                break;
            case M3dView::kHilite :
                borderColor = view.colorAtIndex( HILITE_COLOR);
				glUniform1f(s_drawData.border_uniform, 1.0f);
				glUniform3f(s_drawData.borderColor_uniform, borderColor.r, borderColor.g, borderColor.b);
                break;
            }

			glUniform3f(s_drawData.color_uniform, color.r, color.g, color.b);

			glCallList(s_drawData.display_list);

			if (wireframe)
				glPopAttrib();

			view.endGL();
		}

		bool BaseShapeUI::select( MSelectInfo &selectInfo, MSelectionList &selectionList, MPointArray &worldSpaceSelectPts ) const {
			// Should never happen
			//
			if (!s_drawData.initialized) {
				std::cerr << "Can't do selection, OpenGL context not initialized!" << std::endl;
				return false;
			}

			//BaseShape *shape = (BaseShape *) surfaceShape();
			MStatus status;

			M3dView view = selectInfo.view();
			const MDagPath & path = selectInfo.multiPath();

			view.beginSelect();

			glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, Data::BackboneArrowVerts);

			glDrawArrays(GL_TRIANGLES, 0, Data::BackboneArrowNumVerts);

			glPopClientAttrib();

			if ( view.endSelect() > 0 )     // Hit count > 0
			{
				MSelectionMask priorityMask( MSelectionMask::kSelectObjectsMask );
				MSelectionList item;
				item.add( selectInfo.selectPath() );
				MPoint xformedPt;

				xformedPt *= path.inclusiveMatrix();
				selectInfo.addSelection( item, xformedPt, selectionList, worldSpaceSelectPts, priorityMask, false );

				return true;
			}

			return false;
		}

		void *BaseShapeUI::creator() {
			return new BaseShapeUI();
		}

		void BaseShapeUI::initializeDraw() {
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

			static const char *vertex_shader[] = { BASESHAPE_GLSL_VERTEX_SHADER, NULL }, *fragment_shader[] = { BASESHAPE_GLSL_FRAGMENT_SHADER, NULL };
			
			glShaderSource(s_drawData.vertex_shader, BASESHAPE_GLSL_VERTEX_SHADER_COUNT, vertex_shader, NULL);
			glShaderSource(s_drawData.fragment_shader, BASESHAPE_GLSL_FRAGMENT_SHADER_COUNT, fragment_shader, NULL);

			glCompileShader(s_drawData.vertex_shader);
			glCompileShader(s_drawData.fragment_shader);

			GLint status;

			{
				GLint shaders[] = { s_drawData.vertex_shader, s_drawData.fragment_shader };
				
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

			if ((s_drawData.color_uniform = glGetUniformLocation(s_drawData.program, "color")) == -1) {
				std::cerr << "Couldn't find uniform vec3 array color" << std::endl;
				//s_drawData.failure = true;
			}

			if ((s_drawData.borderColor_uniform = glGetUniformLocation(s_drawData.program, "borderColor")) == -1) {
				std::cerr << "Couldn't find uniform vec3 array borderColor" << std::endl;
				//s_drawData.failure = true;
			}

			if ((s_drawData.border_uniform = glGetUniformLocation(s_drawData.program, "border")) == -1) {
				std::cerr << "Couldn't find uniform float array border" << std::endl;
				//s_drawData.failure = true;
			}

			/*
			 * Display lists can still boost performance a lot on some GPU's
			 */

			s_drawData.display_list = glGenLists(1);

			glNewList(s_drawData.display_list, GL_COMPILE);

			glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_NORMAL_ARRAY);

			glVertexPointer(3, GL_FLOAT, 0, Data::BackboneArrowVerts);
			glNormalPointer(GL_FLOAT, 0, Data::BackboneArrowNormals);

			glDrawArrays(GL_TRIANGLES, 0, Data::BackboneArrowNumVerts);

			glPopClientAttrib();

			glUseProgram(0);

			glEndList();

			//s_drawData.failure = false;
			s_drawData.initialized = true;
		}
	}
}
