/*
 * DNA.h
 *
 *  Created on: Jun 28, 2011
 *      Author: bjorn
 */

#ifndef DNA_H_
#define DNA_H_

#include <Definition.h>

#include <cmath>
#include <iostream>

#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MStringArray.h>
#include <maya/MObjectArray.h>
#include <maya/MVector.h>

#ifndef DEG2RAD
#define DEG2RAD(deg)	((deg) / 180.0 * M_PI)
#endif /* N DEG2RAD */

#ifndef RAD2DEG
#define RAD2DEG(rad)	((rad) / M_PI * 180.0)
#endif /* N RAD2DEG */

#define STRAND_NAMES	"forw", "backw"

/*
 * There's no string concatenation available, if PITCH or STEP is changed below, it must be changed here as well for cylinder texturing to work..
 */

#define PITCH_STR "720.0 / 21.0"
#define STEP_STR "0.334"

namespace DNA {
	/*
	 * Some constants defining DNA behaviour and visual representation
	 */

	const double PITCH = 720.0 / 21.0,														// degrees
				 STEP = 0.334,
				 RADIUS = 1.0,
				 SPHERE_RADIUS = 0.13,														// not dna properties, but visual
				 OPPOSITE_ROTATION = 155.0,
				 HELIX_RADIUS = RADIUS + 0.05,												// From the paper 'Self-assembly of DNA into nanoscale three-dimensional shapes'
				 Z_SHIFT = 0.165,															// Don't know what to call it, just got it from earlier source code
				 ONE_MINUS_SPHERE_RADIUS = (1.0 - SPHERE_RADIUS),
				 SEQUENCE_RENDERING_Y_OFFSET = 0.22,										// Multiplied by RADIUS
				 HONEYCOMB_X_STRIDE = 2.0 * DNA::HELIX_RADIUS * cos(DEG2RAD(30)),			// Constants for the honeycomb lattice
				 HONEYCOMB_Y_STRIDE = 2.0 * DNA::HELIX_RADIUS * (1.0 + sin(DEG2RAD(30))),
				 HONEYCOMB_Y_OFFSET = 1.0 * DNA::HELIX_RADIUS * sin(DEG2RAD(30));

	/*
	 * The create base gui allows the user to specify the number of bases per strand and remembers your choice. The default on startup is defined here
	 */

	const long CREATE_DEFAULT_NUM_BASES = 21;

	const int BASES = 4; // Trivial, but still, cleaner code with defines

	// Since Create, Extend and Import all use the properties above to generate the correct positions for each base
	// This utility method should be used and not the individual parameters above. However, it is not suitable for all cases

	MStatus CalculateBasePairPositions(double index, MVector & forward, MVector & backward, double offset = 0.0, double totalNumBases = 0);

	/*
	 * A small API for managing the DNA enumerations
	 */

	enum Values {
		A = 0,
		T = 1,
		G = 2,
		C = 3,
		Invalid = 4
	};

	inline Values Value_fromChar(char ch) {
		switch(ch) {
		case 'A':
		case 'a':
			return A;
		case 'T':
		case 't':
			return T;
		case 'G':
		case 'g':
			return G;
		case 'C':
		case 'c':
			return C;
		default:
			return Invalid;
		}
	}

	class Name {
	public:
		inline Name() : m_value(Invalid) {

		}

		inline Name(const Name & copy) : m_value(copy.m_value) {

		}

		inline Name(Values v) : m_value(v) {

		}

		inline Name(char ch) : m_value(Value_fromChar(ch)) {
		}

		inline bool operator==(const Name & n) const {
			return m_value == n.m_value;
		}

		inline bool operator!=(const Name & n) const {
			return m_value != n.m_value;
		}

		inline bool operator==(Values v) const {
			return m_value == v;
		}

		inline bool operator!=(Values v) const {
			return m_value != v;
		}

		inline Name & operator=(const Name & n) {
			m_value = n.m_value;

			return *this;
		}

		inline Name & operator=(const Values & v) {
			m_value = v;

			return *this;
		}

		inline Name & operator=(char ch) {
			m_value = Value_fromChar(ch);

			return *this;
		}

		inline char toChar() const {
			switch(m_value) {
			case A:
				return 'A';
			case T:
				return 'T';
			case G:
				return 'G';
			case C:
				return 'C';
			default:
				return '?';
			}
		}

		inline Name opposite() const {
			switch(m_value) {
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

		operator short int() {
			return (short int) m_value;
		}

	private:
		Values m_value;
	};

	/*
	 * Bases going in the positive Z-axis direction are named forw_{i} while the others are named backw_{i} where i >= 1
	 */

	const char *GetStrandName(int index);

	/*
	 * New handling of arrow and molecule. Uses MEL commands to create the models instead of .ma files.
	 * Also aware of New Scene commands that will delete the nodes and invalidate any references
	 * Notice that it will not return the transform called 'BackboneArrow' but all its shapes
	 */

	MStatus GetMoleculeAndArrowShapes(MDagPathArray & shapes);
}

#endif /* DNA_H_ */
