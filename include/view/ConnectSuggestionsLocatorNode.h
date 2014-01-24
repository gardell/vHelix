#ifndef _CONNECT_SUGGESTIONS_LOCATOR_H_
#define _CONNECT_SUGGESTIONS_LOCATOR_H_

#include <Definition.h>
#include <model/Helix.h>
#include <model/Base.h>
#include <view/ConnectSuggestionsContext.h>

#include <maya/MPxLocatorNode.h>

#include <list>
#include <algorithm>

#define CONNECT_SUGGESTIONS_LOCATOR_ID 0x02114123
#define CONNECT_SUGGESTIONS_LOCATOR_NAME "connectSuggestionsLocator"

namespace Helix {
	namespace View {
		/*
		 * Locator that renders suggested connections for helices. Also enables the user to interface with i,
		 * by clicking on the suggestions that show up to make connections
		 * Because performance is an issue, data is only collected on events
		 *
		 * Using gl points to render the GUI as they're oriented towards the camera and really fast
		 */

		void MNodeMessage_Base_AttributeChangedProc(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData);
		void MNodeMessage_Helix_AttributeChangedProc(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData);
		void MModelMessage_ConnectSuggestionsLocatorNode_activeListModified(void *clientData);

		class ConnectSuggestionsLocatorNode : public MPxLocatorNode {
		public:
			struct BasePair {
				Model::Base first, second;
				MCallbackId preRemovalCallbackId[2];

				/*
				 * Also registers preRemoval callback events so that the bases will be removed from the s_closeBasesTable when deleted
				 */

				BasePair(Model::Base & first_, Model::Base & second_);

				inline BasePair() {
					preRemovalCallbackId[0] = 0;
					preRemovalCallbackId[1] = 0;
				}

				/*
				 * Remove callback listeners. Can't be implemented as a destructor as the object will be copied around
				 */

				void detachCallbacks();

				inline bool operator==(const BasePair & basePair) const {
					return first == basePair.first && second == basePair.second;
				}
			};
			friend class ConnectSuggestionsToolCommand;
			friend void MNodeMessage_Base_AttributeChangedProc(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData);
			friend void MNodeMessage_Helix_AttributeChangedProc(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData);
			friend void MNodeMessage_Transform_AttributeChangedProc(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *clientData);
			friend void MMessage_ConnectSuggestionsLocatorNode_preRemovalCallback(MObject &node, void *clientData);
			friend void MModelMessage_ConnectSuggestionsLocatorNode_activeListModified(void *clientData);
			friend MStatus ConnectSuggestionsLocatorNode_GetScale(const MVector & u, const MVector & near_p, BasePair & bases, double & scale_, double & distance_);
			friend void MNodeMessage_closeBasesTable_base_preRemovalCallback(MObject & node, void *clientData);
		public:
			ConnectSuggestionsLocatorNode();
			virtual ~ConnectSuggestionsLocatorNode();

			virtual void postConstructor();

			virtual void draw(M3dView &view, const MDagPath &path, M3dView::DisplayStyle style, M3dView::DisplayStatus status);

			virtual bool isBounded() const;
			virtual bool isTransparent() const;

			static void * creator();
			static MStatus initialize();

			const static MTypeId id;

			/*
			 * These are called by the Context
			 */

			static void onPress(int x, int y, M3dView & view, ConnectSuggestionsContext & context);
			static void onDrag(int x, int y, M3dView & view, ConnectSuggestionsContext & context);
			static void onRelease(int x, int y, M3dView & view, ConnectSuggestionsContext & context);
		
		protected:
			struct DrawData {
				/*
				 * OpenGL object variables
				 * textures: slide, single arrow, double arrow where arrows should be in a 3D texture as there will be an attrib float to select the appropriate one
				 */
				GLuint program, vertex_shader, fragment_shader, texture;
				/* texture uniform binding points, two vec4 for the bases positions and finally a vec3 containing the scroll bar shift (positive and negative), the strength of the binding and the type of arrow to render (double or single) */
				GLint texture_uniforms[2], windowSize_uniform, points_attrib[2], shift_arrow_strength_direction_attrib, color_attrib;

				bool s_gl_initialized, s_gl_failure;

				inline DrawData() :
						program(0),
						vertex_shader(0),
						fragment_shader(0),
						texture(0),
						windowSize_uniform(-1),
						shift_arrow_strength_direction_attrib(-1),
						color_attrib(-1),
						s_gl_initialized(false),
						s_gl_failure(false) { }
			} static s_drawData;

			/*
			 * Here's where we store data about our bases and their potential bindings
			 * They're only updated by the event handler that's registered for changes such as translation of bases and transforms on helices.
			 * they should be triggered on creation of new helices and bases as they're also always translated
			 * whenever a base is translated, iterate over all other bases to find out potential connections,
			 * also iterate over existing connections and remove non-interesting ones
			 * this will avoid the n^2 number of iterations which would be required if we didn't listen to events
			 * note that events triggered by helices will be slightly heavier to process as they will involve all their child bases
			 */

			static std::list<BasePair> s_closeBasesTable;

			/*
			 * Generate connection data for the elements in question
			 */

			static MStatus UpdateBase(Model::Base & base);
			
			class UpdateHelix_BaseOp {
			public:
				inline void operator()(Model::Base & base) const {
					UpdateBase(base);
				}
			};
			
			static inline MStatus UpdateHelix(Model::Helix & helix) {
				std::for_each(helix.begin(), helix.end(), UpdateHelix_BaseOp());
				
				return MStatus::kSuccess;
			}

			/*
			 * When helices are grouped under a transform
			 */

			static MStatus UpdateTransform(Model::Object & object);

			/*
			 * Is a GUI component currently selected?
			 */

			struct SelectedElement {
				BasePair bases;

				double scale, start_scale;
			} static s_selectedElement;

			/*
			 * Is tracking enabled? This degrades performance a lot when working with large models, such as importing JSON models
			 */

			//static bool s_trackingEnabled;

			static void initializeGL();

			MCallbackId m_preRemovalCallbackId, m_activeListModifiedCallbackId;
		};
	}
}

#endif /* N _CONNECT_SUGGESTIONS_LOCATOR_H_ */
