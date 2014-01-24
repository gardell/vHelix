#include <view/ConnectSuggestionsLocatorNode.h>
#include <opengl.h>
#include <Utility.h>
#include <HelixBase.h>
#include <Helix.h>
#include <DNA.h>

#include <maya/MItDag.h>
#include <maya/MPlane.h>
#include <maya/MModelMessage.h>

#include <algorithm>
#include <iterator>
#include <vector>
#include <functional>

#define CONNECT_SUGGESTIONS_MAX_DISTANCE DNA::STEP * 2.0

/*
 * FIXME: Color as an attribute is not required
 */

// FIXME: Can we mark some varying not to be interpolated? I think there's a feature for that in some GLSL environments (pointSize)

// "	Coord2 = ((pVertex32.xy - gl_Position.xy) * windowSize / pointSize + 1.0) / 2.0;\n",

#define CONNECT_SUGGESTIONS_LOCATOR_GLSL_VERTEX_SHADER																		\
		"#version 120\n",																									\
																															\
		"uniform vec2 windowSize;\n",																						\
																															\
		"varying vec4 shiftArrowStrengthDirection;\n",																		\
		"varying vec4 Color;\n",																							\
		"varying vec2 Texcoord;\n",																							\
																															\
		"const float width = 0.1, scale_texcoord_y = 1.0 / 128.0;\n",														\
																															\
		"attribute vec4 vertex1;\n",																						\
		"attribute vec4 vertex2;\n",																						\
		"attribute vec4 shift_arrow_strength_direction;\n",																	\
		"attribute vec4 color;\n",																							\
																															\
		"void main() {\n",																									\
		"	vec4 pVertex1 = gl_ModelViewMatrix * vertex1, pVertex2 = gl_ModelViewMatrix * vertex2;\n",						\
		"	vec3 pVertices[2]; pVertices[0] = pVertex1.xyz / pVertex1.w; pVertices[1] = pVertex2.xyz / pVertex2.w;\n",		\
		"	vec3 delta = (pVertices[1] - pVertices[0]) * vec3(windowSize / 2.0, 1.0);\n",									\
		"	vec3 corner = cross(normalize(delta) * width, vec3(0.0, 0.0, -1.0));\n",										\
																															\
		"	gl_Position = gl_ProjectionMatrix * vec4(pVertices[int(gl_Vertex.y)] + (gl_Vertex.x * 2.0 - 1.0) * corner, 1.0);\n",	\
		"	shiftArrowStrengthDirection = shift_arrow_strength_direction;\n",												\
		"	Color = color;\n",																								\
		"	Texcoord = vec2(gl_Vertex.x, scale_texcoord_y * length(delta) * (gl_Vertex.y * 2.0 - 1.0 - shift_arrow_strength_direction.x) / 2.0);\n",	\
		"}\n"

// 		"	Texcoord = vec2(gl_Vertex.x, scale_texcoord_y * length(delta) * (gl_Vertex.y * 2.0 - 1.0 - shift_arrow_strength_direction.x) / 2.0);\n",

#define CONNECT_SUGGESTIONS_LOCATOR_GLSL_FRAGMENT_SHADER																	\
		"#version 120\n",																									\
																															\
		"uniform sampler3D arrows;\n",																						\
																															\
		"varying vec4 shiftArrowStrengthDirection;\n",																		\
		"varying vec4 Color;\n",																							\
		"varying vec2 Texcoord;\n",																							\
																															\
		"void main() {\n",																									\
		"	float arrowColor = texture3D(arrows, vec3(Texcoord.yx + vec2(0.5, 0.0), shiftArrowStrengthDirection.b)).r;\n",	\
		"	gl_FragColor = vec4(Texcoord.xy, 1.0, 1.0) * Color;\n",															\
		"	gl_FragColor.a *= 0.5 + arrowColor;\n",																			\
		"}\n"

/*
 * When adding or removing uniforms or attributes, remember to modify the call to SETUPOPENGLSHADERS
 */
#define CONNECT_SUGGESTIONS_LOCATOR_GLSL_UNIFORM_NAMES	"slide", "arrows", "windowSize"
#define CONNECT_SUGGESTIONS_LOCATOR_GLSL_ATTRIB_NAMES	"vertex1", "vertex2", "shift_arrow_strength_direction", "color"

#define CONNECT_SUGGESTIONS_SLIDE_SELECT_RADIUS 15.0
#define CONNECT_SUGGESTIONS_SLIDE_SCALE 0.1

#define CONNECT_SUGGESTIONS_LOCATOR_SCALE_CONNECT_THRESHOLD	0.3

/*
 * This is the GUI texture
 * FIXME: Use GIMP and make some nicer textures! It can export to C headers
 */

#define CONNECT_SUGGESTIONS_SLIDE_TEXTURE_WIDTH 8
#define CONNECT_SUGGESTIONS_SLIDE_TEXTURE_HEIGHT 6

#define CONNECT_SUGGESTIONS_SLIDE_TEXTURE																					\
	0, 0, 0, 0, 0, 0, 0, 0,																									\
	0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0,																				\
	0xFF, 0xFF, 0, 0, 0, 0, 0xFF, 0xFF,																						\
	0xFF, 0xFF, 0, 0, 0, 0, 0xFF, 0xFF,																						\
	0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0,																				\
	0, 0, 0, 0, 0, 0, 0, 0

//#define CONNECT_SUGGESTIONS_ARROW_TEXTURE_WIDTH 10
//#define CONNECT_SUGGESTIONS_ARROW_TEXTURE_HEIGHT 8

/*
 * FIXME: It's a 3D texture, first the <> arrow then the <, they must have the same size!
 */

/*#define CONNECT_SUGGESTIONS_ARROW_TEXTURE																					\
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,																							\
	0, 0, 0, 0xFF, 0, 0, 0xFF, 0, 0, 0,																						\
	0, 0, 0xFF, 0, 0, 0, 0, 0xFF, 0, 0,																						\
	0, 0xFF, 0, 0, 0, 0, 0, 0, 0xFF, 0,																						\
	0, 0xFF, 0, 0, 0, 0, 0, 0, 0xFF, 0,																						\
	0, 0, 0xFF, 0, 0, 0, 0, 0xFF, 0, 0,																						\
	0, 0, 0, 0xFF, 0, 0, 0xFF, 0, 0, 0,																						\
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0*/

/*
 * Image defined in double_arrow.c. An image exported using GIMP, which has the feature of generating C files
 * thus eliminating the need of distributing the textures as external files
 */

void arrow_image_call_teximage3d();

namespace Helix {
	namespace View {
		/*
		 * Event listeners for tracking data
		 */

		void MNodeMessage_Base_AttributeChangedProc(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData) {
			if(msg & MNodeMessage::kAttributeSet) {
				/*
				 * Delete this node from the s_closeBasesTable then iterate over all bases in the scene and find out which ones are close to this one
				 */

				if (plug.attribute() == MPxTransform::translate) {
					MStatus status;
					Model::Base base(plug.node(&status));

					if (!status) {
						status.perror("MPlug::node");
						return;
					}

					ConnectSuggestionsLocatorNode::UpdateBase(base);
				}
			}
		}

		void MNodeMessage_Helix_AttributeChangedProc(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData) {
			if(msg & MNodeMessage::kAttributeSet) {
				/*
				 * Delete this node from the s_closeBasesTable then iterate over all bases in the scene and find out which ones are close to this one
				 */

				std::cerr << __FUNCTION__ << std::endl;

				if (plug.attribute() == MPxTransform::translate || plug.attribute() == MPxTransform::rotate) {
					MStatus status;
					Model::Helix helix(plug.node(&status));

					if (!status) {
						status.perror("MPlug::node");
						return;
					}

					std::cerr << "Updating helix: " << helix.getDagPath(status).fullPathName().asChar() << std::endl;

					ConnectSuggestionsLocatorNode::UpdateHelix(helix);
				}
			}
		}

		void MNodeMessage_Transform_AttributeChangedProc(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData) {
			if(msg & MNodeMessage::kAttributeSet) {
				MStatus status;

				/* We don't have access to the attribute MObjects, thus a slower string comparison is the only way. Also no 'translate' node triggers events for groups, translateX, translateY and translateZ does */
				
				if (strstr(plug.name().asChar(), "translate")) {
					Model::Object node(plug.node(&status));
					ConnectSuggestionsLocatorNode::UpdateTransform(node);

					if (!status)
						status.perror("MPlug::node");
				}
			}
		}

		ConnectSuggestionsLocatorNode::ConnectSuggestionsLocatorNode() {

		}

		ConnectSuggestionsLocatorNode::~ConnectSuggestionsLocatorNode() {
			
		}

		std::vector<MCallbackId> g_attributeChangedCallbacks;

		void MMessage_ConnectSuggestionsLocatorNode_preRemovalCallback(MObject &node, void *clientData) {
			std::cerr << __FUNCTION__ << std::endl;

			/*
			 * Remove all tracking events, including activeListModified (?)
			 */

			ConnectSuggestionsLocatorNode & locatorNode = *static_cast<ConnectSuggestionsLocatorNode *> (clientData);

			MNodeMessage::removeCallback(locatorNode.m_preRemovalCallbackId);
			MModelMessage::removeCallback(locatorNode.m_activeListModifiedCallbackId);

			/*
			 * Detach from old helices/bases/transforms tracking
			 */

			std::for_each(g_attributeChangedCallbacks.begin(), g_attributeChangedCallbacks.end(), std::ptr_fun(MModelMessage::removeCallback));
			g_attributeChangedCallbacks.clear();
		}

		void MModelMessage_ConnectSuggestionsLocatorNode_activeListModified(void *clientData) {
			std::cerr << __FUNCTION__ << std::endl;

			MStatus status;
			/*
			 * Detach from old helices/bases/transforms tracking
			 */

			std::for_each(g_attributeChangedCallbacks.begin(), g_attributeChangedCallbacks.end(), std::ptr_fun(MModelMessage::removeCallback));
			g_attributeChangedCallbacks.clear();

			/*
			 * Look for objects selected that are helices, bases or transforms containing helices and track them
			 * notice that we need to be able to remove the trackings, thus we must save the objects we track for future deattachment
			 */

			MSelectionList activeList;

			if (!(status = MGlobal::getActiveSelectionList(activeList))) {
				status.perror("MGlobal::getActiveSelectionList");
				return;
			}

			g_attributeChangedCallbacks.reserve(activeList.length());

			for(unsigned int i = 0; i < activeList.length(); ++i) {
				MObject node;

				if (!(status = activeList.getDependNode(i, node))) {
					status.perror("MSelectionList::getDependNode");
					return;
				}

				switch (node.apiType()) {
				case MFn::kTransform:
				case MFn::kPluginTransformNode:
					{
						MFnDagNode dagNode(node);

						MTypeId typeId = dagNode.typeId(&status);

						if (!status) {
							status.perror("MFnDagNode::typeId");
							return;
						}

						if (typeId == HelixBase::id) {
							/*
							 * This is a base node
							 */

							MCallbackId callback = MNodeMessage::addAttributeChangedCallback(node, MNodeMessage_Base_AttributeChangedProc, NULL, &status);

							if (!status) {
								status.perror("MNodeMessage::addAttributeChangedCallback Base");
								return;
							}

							std::cerr << "Tracking base: " << dagNode.fullPathName().asChar() << std::endl;

							g_attributeChangedCallbacks.push_back(callback);

							/*
							 * Now also trigger the event, because we want to show the trackings right away
							 */
							
							MPlug translatePlug(node, MPxTransform::translate), otherPlug;
							MNodeMessage_Base_AttributeChangedProc(MNodeMessage::kAttributeSet, translatePlug, otherPlug, NULL);
						}
						else if (typeId == Helix::id) {
							/*
							 * This is a helix
							 */

							MCallbackId callback = MNodeMessage::addAttributeChangedCallback(node, MNodeMessage_Helix_AttributeChangedProc, NULL, &status);

							if (!status) {
								status.perror("MNodeMessage::addAttributeChangedCallback Helix");
								return;
							}

							std::cerr << "Tracking helix: " << dagNode.fullPathName().asChar() << std::endl;

							g_attributeChangedCallbacks.push_back(callback);

							/*
							 * Now also trigger the event, because we want to show the trackings right away
							 */
							
							MPlug translatePlug(node, MPxTransform::translate), otherPlug;
							MNodeMessage_Helix_AttributeChangedProc(MNodeMessage::kAttributeSet, translatePlug, otherPlug, NULL);
						}
						else {
							/*
							 * This is any type of a transform thats neither a helix nor base. Track it specifically as a node
							 * that might have helices as its children
							 */

							MCallbackId callback = MNodeMessage::addAttributeChangedCallback(node, MNodeMessage_Transform_AttributeChangedProc, NULL, &status);

							if (!status) {
								status.perror("MNodeMessage::addAttributeChangedCallback Transform");
								return;
							}

							std::cerr << "Tracking unknown transform: " << dagNode.fullPathName().asChar() << std::endl;

							g_attributeChangedCallbacks.push_back(callback);

							/*
							 * Now also trigger the event, because we want to show the trackings right away
							 */
							
							MPlug translatePlug = dagNode.findPlug("translate", &status), otherPlug;

							if (!status) {
								status.perror("MFnDagNode::findPlug(\"translate\", ...");
								continue;
							}

							MNodeMessage_Transform_AttributeChangedProc(MNodeMessage::kAttributeSet, translatePlug, otherPlug, NULL);
						}
					}
					break;
				default:
					break;
				}
			}

			M3dView::active3dView().refresh(true);
		}

		void ConnectSuggestionsLocatorNode::postConstructor() {
			MStatus status;

			/*
			 * Track the destruction of our own object as when this happens, we need to detach our listeners from all bases and helices in the scene
			 */

			MObject thisObject = thisMObject();
			
			m_preRemovalCallbackId = MNodeMessage::addNodePreRemovalCallback(thisObject, &MMessage_ConnectSuggestionsLocatorNode_preRemovalCallback, this, &status);

			if (!status)
				status.perror("MNodeMessage::addNodePreRemovalCallback");

			/*
			 * Attach ourselves to selection events in order to track potential connections
			 */

			m_activeListModifiedCallbackId = MModelMessage::addCallback(MModelMessage::kActiveListModified, &MModelMessage_ConnectSuggestionsLocatorNode_activeListModified, this, &status);

			if (!status)
				status.perror("MModelMessage::addCallback");

			/*
			 * We want to start tracking right away too, on the current selection
			 */

			MModelMessage_ConnectSuggestionsLocatorNode_activeListModified(NULL);
		}

		void ConnectSuggestionsLocatorNode::draw(M3dView &view, const MDagPath &path, M3dView::DisplayStyle style, M3dView::DisplayStatus status) {
			if (s_closeBasesTable.empty())
				return;

			MStatus stat;

			static const float quad[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

			/*
			 * Data gathering
			 * The first version used point sprites, which wasn't too successful, the new one uses quads
			 * The problem with quads is the repetition of data, everything has to be repeated 4 times
			 */

			GLfloat *points = new GLfloat[s_closeBasesTable.size() * 3 * 2 * 4],
					*shift_arrow_strength_directions = new GLfloat[s_closeBasesTable.size() * 4 * 4],
					*colors = new GLfloat[s_closeBasesTable.size() * 4 * 4],
					*vertices = new GLfloat[s_closeBasesTable.size() * 2 * 4];

			unsigned int i = 0;
			for(std::list<BasePair>::iterator it = s_closeBasesTable.begin(); it != s_closeBasesTable.end(); ++it, ++i) {
				MVector coord1, coord2;

				if (!(stat = it->first.getTranslation(coord1, MSpace::kWorld))) {
					stat.perror("Base::getTranslation");
					continue;
				}

				if (!(stat = it->second.getTranslation(coord2, MSpace::kWorld))) {
					stat.perror("Base::getTranslation");
					continue;
				}

				Model::Base::Type base1_type = it->first.type(stat);

				if (!stat) {
					stat.perror("Base::type 1");
					continue;
				}

				Model::Base::Type base2_type = it->second.type(stat);

				if (!stat) {
					stat.perror("Base::type 2");
					continue;
				}

				for(int j = 0; j < 4; ++j) {
					for(int k = 0; k < 3; ++k) {
						points[i * 3 * 2 * 4 + j * 3 * 2 + k] = GLfloat(coord1[k]);
						points[i * 3 * 2 * 4 + j * 3 * 2 + 3 + k] = GLfloat(coord2[k]);
					}

					/*
					 * This is the element currently selected, shift will be non-zero
					 */

					if (s_selectedElement.bases.first.isValid() && s_selectedElement.bases.second.isValid() && s_selectedElement.bases == *it) {
						/* Shift */
						shift_arrow_strength_directions[i * 4 * 4 + j * 4] = GLfloat(s_selectedElement.scale * 2.0 - 1.0);

						for(int k = 0; k < 2; ++k)
							colors[i * 4 * 4 + j * 4 + k] = GLfloat(0.0);
						for(int k = 0; k < 2; ++k)
							colors[i * 4 * 4 + j * 4 + 2 + k] = GLfloat(1.0);
					}
					else {
						/* Shift */
						shift_arrow_strength_directions[i * 4 * 4 + j * 4] = GLfloat(0.0);

						for(int k = 0; k < 4; ++k)
							colors[i * 4 * 4 + j * 4 + k] = GLfloat(1.0);
					}
					

					/* Strength */
					double dist = (coord2 - coord1).length() / DNA::STEP - 1.0;

					shift_arrow_strength_directions[i * 4 * 4 + j * 4 + 1] =  float(MIN(1.0, MAX(0.0, 1.0 - dist * dist)));

					/* Arrow */
					shift_arrow_strength_directions[i * 4 * 4 + j * 4 + 2] = base1_type == base2_type ? 0.0f : 1.0f;

					/* Direction */
					shift_arrow_strength_directions[i * 4 * 4 + j * 4 + 3] = (base1_type == Model::Base::THREE_PRIME_END || base2_type == Model::Base::FIVE_PRIME_END) ? 1.0f : 0.0f;

					/* Vertices for texture generation */

					for(int k = 0; k < 2; ++k)
						vertices[i * 2 * 4 + j * 2 + k] = quad[j * 2 + k];
				}
			}

			view.beginGL();

			if (!s_drawData.s_gl_initialized)
				initializeGL();

			/*
			 * Perhaps a display list for all the settings?
			 */

			GLCALL(glUseProgram(s_drawData.program));
			
			/* FIXME: Cache uniform as it rarely changes */
			GLCALL(glUniform2f(s_drawData.windowSize_uniform, (GLfloat) view.portWidth(), (GLfloat) view.portHeight()));

			/*
			 * Calls that could be put in a display list
			 */
			GLCALL(glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT));
			
			GLCALL(glEnable(GL_BLEND));
			GLCALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
			
			//glDisable(GL_DEPTH_TEST); // I think we want this, instead of the one below
			GLCALL(glDepthMask(GL_FALSE));

			GLCALL(glBindTexture(GL_TEXTURE_3D, s_drawData.texture));
			GLCALL(glUniform1i(s_drawData.texture_uniforms[1], 0));

			/*
			 * End of calls that could be put in a display list
			 */

			GLCALL(glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT));

			GLCALL(glEnableVertexAttribArray(s_drawData.points_attrib[0]));
			GLCALL(glEnableVertexAttribArray(s_drawData.points_attrib[1]));
			GLCALL(glEnableVertexAttribArray(s_drawData.shift_arrow_strength_direction_attrib));
			GLCALL(glEnableVertexAttribArray(s_drawData.color_attrib));
			GLCALL(glEnableClientState(GL_VERTEX_ARRAY));

			/* Note that the stride parameter is a bit weird, zero and 3 * sizeof(GLfloat) actually means the same thing, zero is a special case. So 2 * 3 * sizeof(GLfloat) is required */
			GLCALL(glVertexAttribPointer(s_drawData.points_attrib[0], 3, GL_FLOAT, GL_FALSE, 2 * 3 * sizeof(GLfloat), points));
			GLCALL(glVertexAttribPointer(s_drawData.points_attrib[1], 3, GL_FLOAT, GL_FALSE, 2 * 3 * sizeof(GLfloat), points + 3));
			GLCALL(glVertexAttribPointer(s_drawData.shift_arrow_strength_direction_attrib, 4, GL_FLOAT, GL_FALSE, 0, shift_arrow_strength_directions));
			GLCALL(glVertexAttribPointer(s_drawData.color_attrib, 4, GL_FLOAT, GL_FALSE, 0, colors));
			GLCALL(glVertexPointer(2, GL_FLOAT, 0, vertices));

			GLCALL(glDrawArrays(GL_QUADS, 0, GLsizei(s_closeBasesTable.size() * 4)));

			GLCALL(glPopClientAttrib());

			GLCALL(glPopAttrib());
			GLCALL(glUseProgram(0));

			view.endGL();

			delete[] vertices;
			delete[] colors;
			delete[] shift_arrow_strength_directions;
			delete[] points;
		}

		bool ConnectSuggestionsLocatorNode::isBounded() const {
			return false;
		}

		bool ConnectSuggestionsLocatorNode::isTransparent() const {
			return true;
		}

		void *ConnectSuggestionsLocatorNode::creator() {
			return new ConnectSuggestionsLocatorNode();
		}

		MStatus ConnectSuggestionsLocatorNode::initialize() {

			return MStatus::kSuccess;
		}

		const MTypeId ConnectSuggestionsLocatorNode::id(CONNECT_SUGGESTIONS_LOCATOR_ID);
		ConnectSuggestionsLocatorNode::DrawData ConnectSuggestionsLocatorNode::s_drawData;
		std::list<ConnectSuggestionsLocatorNode::BasePair> ConnectSuggestionsLocatorNode::s_closeBasesTable;
		ConnectSuggestionsLocatorNode::SelectedElement ConnectSuggestionsLocatorNode::s_selectedElement = { ConnectSuggestionsLocatorNode::BasePair(), 0.0, 0.0 };

		MStatus ConnectSuggestionsLocatorNode::UpdateBase(Model::Base & base) {
			MStatus status;

			/*
			 * Remove from already existing suggestions
			 */

			for(std::list<BasePair>::iterator it = s_closeBasesTable.begin(); it != s_closeBasesTable.end();) {
				if (it->first == base || it->second == base) {
					it->detachCallbacks();
					it = s_closeBasesTable.erase(it);
				}
				else
					++it;
			}

			Model::Helix helix;

			helix = base.getParent(status);

			if (!status) {
				status.perror("Base::getParent");
				return status;
			}

			MVector base_translation;

			if (!(status = base.getTranslation(base_translation, MSpace::kWorld))) {
				status.perror("Base::getTranslation");
				return status;
			}

			/*
			 * Store potential connections here
			 */

			MItDag it(MItDag::kDepthFirst, MFn::kTransform, &status);

			if (!status) {
				status.perror("MItDag::#ctor");
				return status;
			}

			for(; !it.isDone(); it.next()) {
				MDagPath path;
				
				if (!(status = it.getPath(path))) {
					status.perror("MItDag::getPath");
					return status;
				}

				MFnTransform transform (path, &status);

				if (!status) {
					status.perror("MFnTransform::#ctor");
					return status;
				}

				if (transform.typeId(&status) == HelixBase::id) {
					/*
					 * NOTE: Do we want suggestions between bases on the same helix?
					 */

					if (transform.parentCount() > 0 && helix == transform.parent(0, &status))
						continue;

					MVector translation = transform.getTranslation(MSpace::kWorld, &status);

					if(!status) {
						status.perror("MFnTransform::getTranslation");
						return status;
					}

					if ((translation - base_translation).length() < CONNECT_SUGGESTIONS_MAX_DISTANCE) {
						Model::Base otherBase(path);
						s_closeBasesTable.push_back(BasePair(base, otherBase));
					}
				}

				if (!status) {
					status.perror("MFnDagNode::typeId");
					return status;
				}
			}

			return MStatus::kSuccess;
		}

		MStatus ConnectSuggestionsLocatorNode::UpdateTransform(Model::Object & object) {
			MStatus status;

			/*
			 * Find any children that are of type Helix
			 */

			MItDag it(MItDag::kDepthFirst, MFn::kPluginTransformNode, &status);

			if (!status) {
				status.perror("MItDag::#ctor");
				return status;
			}

			MObject obj = object.getObject(status);

			if (!status) {
				status.perror("Object::getObject");
				return status;
			}

			if (!(status = it.reset(obj, MItDag::kDepthFirst, MFn::kPluginTransformNode))) {
				status.perror("MItDag::reset");
				return status;
			}

			for(; !it.isDone(); it.next()) {
				MObject child = it.currentItem(&status);

				if (!status) {
					status.perror("MItDag::currentItem");
					return status;
				}

				if (MFnDagNode(child).typeId(&status) == Helix::id) {
					Model::Helix helix(child);
					UpdateHelix(helix);
				}

				if (!status) {
					status.perror("MFnDagNode::typeId");
					return status;
				}
			}

			return MStatus::kSuccess;
		}

		void ConnectSuggestionsLocatorNode::initializeGL() {
			/*
			 * Setup shaders and programs
			 */

			std::cerr << "ConnectSuggestionsLocatorNode::initializeGL" << std::endl;

			MStatus status;

			GLint attrib_locations[4], uniform_locations[3];

			SETUPOPENGLSHADERS(CONNECT_SUGGESTIONS_LOCATOR_GLSL_VERTEX_SHADER, CONNECT_SUGGESTIONS_LOCATOR_GLSL_FRAGMENT_SHADER, CONNECT_SUGGESTIONS_LOCATOR_GLSL_UNIFORM_NAMES, uniform_locations, 3, CONNECT_SUGGESTIONS_LOCATOR_GLSL_ATTRIB_NAMES, attrib_locations, 4, s_drawData.program, s_drawData.vertex_shader, s_drawData.fragment_shader, status);

			if (!status) {
				status.perror("SetupOpenGLShaders");

				s_drawData.s_gl_failure = true;
				return;
			}

			s_drawData.points_attrib[0] = attrib_locations[0];
			s_drawData.points_attrib[1] = attrib_locations[1];
			s_drawData.shift_arrow_strength_direction_attrib = attrib_locations[2];
			s_drawData.color_attrib = attrib_locations[3];

			s_drawData.texture_uniforms[0] = uniform_locations[0];
			s_drawData.texture_uniforms[1] = uniform_locations[1];
			s_drawData.windowSize_uniform = uniform_locations[2];

			/*
			 * Setup textures
			 */

			//const GLubyte slideTexture[] = { CONNECT_SUGGESTIONS_SLIDE_TEXTURE };//, arrowTexture[] = { CONNECT_SUGGESTIONS_ARROW_TEXTURE };

			GLCALL(glPushAttrib(GL_TEXTURE_BIT));
			GLCALL(glGenTextures(1, &s_drawData.texture));
			
			
			GLCALL(glBindTexture(GL_TEXTURE_3D, s_drawData.texture));
			arrow_image_call_teximage3d();
			
			GLCALL(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
			GLCALL(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
			GLCALL(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP));
			GLCALL(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP));
			GLCALL(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP));

			GLCALL(glPopAttrib());

			s_drawData.s_gl_initialized = true;
		}

		MStatus ConnectSuggestionsLocatorNode_GetScale(const MVector & u, const MVector & near_p, ConnectSuggestionsLocatorNode::BasePair & bases, double & scale_, double & distance_) {
			MStatus status;
			MVector base_translation0, base_translation1;

			if (!(status = bases.first.getTranslation(base_translation0, MSpace::kWorld))) {
				status.perror("Base::getTranslation 0");
				return status;
			}

			if (!(status = bases.second.getTranslation(base_translation1, MSpace::kWorld))) {
				status.perror("Base::getTranslation 1");
				return status;
			}

			MVector v = base_translation1 - base_translation0, ucrossv = u ^ v, ab = base_translation0 - near_p;

			double distance = ab * ucrossv.normal();

			if (abs(distance) <= 0.1) {
				/*
					* Now figure out where on the line between the bases the user clicked
					*/

				MVector projEye = u * (ab * u / (u * u)) + near_p;
				MVector projDelta  = projEye - base_translation0, delta = base_translation1 - base_translation0;

				distance_ = projEye.length();

				if (projDelta.normal() * delta.normal() < 0.0)
					return MStatus::kNotFound;

				double scale = ((projDelta).length() / (delta).length());

				if (scale > 1.0)
					return MStatus::kNotFound;

				scale_ = scale;

				return MStatus::kSuccess;
			}

			return MStatus::kNotFound;
		}
		
		struct Hit {
			double scale, distance;
			std::list< ConnectSuggestionsLocatorNode::BasePair >::iterator it;
			
			inline Hit(std::list< ConnectSuggestionsLocatorNode::BasePair >::iterator it_, double scale_, double distance_) : scale(scale_), distance(distance_), it(it_) {
				
			}
			
			inline bool operator<(const Hit & h) const {
				return distance < h.distance;
			}
		};

		void ConnectSuggestionsLocatorNode::onPress(int x, int y, M3dView & view, ConnectSuggestionsContext & context) {
			MStatus status;
			//double shift = 0.0;

			MPoint near_p, far_p;

			if (!(status = view.viewToWorld((short) x, (short) y, near_p, far_p))) {
				status.perror("M3dView::viewToWorld");
				return;
			}

			MVector u = near_p - far_p;

			std::list<Hit> hits;

			for(std::list<BasePair>::iterator it = s_closeBasesTable.begin(); it != s_closeBasesTable.end(); ++it) {
				double scale, distance;

				if (!(status = ConnectSuggestionsLocatorNode_GetScale(u, near_p, *it, scale, distance))) {
					if (status != MStatus::kNotFound)
						status.perror("ConnectSuggestionsLocatorNode_GetScale");
				}
				else {
					/*
					 * The user hit the stripe. Now figure out which hit is the closest
					 */

					hits.push_back(Hit(it, scale, distance));
				}
			}

			/*
			 * Find the closest
			 */

			std::list<Hit>::iterator it;

			if ((it = std::min_element(hits.begin(), hits.end())) != hits.end()) {
				/*
				 * This is the closest element the user selected
				 */

				s_selectedElement.bases = *it->it;
				s_selectedElement.scale = it->scale;
				s_selectedElement.start_scale = it->scale;

				/*
				 * Maya won't do a repaint if not necessary, but as we're animating, we need it to
				 */

				view.refresh(true);
			}
		}

		void ConnectSuggestionsLocatorNode::onDrag(int x, int y, M3dView & view, ConnectSuggestionsContext & context) {
			MStatus status;
			MPoint near_p, far_p;

			if (!(status = view.viewToWorld((short) x, (short) y, near_p, far_p))) {
				status.perror("M3dView::viewToWorld");
				return;
			}

			MVector u = near_p - far_p;
			double distance;

			ConnectSuggestionsLocatorNode_GetScale(u, near_p, s_selectedElement.bases, s_selectedElement.scale, distance);

			/*
			 * Maya won't do a repaint if not necessary, but as we're animating, we need it to
			 */

			view.refresh(true);
		}

		void ConnectSuggestionsLocatorNode::onRelease(int x, int y, M3dView & view, ConnectSuggestionsContext & context) {
			MStatus status;
			MPoint near_p, far_p;

			if (!(status = view.viewToWorld((short) x, (short) y, near_p, far_p))) {
				status.perror("M3dView::viewToWorld");
				return;
			}

			MVector u = near_p - far_p;

			if (s_selectedElement.bases.first.isValid() && s_selectedElement.bases.second.isValid()) {
				double distance;
				ConnectSuggestionsLocatorNode_GetScale(u, near_p, s_selectedElement.bases, s_selectedElement.scale, distance);
				double drag_distance = s_selectedElement.scale - s_selectedElement.start_scale;

				if (abs(drag_distance) > CONNECT_SUGGESTIONS_LOCATOR_SCALE_CONNECT_THRESHOLD) {
					/*
					 * Make the connection, but it must be undoable!
					 */

					/*
					 * TODO: I hope we can assume that negative is towards the first element.
					 */

					if (drag_distance < 0) {

						if (s_selectedElement.bases.first.type(status) != Model::Base::THREE_PRIME_END) {
							std::cerr << "Connect " << s_selectedElement.bases.second.getDagPath(status).fullPathName().asChar() << " -> " << s_selectedElement.bases.first.getDagPath(status).fullPathName().asChar() << std::endl;

							context.connect(s_selectedElement.bases.second, s_selectedElement.bases.first);

							/*
							 * Now remove this suggestion. repainting will be done below
							 */

							s_closeBasesTable.erase(std::remove(s_closeBasesTable.begin(), s_closeBasesTable.end(), s_selectedElement.bases), s_closeBasesTable.end());
						}
					}
					else {
						if (s_selectedElement.bases.first.type(status) != Model::Base::FIVE_PRIME_END) {
							std::cerr << "Connect " << s_selectedElement.bases.second.getDagPath(status).fullPathName().asChar() << " <- " << s_selectedElement.bases.first.getDagPath(status).fullPathName().asChar() << std::endl;

							context.connect(s_selectedElement.bases.first, s_selectedElement.bases.second);

							/*
							 * Now remove this suggestion. repainting will be done below
							 */

							s_closeBasesTable.erase(std::remove(s_closeBasesTable.begin(), s_closeBasesTable.end(), s_selectedElement.bases), s_closeBasesTable.end());
						}
					}
				}

				s_selectedElement.bases.first = Model::Base();
				s_selectedElement.bases.second = Model::Base();

				/*
				 * Maya won't do a repaint if not necessary, but as we're animating, we need it to
				 */

				view.refresh(true);
			}
		}
		
		class Condition {
		public:
			inline Condition(MObject & node_) : node(node_) { }
			
			inline bool operator() (ConnectSuggestionsLocatorNode::BasePair & pair) const {
				return pair.first == node || pair.second == node;
			}
			
			MObject node;
		};

		void MNodeMessage_closeBasesTable_base_preRemovalCallback(MObject & node, void *clientData) {
			/*
			 * This node is about to be deleted and it is in the list of connection suggestions.
			 * It must be removed from there
			 */

			std::cerr << "Removing all references in the suggestions table for node: " << MFnDagNode(node).fullPathName().asChar() << std::endl;

			std::list<ConnectSuggestionsLocatorNode::BasePair>::iterator it;
			
			ConnectSuggestionsLocatorNode::s_closeBasesTable.erase(std::remove_if(ConnectSuggestionsLocatorNode::s_closeBasesTable.begin(), ConnectSuggestionsLocatorNode::s_closeBasesTable.end(), Condition(node)), ConnectSuggestionsLocatorNode::s_closeBasesTable.end());
		}

		ConnectSuggestionsLocatorNode::BasePair::BasePair(Model::Base & first_, Model::Base & second_) : first(first_), second(second_) {
			MStatus status;

			MObject first_object = first.getObject(status);

			if (!status) {
				status.perror("Base::getObject 1");
				return;
			}

			preRemovalCallbackId[0] = MNodeMessage::addNodePreRemovalCallback(first_object, MNodeMessage_closeBasesTable_base_preRemovalCallback, NULL, &status);

			if (!status) {
				status.perror("MNodeMessage::addNodePreRemovalCallback 1");
				return;
			}

			MObject second_object = second.getObject(status);

			if (!status) {
				status.perror("Base::getObject 2");
				return;
			}

			preRemovalCallbackId[1] = MNodeMessage::addNodePreRemovalCallback(second_object, MNodeMessage_closeBasesTable_base_preRemovalCallback, NULL, &status);

			if (!status) {
				status.perror("MNodeMessage::addNodePreRemovalCallback 2");
				return;
			}
		}

		void ConnectSuggestionsLocatorNode::BasePair::detachCallbacks() {
			MStatus status;

			if (preRemovalCallbackId[0]) {
				if (!(status = MNodeMessage::removeCallback(preRemovalCallbackId[0])))
					status.perror("MNodeMessage::removeCallback 1");

				preRemovalCallbackId[0] = 0;
			}

			if (preRemovalCallbackId[1]) {
				if (!(status = MNodeMessage::removeCallback(preRemovalCallbackId[1])))
					status.perror("MNodeMessage::removeCallback 2");

				preRemovalCallbackId[1] = 0;
			}
		}
	}
}
