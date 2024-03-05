#include "object.h"

#include <array>
#include <optional>

#ifdef _WIN32
#include <Windows.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/format.h>

#include "../utils/math.hpp"
#include "../utils/ray.h"
#include "../simulation/solver.h"
#include "../utils/logger.h"

using Eigen::Matrix4f;
using Eigen::Quaternionf;
using Eigen::Vector3f;
using std::array;
using std::make_unique;
using std::optional;
using std::string;
using std::vector;

bool Object::BVH_for_collision   = false;
size_t Object::next_available_id = 0;
std::function<KineticState(const KineticState&, const KineticState&)> Object::step =
    forward_euler_step;

Object::Object(const string& object_name)
    : name(object_name), center(0.0f, 0.0f, 0.0f), scaling(1.0f, 1.0f, 1.0f),
      rotation(1.0f, 0.0f, 0.0f, 0.0f), velocity(0.0f, 0.0f, 0.0f), force(0.0f, 0.0f, 0.0f),
      mass(1.0f), BVH_boxes("BVH", GL::Mesh::highlight_wireframe_color)
{
    visible  = true;
    modified = false;
    id       = next_available_id;
    ++next_available_id;
    bvh                      = make_unique<BVH>(mesh);
    const string logger_name = fmt::format("{} (Object ID: {})", name, id);
    logger                   = get_logger(logger_name);
}

Matrix4f Object::model()
{
    const Quaternionf& r        = rotation; 
    auto [x_angle, y_angle, z_angle] = quaternion_to_ZYX_euler(r.w(), r.x(), r.y(), r.z());
    float Xangle                    = radians(x_angle);
    float Yangle                    = radians(y_angle);
    float Zangle                    = radians(z_angle);
    Matrix4f Rx,Ry,Rz,R;
    Rx << 1, 0, 0, 0, 0, cos(Xangle), -sin(Xangle), 0, 0, sin(Xangle), cos(Xangle),0,0,0,0,1;
    Ry << cos(Yangle), 0, sin(Yangle), 0, 0, 1, 0, 0, -sin(Yangle), 0, cos(Yangle), 0,0,0,0,1;
    Rz << cos(Zangle), -sin(Zangle), 0, 0, sin(Zangle), cos(Zangle),0, 0, 0, 0, 1,0,0,0,0,1;
    R = Rx * Ry * Rz;
    Matrix4f T, S;
    float Tx, Ty, Tz, Sx, Sy, Sz;
    Tx = center.x();
    Ty = center.y();
    Tz = center.z();
    Sx = scaling.x();
    Sy = scaling.y();
    Sz = scaling.z();
    T << 1, 0, 0, Tx, 0, 1, 0, Ty, 0, 0, 1, Tz, 0, 0, 0, 1;
    S << Sx, 0, 0, 0, 0, Sy, 0, 0, 0, 0, Sz, 0, 0, 0, 0, 1;
    Matrix4f model;
    model = T * R * S;
    return model;
}

void Object::update(vector<Object*>& all_objects)
{

    KineticState current_state{center, velocity, force / mass};
    KineticState next_state = step(prev_state, current_state);

    prev_state = current_state;
    //(void)all_objects;
     for (auto object : all_objects) {

        for (size_t i = 0; i < mesh.edges.count(); ++i) {

            if (BVH_for_collision) {
            } else {
            }

        }
        object->center = next_state.position;
        object->velocity = next_state.velocity;
        object->force    = mass * next_state.acceleration;
    }

}

void Object::render(const Shader& shader, WorkingMode mode, bool selected)
{
    if (modified) {
        mesh.VAO.bind();
        mesh.vertices.to_gpu();
        mesh.normals.to_gpu();
        mesh.edges.to_gpu();
        mesh.edges.release();
        mesh.faces.to_gpu();
        mesh.faces.release();
        mesh.VAO.release();
    }
    modified = false;
    unsigned int element_flags = GL::Mesh::faces_flag;
    if (mode == WorkingMode::MODEL) {
        shader.set_uniform("model", I4f);
        shader.set_uniform("normal_transform", I4f);
        element_flags |= GL::Mesh::vertices_flag;
        element_flags |= GL::Mesh::edges_flag;
    } else {
        Matrix4f model = this->model();
        shader.set_uniform("model", model);
        shader.set_uniform("normal_transform", (Matrix4f)(model.inverse().transpose()));
    }
    if (check_picking_enabled(mode) && selected) {
        element_flags |= GL::Mesh::edges_flag;
    }
    mesh.render(shader, element_flags);
}

void Object::rebuild_BVH()
{
    bvh->recursively_delete(bvh->root);
    bvh->build();
    BVH_boxes.clear();
    refresh_BVH_boxes(bvh->root);
    BVH_boxes.to_gpu();
}

void Object::refresh_BVH_boxes(BVHNode* node)
{
    if (node == nullptr) {
        return;
    }
    BVH_boxes.add_AABB(node->aabb.p_min, node->aabb.p_max);
    refresh_BVH_boxes(node->left);
    refresh_BVH_boxes(node->right);
}
