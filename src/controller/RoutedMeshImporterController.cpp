#include <controller/RoutedMeshImporter.h>
#include <controller/PaintStrand.h>
#include <Creator.h>

#include <maya/MQuaternion.h>

#include <fstream>
#include <string>

namespace Helix {
	namespace Controller {
		MStatus RoutedMeshImporter::read(const char *filename) {
			std::ifstream file(filename);

			if (!file) {
				MString errorString(MString("Failed to open \"") + filename + "\" for reading.");
				MGlobal::displayError(errorString);
				HPRINT("%s", errorString.asChar());
				return MStatus::kFailure;
			}

			while (file.good()) {
				std::string line;
				std::getline(file, line);

				if (line.length() == 0 || line[0] == '#')
					continue;

				switch (line[0]) {
				case 'v':
				{
					MVector vertex;
					if (sscanf(line.c_str(), "v %lf %lf %lf", &vertex.x, &vertex.y, &vertex.z) != 3) {
						HPRINT("Unformatted vertex line: %s", line.c_str());
						return MStatus::kFailure;
					}
					m_vertices.push_back(vertex);
				}
					break;
				case 'e':
				{
					Edge edge;
					switch (sscanf(line.c_str(), "e %u %lf %lf %lf", &edge.vertex, &edge.cylinder[0], &edge.cylinder[1], &edge.angle)) {
					case 4:
						HPRINT("Parsed \"%s\" as e %u %f %f %f", line.c_str(), edge.vertex, edge.cylinder[0], edge.cylinder[1], edge.angle);
						edge.hasCylinder = true;
						edge.hasAngle = true;
						break;
					case 3:
						HPRINT("Parsed \"%s\" as e %u %f %f", line.c_str(), edge.vertex, edge.cylinder[0], edge.cylinder[1]);
						edge.hasCylinder = true;
						edge.hasAngle = false;
						break;
					case 1:
						HPRINT("Parsed \"%s\" as e %u", line.c_str(), edge.vertex);
						edge.hasCylinder = false;
						edge.hasAngle = false;
						break;
					default:
						HPRINT("Unformatted edge line: %s", line.c_str());
						return MStatus::kFailure;
					}
					--edge.vertex; // In the file the indices are [1, N] but here we
								   // index arrays so [0, N-1].
					m_edges.push_back(edge);
				}
					break;
				case 'r':
					if (sscanf(line.c_str(), "r %lf", &m_initialRotation) != 1) {
						HPRINT("Unformatted initial rotation line: %s", line.c_str());
						return MStatus::kFailure;
					}
					break;
				default:
					HPRINT("Unknown command: %s", line.c_str());
					return MStatus::kFailure;
					break;
				}
			}

			MStatus status;
			std::vector<Model::Helix> helices;
			helices.reserve(m_edges.size() - 1);

			{
				double rotation(m_initialRotation);
				std::vector<Edge>::const_iterator it = m_edges.begin();
				std::vector<Edge>::const_iterator prev_it = it++;

				if (prev_it->vertex >= m_vertices.size()) {
					HPRINT("Index out of range: %u", prev_it->vertex);
					return MStatus::kInvalidParameter;
				}

				if (it->vertex >= m_vertices.size()) {
					HPRINT("Index out of range: %u", it->vertex);
					return MStatus::kInvalidParameter;
				}

				Creator creator;
				MVector start(m_vertices[prev_it->vertex]);
				MVector end(m_vertices[it->vertex]);
				MVector normal((end - start).normal());

				MVector tangent((normal ^ MVector::yAxis ^ normal).normal().rotateBy(MQuaternion(toRadians(rotation), normal)));
				MVector start_cylinder(start + normal * prev_it->cylinder[0]);
				MVector end_cylinder(start + normal * prev_it->cylinder[1]);
				double length((end_cylinder - start_cylinder).length());

				MVector error_vector(normal * ((length - DNA::HelixLength(length)) / 2));
				start_cylinder += error_vector;
				end_cylinder -= error_vector;

				for (; it != m_edges.end(); ++it, ++prev_it) {
					if (!prev_it->hasCylinder)
						continue;

					if (prev_it->vertex >= m_vertices.size()) {
						HPRINT("Index out of range: %u", prev_it->vertex);
						return MStatus::kInvalidParameter;
					}

					if (it->vertex >= m_vertices.size()) {
						HPRINT("Index out of range: %u", it->vertex);
						return MStatus::kInvalidParameter;
					}

					const MVector center((end_cylinder + start_cylinder) / 2);
					const MVector binormal((tangent ^ normal).normal());

					double transformationMatrix[][4] = {
							{ binormal.x, tangent.x, normal.x, center.x },
							{ binormal.y, tangent.y, normal.y, center.y },
							{ binormal.z, tangent.z, normal.z, center.z },
							{ 0, 0, 0, 1 }
					};

					Model::Helix helix;
					HMEVALUATE_RETURN(status = creator.create(DNA::DistanceToBaseCount(length), MMatrix(transformationMatrix).transpose(), helix), status);
					helices.push_back(helix);

					// Now recalculate start, end, normal, tangent, binormal, start_cylinder, end_cylinder for the upcoming cylinder if any.

					const std::vector<Edge>::const_iterator next_it(it + 1);
					if (next_it != m_edges.end()) {

						const MVector end_base(end_cylinder + tangent.rotateBy(MQuaternion(toRadians(DNA::HelixRotation(length)), normal)));

						HPRINT("end_base: %f %f %f", end_base.x, end_base.y, end_base.z);
						start = m_vertices[it->vertex];
						end = m_vertices[next_it->vertex];
						normal = (end - start).normal();
						start_cylinder = start + normal * it->cylinder[0];
						end_cylinder = start + normal * it->cylinder[1];
						length = (end_cylinder - start_cylinder).length();
						MVector error_vector(normal * ((length - DNA::HelixLength(length)) / 2));
						start_cylinder += error_vector;
						end_cylinder -= error_vector;

						// TODO: In case we have an angle, cleanup the code above
						if (next_it->hasAngle)
							tangent = MVector((normal ^ MVector::yAxis ^ normal).normal().rotateBy(MQuaternion(toRadians(next_it->angle), normal)));
						else {
							const MVector delta(end_base - start_cylinder);
							tangent = (normal ^ (delta ^ normal)).normal();
						}
					}
				}
			}

			// Connect scaffold
			std::vector<Model::Helix>::iterator it = helices.begin();
			std::vector<Model::Helix>::iterator prev_it = it++;

			for (; it != helices.end(); ++it, ++prev_it) {
				Model::Base forward_threeprime, forward_fiveprime;
				HMEVALUATE_RETURN(status = prev_it->getForwardThreePrime(forward_threeprime), status);
				HMEVALUATE_RETURN(status = it->getForwardFivePrime(forward_fiveprime), status);

				HMEVALUATE_RETURN(status = forward_threeprime.connect_forward(forward_fiveprime, true), status);
			}

			if (helices.size() > 1) {
				// Paint the scaffold.
				Controller::PaintMultipleStrandsWithNewColorFunctor functor;
				HMEVALUATE_RETURN(status = functor.loadMaterials(), status);
				Model::Base fiveprime;
				HMEVALUATE_RETURN(status = helices.begin()->getForwardFivePrime(fiveprime), status);
				functor(fiveprime);
				HMEVALUATE_DESCRIPTION("Controller::PaintMultipleStrandsWithNewColorFunctor", functor.status());

				if (m_edges.begin()->vertex == (--m_edges.end())->vertex) {
					// Circular, connect the first and last bases as well.
					std::vector<Model::Helix>::iterator first = helices.begin();
					std::vector<Model::Helix>::iterator last = --helices.end();
					Model::Base threeprime, fiveprime;
					HMEVALUATE_RETURN(status = last->getForwardThreePrime(threeprime), status);
					HMEVALUATE_RETURN(status = first->getForwardFivePrime(fiveprime), status);

					HMEVALUATE_RETURN(status = threeprime.connect_forward(fiveprime, true), status);
				}
			}

			return MStatus::kSuccess;
		}
	}
}
