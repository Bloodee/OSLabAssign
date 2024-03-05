#include <array>
#include <limits>
#include <tuple>
#include <vector>
#include <algorithm>
#include <cmath>
#include <mutex>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <spdlog/spdlog.h>

#include "rasterizer.h"
#include "triangle.h"
#include "render_engine.h"
#include "../utils/math.hpp"



using Eigen::Matrix4f;
using Eigen::Vector2i;
using Eigen::Vector3f;
using Eigen::Vector4f;
using std::fill;
using std::tuple;

bool Rasterizer::inside_triangle(int x, int y, const Vector4f* vertices)
{
    Vector3f v[3];
    Vector3f f[3];

    for (int i = 0; i < 3; i++) {
        v[i] = {vertices[i].x(), vertices[i].y(), 1.0};
       
    }
    for (int i = 0; i < 3; i++) {
        f[i] = v[(i + 1) % 3].cross(v[i]).normalized(); 
    }

    Vector3f p(x, y, 1.);

    for (int i = 0; i < 3; i++) {
        if (p.dot(f[i]) * f[i].dot(v[(i+2)%3]) <= 0) {
            return false;
        }
    }

    return true;
}



tuple<float, float, float> Rasterizer::compute_barycentric_2d(float x, float y, const Vector4f* v)
{
   
    float x1 = v[0].x();
    float x2 = v[1].x();
    float x3 = v[2].x();
    float y1 = v[0].y();
    float y2 = v[1].y();
    float y3 = v[2].y();

  
    float C = (y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3);
    float c1    = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / C;
    float c2    = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / C;
    float c3    = 1.0f - c1 - c2;

    return {c1, c2, c3};
}


void Rasterizer::draw(const std::vector<Triangle>& TriangleList, const GL::Material& material,
                      const std::list<Light>& lights, const Camera& camera)
{
    // these lines below are just for compiling and can be deleted

    // iterate over all triangles in TriangleList
    for (const auto& t : TriangleList) {
        Triangle newtriangle = t;

        // transform vertex position to world space for interpolating

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
            ver = vertex_shader(ver);
            newtriangle.vertex[i] = ver.position;
            newtriangle.normal[i] = ver.normal;
        }
        
        // call rasterize_triangle()
        rasterize_triangle(newtriangle, worldspace_pos, material, lights, camera);
    }
    
}



Vector3f Rasterizer::interpolate(float alpha, float beta, float gamma, const Eigen::Vector3f& vert1,
                                 const Eigen::Vector3f& vert2, const Eigen::Vector3f& vert3,
                                 const Eigen::Vector3f& weight, const float& Z)
{
    Vector3f interpolated_res;
    for (int i = 0; i < 3; i++) {
        interpolated_res[i] = alpha * vert1[i] / weight[0] + beta * vert2[i] / weight[1] +
                              gamma * vert3[i] / weight[2];
    }
    interpolated_res *= Z;
    return interpolated_res;
}

 void Rasterizer::rasterize_triangle(const Triangle& t,
                                         const std::array<Vector3f, 3>& world_pos,
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

    for (int x = Xmin-1; x <= Xmax+1; x++) {
        for (int y = Ymin-1; y <= Ymax+1; y++) {
            Vector2i p(x, y); // current pixel position
            
            // if current pixel is in current triangle:

            if (!inside_triangle(x, y, t.vertex)) {
                // pixel is outside the triangle
                continue;
            }

            // use barycentric coordinates to determine if the pixel is inside the triangle
            tuple cog = compute_barycentric_2d(x, y, t.vertex);

            Vector3f Cog = {std::get<0>(cog), std::get<1>(cog), std::get<2>(cog)};
            
            // 1. interpolate depth(use projection correction algorithm)
            // use the inverse of clip space w coordinates to interpolate depth
            
            float z = 1 / (Cog.x() / t.vertex[0].w() +
                           Cog.y() / t.vertex[1].w() +
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


void Rasterizer::clear(BufferType buff)
{
    if ((buff & BufferType::Color) == BufferType::Color) {
        fill(frame_buf.begin(), frame_buf.end(), RenderEngine::background_color * 255.0f);
    }
    if ((buff & BufferType::Depth) == BufferType::Depth) {
        fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

Rasterizer::Rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
}


int Rasterizer::get_index(int x, int y)
{
    return (height - 1 - y) * width + x;
}


void Rasterizer::set_pixel(const Vector2i& point, const Vector3f& res)
{
    int idx        = get_index(point.x(), point.y());
    frame_buf[idx] = res;
}
