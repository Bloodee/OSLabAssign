#include "ray.h"

#include <cmath>
#include <array>

#include <Eigen/Dense>
#include <spdlog/spdlog.h>

#include "../utils/math.hpp"

using Eigen::Matrix3f;
using Eigen::Matrix4f;
using Eigen::Vector2f;
using Eigen::Vector3f;
using Eigen::Vector4f;
using std::numeric_limits;
using std::optional;
using std::size_t;

constexpr float infinity = 1e5f;
constexpr float eps      = 1e-5f;

Intersection::Intersection() : t(numeric_limits<float>::infinity()), face_index(0)
{
}

Ray generate_ray(int width, int height, int x, int y, Camera& camera, float depth)
{

    Vector2f pos((float)x + 0.5f, (float)y + 0.5f);
    Vector2f center((float)width / 2.0f, (float)height / 2.0f);
    Matrix4f inv_view = camera.view().inverse();
    float fov_y                  = radians(camera.fov_y_degrees);
    float image_plane_height     = 2.0f * tan(fov_y*0.5f) * depth;
    float ratio                  = image_plane_height / (float)height;
    Vector4f view_pos=
        Vector4f((pos.x() - center.x()) * ratio, -(pos.y() - center.y()) * ratio, -depth, 1.0f);
    Vector4f world_pos_specified = inv_view * view_pos;

    Vector3f world_pos = world_pos_specified.head(3);


    Ray ray;
    ray.origin = camera.position;
    ray.direction = (world_pos - camera.position).normalized();

    return ray;
}

optional<Intersection> ray_triangle_intersect(const Ray& ray, const GL::Mesh& mesh, size_t index)
{
    // these lines below are just for compiling and can be deleted
    (void)ray;
    (void)mesh;
    (void)index;
    // these lines above are just for compiling and can be deleted
    Intersection result;
    
    if (result.t - infinity < -eps) {
        return result;
    } else {
        return std::nullopt;
    }
}

optional<Intersection> naive_intersect(const Ray& ray, const GL::Mesh& mesh, const Matrix4f model)
{

    Intersection result;


    for (size_t i = 0; i < mesh.faces.count(); ++i) {
        Vector3f v[3];
        Vector4f V[3];
        Vector3f nor[3];
        auto temp = mesh.face(i);
        for (size_t j = 0; j < 3; j++) {
            nor[j] = mesh.normal(temp[j]);
            v[j]    = mesh.vertex(temp[j]);
            V[j]   = model * Vector4f(v[j].x(), v[j].y(), v[j].z(), 1.0f);
            v[j]    = V[j].head(3)/V[j].w();

        } 
        Vector3f S1 = (ray. direction).cross(v[2] - v[0]);
        Vector3f S2 = (ray.origin - v[0]).cross(v[1] - v[0]);

        float t     = S2.dot(v[2] - v[0]) / S1.dot(v[1] - v[0]);
        float beta  = S1.dot(ray.origin - v[0]) / S1.dot(v[1] - v[0]);
        float gamma = S2.dot(ray.direction) / S1.dot(v[1] - v[0]);
        float alpha = 1 - beta - gamma;
        
        if ((t - result.t < -eps )&& alpha >0 && alpha <1 && beta >0 && beta <1 && alpha +beta<1) {
            result.t                 = t;
            result.face_index = i;
            result.barycentric_coord = {alpha, beta, gamma};
            result.normal            = (alpha * nor[0] + beta * nor[1] + gamma * nor[2]).normalized();
        }
    
    }
    if (result.t < eps)
        return std::nullopt;
    if (result.t - infinity < -eps) {
        return result;
    }
    return std::nullopt;
}
