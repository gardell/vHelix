/*
 * Helix.h
 *
 *  Created on: 9 maj 2012
 *      Author: johan
 */

#ifndef _VHELIX_MA_PARSER_HELIX_H_
#define _VHELIX_MA_PARSER_HELIX_H_

#include <Vector.h>
#include <Matrix.h>

#include <string>
#include <vector>
#include <list>

/* C++11 compilers and Visual Studio 2010 includes smart pointer support */
#ifdef _MSC_VER

#include <memory>
using std::shared_ptr;
using std::weak_ptr;

#else /* While GCC implements them in the TR1 specification */

#include <tr1/memory>
using std::tr1::shared_ptr;
using std::tr1::weak_ptr;

#endif

#include <exception>
#include <iostream>
#include <algorithm>

namespace Helix {
	/*
	 * parse_exception: The base of the exceptions thrown during parsing
	 */

	class parse_exception : public std::exception {
	public:
		inline parse_exception(const std::string & what) throw() : m_what(what) {

		}

		inline parse_exception(const char *what) throw() : m_what(what) {

		}

		inline ~parse_exception() throw() {

		}

		inline virtual const char* what() const throw() {
			return m_what.c_str();
		}
	private:
		std::string m_what;
	};

	/*
	 * Node: A named structure with a list of parents and children Nodes
	 * The base class to Helix and Base objects but also used for other transform nodes that might occur
	 */

#define NODE_INHERITING_CLASSES_OPERATORS(ClassName)															\
	bool operator==(const ClassName & n) const { return static_cast<const Node &> (*this) == n; }				\
																												\
	bool operator!=(const ClassName & n) const { return static_cast<const Node &> (*this) != n; }						

	class Node {
	public:
		enum Type {
			NODE = 0,
			HELIX = 1,
			BASE = 2
		};

		inline Node() {

		}

		inline Node(const char *name) : m_name(name), m_update_cache_transform(true) {

		}

		inline Node(const Node & copy) : m_parents(copy.m_parents), m_children(copy.m_children), m_name(copy.m_name) {

		}

		inline virtual Type getType() const {
			return NODE;
		}

		typedef std::list<weak_ptr<Node> > List;

		inline List::iterator begin_parents() {
			return m_parents.begin();
		}

		inline List::iterator end_parents() {
			return m_parents.end();
		}

		inline List::iterator begin_children() {
			return m_children.begin();
		}

		inline List::iterator end_children() {
			return m_children.end();
		}

		inline List::const_iterator begin_parents() const {
			return m_parents.begin();
		}

		inline List::const_iterator end_parents() const {
			return m_parents.end();
		}

		inline List::const_iterator begin_children() const {
			return m_children.begin();
		}

		inline List::const_iterator end_children() const {
			return m_children.end();
		}

		inline void addParent(shared_ptr<Node> & node) {
			m_parents.push_back(node);
		}

		inline void addChild(shared_ptr<Node> & node) {
			m_children.push_back(node);
		}

		inline const char *getName() const {
			return m_name.c_str();
		}

		inline void setName(const char *name) {
			m_name = name;
		}

		/*
		 * Access the translation vector
		 */

		inline const Vector & getTranslation() const {
			return m_translate;
		}

		inline void setTranslation(const Vector & translation) {
			m_translate = translation;
			m_update_cache_transform = true;
		}

		/*
		 * Notice that the rotation is in *degrees*!
		 */

		inline const Vector & getRotation() const {
			return m_rotate;
		}

		inline void setRotation(const Vector & rotation) {
			m_rotate = rotation;
			m_update_cache_transform = true;
		}

		/*
		 * This method is slow, it generates a matrix using the internal translation and rotation
		 */

		inline const Matrix4x4 & getTransform() {
			if (m_update_cache_transform) {
				m_cache_transform = Matrix4x4::Translate(m_translate) * Matrix4x4::Rotate(m_rotate);
				m_update_cache_transform = false;
			}

			return m_cache_transform;
		}

		/*
		 * Even slower, iterates over the node's parents to generate the world transformation matrix
		 * Note that we don't concern cases with multiple parents (shouldn't exist in a helix scene although Maya supports it)
		 */

		inline Matrix4x4 getWorldTransform() {
			Matrix4x4 matrix = getTransform();
			Node *node = this;
			while(node->m_parents.size() > 0) {
				node = node->m_parents.begin()->lock().get();
				matrix = node->getTransform() * matrix;
			}

			return matrix;
		}

		/*
		 * For identifying the base
		 */

		bool operator==(const Node & n) const {
			if (m_name != n.m_name)
				return false;

			/*
			 * Find at least one of our parents that the other object shares
			 */

			bool found = false;
			for(List::const_iterator it = begin_parents(); it != end_parents(); ++it) {
				for(List::const_iterator n_it = n.begin_parents(); n_it != n.end_parents(); ++n_it) {
					if (*it->lock() == *n_it->lock())
						return true;

					found = true;
				}
			}

			/*
			 * As the names match, and we have no parents, the objects must equal
			 */

			return m_parents.empty() && n.m_parents.empty();
		}

		bool operator!=(const Node & n) const {
			return !this->operator==(n);
		}

	protected:
		List m_parents, m_children;
		std::string m_name;
		Vector m_translate, m_rotate; /* Note that rotation is in *degrees*! */

		/*
		 * The transform is generated when needed and cached here
		 */
		Matrix4x4 m_cache_transform;
		bool m_update_cache_transform;
	};

	/*
	 * Added the Strand class as a way of identifying a strand. Notice that the strand object does not exist in the .ma files, but are generated on import
	 */

	class Base;

	class Strand {
	public:
		inline Strand(const char *name, weak_ptr<Node> base) : m_base(base), m_name(name) {

		}

		bool contains_base(Base & base) const;

		const char *getName() const {
			return m_name.c_str();
		}

	private:
		weak_ptr<Node> m_base;
		std::string m_name;
	};

	/*
	 * Helix: Encapsulates a helix object, which contains a list of forward and backward strands as well as
	 * a translation vector and a name
	 */

	class Base : public Node {
		friend std::string labelToString(int label);
	public:
		NODE_INHERITING_CLASSES_OPERATORS(Base);

		inline virtual Type getType() const {
			return Node::BASE;
		}

		/*
		 * Return the base type (A, T, G, C or Invalid = not assigned)
		 * Note that connected bases share their type in the way that the 'source' of the connection in Maya has the value
		 * while the 'destination' node is not allowed to have a value
 		 */

		inline int getLabel() const {
			if (!m_isDestination)
				return m_label;

			shared_ptr<Node> opposite(m_opposite);

			if (!opposite)
				return Invalid;

			Base & base = static_cast<Base &> (*opposite);

			return getOppositeLabel(base.m_label);
		}

		inline void setLabel(int label) {
			shared_ptr<Node> opposite(m_opposite);

			if (opposite && m_isDestination)
				static_cast<Base &> (*opposite).m_label = (Label) label;
			else
				m_label = (Label) label;
		}

		/*
		 * Access the connected forward, backward and opposite bases
		 * As i don't use pointers but references, in order to avoid null pointers the has* methods are used
		 * before accessing the members
		 */

		inline bool hasBackwardConnectedBase() const {
			return m_backward.lock();
		}

		inline bool hasForwardConnectedBase() const {
			return m_forward.lock();
		}

		inline Base & getBackwardConnectedBase() const {
			return static_cast<Base &>(*m_backward.lock());
		}

		inline Base & getForwardConnectedBase() const {
			return static_cast<Base &>(*m_forward.lock());
		}

		inline bool hasOppositeConnectedBase() const {
			return m_opposite.lock().get() != NULL;
		}

		inline Base & getOppositeConnectedBase() const {
			return static_cast<Base &>(*m_opposite.lock());
		}

		inline void setBackwardConnectedBase(weak_ptr<Node> base) {
			m_backward = base;
		}

		inline void setForwardConnectedBase(weak_ptr<Node> base) {
			m_forward = base;
		}

		inline void setOppositeConnectedBase(weak_ptr<Node> base, bool isDestination) {
			m_opposite = base;
			m_isDestination = isDestination;
		}

		/*
		 * Notice that Scene::generate_strands() must have been called first
		 */

		inline weak_ptr<Strand> & getStrand() {
			return m_strand;
		}

		inline const weak_ptr<Strand> & getStrand() const {
			return m_strand;
		}

		inline void setStrand(weak_ptr<Strand> strand) {
			m_strand =  strand;
		}

		/*
		 * Constructors
		 */

		inline Base(const char *name, bool isDestination = false, int label = Invalid) : Node(name), m_isDestination(isDestination), m_label((Label) label) {

		}

		inline Base(const Base & base) : Node(base), m_opposite(base.m_opposite), m_forward(base.m_forward), m_backward(base.m_backward), m_isDestination(base.m_isDestination), m_label(base.m_label), m_translate(base.m_translate) {

		}

		enum Label { // GCC didn't allow for anonymous enums
			A = 0,
			T = 1,
			G = 2,
			C = 3,
			Invalid = 4
		};

	private:
		weak_ptr<Node> m_opposite, m_forward, m_backward;
		bool m_isDestination;
		Label m_label; /* Note that the value of m_label is only valid if m_isSource is true. Use the getLabel above instead */

		inline static int getOppositeLabel(int label) {
			switch(label) {
			case A:
				return T;
			case T:
				return A;
			case G:
				return C;
			case C:
				return G;
			default:
				return Invalid;
			}
		}

		Vector m_translate;

		/*
		 * Added reference to a common Strand class identifying the strand this Helix belongs to
		 */

		weak_ptr<Strand> m_strand;
	};

	class Helix : public Node {
	public:
		NODE_INHERITING_CLASSES_OPERATORS(Helix);

		inline virtual Type getType() const {
			return Node::HELIX;
		}

		inline Helix() {

		}

		inline Helix(const char *name) : Node(name) {

		}
	};

	/*
	 * Encapsulates all the helices from the file and some file information
	 * Can be used for future flags and options perhaps?
	 */

	class Scene {
	public:
		shared_ptr<Node> Root;

		inline Scene() : Root(new Node("|")) {

		}

		inline Scene(const char *filename) : Root(new Node("|")) {
			parse(filename);
		}

		void parse(const char *filename);

		bool getNodeByName(const char *uri, shared_ptr<Node> & result);

		typedef std::list<weak_ptr<Node> > HelixList;
		typedef std::list<shared_ptr<Node> > NodeList;
		typedef std::list<shared_ptr<Strand> > StrandList;

		inline HelixList::iterator begin_helices() {
			return m_helices.begin();
		}

		inline HelixList::const_iterator begin_helices() const {
			return m_helices.begin();
		}

		inline HelixList::iterator end_helices() {
			return m_helices.end();
		}

		inline HelixList::const_iterator end_helices() const {
			return m_helices.end();
		}

		inline NodeList::iterator begin_nodes() {
			return m_nodes.begin();
		}

		inline NodeList::const_iterator begin_nodes() const {
			return m_nodes.begin();
		}

		inline NodeList::iterator end_nodes() {
			return m_nodes.end();
		}

		inline NodeList::const_iterator end_nodes() const {
			return m_nodes.end();
		}

		inline void append_helix(shared_ptr<Helix> & helix) {
			m_nodes.push_back(helix);
			m_helices.push_back(helix);
		}

		/*
		 * For helices, use the above method!
		 */

		inline void append_node(shared_ptr<Node> & base) {
			m_nodes.push_back(base);
		}

		/*
		 * Generate or get a list of all the strands
		 */

		inline StrandList::iterator begin_strands() {
			if (m_strands.empty())
				generate_strands();

			return m_strands.begin();
		}

		inline StrandList::iterator end_strands() {
			return m_strands.begin();
		}

		void generate_strands();

	private:
		HelixList m_helices;
		NodeList m_nodes;
		StrandList m_strands;
	};
}

/*
 * Just for debugging
 */

std::ostream & operator<< (std::ostream &, const Helix::Node &);
std::ostream & operator<< (std::ostream &, const Helix::Scene &);
std::string labelToString(int label);


#endif /* _VHELIX_MA_PARSER_HELIX_H_ */
