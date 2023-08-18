#pragma once

#include <limits>
#include <mesh.h>
#include <assimp/Importer.hpp>
#include <glad/glad.h> 
#include <GL/gl.h>
#include <GLFW/glfw3.h>


#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//#include <filesystem.h>
#include <shader_m.h>
#include <camera.h>
#include <model.h>

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>
#include <tuple>
#include <filesystem>
#include <iostream>


#include <unistd.h>



using std::unordered_map;
using std::vector;
using std::string;
using std::unordered_set;
using std::optional;
using std::clog;
using std::endl;


struct Image
{
    Image() = default; //Empty image
    Image(unique_ptr<unsigned char[]>&& data, unsigned int width, unsigned int height, unsigned int channels) : data(std::move(data)), width(width), height(height), channels(channels) {};

    unique_ptr<unsigned char[]> data;
    unsigned int width;
    unsigned int height;
    unsigned int channels;
};


struct ObjectAttributes
{
    ObjectAttributes() = default;
    ObjectAttributes(
            float scale,
            float x,
            float y,
            float z,
            float yaw,
            float pitch,
            float roll,
            int semantic_class_id = 0) : scale(scale), yaw(yaw), pitch(pitch), roll(roll), x(x), y(y), z(z), semantic_class_id(semantic_class_id) {};

    float scale = 1.0f;

    float yaw;
    float pitch;
    float roll;

    float x;
    float y;
    float z;

    int semantic_class_id;
};


struct Rect {
    glm::vec2 bottom_left;
    glm::vec2 top_right;

    Rect() : bottom_left(numeric_limits<float>::max(), numeric_limits<float>::max()),
        top_right(numeric_limits<float>::min(), numeric_limits<float>::min()) {}


    void updateBounds(float x, float y)
    {
        bottom_left.x = std::min(bottom_left.x, x);
        bottom_left.y = std::min(bottom_left.y, y);
        top_right.x = std::max(top_right.x, x);
        top_right.y = std::max(top_right.y, y);
    }
};


struct SyntheticResult
{
    optional<Image> semantic_segmentation;
    Image image;
    vector< pair<string, Rect> > object_name_to_bounding_rect;
};


class SynthRenderer
{
        const unordered_set<string> known_images_extensions = {".jpg", ".jpeg", ".png", ".bmp", ".tga", ".gif", ".psd", ".hdr", ".pic"};
        const GLfloat background_vertices[20] = {
            // Positions          // Texture Coords
            1.0f,  1.0f, .999f,   1.0f, 1.0f, // Top Right
            1.0f, -1.0f, .999f,   1.0f, 0.0f, // Bottom Right
            -1.0f, -1.0f, .999f,   0.0f, 0.0f, // Bottom Left
            -1.0f,  1.0f, .999f,   0.0f, 1.0f  // Top Left 
        };

        vector<GLuint> background_texture_objects;

        unordered_map<string, Model> models;
        unsigned int generate_image_width;
        unsigned int generate_image_height;
        GLFWwindow* offscreen_window;

        optional<Shader> background_shader; 
        optional<Shader> model_shader;
        optional<Shader> semantic_segmentation_shader;
       
        GLuint background_VAO, background_VBO;

        void initBackgroundObjects();

        bool initGL();

        void drawBackground(int background_index);

        void drawModels(
                vector< pair<string, ObjectAttributes> > &models_to_positions,
                Shader &shader
        );

public:
        SynthRenderer(
                unsigned int generate_image_width, 
                unsigned int generate_image_height
                     ) : generate_image_width(generate_image_width), generate_image_height(generate_image_height)
        {
            initGL();
            //loadModels({{"cube", "models/cube/cube.obj"}});
        };

        ~SynthRenderer()
        {
            glfwDestroyWindow(this->offscreen_window);
            glfwTerminate();
        };

        int addBackgroundImagesDirectory(const string &background_images_directory);

        int getBackgroundImagesCount() const;

        void loadModels(const vector<pair<string, string>> &models_aliases_to_filenames);

        vector< tuple<string, glm::vec3, glm::vec3> > getModelsExtent() const;

        vector< pair<string, Rect>> computeObjectsBoundingRects(
            vector< pair<string, ObjectAttributes> > &models_to_positions,
            const glm::mat4 &projection_view_matrix
        );

        SyntheticResult renderImage(
                vector< pair<string, ObjectAttributes> > &models_to_positions,
                int background_image_index,
                glm::vec3 camera_position,
                glm::vec3 camera_target,
                glm::vec3 camera_up,
                glm::vec3 sun_light_direction,
                glm::vec3 sun_light_color,
                glm::vec3 ambient_light_color,
                glm::vec3 search_light_color,
                float search_light_angle,
                bool generate_semantic_segmentation = false
        );
};

