#include "shader.h"
#include "../utils/math.hpp"

#ifdef _WIN32
#undef min
#undef max
#endif

using Eigen::Vector3f;
using Eigen::Vector4f;

// vertex shader & fragement shader can visit   
// all the static variables below from Uniforms structure
Eigen::Matrix4f Uniforms::MVP;
Eigen::Matrix4f Uniforms::inv_trans_M;
int Uniforms::width;
int Uniforms::height;

// vertex shader
VertexShaderPayload vertex_shader(const VertexShaderPayload& payload)
{
    VertexShaderPayload output_payload = payload;

    // Vertex position transformation
    output_payload.position = Uniforms::MVP * output_payload.position;

    output_payload.position.x() /= output_payload.position.w();
    output_payload.position.y() /= output_payload.position.w();
    output_payload.position.z() /= output_payload.position.w();

    output_payload.position.x() = (output_payload.position.x() + 1.0) * Uniforms::width / 2; 
    output_payload.position.y() = (output_payload.position.y() + 1.0) * Uniforms::height / 2; 
    float f1                    = (50 - 0.1) / 2;
    float f2                    = (50 + 0.1) / 2;
    output_payload.position.z() = output_payload.position.w() * f1 + f2;

    // Vertex normal transformation
    Vector4f Normal       = {output_payload.normal.x(), output_payload.normal.y(),
                             output_payload.normal.z(), 0};
    Normal                = Uniforms::inv_trans_M * Normal;
    output_payload.normal = {Normal.x(), Normal.y(), Normal.z()};

    return output_payload;
}

Vector3f phong_fragment_shader(const FragmentShaderPayload& payload, GL::Material material,
                               const std::list<Light>& lights, Camera camera)
{
    Vector3f result = {0, 0, 0};

    Vector3f ka = material.ambient;
    Vector3f kd = material.diffuse;
    Vector3f ks = material.specular;

    for (const Light& L : lights) {
        Vector3f LD = (L.position - payload.world_pos).normalized();
        Vector3f VD = (camera.position - payload.world_pos).normalized();
        Vector3f HV = (LD + VD).normalized();
        float d     = (L.position - payload.world_pos).norm();
        if (d != 0.0) {
            float attenuation = L.intensity / (d * d);

            Vector3f Ambient = 0.5 * attenuation * ka;
            float NdotL      = std::max(0.0f, payload.world_normal.dot(LD));
            Vector3f Diffuse = 0.8 * attenuation * kd * NdotL;

            float NdotH       = std::max(0.0f, payload.world_normal.dot(HV));
            float p           = material.shininess;
            Vector3f Specular = attenuation * ks * std::pow(NdotH, p);

            result += (Ambient + Diffuse + Specular);
        }
    }
    result *= 255.f;
    result = result.cwiseMin(Vector3f(255, 255, 255)).cwiseMax(Vector3f(0, 0, 0));

    return result;
}


