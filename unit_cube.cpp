#include <ringmesh/geomodel/core/geomodel.h>
#include <ringmesh/geomodel/core/geomodel_mesh_entity.h>
#include <ringmesh/geomodel/core/geomodel_geological_entity.h>
#include <ringmesh/geomodel/core/geomodel_mesh.h>

#include <ringmesh/geomodel/builder/geomodel_builder.h>

#include <ringmesh/geomodel/tools/geomodel_repair.h>

#include <ringmesh/io/io.h>

#include <ringmesh/basic/algorithm.h>
#include <ringmesh/basic/command_line.h>

#include <geogram/basic/command_line.h>

using RINGMesh::index_t;
using RINGMesh::vec3;
using RINGMesh::range;

void parse_command_line( int argc, char** argv ) {
    std::vector< std::string > filenames;
    if( !GEO::CmdLine::parse( argc, argv, filenames ) ) {
        throw std::runtime_error( "Cannot parse the arguments" );
    }
}
void init_commands(){
    GEO::CmdLine::declare_arg("nb_hexs", 8 , "Number of hexs (e.g. if you enter 10, you will have a cube meshed with 10x10x10 hexs");
    RINGMesh::CmdLine::import_arg_group( "out" );
}

int main( int argc, char** argv ) {
    init_commands();
    parse_command_line( argc, argv );

    RINGMesh::GeoModel3D geomodel;
    RINGMesh::GeoModelBuilder3D gm_builder( geomodel );

    // Number of Mesh Entities
    index_t nb_corners = 8;
    index_t nb_lines = 12;
    index_t nb_surfaces = 6;
    index_t nb_regions = 1;

    // Number of Geological Entities
    index_t nb_contacts = nb_lines;
    index_t nb_interfaces = nb_surfaces;
    index_t nb_layers = nb_regions;


    // Setting Mesh Entities
    gm_builder.topology.create_mesh_entities( RINGMesh::Corner3D::type_name_static(), nb_corners );
    gm_builder.topology.create_mesh_entities( RINGMesh::Line3D::type_name_static(), nb_lines );
    gm_builder.topology.create_mesh_entities( RINGMesh::Surface3D::type_name_static(), nb_surfaces );
    gm_builder.topology.create_mesh_entities( RINGMesh::Region3D::type_name_static(), nb_regions );

    // Get the number of hexs
    index_t nb_hexs = GEO::CmdLine::get_arg_uint( "nb_hexs" );
    double mesh_size = 1. / nb_hexs ;
    index_t nb_points = nb_hexs + 1;

    // Setting the Geomety ...
    //
    // ... of the corners
    std::vector< vec3 > corners( nb_corners );
    corners[0] = ( vec3( 0., 0., 0. ) );
    corners[1] = ( vec3( 1., 0., 0. ) );
    corners[2] = ( vec3( 1., 1., 0. ) );
    corners[3] = ( vec3( 0., 1., 0. ) );
    corners[4] = ( vec3( 0., 0., 1. ) );
    corners[5] = ( vec3( 1., 0., 1. ) );
    corners[6] = ( vec3( 1., 1., 1. ) );
    corners[7] = ( vec3( 0., 1., 1. ) );
    for( auto corner_index : range ( corners.size() ) ) {
        gm_builder.geometry.set_corner( corner_index, corners[corner_index] );
    }

    // ... of the lines
    std::vector< vec3 > cur_coor_lines( nb_points );
    std::vector< index_t > starting_corners( 4 ); // Will contains the indexes of starting corners
    starting_corners[0] = 0;
    starting_corners[1] = 2;
    starting_corners[2] = 5;
    starting_corners[3] = 7;
    index_t line_index = 0;
    for( auto i : range( starting_corners.size() ) ) {
        for (auto dir : range( 3 ) ) {
            std::vector< vec3 > line_vertices( nb_points );
            line_vertices[0] = corners[starting_corners[i]];
            double mult_dir = 1;
            if( line_vertices[0][dir] == 1 ){
                mult_dir = -1.; 
            }
            for( auto local_point_index : range(1, nb_points ) ) {
                line_vertices[local_point_index] = 
                    line_vertices[local_point_index - 1];
                line_vertices[local_point_index][dir] += mult_dir * mesh_size;
            }
            gm_builder.geometry.set_line( line_index++, line_vertices );
        }
    }

    // ... of the surfaces
    std::vector< vec3 > surface_vertices( nb_points * nb_points );
    std::vector< index_t > quads( nb_hexs * nb_hexs * 4 );
    std::vector< index_t > quads_ptr( nb_hexs * nb_hexs +1 );
    for( auto i : range( nb_points ) ) {
        for( auto j : range( nb_points ) ) {
            surface_vertices[nb_points*i + j] = corners[0];
            surface_vertices[nb_points*i + j].x += j * mesh_size;
            surface_vertices[nb_points*i + j].z += i * mesh_size;
        }
    }
    index_t quad_id = 0;
    for( auto i : range( nb_hexs ) ) {
        for( auto j : range( nb_hexs ) ) {
            quads[4*quad_id + 0] = nb_points*i + j;
            quads[4*quad_id + 1] = nb_points*i + j + 1;
            quads[4*quad_id + 2] = nb_points * ( i + 1 ) + 1 + j;
            quads[4*quad_id + 3] = nb_points * ( i + 1 ) + j;
            quad_id++;
        }
    }
    for( auto i : range( 0, nb_hexs*nb_hexs + 1 ) ) {
        quads_ptr[i] = i*4;
    }
    gm_builder.geometry.set_surface_geometry( 0, surface_vertices, quads, quads_ptr );
    for( auto i : range( nb_points * nb_points ) ) {
        surface_vertices[i].y +=1;
    }
    gm_builder.geometry.set_surface_geometry( 1, surface_vertices, quads, quads_ptr );
    for( auto i : range( nb_points * nb_points ) ) {
        vec3 vertex = surface_vertices[i];
        surface_vertices[i].x = vertex.y;
        surface_vertices[i].y = vertex.x;
    }
    gm_builder.geometry.set_surface_geometry( 2, surface_vertices, quads, quads_ptr );
    for( auto i : range( nb_points * nb_points ) ) {
        surface_vertices[i].x -=1;
    }
    gm_builder.geometry.set_surface_geometry( 3, surface_vertices, quads, quads_ptr );
    for( auto i : range( nb_points * nb_points ) ) {
        vec3 vertex = surface_vertices[i];
        surface_vertices[i].x = vertex.z;
        surface_vertices[i].z = vertex.x;
    }
    gm_builder.geometry.set_surface_geometry( 4, surface_vertices, quads, quads_ptr );
    for( auto i : range( nb_points * nb_points ) ) {
        surface_vertices[i].z +=1;
    }
    gm_builder.geometry.set_surface_geometry( 5, surface_vertices, quads, quads_ptr );

    // ... of the region
    std::vector< vec3 > region_vertices( nb_points * nb_points * nb_points );
    for( auto i : range( nb_points ) ) {
        for( auto j : range( nb_points ) ) {
            for( auto k : range( nb_points ) ) {
                region_vertices[nb_points*nb_points*i + nb_points*j +k] = corners[0];
                region_vertices[nb_points*nb_points*i + nb_points*j +k].x += j * mesh_size;
                region_vertices[nb_points*nb_points*i + nb_points*j +k].y += k * mesh_size;
                region_vertices[nb_points*nb_points*i + nb_points*j +k].z += i * mesh_size;
            }
        }
    }
    gm_builder.geometry.set_mesh_entity_vertices( 
            geomodel.region( 0 ).gmme(), region_vertices, false);
    for( auto i : range( nb_hexs ) ) {
        for( auto j : range( nb_hexs ) ) {
            for( auto k : range( nb_hexs ) ) {
                std::vector< index_t > hex( 8 );
                hex[0] = nb_points*nb_points*i + nb_points*j + k;
                hex[1] = nb_points*nb_points*i + nb_points*j + k + 1;
                hex[2] = nb_points*nb_points*( i + 1 ) + nb_points*j + k;
                hex[3] = nb_points*nb_points*( i + 1 ) + nb_points*j + k + 1;
                hex[4] = nb_points*nb_points*i + nb_points*( j + 1 ) + k;
                hex[5] = nb_points*nb_points*i + nb_points*( j + 1 ) + k + 1;
                hex[6] = nb_points*nb_points*( i + 1 ) + nb_points*( j + 1 ) + k;
                hex[7] = nb_points*nb_points*( i + 1 ) + nb_points*( j + 1 ) + k + 1;
                gm_builder.geometry.create_region_cell(0, RINGMesh::CellType::HEXAHEDRON, hex );
            }
        }
    }
    gm_builder.geometry.compute_region_adjacencies( 0, false);

    // Setting the Boundary relations ...
    // ... Lines and Corners
    for( auto corner_index : range( geomodel.nb_corners() ) ) {
        index_t vertex_id_in_gm = geomodel.mesh.vertices.geomodel_vertex_id(
                geomodel.corner( corner_index ).gmme() );
        auto gmmv = geomodel.mesh.vertices.gme_type_vertices(
                RINGMesh::Line3D::type_name_static(), vertex_id_in_gm );
        for( auto lines_gmmv : range ( gmmv.size() ) ) {
            gm_builder.topology.add_line_corner_boundary_relation( gmmv[lines_gmmv].gmme.index(),
                    corner_index);
        }
    }

    // .. Surfaces and Lines
    for( auto line_index : range( geomodel.nb_lines() ) ) {
        index_t corner_index_0 = geomodel.line( line_index ).boundary( 0 ).index();
        index_t corner_index_1 = geomodel.line( line_index ).boundary( 1 ).index();
        index_t corner_vertex_index_in_gm_0 = geomodel.mesh.vertices.geomodel_vertex_id(
                geomodel.corner( corner_index_0 ).gmme() );
        index_t corner_vertex_index_in_gm_1 = geomodel.mesh.vertices.geomodel_vertex_id(
                geomodel.corner( corner_index_1 ).gmme() );
        auto gmmv_0 = geomodel.mesh.vertices.gme_type_vertices(
                RINGMesh::Surface3D::type_name_static(), corner_vertex_index_in_gm_0 );
        auto gmmv_1 = geomodel.mesh.vertices.gme_type_vertices(
                RINGMesh::Surface3D::type_name_static(), corner_vertex_index_in_gm_1 );
        for( auto surface_gmmv_0 : range ( gmmv_0.size() ) ) {
            for( auto surface_gmmv_1 : range ( gmmv_1.size() ) ) {
                if( gmmv_0[surface_gmmv_0].gmme == gmmv_1[surface_gmmv_1].gmme ) {
                    gm_builder.topology.add_surface_line_boundary_relation(
                            gmmv_0[surface_gmmv_0].gmme.index(), line_index );
                }
            }
        }
    }

    // ... Regions and Surfaces
    for( auto surface_index : range( geomodel.nb_surfaces() ) ) {
        gm_builder.topology.add_region_surface_boundary_relation( 0, surface_index, false );
    }

    // Setting the Geological Entitities
    gm_builder.geology.create_geological_entities(
            RINGMesh::Contact3D::type_name_static(), nb_contacts );
    gm_builder.geology.create_geological_entities(
            RINGMesh::Interface3D::type_name_static(), nb_interfaces );
    gm_builder.geology.create_geological_entities(
            RINGMesh::Layer3D::type_name_static(), nb_layers );

    // Setting the child / parent relation
    
    // ... between lines and contacts
    for( auto line_index : range( nb_lines ) ) {
        gm_builder.geology.add_parent_children_relation( geomodel.geological_entity(
                    RINGMesh::Contact3D::type_name_static(), line_index).gmge(),
                geomodel.line( line_index ).gmme() );
    }
    
    // ... between surfaces and interfaces
    for( auto surface_index : range( nb_surfaces ) ) {
        gm_builder.geology.add_parent_children_relation( geomodel.geological_entity(
                    RINGMesh::Interface3D::type_name_static(), surface_index).gmge(),
                geomodel.surface( surface_index ).gmme() );
    }
    
    // ... between regions and layers
    for( auto region_index : range( nb_regions ) ) {
        gm_builder.geology.add_parent_children_relation( geomodel.geological_entity(
                    RINGMesh::Layer3D::type_name_static(), region_index).gmge(),
                geomodel.region( region_index ).gmme() );
    }

    RINGMesh::repair_geomodel( geomodel, RINGMesh::RepairMode::LINE_BOUNDARY_ORDER );

    gm_builder.end_geomodel();
    RINGMesh::geomodel_save( geomodel, GEO::CmdLine::get_arg("out:geomodel") );

    return 0;
};

