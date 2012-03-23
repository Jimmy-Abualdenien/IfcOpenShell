/********************************************************************************
 *                                                                              *
 * This file is part of IfcOpenShell.                                           *
 *                                                                              *
 * IfcOpenShell is free software: you can redistribute it and/or modify         *
 * it under the terms of the Lesser GNU General Public License as published by  *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * IfcOpenShell is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * Lesser GNU General Public License for more details.                          *
 *                                                                              *
 * You should have received a copy of the Lesser GNU General Public License     *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

/********************************************************************************
 *                                                                              *
 * This is a brief example of how IfcOpenShell can be interfaced from within    *
 * a C++ context. The application reads an .ifc file and outputs geometry in    *
 * the Wavefront .obj file format.                                              *
 *                                                                              *
 ********************************************************************************/

#include <fstream>
#include <sstream>
#include <set>
#include <time.h>

#include "../ifcgeom/IfcGeomObjects.h"
#include "../ifcobj/ObjMaterials.h"

int main ( int argc, char** argv ) {
	if ( argc != 2 ) {
		std::cout << "usage: IfcObj <filename.ifc>" << std::endl;
		return 1;
	}
	const std::string fnObj = std::string(argv[1]) + ".obj";
	const std::string fnMtl = std::string(argv[1]) + ".mtl";
	ofstream fObj(fnObj.c_str());
	ofstream fMtl(fnMtl.c_str());
	if ( ! ( fObj.is_open() && fMtl.is_open() ) ) {
		std::cout << "[Error] unable to open output file for writing" << std::endl;
		return 1;
	}

	IfcGeomObjects::Settings(IfcGeomObjects::USE_WORLD_COORDS,true);
	IfcGeomObjects::Settings(IfcGeomObjects::WELD_VERTICES,false);
	IfcGeomObjects::Settings(IfcGeomObjects::SEW_SHELLS,true);

	// Stream for log messages, we don't want to interupt our new progress bar...
	std::stringstream ss;

	// Parse the file supplied in argv[1]. Returns true on succes.
	if ( ! IfcGeomObjects::Init(argv[1],&std::cout,&ss) ) {
		std::cout << "[Error] unable to parse .ifc file or no geometrical entities found" << std::endl;
		return 1;
	}

	fObj << "# File generated by IfcOpenShell " << IFCOPENSHELL_VERSION << std::endl;
	fObj << "mtllib " << fnMtl << std::endl;
	std::set<std::string> materials;

	time_t start,end;
	time(&start);
	int old_progress = -1;
	std::cout << "Creating geometry..." << std::endl;

	// The functions IfcGeomObjects::Get() and IfcGeomObjects::Next() wrap an iterator of all geometrical entities in the Ifc file.
	// IfcGeomObjects::Get() returns an IfcGeomObjects::IfcGeomObject (see IfcObjects.h for definition)
	// IfcGeomObjects::Next() is used to poll whether more geometrical entities are available
	do {
		const IfcGeomObjects::IfcGeomObject* o = IfcGeomObjects::Get();
		if ( o->type == "IfcSpace" || o->type == "IfcOpeningElement" ) continue;
		const std::string name = o->name.empty() ? o->guid : o->name;
		fObj << "g " << name << std::endl;
		fObj << "s 1" << std::endl;
		fObj << "usemtl " << o->type << std::endl;
		materials.insert(o->type);
		const int vcount = o->mesh->verts.size() / 3;
		for ( IfcGeomObjects::FltIt it = o->mesh->verts.begin(); it != o->mesh->verts.end(); ) {
			const double x = *(it++);
			const double y = *(it++);
			const double z = *(it++);
			fObj << "v " << x << " " << y << " " << z << std::endl;
		}
		for ( IfcGeomObjects::FltIt it = o->mesh->normals.begin(); it != o->mesh->normals.end(); ) {
			const double x = *(it++);
			const double y = *(it++);
			const double z = *(it++);
			fObj << "vn " << x << " " << y << " " << z << std::endl;
		}
		for ( IfcGeomObjects::IntIt it = o->mesh->faces.begin(); it != o->mesh->faces.end(); ) {
			const int v1 = *(it++)-vcount;
			const int v2 = *(it++)-vcount;
			const int v3 = *(it++)-vcount;
			fObj << "f " << v1 << "//" << v1 << " " << v2 << "//" << v2 << " " << v3 << "//" << v3 << std::endl;
		}

		const int progress = IfcGeomObjects::Progress() / 2;
		if ( old_progress!= progress ) std::cout << "\r[" << std::string(progress,'#') << std::string(50 - progress,' ') << "]" << std::flush;
		old_progress = progress;
	
	} while ( IfcGeomObjects::Next() );
	std::cout << "\rDone creating geometry                                " << std::endl;

	// Writes the material settings, defined in Materials.h
	fMtl << "# File generated by IfcOpenShell " << IFCOPENSHELL_VERSION << std::endl;
	for( std::set<std::string>::iterator it = materials.begin(); it != materials.end(); ++ it ) {
		fMtl << GetMaterial(*it);
	}

	std::string log = ss.str();
	if ( log.size() ) {
		std::cout << std::endl << "Log:" << std::endl;
		std::cout << ss.str();
	}

	time(&end);
	int dif = (int) difftime (end,start);	
	printf ("\nConversion took %d seconds\n", dif );
}
