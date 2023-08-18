#include <SynthRenderer.h>

#include <algorithm>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <system_error>
#include <limits>
#include <tuple>


#include <glm/gtx/string_cast.hpp>


using std::numeric_limits;
using std::tuple;

void SynthRenderer::initBackgroundObjects()
{
    glGenBuffers(1, &background_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, background_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(background_vertices), background_vertices, GL_STATIC_DRAW);
    glGenVertexArrays(1, &background_VAO);
    glBindVertexArray(background_VAO);
    //Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    //2D texture coordinates attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0); //Unbind background's VAO
}


int SynthRenderer::getBackgroundImagesCount() const
{
    return background_texture_objects.size();
};


int SynthRenderer::addBackgroundImagesDirectory(
        const string &background_images_directory
        )
{
    int added_images_count = 0;
    stbi_set_flip_vertically_on_load(true);
    for (const auto &entry : std::filesystem::directory_iterator(background_images_directory))
    {
        if (entry.is_regular_file())
        {
            string extension = entry.path().extension();
            //lowercase extension
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            if (known_images_extensions.count(extension) != 0)
            {
                //Load image
                int loaded_image_width, loaded_image_height, channels;
                string filename = entry.path().string();
                unsigned char *image_data = stbi_load(filename.c_str(), &loaded_image_width, &loaded_image_height, &channels, 0);

                if (image_data == nullptr)
                {
                    clog << "Failed to load image: " << filename << endl;
                }
                else
                {
                    clog << "Image loaded: " << filename << " " << loaded_image_width<< "x" << loaded_image_height << "x" << channels << endl;
                };

                //
                GLuint texture_object;
                glGenTextures(1, &texture_object);
                glBindTexture(GL_TEXTURE_2D, texture_object);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                // set texture filtering parameters
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, loaded_image_width, loaded_image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
                glGenerateMipmap(GL_TEXTURE_2D);

                glBindTexture(GL_TEXTURE_2D, 0);

                background_texture_objects.push_back(texture_object);
                added_images_count++;
            };
        }
    };

    return added_images_count;
};


void SynthRenderer::loadModels(const vector<pair<string, string>> &models_aliases_to_filenames) 
{
    for (const auto &model_alias_filename : models_aliases_to_filenames)
    {
        clog << "Loading model with alias: " << model_alias_filename.first
              << " filename: " << model_alias_filename.second << endl;
        models.emplace(model_alias_filename.first, Model(model_alias_filename.second));
    };
};


bool SynthRenderer::initGL()
{
    glfwInit();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    this->offscreen_window = glfwCreateWindow(generate_image_width, generate_image_height, "", NULL, NULL);
    if (this->offscreen_window == NULL)
    {
        clog << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(this->offscreen_window);

    //glad: Load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        clog << "Failed to initialize GLAD" << endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, generate_image_width, generate_image_height);

    clog << "Loading background shader" << endl;
    this->background_shader.emplace("shaders/vertex_texture_passthrough.glsl", "shaders/fragment_flat_texture.glsl");

    clog << "Loading model rendering shader" << endl;
    this->model_shader.emplace("shaders/vertex_project.glsl", "shaders/fragment_light.glsl");

    clog << "Loading semantic segmentation shader" << endl;
    this->semantic_segmentation_shader.emplace("shaders/vertex_project.glsl", "shaders/fragment_uniform.glsl");

    initBackgroundObjects();

    return true;
}


void SynthRenderer::drawBackground(int background_index)
{
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    //Draw background
    background_shader.value().use();
    glBindVertexArray(background_VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, background_texture_objects[background_index]);
    glUniform1i(glGetUniformLocation(background_shader.value().ID, "texture1"), 0);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);

    //??? TODO: Why is this necessary? Without it, the model is not drawn, but Z-coords currespond to the far end of the normalized device coordinates cube
    glClear(GL_DEPTH_BUFFER_BIT);
}

glm::mat4 getModelMatrix(const ObjectAttributes& attributes)
{
        glm::mat4 rotation_matrix = glm::yawPitchRoll(
                glm::radians(attributes.yaw),
                glm::radians(attributes.pitch),
                glm::radians(attributes.roll));
        glm::mat4 model_matrix(1.0f);
        model_matrix = glm::translate(model_matrix, glm::vec3(attributes.x, attributes.y, attributes.z));
        model_matrix = glm::scale(model_matrix, glm::vec3(attributes.scale, attributes.scale, attributes.scale));
        model_matrix = model_matrix * rotation_matrix;
        return model_matrix;
}

void SynthRenderer::drawModels(
        vector< pair<string, ObjectAttributes> > &models_to_positions,
        Shader &shader)
{
    for (auto name_object : models_to_positions)
    {
        //Lookup the 3d model by name:
        Model& model = models.find(name_object.first)->second;

        //Position the model for rendering:
        glm::mat4 model_matrix = getModelMatrix(name_object.second);

        //Draw the model
        shader.setMat4("model", model_matrix);

        uint8_t class_id_lo = name_object.second.semantic_class_id & 0xFF;
        uint8_t class_id_med = (name_object.second.semantic_class_id >> 8) & 0xFF;
        uint8_t class_id_hi = (name_object.second.semantic_class_id >> 16) & 0xFF;        
        glm::vec3 class_id_vec(class_id_lo / 255.0f, class_id_med / 255.0f, class_id_hi / 255.0f);
        shader.setVec3("class_id_vec", class_id_vec);

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDepthFunc(GL_LESS);
        model.Draw(shader);
    }
}


vector< pair<string, Rect> > SynthRenderer::computeObjectsBoundingRects(
        vector< pair<string, ObjectAttributes> > &models_to_positions,
        const glm::mat4 &projection_view_matrix
)
{
    vector< pair<string, Rect> > result;

    for (auto name_object : models_to_positions)
    {
        //Lookup the 3d model by name:
        Model& model = models.find(name_object.first)->second;

        glm::mat4 model_matrix = getModelMatrix(name_object.second);

        //Compute the bounding rect of the model (actually, of the model's convex hull)
        glm::mat4 PVM_matrix = projection_view_matrix * model_matrix;

        Rect bounding_rect;
        for (glm::vec3 convex_hull_point : model.convexHullPoints)
        {
            glm::vec4 projected_point = PVM_matrix * glm::vec4(convex_hull_point, 1.0f);
            projected_point /= projected_point.w;
            bounding_rect.updateBounds(projected_point.x, projected_point.y);
        }

        //Convert the bounding rect to image coordinates
        bounding_rect.bottom_left.x = (1.0 + bounding_rect.bottom_left.x) / 2.0f * generate_image_width;
        bounding_rect.bottom_left.y = (1.0 - bounding_rect.bottom_left.y) / 2.0f * generate_image_height;

        bounding_rect.top_right.x = (1.0f + bounding_rect.top_right.x) / 2.0f * generate_image_width;
        bounding_rect.top_right.y = (1.0f - bounding_rect.top_right.y) / 2.0f * generate_image_height;
   
        result.push_back(make_pair(name_object.first, bounding_rect));
    }

    return result;
}


SyntheticResult SynthRenderer::renderImage(
        vector< pair<string, ObjectAttributes> > &models_to_attributes,
        int background_image_index,
        glm::vec3 camera_position,
        glm::vec3 camera_target,
        glm::vec3 camera_up,
        glm::vec3 sun_light_direction,
        glm::vec3 sun_light_color,
        glm::vec3 ambient_light_color,
        glm::vec3 search_light_color,
        float search_light_angle,
        bool generate_semantic_segmentation
        )
{
    SyntheticResult synthetic_result;

    //Create and bind the framebuffer for synthetic image generation
    unsigned int framebuffer_object;
    glGenFramebuffers(1, &framebuffer_object);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_object);

    //Create the texture
    GLuint generated_image_texture_object;
    glGenTextures(1, &generated_image_texture_object);
    glBindTexture(GL_TEXTURE_2D, generated_image_texture_object);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, generate_image_width, generate_image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //Attach the texture to the framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, generated_image_texture_object, 0);

    //Create a renderbuffer object for depth and stencil attachment
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, generate_image_width, generate_image_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);    

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw runtime_error("Framebuffer is not complete!");
    };

    drawBackground(background_image_index);

    //Initialize the camera
    Camera camera(camera_position, camera_target, camera_up);
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)generate_image_width / (float)generate_image_height, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    model_shader.value().use();
    model_shader.value().setMat4("projection", projection);
    model_shader.value().setMat4("view", view);

    model_shader.value().setVec3("viewPos", camera.Position);
    model_shader.value().setVec3("searchLightPos", camera.Position);
    model_shader.value().setVec3("externalLightDir", sun_light_direction);
    model_shader.value().setVec3("searchLightDir", camera.Front);

    model_shader.value().setVec3("external_light_color", sun_light_color);
    model_shader.value().setVec3("ambient_light_color", ambient_light_color);
    model_shader.value().setVec3("searchlight_color", search_light_color);

    model_shader.value().setFloat("searchlight_cone_angle_sin", sin(search_light_angle));

    drawModels(models_to_attributes, model_shader.value());  //Render synthetic image
    
    if (generate_semantic_segmentation)
    {
        //Create and bind the framebuffer for synthetic image generation
        unsigned int segments_framebuffer_object;
        glGenFramebuffers(1, &segments_framebuffer_object);
        glBindFramebuffer(GL_FRAMEBUFFER, segments_framebuffer_object);

        //Create the texture
        GLuint generated_segments_image_texture_object;
        glGenTextures(1, &generated_segments_image_texture_object);
        glBindTexture(GL_TEXTURE_2D, generated_segments_image_texture_object);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, generate_image_width, generate_image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        //Attach the texture to the framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, generated_segments_image_texture_object, 0);

        //Create a renderbuffer object for depth and stencil attachment
        GLuint rbo;
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, generate_image_width, generate_image_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);    

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            throw runtime_error("Segments framebuffer is not complete!"); 
        };

        semantic_segmentation_shader.value().use();
        semantic_segmentation_shader.value().setMat4("view", view);
        semantic_segmentation_shader.value().setMat4("projection", projection);

        drawModels(models_to_attributes, semantic_segmentation_shader.value());  //Render synthetic image
        
        //We will interpret RGB colors of the pixels as 24-bit integer (semantic index)
        auto pixels_buff_ptr = make_unique<GLubyte[]>(generate_image_width * generate_image_height * 3);
        glReadPixels(0, 0, generate_image_width, generate_image_height, GL_RGB, GL_UNSIGNED_BYTE, pixels_buff_ptr.get());

        auto semantic_image = Image(std::move(pixels_buff_ptr), generate_image_width, generate_image_height, 3);
        synthetic_result.semantic_segmentation = std::move(semantic_image);         

        //Bind default framebuffer and delete the segmentation framebuffer object
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &segments_framebuffer_object);
    }

    //Read the pixels from the framebuffer, so we can return and access them
    auto pixels_buff_ptr = make_unique<GLubyte[]>(generate_image_width * generate_image_height * 3);
    glReadPixels(0, 0, generate_image_width, generate_image_height, GL_RGB, GL_UNSIGNED_BYTE, pixels_buff_ptr.get());

    //We are done with the framebuffer, bind default framebuffer now
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &framebuffer_object);

    auto renered_image = Image(std::move(pixels_buff_ptr), generate_image_width, generate_image_height, 3);
    synthetic_result.image = std::move(renered_image);         

    //Compute the bounding rects
    glm::mat4 projection_view = projection * view;
    vector< pair<string, Rect> > bounding_rects = computeObjectsBoundingRects(models_to_attributes, projection_view);
    synthetic_result.object_name_to_bounding_rect = std::move(bounding_rects);

    return synthetic_result;
};


vector< tuple<string, glm::vec3, glm::vec3>> SynthRenderer::getModelsExtent() const
{
    vector< tuple<string, glm::vec3, glm::vec3> > models_to_extent;
    for (const auto& model : models)
    {
        glm::vec3 min(numeric_limits<float>::max(), numeric_limits<float>::max(), numeric_limits<float>::max());
        glm::vec3 max(numeric_limits<float>::min(), numeric_limits<float>::min(), numeric_limits<float>::min());
        //Loop through convex hull of the model and find extent along all the axes
        for (const auto& point : model.second.convexHullPoints)
        {
                min.x = std::min(min.x, point.x);
                min.y = std::min(min.y, point.y);
                min.z = std::min(min.z, point.z);

                max.x = std::max(max.x, point.x);
                max.y = std::max(max.y, point.y);
                max.z = std::max(max.z, point.z);
        }

        models_to_extent.push_back(make_tuple(model.first, min, max));
    }

    return models_to_extent;
}

