// This has been adapted from the Vulkan tutorial

#include "Starter.hpp"
#include <iostream>
#include <fstream>
#define MESH 209

// The uniform buffer objects data structures
// Remember to use the correct alignas(...) value
//        float : alignas(4)
//        vec2  : alignas(8)
//        vec3  : alignas(16)
//        vec4  : alignas(16)
//        mat3  : alignas(16)
//        mat4  : alignas(16)
// Example:
struct UniformBufferObject {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
    alignas(16) glm::mat4 nMat;
};

struct GlobalUniformBufferObject {
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec3 eyePos;
};

// The vertices data structures
// Example
struct Vertex {
    glm::vec3 pos;
    glm::vec2 UV;
    glm::vec3 normal;
};

// MAIN ! 
class MeshLoader : public BaseProject {
protected:

    // Current aspect ratio (used by the callback that resized the window
    float Ar;

    // Descriptor Layouts ["classes" of what will be passed to the shaders]
    DescriptorSetLayout DSL,DSLcity;

    // Vertex formats
    VertexDescriptor VD, VDcity;

    // Pipelines [Shader couples]
    Pipeline P, Pcity;

    // Models, textures and Descriptors (values assigned to the uniforms)
    // Please note that Model objects depends on the corresponding vertex structure
    // Models
    Model<Vertex>  Mtaxi;
    Model<Vertex> Mcity[MESH];
    // Descriptor sets
    DescriptorSet DStaxi, DScity[MESH];
    // Textures
    Texture Tcity;

    // C++ storage for uniform variables
    UniformBufferObject uboTaxi;
    UniformBufferObject ubocity[MESH];
    GlobalUniformBufferObject guboTaxi;
    GlobalUniformBufferObject gubocity[MESH];

    // Other application parameters
    glm::vec3 CamPos = glm::vec3(0.0, 1.5, 7.0); //initial pos of camera?
    float CamAlpha = 0.0f;
    float CamBeta = 0.0f;

    // Here you set the main application parameters
    void setWindowParameters() {
        // window size, titile and initial background
        windowWidth = 1280;
        windowHeight = 720;
        windowTitle = "Computer graphics' project";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = {0.0f, 0.005f, 0.01f, 1.0f};

        // Descriptor pool sizes
        uniformBlocksInPool =  (2 * MESH) + 2;
        texturesInPool = MESH + 1;
        setsInPool = MESH + 1;

        Ar = (float)windowWidth / (float)windowHeight;
    }

    // What to do when the window changes size
    void onWindowResize(int w, int h) {
        Ar = (float)w / (float)h;
    }

    // Here you load and setup all your Vulkan Models and Texutures.
    // Here you also create your Descriptor set layouts and load the shaders for the pipelines
    void localInit() {
        // Descriptor Layouts [what will be passed to the shaders]
        DSL.init(this, {
                // this array contains the bindings:
                // first  element : the binding number
                // second element : the type of element (buffer or texture)
                //                  using the corresponding Vulkan constant
                // third  element : the pipeline stage where it will be used
                //                  using the corresponding Vulkan constant
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
        });
        DSLcity.init(this, {
                // this array contains the bindings:
                // first  element : the binding number
                // second element : the type of element (buffer or texture)
                //                  using the corresponding Vulkan constant
                // third  element : the pipeline stage where it will be used
                //                  using the corresponding Vulkan constant
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
        });

        // Vertex descriptors
        VD.init(this, {
                // this array contains the bindings
                // first  element : the binding number
                // second element : the stride of this binging
                // third  element : whether this parameter change per vertex or per instance
                //                  using the corresponding Vulkan constant
                {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
        }, {
                        // this array contains the location
                        // first  element : the binding number
                        // second element : the location number
                        // third  element : the offset of this element in the memory record
                        // fourth element : the data type of the element
                        //                  using the corresponding Vulkan constant
                        // fifth  elmenet : the size in byte of the element
                        // sixth  element : a constant defining the element usage
                        //                   POSITION - a vec3 with the position
                        //                   NORMAL   - a vec3 with the normal vector
                        //                   UV       - a vec2 with a UV coordinate
                        //                   COLOR    - a vec4 with a RGBA color
                        //                   TANGENT  - a vec4 with the tangent vector
                        //                   OTHER    - anything else
                        //
                        // ***************** DOUBLE CHECK ********************
                        //    That the Vertex data structure you use in the "offsetoff" and
                        //	in the "sizeof" in the previous array, refers to the correct one,
                        //	if you have more than one vertex format!
                        // ***************************************************
                        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                                sizeof(glm::vec3), POSITION},
                        {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                                sizeof(glm::vec2), UV},
                        {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal),
                                sizeof(glm::vec3), NORMAL}
                });
        VDcity.init(this, {
                // this array contains the bindings
                // first  element : the binding number
                // second element : the stride of this binging
                // third  element : whether this parameter change per vertex or per instance
                //                  using the corresponding Vulkan constant
                {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
        }, {
                        // ***************************************************
                        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                                sizeof(glm::vec3), POSITION},
                        {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                                sizeof(glm::vec2), UV},
                        {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal),
                                sizeof(glm::vec3), NORMAL}
                });

        // Pipelines [Shader couples]
        // The second parameter is the pointer to the vertex definition
        // Third and fourth parameters are respectively the vertex and fragment shaders
        // The last array, is a vector of pointer to the layouts of the sets that will
        // be used in this pipeline. The first element will be set 0, and so on..
        P.init(this, &VD, "shaders/TaxiVert.spv", "shaders/TaxiFrag.spv", {&DSL});
        Pcity.init(this, &VDcity, "shaders/TaxiVert.spv", "shaders/TaxiFrag.spv", {&DSLcity});
        // Models, textures and Descriptors (values assigned to the uniforms)

        // Create models
        // The second parameter is the pointer to the vertex definition for this model
        // The third parameter is the file name
        // The last is a constant specifying the file type: currently only OBJ or GLTF
        Mtaxi.init(this, &VD, "Models/transport_purpose_003_transport_purpose_003.001.mgcg", MGCG );


        // Create the textures
        Tcity.init(this,"textures/Textures_City.png");

        nlohmann::json js;
        std::ifstream ifs("models/city.json");
        if (!ifs.is_open()) {
            std::cout << "Error! Scene file not found!";
            exit(-1);
        }
        try{
            json j;
            ifs>>j;

            for(int k = 0; k < MESH; k++) {
                std::string modelPath= j["models"][k]["model"];
                std::string format = j["models"][k]["format"];

                Mcity[k].init(this, &VDcity, modelPath, (format[0] == 'O') ? OBJ : ((format[0] == 'G') ? GLTF : MGCG));

            }
        }catch (const nlohmann::json::exception& e) {
            std::cout << e.what() << '\n';
        }

        // Init local variables
    }

    // Here you create your pipelines and Descriptor Sets!
    void pipelinesAndDescriptorSetsInit() {
        // This creates a new pipeline (with the current surface), using its shaders
        P.create();
        Pcity.create();

        // Here you define the data set
        DStaxi.init(this, &DSL, {
                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                {1, TEXTURE, 0, &Tcity},
                {2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr}
        });

        for(int i = 0; i < MESH; i++) {
            DScity[i].init(this, &DSLcity, {
                    {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                    {1, TEXTURE, 0, &Tcity},
                    {2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr}
            });
        }
    }

    // Here you destroy your pipelines and Descriptor Sets!
    // All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
    void pipelinesAndDescriptorSetsCleanup() {
        // Cleanup pipelines
        P.cleanup();
        Pcity.cleanup();

        // Cleanup datasets
        DStaxi.cleanup();

        for(int i = 0; i < MESH; i++) {
            DScity[i].cleanup();
        }
    }

    // Here you destroy all the Models, Texture and Desc. Set Layouts you created!
    // All the object classes defined in Starter.hpp have a method .cleanup() for this purpose
    // You also have to destroy the pipelines: since they need to be rebuilt, they have two different
    // methods: .cleanup() recreates them, while .destroy() delete them completely
    void localCleanup() {
        // Cleanup textures
        Tcity.cleanup();

        // Cleanup models
        Mtaxi.cleanup();

        for(int i = 0; i < MESH; i++) {
            Mcity[i].cleanup();
        }

        // Cleanup descriptor set layouts
        DSL.cleanup();
        DSLcity.cleanup();

        // Destroies the pipelines
        P.destroy();
        Pcity.destroy();
    }

    // Here it is the creation of the command buffer:
    // You send to the GPU all the objects you want to draw,
    // with their buffers and textures

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
        // binds the pipeline
        P.bind(commandBuffer);
        Pcity.bind(commandBuffer);
        // For a pipeline object, this command binds the corresponing pipeline to the command buffer passed in its parameter

        // binds the data set
        // For a Dataset object, this command binds the corresponing dataset
        // to the command buffer and pipeline passed in its first and second parameters.
        // The third parameter is the number of the set being bound
        // As described in the Vulkan tutorial, a different dataset is required for each image in the swap chain.
        // This is done automatically in file Starter.hpp, however the command here needs also the index
        // of the current image in the swap chain, passed in its last parameter

        // binds the model
        // For a Model object, this command binds the corresponing index and vertex buffer
        // to the command buffer passed in its parameter

        // record the drawing command in the command buffer
        // the second parameter is the number of indexes to be drawn. For a Model object,
        // this can be retrieved with the .indices.size() method.

        DStaxi.bind(commandBuffer, P, 0, currentImage);
        Mtaxi.bind(commandBuffer);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(Mtaxi.indices.size()), 1, 0, 0, 0);

        for(int i = 0; i < MESH; i++) {
            DScity[i].bind(commandBuffer, P, 0, currentImage);
            Mcity[i].bind(commandBuffer);
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(Mcity[i].indices.size()), 1, 0, 0, 0);
        }
    }

    // Here is where you update the uniforms.
    // Very likely this will be where you will be writing the logic of your application.
    void updateUniformBuffer(uint32_t currentImage) {

        static bool autoTime = true;
        static float cTime = 0.0f;
        const float turnTime = 72.0f;
		const float angTurnTimeFact = 2.0f * M_PI / turnTime;

        // Standard procedure to quit when the ESC key is pressed
        if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        // Integration with the timers and the controllers
        float deltaT;
        glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
        bool fire = false;
        getSixAxis(deltaT, m, r, fire);
        // getSixAxis() is defined in Starter.hpp in the base class.
        // It fills the float point variable passed in its first parameter with the time
        // since the last call to the procedure.
        // It fills vec3 in the second parameters, with three values in the -1,1 range corresponding
        // to motion (with left stick of the gamepad, or ASWD + RF keys on the keyboard)
        // It fills vec3 in the third parameters, with three values in the -1,1 range corresponding
        // to motion (with right stick of the gamepad, or Arrow keys + QE keys on the keyboard, or mouse)
        // If fills the last boolean variable with true if fire has been pressed:
        //          SPACE on the keyboard, A or B button on the Gamepad, Right mouse button

        if(autoTime) {
            cTime += deltaT;
            cTime = (cTime > turnTime) ? (cTime - turnTime) : cTime;
        }
        
        const float ROT_SPEED = glm::radians(240.0f);
        const float MOVE_SPEED = 4.0f;

        CamAlpha = CamAlpha - ROT_SPEED * deltaT * r.y;
        CamBeta = CamBeta - ROT_SPEED * deltaT * r.x;
        CamBeta = CamBeta < glm::radians(-90.0f) ? glm::radians(-90.0f) :
            (CamBeta > glm::radians(90.0f) ? glm::radians(90.0f) : CamBeta);

        glm::vec3 ux = glm::rotate(glm::mat4(1.0f), CamAlpha, glm::vec3(0, 1, 0)) * glm::vec4(1, 0, 0, 1);
        glm::vec3 uz = glm::rotate(glm::mat4(1.0f), CamAlpha, glm::vec3(0, 1, 0)) * glm::vec4(0, 0, -1, 1);
        CamPos = CamPos + MOVE_SPEED * m.x * ux * deltaT;
        CamPos = CamPos + MOVE_SPEED * m.y * glm::vec3(0, 1, 0) * deltaT;
        CamPos = CamPos + MOVE_SPEED * m.z * uz * deltaT;

        /* codice prof che c'era prima
        // Parameters
        // Camera FOV-y, Near Plane and Far Plane
        const float FOVy = glm::radians(90.0f);
        const float nearPlane = 0.1f;
        const float farPlane = 100.0f;

        glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
        Prj[1][1] *= -1;
        glm::vec3 camTarget = glm::vec3(0,0,0);
        glm::vec3 camPos    = camTarget + glm::vec3(6,3,10) / 2.0f;
        glm::mat4 View = glm::lookAt(camPos, camTarget, glm::vec3(0,1,0)); */
        
        const float nearPlane = 0.1f;
        const float farPlane = 250.0f;
        glm::mat4 Prj = glm::perspective(glm::radians(45.0f), Ar, nearPlane, farPlane);
        Prj[1][1] *= -1;

        glm::mat4 mView =  glm::rotate(glm::mat4(1.0), -CamBeta, glm::vec3(1, 0, 0)) *
                        glm::rotate(glm::mat4(1.0), -CamAlpha, glm::vec3(0, 1, 0)) *
                        glm::translate(glm::mat4(1.0), -CamPos);


        glm::mat4 mWorld;

        // the .map() method of a DataSet object, requires the current image of the swap chain as first parameter
        // the second parameter is the pointer to the C++ data structure to transfer to the GPU
        // the third parameter is its size
        // the fourth parameter is the location inside the descriptor set of this uniform block

        mWorld = glm::translate(glm::mat4(1), glm::vec3(0, 0, 3)) *
                glm::rotate(glm::mat4(1), glm::radians(180.0f), glm::vec3(0, 1, 0));
        uboTaxi.mvpMat = Prj * mView * mWorld;
        uboTaxi.mMat = mView * mWorld;
        uboTaxi.nMat = glm::transpose(glm::inverse(uboTaxi.mMat));
        DStaxi.map(currentImage, &uboTaxi, sizeof(uboTaxi), 0);
        guboTaxi.lightDir = glm::vec3(cos(glm::radians(135.0f)) * cos(cTime * angTurnTimeFact), sin(glm::radians(135.0f)), cos(glm::radians(135.0f)) * sin(cTime * angTurnTimeFact));
        guboTaxi.lightColor = glm::vec4(1.0f);
        guboTaxi.eyePos = CamPos;
        DStaxi.map(currentImage, &guboTaxi, sizeof(guboTaxi), 2);

        nlohmann::json js;
        std::ifstream ifs2("models/city.json");
        if (!ifs2.is_open()) {
            std::cout << "Error! Scene file not found!";
            exit(-1);
        }
        try{
            json j;
            ifs2>>j;

            float TMj[16];

            for(int k = 0; k < MESH; k++) {
                nlohmann::json TMjson = j["instances"][k]["transform"];
                for(int l = 0; l < 16; l++) {TMj[l] = TMjson[l];}
                mWorld=glm::mat4(TMj[0],TMj[4],TMj[8],TMj[12],TMj[1],TMj[5],TMj[9],TMj[13],TMj[2],TMj[6],TMj[10],TMj[14],TMj[3],TMj[7],TMj[11],TMj[15]);
                ubocity[k].mvpMat = Prj * mView * mWorld;
                ubocity[k].mMat = mView * mWorld;
                ubocity[k].nMat = glm::transpose(glm::inverse(ubocity[k].mMat));
                DScity[k].map(currentImage, &ubocity[k], sizeof(ubocity[k]), 0);
                gubocity[k].lightDir = glm::vec3(cos(glm::radians(135.0f)) * cos(cTime * angTurnTimeFact), sin(glm::radians(135.0f)), cos(glm::radians(135.0f)) * sin(cTime * angTurnTimeFact));
                gubocity[k].lightColor = glm::vec4(1.0f);
                gubocity[k].eyePos = CamPos;
                DScity[k].map(currentImage, &gubocity[k], sizeof(gubocity[k]), 2);
            }

        }catch (const nlohmann::json::exception& e) {
            std::cout << e.what() << '\n';
        }

    }
};


// This is the main: probably you do not need to touch this!
int main() {
    MeshLoader app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}