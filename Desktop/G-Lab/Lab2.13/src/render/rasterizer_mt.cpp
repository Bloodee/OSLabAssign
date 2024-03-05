#include "rasterizer.h"

#include <array>
#include <limits>
#include <tuple>
#include <vector>
#include <algorithm>
#include <mutex>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include "triangle.h"
#include "../utils/math.hpp"

using Eigen::Matrix4f;
using Eigen::Vector2i;
using Eigen::Vector3f;
using Eigen::Vector4f;

void Rasterizer::draw_mt(const std::vector<Triangle>& TriangleList, const GL::Material& material,
                         const std::list<Light>& lights, const Camera& camera)
{
    std::vector<std::thread> threads; 

    auto process_triangle = [&](const Triangle& t) {
        Triangle newtriangle = t;
        std::array<Vector3f, 3> worldspace_pos;

        for (int i = 0; i < 3; i++) {
            Vector4f temp     = model * newtriangle.vertex[i];
            worldspace_pos[i] = {temp.x(), temp.y(), temp.z()};
            worldspace_pos[i].x() /= temp.w();
            worldspace_pos[i].y() /= temp.w();
            worldspace_pos[i].z() /= temp.w();
        }

        for (int i = 0; i < 3; i++) {
            VertexShaderPayload ver = {t.vertex[i], t.normal[i]};
            ver                     = vertex_shader(ver);
            newtriangle.vertex[i]   = ver.position;
            newtriangle.normal[i]   = ver.normal;
        }

        rasterize_triangle(newtriangle, worldspace_pos, material, lights, camera);
    };

    for (const auto& t : TriangleList) {
        threads.emplace_back(process_triangle, t);
    }


    for (auto& thread : threads) {
        thread.join();
    }
}




// Screen space rasterization
void Rasterizer::rasterize_triangle_mt(const Triangle& t, const std::array<Vector3f, 3>& world_pos,
                                       GL::Material material, const std::list<Light>& lights,
                                       Camera camera)
{
    // these lines below are just for compiling and can be deleted

    // discard all pixels out of the range(including x,y,z)
    std::array<Vector3f, 3> screen_pos;
    for (int i = 0; i < 3; i++) {
        screen_pos[i] = {t.vertex[i].x(), t.vertex[i].y(), t.vertex[i].z()};
    }
    int Xmin = std::min({screen_pos[0].x(), screen_pos[1].x(), screen_pos[2].x()});
    int Xmax = std::max({screen_pos[0].x(), screen_pos[1].x(), screen_pos[2].x()});
    int Ymin = std::min({screen_pos[0].y(), screen_pos[1].y(), screen_pos[2].y()});
    int Ymax = std::max({screen_pos[0].y(), screen_pos[1].y(), screen_pos[2].y()});

    for (int x = Xmin - 1; x <= Xmax + 1; x++) {
        for (int y = Ymin - 1; y <= Ymax + 1; y++) {
            Vector2i p(x, y); // current pixel position

            // if current pixel is in current triangle:

            if (!inside_triangle(x, y, t.vertex)) {
                // pixel is outside the triangle
                continue;
            } 

            // use barycentric coordinates to determine if the pixel is inside the triangle
            std::tuple cog = compute_barycentric_2d(x, y, t.vertex);

            Vector3f Cog = {std::get<0>(cog), std::get<1>(cog), std::get<2>(cog)};
       

            // 1. interpolate depth(use projection correction algorithm)
            // use the inverse of clip space w coordinates to interpolate depth

            float z = 1 / (Cog.x() / t.vertex[0].w() + Cog.y() / t.vertex[1].w() +
                           Cog.z() / t.vertex[2].w());
       
            int index = get_index(x, y);
         

      
            if (z < depth_buf[index]) {
                depth_buf[index] = z;
             

                Vector3f weight = {t.vertex[0].w(), t.vertex[1].w(), t.vertex[2].w()};
            

                //  2. interpolate vertex positon & normal(use function:interpolate())
                //  use the world space position and normal from the triangle
                Vector3f World_pos = interpolate(Cog.x(), Cog.y(), Cog.z(), world_pos[0],
                                                 world_pos[1], world_pos[2], weight, z);
                ;
             
                Vector3f World_nor = interpolate(Cog.x(), Cog.y(), Cog.z(), t.normal[0],
                                                 t.normal[1], t.normal[2], weight, z);
               

                // 3. fragment shading(use function:fragment_shader())
                // use the material, lights and camera to compute the fragment color
                FragmentShaderPayload payload(World_pos, World_nor);
                Vector3f res = phong_fragment_shader(payload, material, lights, camera);
            

                // 4. set pixel
                set_pixel(Vector2i(x, y), res);
           
            }
        }
    }
}
