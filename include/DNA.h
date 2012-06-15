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
#include <maya/MStringArray.h>
#include <maya/MObjectArray.h>
#include <maya/MVector.h>

#ifndef DEG2RAD
#define DEG2RAD(deg)	((deg) / 180.0 * M_PI)
#endif /* N DEG2RAD */

#ifndef RAD2DEG
#define RAD2DEG(rad)	((rad) / M_PI * 180.0)
#endif /* N RAD2DEG */

#define HELIX_GEOMETRY_PATH			""

#define HELIXBASE_MODEL_SOURCE_FILES HELIX_GEOMETRY_PATH "BackboneArrow.ma"

namespace DNA {
	// Some DNA properties as well as visual settings.
	//

	const double PITCH = 720.0 / 21.0, // degrees
				 STEP = 0.334,
				 RADIUS = 1.0,
				 SPHERE_RADIUS = 0.13, // not dna properties, but visual
				 HELIX_RADIUS = RADIUS + SPHERE_RADIUS + 0.20, // TODO: This looked visually correct :-)
				 Z_SHIFT = 0.165; // Don't know what to call it, just got it from earlier source code
	const double ONE_MINUS_SPHERE_RADIUS = (1.0 - SPHERE_RADIUS);

	// Since Create, Extend and Import all use the properties above to generate the correct positions for each base
	// This utility method should be used and not the parameters above

	MStatus CalculateBasePairPositions(double index, MVector & forward, MVector & backward, double offset = 0.0, double totalNumBases = 0);

	const float SEQUENCE_RENDERING_Y_OFFSET = 0.22f; // Multiplied by RADIUS

	// Constants for generating the honeycomb lattice

	const double HONEYCOMB_X_STRIDE = 2.0 * DNA::HELIX_RADIUS * cos(DEG2RAD(30)),
						 HONEYCOMB_Y_STRIDE = 2.0 * DNA::HELIX_RADIUS * (1.0 + sin(DEG2RAD(30))),
						 HONEYCOMB_Y_OFFSET = 1.0 * DNA::HELIX_RADIUS * sin(DEG2RAD(30));

	// Some defines that makes the code look nicer and easier to read, when dealing directly with DNA bases
	//

	/*
	 * A small API for managing the DNA enumerations
	 */

	/*
	 * Enumerates the nucleotides. Notice the 'Invalid' that is represented as a '?' on the base. This is the default
	 */

	enum Values {
		A = 0,
		T = 1,
		G = 2,
		C = 3,
		Invalid = 4
	};

	class Names {
	public:
		inline Names() : m_value(Invalid) {

		}

		inline Names(const Names & copy) : m_value(copy.m_value) {

		}

		inline Names(Values v) : m_value(v) {

		}

		inline Names(char c) {
			this->operator=(c);
		}

		inline bool operator==(const Names & n) const {
			return m_value == n.m_value;
		}

		inline bool operator!=(const Names & n) const {
			return m_value != n.m_value;
		}

		inline bool operator==(Values v) const {
			return m_value == v;
		}

		inline bool operator!=(Values v) const {
			return m_value != v;
		}

		inline Names & operator=(const Names & n) {
			m_value = n.m_value;

			return *this;
		}

		inline Names & operator=(const Values & v) {
			m_value = v;

			return *this;
		}

		inline Names & operator=(char c) {
			switch(c) {
			case 'A':
			case 'a':
				m_value = A;
				break;
			case 'T':
			case 't':
				m_value = T;
				break;
			case 'G':
			case 'g':
				m_value = G;
				break;
			case 'C':
			case 'c':
				m_value = C;
				break;
			default:
				m_value = Invalid;
				break;
			}

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

		inline Names opposite() const {
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
	 * The create base gui allows the user to specify the number of bases per strand and remembers your choice. The default on startup is defined here
	 */

	const long CREATE_DEFAULT_NUM_BASES = 21;

	const int BASES = 4; // Trivial, but still, cleaner code with defines

	/*
	 * FIXME: The molecule and arrow models should be baked into the executable and not loaded as external models
	 */

	MStatus GetMoleculeModel(MDagPath & result);
	MStatus GetArrowModel(MDagPath & result);
}

#endif /* DNA_H_ */
