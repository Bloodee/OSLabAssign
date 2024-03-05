#include <algorithm>
#include <cmath>
#include <fstream>
#include <memory>
#include <vector>
#include <optional>
#include <iostream>
#include <chrono>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "render_engine.h"
#include "../scene/light.h"
#include "../utils/math.hpp"
#include "../utils/ray.h"
#include "../utils/logger.h"

using std::chrono::steady_clock;
using duration   = std::chrono::duration<float>;
using time_point = std::chrono::time_point<steady_clock, duration>;
using Eigen::Vector3f;


constexpr int MAX_DEPTH        = 5;
constexpr float INFINITY_FLOAT = std::numeric_limits<float>::max();

constexpr float EPSILON = 0.00001f;


enum class MaterialType
{
    DIFFUSE_AND_GLOSSY,
    REFLECTION
};


void UpdateProgress(float progress)
{
    int barwidth = 70;
    std::cout << "[";
    int pos = static_cast<int>(barwidth * progress);
    for (int i = 0; i < barwidth; i++) {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "]" << int(progress * 100.0) << " %\r";
    std::cout.flush();
}

WhittedRenderer::WhittedRenderer(RenderEngine& engine)
    : width(engine.width), height(engine.height), n_threads(engine.n_threads), use_bvh(false),
      rendering_res(engine.rendering_res)
{
    logger = get_logger("Whitted Renderer");
}


void WhittedRenderer::render(Scene& scene)
{
    time_point begin_time = steady_clock::now();
    width                 = std::floor(width);
    height                = std::floor(height);

    // initialize frame buffer
    std::vector<Vector3f> framebuffer(static_cast<size_t>(width * height));
    for (auto& v : framebuffer) {
        v = Vector3f(0.0f, 0.0f, 0.0f);
    }

    int idx = 0;
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            // generate ray
            Ray ray = generate_ray(static_cast<int>(width), static_cast<int>(height), i, j,
                                   scene.camera, 1.0f);
            // cast ray
            framebuffer[idx++] = cast_ray(ray, scene, 0);
        }
        UpdateProgress(j / height);
    }
    // save result to whitted_res.ppm
    FILE* fp = fopen("whitted_res.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", (int)width, (int)height);
    static unsigned char color_res[3];
    rendering_res.clear();
    for (long unsigned int i = 0; i < framebuffer.size(); i++) {
        color_res[0] = static_cast<unsigned char>(255 * clamp(0.f, 1.f, framebuffer[i][0]));
        color_res[1] = static_cast<unsigned char>(255 * clamp(0.f, 1.f, framebuffer[i][1]));
        color_res[2] = static_cast<unsigned char>(255 * clamp(0.f, 1.f, framebuffer[i][2]));
        fwrite(color_res, 1, 3, fp);
        rendering_res.push_back(color_res[0]);
        rendering_res.push_back(color_res[1]);
        rendering_res.push_back(color_res[2]);
    }
    time_point end_time         = steady_clock::now();
    duration rendering_duration = end_time - begin_time;
    logger->info("rendering takes {:.6f} seconds", rendering_duration.count());
}


float WhittedRenderer::fresnel(const Vector3f& I, const Vector3f& N, const float& ior)
{
    
    float NdotI = std::max(0.0f, -I.dot(N));
    float n1       = 1.0; 
    float n2       = ior; 

    float R0 = ((n1 - n2) / (n1 + n2)) * ((n1 - n2) / (n1 + n2));
    float F  = R0 + (1 - R0) * pow(1 - NdotI, 5);
    return F;

}


std::optional<std::tuple<Intersection, GL::Material>> WhittedRenderer::trace(const Ray& ray, const Scene& scene)
{

    std::optional<Intersection> payload;
    Eigen::Matrix4f M ;
    GL::Material material;
    for (const auto& group : scene.groups) {
        for (const auto& object : group->objects) {

           
            M = object->model();
            std::optional<Intersection> intersection =
                naive_intersect(ray, object->mesh, M);

            if (intersection.has_value()) {
                if (!payload.has_value() || intersection.value().t < payload.value().t) {
                    payload = intersection;
                    material = object->mesh.material;
                }
          
            }
        }
    }
        if (!payload.has_value()) {
            return std::nullopt;
        }
    return std::make_tuple(payload.value(), material);
}

Vector3f WhittedRenderer::cast_ray(const Ray& ray, const Scene& scene, int depth)
{
    if (depth > MAX_DEPTH) {
        return Vector3f(0.0f, 0.0f, 0.0f);
    }
    // initialize hit color
    Vector3f hitcolor = RenderEngine::background_color;
    // get the result of trace()
    auto result = trace(ray, scene);
    
   
    if (result.has_value()) {
        Intersection intersection = std::get<0>(result.value());
        GL::Material material     = std::get<1>(result.value());
       
        if (material.shininess >= 1000.f) {
        
            float ior = 1.4;
        
            float kr = fresnel(ray.direction, intersection.normal, ior);
            Vector3f reflection_direction =
            2.0f * intersection.normal.dot(ray.direction) * intersection.normal - ray.direction;
            Ray reflection_ray;
            reflection_ray.origin = intersection.barycentric_coord + EPSILON * reflection_direction;
            
            reflection_ray.direction = reflection_direction;
            hitcolor = cast_ray(reflection_ray, scene, depth + 1) * kr;
        } else {
           
            Vector3f point = intersection.barycentric_coord;
            Vector3f normal = intersection.normal;
            Vector3f kd     = material.diffuse;
            Vector3f ks     = material.specular;
            for (const auto& light : scene.lights) {
                Vector3f LD = light.position - point;
                Vector3f LD_N = LD.normalized();
                Ray shadow_ray;
                shadow_ray.origin = point;
                shadow_ray.direction = LD_N;
                auto shadow_result   = trace(shadow_ray, scene);
                if (shadow_result.has_value()) {
                    float diffuse =std::max(0.0f, normal.dot(-LD_N));
                    Vector3f reflection_direction =2.0f * normal.dot(LD_N) * normal -LD_N;
                    float specular =std::pow(std::max(0.0f, -ray.direction.dot(reflection_direction)),material.shininess);
                    Vector3f lighting = light.intensity * (diffuse * kd + specular* ks);
                    hitcolor += lighting;

                }
            }
        }
    }
   

    return hitcolor;
}
