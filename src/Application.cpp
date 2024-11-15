// This has been adapted from the Vulkan tutorial

#include "headers/Starter.hpp"
#include <iostream>
#include <fstream>
#define MESH 209

// float : alignas(4), vec2  : alignas(8), vec3, vec4, mat3, mat4  : alignas(16)
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

struct Vertex {
    glm::vec3 pos;
    glm::vec2 UV;
    glm::vec3 normal;
};

class Application : public BaseProject {
protected:

    // aspect ratio
    float Ar;

    DescriptorSetLayout DSL,DSLcity;

    VertexDescriptor VD, VDcity;

    Pipeline P, Pcity;

    Model<Vertex>  Mtaxi;
    Model<Vertex> Mcity[MESH];

    DescriptorSet DStaxi, DScity[MESH];

    Texture Tcity;


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

    void onWindowResize(int w, int h) {
        Ar = (float)w / (float)h;
    }

    void localInit() {

        DSL.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
        });
        DSLcity.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
        });

        VD.init(this, {
                {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
        }, {
                        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                                sizeof(glm::vec3), POSITION},
                        {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                                sizeof(glm::vec2), UV},
                        {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal),
                                sizeof(glm::vec3), NORMAL}
                });
        VDcity.init(this, {
                {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
        }, {
                        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                                sizeof(glm::vec3), POSITION},
                        {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                                sizeof(glm::vec2), UV},
                        {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal),
                                sizeof(glm::vec3), NORMAL}
                });

        P.init(this, &VD, "shaders/TaxiVert.spv", "shaders/TaxiFrag.spv", {&DSL});
        Pcity.init(this, &VDcity, "shaders/TaxiVert.spv", "shaders/TaxiFrag.spv", {&DSLcity});

        Mtaxi.init(this, &VD, "Models/transport_purpose_003_transport_purpose_003.001.mgcg", MGCG );

        Tcity.init(this,"textures/Textures_City.png");

        nlohmann::json js;
        std::ifstream ifs("models/city.json");
        if (!ifs.is_open()) {
            std::cout << "[ ERROR ]: Scene file not found!" << std::endl;
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
            std::cout << "[ EXCEPTION ]: " << e.what() << std::endl;
        }
    }

    void pipelinesAndDescriptorSetsInit() {

        P.create();
        Pcity.create();

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

    void pipelinesAndDescriptorSetsCleanup() {
        P.cleanup();
        Pcity.cleanup();

        DStaxi.cleanup();

        for(int i = 0; i < MESH; i++) {
            DScity[i].cleanup();
        }
    }

    void localCleanup() {

        Tcity.cleanup();

        Mtaxi.cleanup();

        for(int i = 0; i < MESH; i++) {
            Mcity[i].cleanup();
        }

        DSL.cleanup();
        DSLcity.cleanup();

        P.destroy();
        Pcity.destroy();
    }

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

        P.bind(commandBuffer);
        Pcity.bind(commandBuffer);

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

    // main application loop
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
        
        const float nearPlane = 0.1f;
        const float farPlane = 250.0f;
        glm::mat4 Prj = glm::perspective(glm::radians(45.0f), Ar, nearPlane, farPlane);
        Prj[1][1] *= -1;

        glm::mat4 mView =  glm::rotate(glm::mat4(1.0), -CamBeta, glm::vec3(1, 0, 0)) *
                        glm::rotate(glm::mat4(1.0), -CamAlpha, glm::vec3(0, 1, 0)) *
                        glm::translate(glm::mat4(1.0), -CamPos);


        glm::mat4 mWorld;
        mWorld = glm::translate(glm::mat4(1), glm::vec3(0, 0, 3)) * glm::rotate(glm::mat4(1), glm::radians(180.0f), glm::vec3(0, 1, 0));


    
        uboTaxi.mvpMat = Prj * mView * mWorld;
        uboTaxi.mMat = glm::mat4(1);
        uboTaxi.nMat = glm::inverse(glm::transpose(uboTaxi.mMat));
        DStaxi.map(currentImage, &uboTaxi, sizeof(uboTaxi), 0);
        guboTaxi.lightDir = glm::vec3(cos(glm::radians(135.0f)) * cos(cTime * angTurnTimeFact), sin(glm::radians(135.0f)), cos(glm::radians(135.0f)) * sin(cTime * angTurnTimeFact));
        guboTaxi.lightColor = glm::vec4(1.0f);
        guboTaxi.eyePos = CamPos;
        DStaxi.map(currentImage, &guboTaxi, sizeof(guboTaxi), 2);

        nlohmann::json js;
        std::ifstream ifs2("models/city.json");
        if (!ifs2.is_open()) {
            std::cout << "[ ERROR ]: Scene file not found!" << std::endl;
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
                ubocity[k].mMat = glm::mat4(1);
                ubocity[k].nMat = glm::inverse(glm::transpose(ubocity[k].mMat));
                ubocity[k].mvpMat = Prj * mView * mWorld;
                DScity[k].map(currentImage, &ubocity[k], sizeof(ubocity[k]), 0);
                gubocity[k].lightDir = glm::vec3(cos(glm::radians(135.0f)) * cos(cTime * angTurnTimeFact), sin(glm::radians(135.0f)), cos(glm::radians(135.0f)) * sin(cTime * angTurnTimeFact));
                gubocity[k].lightColor = glm::vec4(1.0f);
                gubocity[k].eyePos = CamPos;
                DScity[k].map(currentImage, &gubocity[k], sizeof(gubocity[k]), 2);
            }

        }catch (const nlohmann::json::exception& e) {
            std::cout << "[ EXCEPTION ]: " << e.what() << std::endl;
        }

    }
};


// This is the main: probably you do not need to touch this!
int main() {
    Application app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "[ EXCEPTION ]:" << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}