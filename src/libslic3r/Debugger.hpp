#ifndef slic3r_Debugger_hpp_
#define slic3r_Debugger_hpp_

#include "libslic3r/Print.hpp"
#include "libslic3r/GCode/SeamPlacer.hpp"

namespace Slic3r {

	class Debugger
	{
	public:
		virtual ~Debugger() {}

		virtual void volume_slices(Print* print, PrintObject* object, const std::vector<VolumeSlices>& slices, const std::vector<float>& slice_zs) = 0;
		virtual void raw_lslices(Print* print, PrintObject* object) = 0;

		virtual void begin_concial_overhang(Print* print, PrintObject* object) = 0;
		virtual void end_concial_overhang(Print* print, PrintObject* object) = 0;

		virtual void begin_hole_to_polyhole(Print* print, PrintObject* object) = 0;
		virtual void end_hole_to_polyhole(Print* print, PrintObject* object) = 0;

		virtual void perimeters(Print* print, PrintObject* object) = 0;
		virtual void curled_extrusions(Print* print, PrintObject* object) = 0;

		virtual void surfaces_type(Print* print, PrintObject* object) = 0;
		virtual void infill_surfaces_type(Print* print, PrintObject* object, int step) = 0;

		virtual void infills(Print* print, PrintObject* object) = 0;

		virtual void seamplacer(Print* print, SeamPlacer* placer) = 0;
		virtual void gcode(const std::string& name, int layer, bool by_object, const std::string& gcode) = 0;
	};

	/*******************using immediate window for debug****************************/
	////////////////////output 2D shapes to svg ////////////////////////////

	void to_svg(const char* path, const Polygons& polys0, const Polygons& polys1, const Polygons& polys2);

	//------------------------------------------------------------------------------
	void to_svg(const char* path, const Polygon& polygon);
	void to_svg(const char* path, const Polygon& poly0, const Polygon& poly1);
	void to_svg(const char* path, const Polygon& poly, const Polylines& polylines);
	void to_svg(const char* path, const Polygon& poly, const BoundingBox& bbox);
	void to_svg(const char* path, const Polygons& polygons);
	void to_svg(const char* path, const Polygons& polys0, const Polygons& polys1);
	void to_svg(const char* path, const Polygons& polys, const Polylines& polylines);
	void to_svg(const char* path, const Polygons& polys, const BoundingBox& bbox);
	void to_svg(const char* path, const Polygons& polys, const Point& pt);
	void to_svg(const char* path, const Polygons& polys, const Point& pt0, const Point& pt1);
	void to_svg(const char* path, const Polygons& polys, const Point& pt0, const Point& pt1, const Point& pt2);

	void to_svg(const char* dir, const std::vector<Polygons>& polyss);

	//------------------------------------------------------------------------------
	void to_svg(const char* path, const ExPolygon& expolygon);
	void to_svg(const char* path, const ExPolygon& expoly0, const ExPolygon& expoly1);
	void to_svg(const char* path, const ExPolygon& expoly, const ExPolygons& expolys1);
	void to_svg(const char* path, const ExPolygons& expolygons);
	void to_svg(const char* path, const ExPolygons& expolys0, const ExPolygons& expolys1);
	void to_svg(const char* path, const ExPolygons& expolys0, const ExPolygon& expoly);

	void to_svg(const char* dir, const std::vector<ExPolygons>& expolyss);

	//------------------------------------------------------------------------------
	void to_svg(const char* path, const Polyline& polyline);
	void to_svg(const char* path, const Polyline& polyline0, const Polyline& polyline1);
	void to_svg(const char* path, const Polylines& polylines);
	void to_svg(const char* dir, const std::vector<Polylines>& polyliness);

	//------------------------------------------------------------------------------
	void to_svg(const char* dir, const ClipperLib_Z::Paths& paths0, const ClipperLib_Z::Paths& paths1);

	//------------------------------------------------------------------------------
	void to_svg(const char* file, const ClipperLib::Paths &paths);
	

	////////////////////output 3D shapes to obj ////////////////////////////
	void to_obj(const char* path, const indexed_triangle_set& its);
	void to_obj(const char* path, const std::vector<indexed_triangle_set>& itss);
	void to_obj(const char* dir, const std::vector<std::vector<indexed_triangle_set>>& itsss);

	void to_obj(const char* path, const TriangleMesh& tm);
	void to_obj(const char* path, const std::vector<TriangleMesh>& tms);
	void to_obj(const char* dir, const std::vector<std::vector<TriangleMesh>>& tmss, bool sperate_file = false);

	void export_fill_surfaces(PrintObject* po, const char* func_name, int counter);

	/*******************using immediate window for debug****************************/
}

#endif
