#include <boost/python.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/module.hpp>
#include <boost/python/numpy/ndarray.hpp>
#include <boost/python/object.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/numpy.hpp>

#include "SynthRenderer.h"

namespace bp = boost::python;
namespace np = boost::python::numpy;

const char * const XKEY = "x";
const char * const YKEY = "y";
const char * const ZKEY = "z";
const char * const YAWKEY = "yaw";
const char * const PITCHKEY = "pitch";
const char * const ROLLKEY = "roll";
const char * const SEMANTIC_CLASS_KEY = "semantic_class";
const char * const SCALING_KEY = "scale";


template<typename T>
void extractValueFromDict(bp::dict dict, string key, T& value)
{
    if (dict.has_key(key))
    {
        value = bp::extract<T>(dict[key]);
    }
    else
    {
        throw std::runtime_error("Mandatory key " + key + " not found in object property dictionary");
    }
}

template<typename T>
void extractValueFromDictOrDefault(bp::dict dict, string key, T& value, T default_value)
{
    if (dict.has_key(key))
    {
        value = bp::extract<T>(dict[key]);
    }
    else
    {
        value = default_value;
    }
}


class PySynthRendererWrapper
{
    SynthRenderer renderer;
public:
    PySynthRendererWrapper(int width, int height) : renderer(width, height)
    {
    }

    int add_background_images_folder(std::string folder)
    {
        return renderer.addBackgroundImagesDirectory(folder);
    }

    int get_number_of_background_images()
    {
        return renderer.getBackgroundImagesCount();
    }

    void load_models(bp::dict models_to_paths)
    {
        vector< pair<string, string> > models_paths;

        bp::list object_items = models_to_paths.items();
        for (int i = 0; i < bp::len(object_items); ++i)
        {
            bp::tuple item = bp::extract<bp::tuple>(object_items[i]);
            string model_name = bp::extract<string>(item[0]);
            string model_path = bp::extract<string>(item[1]);
            models_paths.push_back({model_name, model_path});
        }

        renderer.loadModels(models_paths);
    }


    bp::dict get_models_extent()
    {
        vector< tuple<string, glm::vec3, glm::vec3> > models_extent = renderer.getModelsExtent();

        bp::dict models_extent_dict;
        for (auto model_extent : models_extent)
        {
            const auto& [model_name, extent_min, extent_max] = model_extent;
            models_extent_dict[model_name] = bp::make_tuple(
                    extent_min.x, extent_min.y, extent_min.z,
                    extent_max.x, extent_max.y, extent_max.z);
        }

        return models_extent_dict;
    }




    bp::tuple renderScene(
            int background_image_index,
            bp::tuple camera_position,
            bp::tuple camera_target,
            bp::tuple camera_up,
            bp::tuple sun_light_direction,
            bp::tuple sun_light_color,
            bp::tuple ambient_light_color,
            bp::tuple search_light_color,
            double search_light_angle,
            bp::list models,   // list of tuples (model_name, {scaling : <scaling>, x : <x>, y : <y>, z : <z>, yaw : <yaw>, pitch: <pitch>, roll : <roll>, semantic_class: <semantic class index>})
            bool render_semantic_labels
    )
    {
        //Extract objects information from python dictionary
        vector< pair<string, ObjectAttributes> > models_attributes;

        bp::list object_items = models;
        for (int i = 0; i < bp::len(models); ++i)
        {

            bp::tuple item = bp::extract<bp::tuple>(object_items[i]);
            string model_name = bp::extract<string>(item[0]);
            bp::dict object_properties_dict = bp::extract<bp::dict>(item[1]);

            //bp::tuple object_position_tuple = bp::extract<bp::tuple>(item[1]);

            float scaling, x, y, z, yaw, pitch, roll;
            int semantic_label;

            extractValueFromDict(object_properties_dict, SCALING_KEY, scaling);
clog << "[PySynthRendererWrapper] scaling: " << scaling << endl;
            extractValueFromDict(object_properties_dict, XKEY, x);
clog << "[PySynthRendererWrapper] x: " << x << endl;            
            extractValueFromDict(object_properties_dict, YKEY, y);
clog << "[PySynthRendererWrapper] y: " << y << endl;           
            extractValueFromDict(object_properties_dict, ZKEY, z);
clog << "[PySynthRendererWrapper] z: " << z << endl;
            extractValueFromDict(object_properties_dict, YAWKEY, yaw);
clog << "[PySynthRendererWrapper] rx: " << yaw << endl;            
            extractValueFromDict(object_properties_dict, PITCHKEY, pitch);
clog << "[PySynthRendererWrapper] ry: " << pitch << endl;            
            extractValueFromDict(object_properties_dict, ROLLKEY, roll);
clog << "[PySynthRendererWrapper] rz: " << roll << endl;            
            extractValueFromDictOrDefault(object_properties_dict, SEMANTIC_CLASS_KEY, semantic_label, 0);
clog << "[PySynthRendererWrapper] semantic_label: " << semantic_label << endl;

            models_attributes.push_back({model_name, ObjectAttributes(scaling, x, y, z, yaw, pitch, roll, semantic_label)});
        }
        
        //Extract camera and light positions from python tuples
        glm::vec3 camera_position_vec = glm::vec3(
            bp::extract<float>(camera_position[0]),
            bp::extract<float>(camera_position[1]),
            bp::extract<float>(camera_position[2])
        );

        glm::vec3 camera_target_vec = glm::vec3(
            bp::extract<float>(camera_target[0]),
            bp::extract<float>(camera_target[1]),
            bp::extract<float>(camera_target[2])
        );

        glm::vec3 camera_up_vec = glm::vec3(
            bp::extract<float>(camera_up[0]),
            bp::extract<float>(camera_up[1]),
            bp::extract<float>(camera_up[2])
        );

        glm::vec3 sun_light_direction_vec = glm::vec3(
            bp::extract<float>(sun_light_direction[0]),
            bp::extract<float>(sun_light_direction[1]),
            bp::extract<float>(sun_light_direction[2])
        );

        glm::vec3 sun_light_color_vec = glm::vec3(
            bp::extract<float>(sun_light_color[0]),
            bp::extract<float>(sun_light_color[1]),
            bp::extract<float>(sun_light_color[2])
        );

        glm::vec3 ambient_light_color_vec = glm::vec3(
            bp::extract<float>(ambient_light_color[0]),
            bp::extract<float>(ambient_light_color[1]),
            bp::extract<float>(ambient_light_color[2])
        );

        glm::vec3 search_light_color_vec = glm::vec3(
            bp::extract<float>(search_light_color[0]),
            bp::extract<float>(search_light_color[1]),
            bp::extract<float>(search_light_color[2])
        );

        //Render image
        SyntheticResult rendering_results = renderer.renderImage(
            models_attributes,
            background_image_index,
            camera_position_vec,
            camera_target_vec,
            camera_up_vec,
            sun_light_direction_vec,
            sun_light_color_vec,
            ambient_light_color_vec,
            search_light_color_vec,
            search_light_angle,
            render_semantic_labels);

        //Copy image data into numpy array
        bp::tuple image_shape = bp::make_tuple(
                rendering_results.image.height,
                rendering_results.image.width, 
                3);
        np::ndarray image_result = np::zeros(image_shape, np::dtype::get_builtin<unsigned char>());
        std::copy(
                rendering_results.image.data.get(),
                rendering_results.image.data.get() + rendering_results.image.width * rendering_results.image.height * 3,
                image_result.get_data());

        bp::object semantic_segmentation_result_object;
        //Copy semantic segmentation data into numpy array (if available)
        if (rendering_results.semantic_segmentation.has_value())
        {
            bp::tuple segmentation_shape = bp::make_tuple(
                    rendering_results.image.height,
                    rendering_results.image.width);
            np::ndarray semantic_segmentation_result = np::zeros(segmentation_shape, np::dtype::get_builtin<int>());
            for (int i = 0; i < rendering_results.image.width * rendering_results.image.height; ++i)
            {            
                int semantic_label = int(rendering_results.semantic_segmentation.value().data.get()[i * 3 + 2]) * 256 * 256 +
                    int(rendering_results.semantic_segmentation.value().data.get()[i * 3 + 1]) * 256 +
                    int(rendering_results.semantic_segmentation.value().data.get()[i * 3]);
                semantic_segmentation_result.get_data()[i] = semantic_label;
            }
            semantic_segmentation_result_object = semantic_segmentation_result;
        }

        //Extract objects bounding boxes and convert them into python list
        bp::list objects_bounding_rects = bp::list();
        for (auto& object_bounding_rect : rendering_results.object_name_to_bounding_rect)
        {
            //Convert OpenGL "device" coordinates to image pixels coordinates


            bp::tuple object_bounding_rect_tuple = bp::make_tuple(
                    object_bounding_rect.first, //object name
                    object_bounding_rect.second.top_right.y,  //top
                    object_bounding_rect.second.bottom_left.x, //left
                    object_bounding_rect.second.bottom_left.y, //bottom 
                    object_bounding_rect.second.top_right.x //right
            );

            objects_bounding_rects.append(object_bounding_rect_tuple);
        }

        bp::tuple result = bp::make_tuple(image_result, objects_bounding_rects, semantic_segmentation_result_object);
        return result;
    }
};

BOOST_PYTHON_MODULE(SynthRenderer)
{
    using namespace boost::python;
    Py_Initialize();
    boost::python::numpy::initialize();   
    class_<PySynthRendererWrapper>("SynthRenderer", init<int, int>())
        .def("add_background_images_folder", &PySynthRendererWrapper::add_background_images_folder)
        .def("render_scene", &PySynthRendererWrapper::renderScene)
        .def("get_background_images_count", &PySynthRendererWrapper::get_number_of_background_images)
        .def("load_models", &PySynthRendererWrapper::load_models)
        .def("get_models_extent", &PySynthRendererWrapper::get_models_extent)
    ;
}

