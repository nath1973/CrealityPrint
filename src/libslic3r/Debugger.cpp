#include "Debugger.hpp"
#include "Layer.hpp"
#include "ClipperZUtils.hpp"

namespace Slic3r {
	void bench_debug_volume_slices(Print* print, PrintObject* object, const std::vector<VolumeSlices>& slices, const std::vector<float>& slice_zs)
	{
#ifdef BENCH_DEBUG
		print->debugger->volume_slices(print, object, slices, slice_zs);
#endif
	}

	void bench_debug_lslices(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->raw_lslices(print, object);
#endif
	}

	void begin_debug_concial_overhang(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->begin_concial_overhang(print, object);
#endif
	}

	void end_debug_concial_overhang(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->end_concial_overhang(print, object);
#endif
	}

	void begin_debug_hole_to_polyhole(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->begin_hole_to_polyhole(print, object);
#endif
	}

	void end_debug_hole_to_polyhole(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->end_hole_to_polyhole(print, object);
#endif
	}

	void debug_perimeters(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->perimeters(print, object);
#endif
	}

	void debug_estimate_curled_extrusions(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->curled_extrusions(print, object);
#endif
	}

	void debug_surfaces_type(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->surfaces_type(print, object);
#endif
	}

	void debug_infill_surfaces_type(Print* print, PrintObject* object, int step)
	{
#ifdef BENCH_DEBUG
		print->debugger->infill_surfaces_type(print, object, step);
#endif
	}

	void debug_infills(Print* print, PrintObject* object)
	{
#ifdef BENCH_DEBUG
		print->debugger->infills(print, object);
#endif
	}

	void bench_debug_generate(Print* print, int layer, const std::string& code, bool by_object)
	{
#ifdef BENCH_DEBUG
		print->debugger->gcode("generate", layer, by_object, code);
#endif
	}

	void bench_debug_cooling(Print* print, int layer, const std::string& code, bool by_object)
	{
#ifdef BENCH_DEBUG
		print->debugger->gcode("cooling", layer, by_object, code);
#endif
	}

	void bench_debug_fanmove(Print* print, int layer, const std::string& code, bool by_object)
	{
#ifdef BENCH_DEBUG
		print->debugger->gcode("fanmove", layer, by_object, code);
#endif
	}

	void bench_debug_output(Print* print, int layer, const std::string& code, bool by_object)
	{
#ifdef BENCH_DEBUG
		print->debugger->gcode("output", layer, by_object, code);
#endif
	}

	void bench_debug_seamplacer(Print* print, SeamPlacer* placer)
	{
#ifdef BENCH_DEBUG
		print->debugger->seamplacer(print, placer);
#endif
	}

	/*******************using immediate window for debug****************************/

    ////////////////////using immediate window for debug////////////////////////////
    void to_svg(const char* path, const Polygons& polys0, const Polygons& polys1, const Polygons& polys2)
    {
        BoundingBox bb = get_extents(polys0);
        bb.merge(get_extents(polys1));
        bb.merge(get_extents(polys2));

        SVG svg(path, bb);
        svg.draw_outline(polys0, "red", 10000);
        svg.draw_outline(polys1, "green", 10000);
        svg.draw_outline(polys2, "blue", 10000);
        svg.Close();
    }

    void to_svg(const char* path, const ExPolygon& expoly0, const ExPolygon& expoly1)
    {
        Polygons polygons_0 = to_polygons(expoly0);
        Polygons polygons_1 = to_polygons(expoly1);

        to_svg(path, polygons_0, polygons_1);
    }

    void to_svg(const char* path, const ExPolygon& expolygon)
    {
        Polygons polygons = to_polygons(expolygon);
        to_svg(path, polygons);
    }

    void to_svg(const char* path, const ExPolygon& expoly, const ExPolygons& expolys1)
    {
        Polygons polygons_0 = to_polygons(expoly);
        Polygons polygons_1 = to_polygons(expolys1);

        to_svg(path, polygons_0, polygons_1);
    }

    void to_svg(const char* path, const ExPolygons& expolys0, const ExPolygons& expolys1)
    {
        Polygons polygons_0 = to_polygons(expolys0);
        Polygons polygons_1 = to_polygons(expolys1);

        to_svg(path, polygons_0, polygons_1);
    }

    void to_svg(const char* path, const ExPolygons& expolys0, const ExPolygon& expoly)
    {
        Polygons polygons_0 = to_polygons(expolys0);
        Polygons polygons_1 = to_polygons(expoly);

        to_svg(path, polygons_0, polygons_1);
    }

    void to_svg(const char* path, const ExPolygons& expolygons)
    {
        Polygons polygons = to_polygons(expolygons);
        to_svg(path, polygons);
    }

	void to_svg(const char* dir, const std::vector<ExPolygons>& expolyss)
	{
		std::string file = std::string(dir) + "\\test";

		for (int i = 0; i < expolyss.size(); i++)
		{
			if (expolyss[i].empty())
			{
				continue;
			}

			std::string tem_file = file + std::to_string(i) + ".svg";

			to_svg(tem_file.c_str(), expolyss[i]);
		}
	}

	//------------------------------------------------------------------------------
	void to_svg(const char* path, const Polygon& poly0, const Polygon& poly1)
	{
		std::string stroke_str0 = "blue";
		std::string stroke_str1 = "green";

		BoundingBox bb = get_extents(poly0);
		bb.merge(get_extents(poly1));

		SVG svg(path, bb);
		svg.draw_outline(poly0, stroke_str0, 10000);
		svg.draw_outline(poly1, stroke_str1, 10000);
		svg.Close();
	}

	void to_svg(const char* path, const Polygon& poly, const Polylines& polylines)
	{

	}

	void to_svg(const char* path, const Polygon& poly, const BoundingBox& bbox)
	{
		BoundingBox bb = get_extents(poly);
		bb.merge(bbox);

		SVG svg(path, bb);
		svg.draw_outline(poly, "red", 10000);
		svg.draw_outline(bbox.polygon(), "green", 1000);
		svg.Close();
	}

	void to_svg(const char* path, const Polygon& polygon)
	{
		SVG svg(path, get_extents(polygon));
		svg.draw_outline(polygon, "red", 10000);
		svg.Close();
	}

	void to_svg(const char* path, const Polygons& polygons)
	{
		SVG svg(path, get_extents(polygons));
		svg.draw_outline(polygons, "red", 10000);
		svg.Close();
	}

	void to_svg(const char* dir, const std::vector<Polygons>& polyss)
	{
		std::string file = std::string(dir) + "\\test";

		for (int i = 0; i < polyss.size(); i++)
		{
			if (polyss[i].empty())
			{
				continue;
			}

			std::string tem_file = file + std::to_string(i) + ".svg";

			to_svg(tem_file.c_str(), polyss[i]);
		}
	}

	void to_svg(const char* path, const Polygons& polys0, const Polygons& polys1)
	{
		std::string stroke_str0 = "red";
		std::string stroke_str1 = "green";

		BoundingBox bb = get_extents(polys0);
		bb.merge(get_extents(polys1));

		SVG svg(path, bb);
		svg.draw_outline(polys0, stroke_str0, 10000);
		svg.draw_outline(polys1, stroke_str1, 10000);
		svg.Close();
	}

	void to_svg(const char* path, const Polygons& polys, const Polylines& polylines)
	{
		std::string stroke_str0 = "red";
		std::string stroke_str1 = "green";

		BoundingBox bb = get_extents(polylines);
		bb.merge(get_extents(polys));

		SVG svg(path, bb);
		svg.draw_outline(polys, stroke_str0, 10000);
		for (auto& line : polylines)
		{
			for (auto& point : line)
			{
				svg.draw(point, stroke_str1, 10000);
			}
		}

		svg.Close();
	}

	void to_svg(const char* path, const Polygons& polys, const BoundingBox& bbox)
	{
		BoundingBox bb = get_extents(polys);

		bb.merge(bbox);

		SVG svg(path, bb);
		svg.draw_outline(polys, "red", 10000);
		svg.draw_outline(bbox.polygon(), "green", 10000);
		svg.Close();
	}

	void to_svg(const char* path, const Polygons& polys, const Point& pt)
	{
		BoundingBox bb = get_extents(polys);

		bb.merge(pt);

		SVG svg(path, bb);
		svg.draw_outline(polys, "black", 10000);
		svg.draw(pt, "red", 10000);
		svg.Close();
	}
	void to_svg(const char* path, const Polygons& polys, const Point& pt0, const Point& pt1)
	{
		BoundingBox bb = get_extents(polys);

		bb.merge(pt0);
		bb.merge(pt1);

		SVG svg(path, bb);
		svg.draw_outline(polys, "black", 10000);
		svg.draw(pt0, "red", 10000);
		svg.draw(pt1, "green", 10000);
		svg.Close();
	}

	void to_svg(const char* path, const Polygons& polys, const Point& pt0, const Point& pt1, const Point& pt2)
	{
		BoundingBox bb = get_extents(polys);

		bb.merge(pt0);
		bb.merge(pt1);
		bb.merge(pt2);

		SVG svg(path, bb);
		svg.draw_outline(polys, "black", 10000);
		svg.draw(pt0, "red", 10000);
		svg.draw(pt1, "green", 10000);
		svg.draw(pt2, "blue", 10000);
		svg.Close();
	}

	//-------------------------------------------------------------------------------
	void to_svg(const char* path, const Polyline& polyline)
	{
		SVG svg(path, get_extents(polyline));
		svg.draw(polyline, "red", 10000);
		svg.Close();
	}

	void to_svg(const char* path, const Polyline& polyline0, const Polyline& polyline1)
	{
		BoundingBox bb = get_extents(polyline0);
		bb.merge(get_extents(polyline1));

		SVG svg(path, bb);
		svg.draw(polyline0, "red", 10000);
		svg.draw(polyline1, "green", 10000);
		svg.Close();
	}

	void to_svg(const char* path, const Polylines& polylines)
	{
		SVG svg(path, get_extents(polylines));
		svg.draw(polylines, "red", 10000);
		svg.Close();
	}

	void to_svg(const char* dir, const std::vector<Polylines>& polyliness)
	{
		std::string file = std::string(dir) + "\\test";

		for (int i = 0; i < polyliness.size(); i++)
		{
			if (polyliness[i].empty())
			{
				continue;
			}

			std::string tem_file = file + std::to_string(i) + ".svg";

			to_svg(tem_file.c_str(), polyliness[i]);
		}
	}

	//-------------------------------------------------------------------------------
	void to_svg(const char* dir, const ClipperLib_Z::Paths& paths0, const ClipperLib_Z::Paths& paths1)
	{
        VecOfPoints vec_of_points_0 = ClipperZUtils::from_zpaths(paths0);
        Polygons polygons_0 = to_polygons(vec_of_points_0);

        VecOfPoints vec_of_points_1 = ClipperZUtils::from_zpaths(paths1);
        Polygons polygons_1 = to_polygons(vec_of_points_1);

        to_svg(dir, polygons_0, polygons_1);
	}

	//-------------------------------------------------------------------------------
	void to_svg(const char* file, const ClipperLib::Paths& paths)
	{
		 Polygons polygons = to_polygons(paths);
		 to_svg(file, polygons);
	}

	////////////////////output 3D shapes to obj ////////////////////////////
	void to_obj(const char* path, const indexed_triangle_set& its)
	{
		its_write_obj(its, path);
	}

	void to_obj(const char* path, const std::vector<indexed_triangle_set>& itss)
	{
		if (itss.empty())
		{
			return;
		}

		indexed_triangle_set its = itss[0];

		for (int i = 1; i < itss.size(); i++)
		{
			its_merge(its, itss[i]);
		}

		to_obj(path, its);
	}

	//one file for std::vector<indexed_triangle_set>
	void to_obj(const char* dir, const std::vector<std::vector<indexed_triangle_set>>& itsss, bool sperate_file/* = false*/)
	{
		if (itsss.empty())
		{
			return;
		}

		//put into one file
		if (sperate_file == false)
		{
			std::string file = std::string(dir) + "\\test0.obj";

			indexed_triangle_set its_temp;
			for (int i = 0; i < itsss.size(); i++)
			{
				for (int j = 0; j < itsss[i].size(); j++)
				{
					its_merge(its_temp, itsss[i][j]);
				}
			}

			if (!its_temp.empty())
			{
				to_obj(file.c_str(), its_temp);
			}
		}
		else
		{
			std::string file = std::string(dir) + "\\test";

			for (int i = 0; i < itsss.size(); i++)
			{
				if (itsss[i].empty())
				{
					continue;
				}

				std::string tem_file = file + std::to_string(i) + ".obj";

				to_obj(tem_file.c_str(), itsss[i]);
			}
		}
	}

	void to_obj(const char* path, const TriangleMesh& tm)
	{

	}

	void to_obj(const char* path, const std::vector<TriangleMesh>& tms)
	{

	}

	void to_obj(const char* dir, const std::vector<std::vector<TriangleMesh>>& tmss)
	{

	}

	////////////////////print slice data to file////////////////////////////
	void export_fill_surfaces(PrintObject* po, const char* func_name, int counter)
	{
		std::string fine_name = "D:\\svg_debug_files\\" + std::to_string(counter) + std::string(func_name) + "_cp.txt";
		std::ofstream fs(fine_name);

		for (int layer_idx = 0; layer_idx < po->layer_count(); layer_idx++)
		{
			Layer* layer = po->get_layer(layer_idx);
			fs << layer_idx << "\t" << "\t";

			for (int region_idx = 0; region_idx < layer->region_count(); region_idx++)
			{
				fs << "\t" << layer->get_region(region_idx)->fill_surfaces.size();
			}

			fs << std::endl;
		}
	}

	/*******************using immediate window for debug****************************/
}
