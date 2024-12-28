// This has been adapted from the Vulkan tutorial
#include "headers/Starter.hpp"
#include "headers/TextMaker.hpp"
#include <iostream>
#include <fstream>

#define MESH 210
#define CARS 9
#define SPHERES 37
#define PEOPLE 40

#define DEBUG 0

std::vector<SingleText> outText = {
        {2, {"Third person view", "Press SPACE to access photo mode","",""}, 0, 0},
        {2, {"Photo mode", "Press SPACE to access third person view", "",""}, 0, 0}
};

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
    alignas(4) float gamma;
    alignas(4) float metallic;
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

    DescriptorSetLayout DSL,DSLcity, DSLsky, DSLcars, DSLpeople;

    VertexDescriptor VD, VDcity, VDsky, VDcars, VDpeople;

    Pipeline P, Pcity, Psky, Pcars, Ppeople;

    TextMaker txt;

    Model  Mtaxi, Msky, Mcars[CARS], Mpeople[PEOPLE];
    Model Mcity[MESH];

    DescriptorSet DStaxi, DScity[MESH], DSsky, DScars[CARS], DSpeople[PEOPLE];

    Texture Tcity, Tsky, Tpeople;


    UniformBufferObject uboTaxi, uboSky, uboCars[CARS];
    UniformBufferObject ubocity[MESH], uboPeople[PEOPLE];
    GlobalUniformBufferObject guboTaxi,guboSky, guboCars[CARS];
    GlobalUniformBufferObject gubocity[MESH], guboPeople[PEOPLE];

#if DEBUG
    DescriptorSetLayout DSLsphere;
        VertexDescriptor VDsphere;
        Pipeline Psphere;
        Model Msphere[SPHERES];
        DescriptorSet DSsphere[SPHERES];
        UniformBufferObject ubosphere[SPHERES];
#endif

    // Other application parameters
    int currScene = 0;
    glm::vec3 camPos = glm::vec3(0.0, 1.5f, -5.0f); //initial pos of camera
    glm::vec3 camPosInPhotoMode;
    glm::vec3 taxiPos = glm::vec3(0.0, -0.2, 0.0); //initial pos of taxi

    glm::vec3 carPos1 = glm::vec3(-72.0, -0.2, 36.0);
    glm::vec3 carPos2 = glm::vec3(5.0, -0.2, 36.0); //initial pos of car
    glm::vec3 carPos3 = glm::vec3(72.0, -0.2, 36.0);
    glm::vec3 carPos4 = glm::vec3(-36.0, -0.2, -108.0);
    glm::vec3 carPos5 = glm::vec3(36.0, -0.2, -108.0);
    glm::vec3 carPos6 = glm::vec3(108.0, -0.2, -108.0);
    glm::vec3 carPos7 = glm::vec3(-36.0, -0.2, -112.0);
    glm::vec3 carPos8 = glm::vec3(36.0, -0.2, -112.0);
    glm::vec3 carPos9 = glm::vec3(108.0, -0.2, -112.0);


    std::vector<glm::vec3> wayPoints1={glm::vec3(-69.0f, -0.2f, 33.0f),
                                       glm::vec3(-69.0f, -0.2f, -33.0f),
                                       glm::vec3(-3.0f, -0.2f, -33.0f),
                                       glm::vec3(-3.0f, -0.2f, 33.0f)};
    std::vector<glm::vec3> wayPoints2={glm::vec3(3.0f, -0.2f, 33.0f),
                                       glm::vec3(3.0f, -0.2f, -33.0f),
                                       glm::vec3(69.0f, -0.2f, -33.0f),
                                       glm::vec3(69.0f, -0.2f, 33.0f)};
    std::vector<glm::vec3> wayPoints3={glm::vec3(75.0f, -0.2f, 33.0f),
                                       glm::vec3(75.0f, -0.2f, -33.0f),
                                       glm::vec3(141.0f, -0.2f, -33.0f),
                                       glm::vec3(141.0f, -0.2f, 33.0f)};
    std::vector<glm::vec3> wayPoints4={glm::vec3(-3.0f, -0.2f, -105.0f),
                                       glm::vec3(-3.0f, -0.2f, -39.0f),
                                       glm::vec3(-69.0f, -0.2f, -39.0f),
                                       glm::vec3(-69.0f, -0.2f, -105.0f)};
    std::vector<glm::vec3> wayPoints5={glm::vec3(69.0f, -0.2f, -105.0f),
                                       glm::vec3(69.0f, -0.2f, -39.0f),
                                       glm::vec3(3.0f, -0.2f, -39.0f),
                                       glm::vec3(3.0f, -0.2f, -105.0f)};
    std::vector<glm::vec3> wayPoints6={glm::vec3(141.0f, -0.2f, -105.0f),
                                       glm::vec3(141.0f, -0.2f, -39.0f),
                                       glm::vec3(75.0f, -0.2f, -39.0f),
                                       glm::vec3(75.0f, -0.2f, -105.0f)};
    std::vector<glm::vec3> wayPoints7={glm::vec3(-69.0f, -0.2f, -111.0f),
                                       glm::vec3(-69.0f, -0.2f, -177.0f),
                                       glm::vec3(-3.0f, -0.2f, -177.0f),
                                       glm::vec3(-3.0f, -0.2f, -111.0f)};
    std::vector<glm::vec3> wayPoints8={glm::vec3(3.0f, -0.2f, -111.0f),
                                       glm::vec3(3.0f, -0.2f, -177.0f),
                                       glm::vec3(69.0f, -0.2f, -177.0f),
                                       glm::vec3(69.0f, -0.2f, -111.0f)};
    std::vector<glm::vec3> wayPoints9={glm::vec3(75.0f, -0.2f, -111.0f),
                                       glm::vec3(75.0f, -0.2f, -177.0f),
                                       glm::vec3(141.0f, -0.2f, -177.0f),
                                       glm::vec3(141.0f, -0.2f, -111.0f)};

    int currentPoint1=0;
    int currentPoint2=0;
    int currentPoint3=0;
    int currentPoint4=0;
    int currentPoint5=0;
    int currentPoint6=0;
    int currentPoint7=0;
    int currentPoint8=0;
    int currentPoint9=0;
    float CamAlpha = 0.0f;
    float CamBeta = 0.0f;
    bool alreadyInPhotoMode = false;

    // Here you set the main application parameters
    void setWindowParameters() {

        windowWidth = 1280;
        windowHeight = 720;
        windowTitle = "Computer graphics' project";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = {0.0f, 0.005f, 0.01f, 1.0f};

        // Descriptor pool sizes
        uniformBlocksInPool =  (2 * MESH) + 2+2+2*CARS+2*PEOPLE + (DEBUG ? 2 * SPHERES : 0);
        texturesInPool = MESH + 1 +1+1+CARS+PEOPLE; //city, taxi, text, sky, autonomous cars, people
        setsInPool = MESH + 1 +1+1+CARS+PEOPLE+ (DEBUG ? SPHERES : 0);

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
        DSLsky.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
        });
        DSLcars.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
        });
        DSLpeople.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
                {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
        });

#if DEBUG
        DSLsphere.init(this, {
                    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
            });
#endif

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
        VDsky.init(this, {
                {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
        }, {
                           {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                                   sizeof(glm::vec3), POSITION},
                           {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                                   sizeof(glm::vec2), UV},
                           {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal),
                                   sizeof(glm::vec3), NORMAL}
                   });
        VDcars.init(this, {
                {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
        }, {
                            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                                    sizeof(glm::vec3), POSITION},
                            {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                                    sizeof(glm::vec2), UV},
                            {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal),
                                    sizeof(glm::vec3), NORMAL}
                    });
        VDpeople.init(this, {
                {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
        }, {
                              {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                                      sizeof(glm::vec3), POSITION},
                              {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                                      sizeof(glm::vec2), UV},
                              {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal),
                                      sizeof(glm::vec3), NORMAL}
                      });

#if DEBUG
        VDsphere.init(this, {
                    {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                                    sizeof(glm::vec3), POSITION},
                            {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                                    sizeof(glm::vec2), UV},
                            {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal),
                                    sizeof(glm::vec3), NORMAL}
                    });
#endif

        P.init(this, &VD, "shaders/BaseVert.spv", "shaders/TaxiFrag.spv", {&DSL});
        Pcity.init(this, &VDcity, "shaders/BaseVert.spv", "shaders/TaxiFrag.spv", {&DSLcity});
        Pcity.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
        Psky.init(this, &VDsky, "shaders/SkyVert.spv", "shaders/SkyFrag.spv", {&DSLsky});
        Psky.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false); //todo cosa dovevamo fare quando telecamera dentro una stanza
        Pcars.init(this, &VDcars, "shaders/BaseVert.spv", "shaders/TaxiFrag.spv", {&DSLcars});
        Ppeople.init(this, &VDpeople, "shaders/BaseVert.spv", "shaders/TaxiFrag.spv", {&DSLpeople});

#if DEBUG
        Psphere.init(this, &VDsphere, "shaders/BaseVert.spv", "shaders/DEBUGFrag.spv", {&DSLsphere});
#endif


        Mtaxi.init(this, &VD, "models/transport_purpose_003_transport_purpose_003.001.mgcg", MGCG );
        //Mtaxi.init(this, &VD, "models/uploads-files-2104560-Humano_01Business_01_30K.obj", OBJ );
        Msky.init(this, &VDsky, "models/Sphere2.obj", OBJ);
        Mcars[0].init(this, &VDcars, "models/transport_cool_001_transport_cool_001.001.mgcg" , MGCG);
        Mcars[1].init(this, &VDcars, "models/transport_cool_003_transport_cool_003.001.mgcg" , MGCG);
        Mcars[2].init(this, &VDcars, "models/transport_cool_004_transport_cool_004.001.mgcg" , MGCG);
        Mcars[3].init(this, &VDcars, "models/transport_cool_010_transport_cool_010.001.mgcg" , MGCG);
        Mcars[4].init(this, &VDcars, "models/transport_jeep_001_transport_jeep_001.001.mgcg" , MGCG);
        Mcars[5].init(this, &VDcars, "models/transport_jeep_010_transport_jeep_010.001.mgcg" , MGCG);
        Mcars[6].init(this, &VDcars, "models/transport_cool_001_transport_cool_001.001.mgcg" , MGCG);
        Mcars[7].init(this, &VDcars, "models/transport_cool_004_transport_cool_004.001.mgcg" , MGCG);
        Mcars[8].init(this, &VDcars, "models/transport_cool_010_transport_cool_010.001.mgcg" , MGCG);

#if DEBUG
        for (int i = 0; i < SPHERES; i++) {
                Msphere[i].init(this, &VDsphere, "models/Sphere2.obj", OBJ);
            }
#endif

        Tcity.init(this,"textures/Textures_City.png");
        Tsky.init(this, "textures/images.png");
        Tpeople.init(this, "textures/Textures_City.png");

        txt.init(this, &outText);

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

        nlohmann::json js2;
        std::ifstream ifs2("models/people.json");
        if (!ifs2.is_open()) {
            std::cout << "[ ERROR ]: Scene file not found!" << std::endl;
            exit(-1);
        }
        try{
            json j2;
            ifs2>>j2;

            for(int k = 0; k < PEOPLE; k++) {
                std::string modelPath= j2["models"][k]["model"];
                std::string format = j2["models"][k]["format"];

                Mpeople[k].init(this, &VDpeople, modelPath, (format[0] == 'O') ? OBJ : ((format[0] == 'G') ? GLTF : MGCG));

            }
        }catch (const nlohmann::json::exception& e) {
            std::cout << "[ EXCEPTION ]: " << e.what() << std::endl;
        }
    }

    void pipelinesAndDescriptorSetsInit() {

        P.create();
        Pcity.create();
        Psky.create();
        Pcars.create();
        Ppeople.create();

#if DEBUG
        Psphere.create();
#endif

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

        DSsky.init(this, &DSLsky, {
                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                {1, TEXTURE, 0, &Tsky},
                {2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr}
        });

        for(int i = 0; i < CARS; i++) {
            DScars[i].init(this, &DSLcars, {
                    {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                    {1, TEXTURE, 0, &Tcity},
                    {2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr}
            });
        }
        for(int i = 0; i < PEOPLE; i++) {
            DSpeople[i].init(this, &DSLpeople, {
                    {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                    {1, TEXTURE, 0, &Tpeople},
                    {2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr}
            });
        }

#if DEBUG
        for (int i = 0; i < SPHERES; i++) {
                DSsphere[i].init(this, &DSLsphere, {
                        {0, UNIFORM, sizeof(UniformBufferObject), nullptr}
                });
            }
#endif

        txt.pipelinesAndDescriptorSetsInit();
    }

    void pipelinesAndDescriptorSetsCleanup() {

        P.cleanup();
        Pcity.cleanup();
        Psky.cleanup();
        Pcars.cleanup();
        Ppeople.cleanup();

#if DEBUG
        Psphere.cleanup();
#endif

        DStaxi.cleanup();

        for(int i = 0; i < MESH; i++) {
            DScity[i].cleanup();
        }

        DSsky.cleanup();

        for(int i = 0; i < CARS; i++) {
            DScars[i].cleanup();
        }

        for(int i = 0; i < PEOPLE; i++) {
            DSpeople[i].cleanup();
        }

#if DEBUG
        for (int i = 0; i < SPHERES; i++) {
                DSsphere[i].cleanup();
            }
#endif

        txt.pipelinesAndDescriptorSetsCleanup();
    }

    void localCleanup() {

        Tcity.cleanup();
        Tsky.cleanup();
        Tpeople.cleanup();

        Mtaxi.cleanup();

        for(int i = 0; i < MESH; i++) {
            Mcity[i].cleanup();
        }

        Msky.cleanup();

        for(int i = 0; i < CARS; i++) {
            Mcars[i].cleanup();
        }
        for(int i = 0; i < PEOPLE; i++) {
            Mpeople[i].cleanup();
        }

#if DEBUG
        for (int i = 0; i < SPHERES; i++) {
                Msphere[i].cleanup();
            }
#endif

        DSL.cleanup();
        DSLcity.cleanup();
        DSLsky.cleanup();
        DSLcars.cleanup();
        DSLpeople.cleanup();

#if DEBUG
        DSLsphere.cleanup();
#endif

        P.destroy();
        Pcity.destroy();
        Psky.destroy();
        Pcars.destroy();
        Ppeople.destroy();

#if DEBUG
        Psphere.destroy();
#endif

        txt.localCleanup();
    }

    void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

        P.bind(commandBuffer);

        DStaxi.bind(commandBuffer, P, 0, currentImage);
        Mtaxi.bind(commandBuffer);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(Mtaxi.indices.size()), 1, 0, 0, 0);

        Pcity.bind(commandBuffer);

        for(int i = 0; i < MESH; i++) {
            DScity[i].bind(commandBuffer, Pcity, 0, currentImage);
            Mcity[i].bind(commandBuffer);
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(Mcity[i].indices.size()), 1, 0, 0, 0);
        }

        Psky.bind(commandBuffer);

        DSsky.bind(commandBuffer, Psky, 0, currentImage);
        Msky.bind(commandBuffer);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(Msky.indices.size()), 1, 0, 0, 0);

        Pcars.bind(commandBuffer);

        for(int i = 0; i < CARS; i++) {
            DScars[i].bind(commandBuffer, Pcars, 0, currentImage);
            Mcars[i].bind(commandBuffer);
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(Mcars[i].indices.size()), 1, 0, 0, 0);
        }

        Ppeople.bind(commandBuffer);

        for(int i = 0; i < PEOPLE; i++) {
            DSpeople[i].bind(commandBuffer, Ppeople, 0, currentImage);
            Mpeople[i].bind(commandBuffer);
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(Mpeople[i].indices.size()), 1, 0, 0, 0);
        }


#if DEBUG
        Psphere.bind(commandBuffer);

            for (int i = 0; i < SPHERES; i++) {
                DSsphere[i].bind(commandBuffer, Psphere, 0, currentImage);
                Msphere[i].bind(commandBuffer);
                vkCmdDrawIndexed(commandBuffer,
                                 static_cast<uint32_t>(Msphere[i].indices.size()), 1, 0, 0, 0);
            }
#endif

        txt.populateCommandBuffer(commandBuffer, currentImage, currScene);
    }

    // main application loop
    void updateUniformBuffer(uint32_t currentImage) {
        static bool debounce = false;
        static int curDebounce = 0;

        static bool autoTime = true;
        static float cTime = 0.0f;
        const float turnTime = 72.0f;
        const float angTurnTimeFact = 2.0f * M_PI / turnTime;

        glm::vec3 direction1 = glm::normalize(wayPoints1[currentPoint1] - carPos1);
        glm::vec3 direction2 = glm::normalize(wayPoints2[currentPoint2] - carPos2);
        glm::vec3 direction3 = glm::normalize(wayPoints3[currentPoint3] - carPos3);
        glm::vec3 direction4 = glm::normalize(wayPoints4[currentPoint4] - carPos4);
        glm::vec3 direction5 = glm::normalize(wayPoints5[currentPoint5] - carPos5);
        glm::vec3 direction6 = glm::normalize(wayPoints6[currentPoint6] - carPos6);
        glm::vec3 direction7 = glm::normalize(wayPoints7[currentPoint7] - carPos7);
        glm::vec3 direction8 = glm::normalize(wayPoints8[currentPoint8] - carPos8);
        glm::vec3 direction9 = glm::normalize(wayPoints9[currentPoint9] - carPos9);

        float speedCar= 4.0f;

        // Standard procedure to quit when the ESC key is pressed
        if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        if(glfwGetKey(window, GLFW_KEY_SPACE)) {
            if(!debounce) {
                debounce = true;
                curDebounce = GLFW_KEY_SPACE;
                if(currScene==0) {
                    currScene = 1;
                } else {
                    currScene = 0;
                }
                std::cout << "Scene : " << currScene << "\n";

                RebuildPipeline();
            }
        } else {
            if((curDebounce == GLFW_KEY_SPACE) && debounce) {
                debounce = false;
                curDebounce = 0;
            }
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
        // to motion (with left stick of the gamepad, or WASD + RF keys on the keyboard)
        // It fills vec3 in the third parameters, with three values in the -1,1 range corresponding
        // to motion (with right stick of the gamepad, or Arrow keys + QE keys on the keyboard, or mouse)
        // If fills the last boolean variable with true if fire has been pressed:
        //          SPACE on the keyboard, A or B button on the Gamepad, Right mouse button

        if (autoTime) {
            cTime += deltaT;
            cTime = (cTime > turnTime) ? (cTime - turnTime) : cTime;
        }


        carPos1 += direction1 * speedCar * deltaT;
        if (glm::distance(carPos1, wayPoints1[currentPoint1]) < 0.25f) {
            currentPoint1 = (currentPoint1 + 1) % wayPoints1.size();
        }
        static float steeringAngCar1 = 0.0f;
        float targetSteering1 = atan2(direction1.x, direction1.z);
        //steeringAngCar1 += (targetSteering1 - steeringAngCar1) * 0.1f;
        float angleDifference1 = targetSteering1 - steeringAngCar1;
        angleDifference1 = fmod(angleDifference1 + M_PI, 2.0f * M_PI) - M_PI;
        steeringAngCar1 += angleDifference1 * 0.1f;

        carPos2 += direction2 * speedCar * deltaT;
        if (glm::distance(carPos2, wayPoints2[currentPoint2]) < 0.1f) {
            currentPoint2 = (currentPoint2 + 1) % wayPoints2.size();
        }
        static float steeringAngCar2 = 0.0f;
        float targetSteering2 = atan2(direction2.x, direction2.z);
        float angleDifference2 = targetSteering2 - steeringAngCar2;
        angleDifference2 = fmod(angleDifference2 + M_PI, 2.0f * M_PI) - M_PI;
        steeringAngCar2 += angleDifference2 * 0.1f;
        //carPos += glm::vec3(speedCar * sin(steeringAngCar), 0.0f, speedCar * cos(steeringAngCar)) * deltaT;

        carPos3 += direction3 * speedCar * deltaT;
        if (glm::distance(carPos3, wayPoints3[currentPoint3]) < 0.1f) {
            currentPoint3 = (currentPoint3 + 1) % wayPoints3.size();
        }
        static float steeringAngCar3 = 0.0f;
        float targetSteering3 = atan2(direction3.x, direction3.z);
        float angleDifference3 = targetSteering3 - steeringAngCar3;
        angleDifference3 = fmod(angleDifference3 + M_PI, 2.0f * M_PI) - M_PI;
        steeringAngCar3 += angleDifference3 * 0.1f;

        carPos4 += direction4 * speedCar * deltaT;
        if (glm::distance(carPos4, wayPoints4[currentPoint4]) < 0.1f) {
            currentPoint4 = (currentPoint4 + 1) % wayPoints4.size();
        }
        static float steeringAngCar4 = 0.0f;
        float targetSteering4 = atan2(direction4.x, direction4.z);
        float angleDifference4 = targetSteering4 - steeringAngCar4;
        angleDifference4 = fmod(angleDifference4 + M_PI, 2.0f * M_PI) - M_PI;
        steeringAngCar4 += angleDifference4 * 0.1f;

        carPos5 += direction5 * speedCar * deltaT;
        if (glm::distance(carPos5, wayPoints5[currentPoint5]) < 0.1f) {
            currentPoint5 = (currentPoint5 + 1) % wayPoints5.size();
        }
        static float steeringAngCar5 = 0.0f;
        float targetSteering5 = atan2(direction5.x, direction5.z);
        float angleDifference5 = targetSteering5 - steeringAngCar5;
        angleDifference5 = fmod(angleDifference5 + M_PI, 2.0f * M_PI) - M_PI;
        steeringAngCar5 += angleDifference5 * 0.1f;

        carPos6 += direction6 * speedCar * deltaT;
        if (glm::distance(carPos6, wayPoints6[currentPoint6]) < 0.1f) {
            currentPoint6 = (currentPoint6 + 1) % wayPoints6.size();
        }
        static float steeringAngCar6 = 0.0f;
        float targetSteering6 = atan2(direction6.x, direction6.z);
        float angleDifference6 = targetSteering6 - steeringAngCar6;
        angleDifference6 = fmod(angleDifference6 + M_PI, 2.0f * M_PI) - M_PI;
        steeringAngCar6 += angleDifference6 * 0.1f;

        carPos7 += direction7 * speedCar * deltaT;
        if (glm::distance(carPos7, wayPoints7[currentPoint7]) < 0.1f) {
            currentPoint7 = (currentPoint7 + 1) % wayPoints7.size();
        }
        static float steeringAngCar7 = 0.0f;
        float targetSteering7 = atan2(direction7.x, direction7.z);
        float angleDifference7 = targetSteering7 - steeringAngCar7;
        angleDifference7 = fmod(angleDifference7 + M_PI, 2.0f * M_PI) - M_PI;
        steeringAngCar7 += angleDifference7 * 0.1f;

        carPos8 += direction8 * speedCar * deltaT;
        if (glm::distance(carPos8, wayPoints8[currentPoint8]) < 0.1f) {
            currentPoint8 = (currentPoint8 + 1) % wayPoints8.size();
        }
        static float steeringAngCar8 = 0.0f;
        float targetSteering8 = atan2(direction8.x, direction8.z);
        float angleDifference8 = targetSteering8 - steeringAngCar8;
        angleDifference8 = fmod(angleDifference8 + M_PI, 2.0f * M_PI) - M_PI;
        steeringAngCar8 += angleDifference8 * 0.1f;

        carPos9 += direction9 * speedCar * deltaT;
        if (glm::distance(carPos9, wayPoints9[currentPoint9]) < 0.1f) {
            currentPoint9 = (currentPoint9 + 1) % wayPoints9.size();
        }
        static float steeringAngCar9 = 0.0f;
        float targetSteering9 = atan2(direction9.x, direction9.z);
        float angleDifference9 = targetSteering9 - steeringAngCar9;
        angleDifference9 = fmod(angleDifference9 + M_PI, 2.0f * M_PI) - M_PI;
        steeringAngCar9 += angleDifference9 * 0.1f;

        static float steeringAng = 0.0f;
        glm::mat4 mView;

        if(currScene == 0) {
            alreadyInPhotoMode = false;
            // Third person view
            const float steeringSpeed = glm::radians(45.0f);
            const float moveSpeed = 7.5f;

            static float currentSpeed = 0.0f;
            float targetSpeed = moveSpeed * -m.z;
            const float dampingFactor = 3.0f; // Adjust this value to control the damping effect
            currentSpeed += (targetSpeed - currentSpeed) * dampingFactor * deltaT;
            float speed = currentSpeed * deltaT;
            if (speed != 0.0f) {
                steeringAng += (speed > 0 ? -m.x : m.x) * steeringSpeed * deltaT;
                taxiPos = taxiPos + glm::vec3(speed * sin(steeringAng), 0.0f, speed * cos(steeringAng));
            }

            //update camera position for lookAt view, keeping into account the current SteeringAng
            float x, y;
            float radius = 5.0f;
            static float camOffsetAngle = 0.0f;
            const float camRotationSpeed = glm::radians(90.0f);

            // Update camera offset angle based on mouse movement
            if (r.y != 0.0f) {
                camOffsetAngle += camRotationSpeed * deltaT * r.y;
            }

            // Calculate the camera position around the taxi in a circular path with optional offset angle from the user
            x = -radius * sin(steeringAng + camOffsetAngle);
            y = -radius * cos(steeringAng + camOffsetAngle);
            camPos = glm::vec3(taxiPos.x + x, taxiPos.y + 1.5f, taxiPos.z + y);
            mView = glm::lookAt(camPos,
                                taxiPos,
                                glm::vec3(0, 1, 0));
            //REMEMBER: primo parametro: spostamento dx e sx, secondo parametro: altezza su e giù, terzo avanti e indietro
        } else if (currScene == 1){

            // Photo mode
            if(!alreadyInPhotoMode) {
                //TODO: set the camera orientation towards the taxi
                camPosInPhotoMode = camPos;
                alreadyInPhotoMode = true;
            }
            const float ROT_SPEED2 = glm::radians(240.0f);
            const float MOVE_SPEED2 = 7.5f;

            CamAlpha = CamAlpha - ROT_SPEED2 * deltaT * r.y;
            CamBeta = CamBeta - ROT_SPEED2 * deltaT * r.x;
            CamBeta = CamBeta < glm::radians(-90.0f) ? glm::radians(-90.0f) :
                      (CamBeta > glm::radians(90.0f) ? glm::radians(90.0f) : CamBeta);

            glm::vec3 ux = glm::rotate(glm::mat4(1.0f), CamAlpha, glm::vec3(0, 1, 0)) * glm::vec4(1, 0, 0, 1);
            glm::vec3 uz = glm::rotate(glm::mat4(1.0f), CamAlpha, glm::vec3(0, 1, 0)) * glm::vec4(0, 0, -1, 1);
            camPosInPhotoMode = camPosInPhotoMode + MOVE_SPEED2 * m.x * ux * deltaT;
            camPosInPhotoMode = camPosInPhotoMode + MOVE_SPEED2 * m.y * glm::vec3(0, 1, 0) * deltaT;
            camPosInPhotoMode = camPosInPhotoMode + MOVE_SPEED2 * -m.z * uz * deltaT;

            mView = glm::rotate(glm::mat4(1.0), -CamBeta, glm::vec3(1, 0, 0)) *
                    glm::rotate(glm::mat4(1.0), -CamAlpha, glm::vec3(0, 1, 0)) *
                    glm::translate(glm::mat4(1.0), -camPosInPhotoMode); //View matrix


        }

        const float nearPlane = 1.0f;
        const float farPlane = 280.0f;
        glm::mat4 Prj = glm::perspective(glm::radians(45.0f), Ar, nearPlane, farPlane);
        Prj[1][1] *= -1; //Projection matrix


        glm::mat4 mWorld; //World matrix for city
        mWorld = glm::translate(glm::mat4(1), glm::vec3(0, 0, 3)) * glm::rotate(glm::mat4(1), glm::radians(180.0f), glm::vec3(0, 1, 0));


        //glm::vec3 sunPos = glm::vec3(5.5f, 30.0f, 7.5f);
        //glm::vec3 sunPos = glm::vec3(cos(glm::radians(135.0f)) * cos(cTime * angTurnTimeFact), sin(glm::radians(135.0f)), cos(glm::radians(135.0f)) * sin(cTime * angTurnTimeFact));

        glm::vec3 sunPos = glm::vec3(100.0f * cos(cTime * angTurnTimeFact), // x
                                     100.0f * sin(cTime * angTurnTimeFact), // y
                                     100.0f * sin(glm::radians(90.0f))              // z (fisso)
        );

#if DEBUG
        int index = SPHERES - 1;
            glm::mat4 mWorldSphere = glm::translate(glm::mat4(1.0f), sunPos);
            ubosphere[index].mvpMat = Prj * mView * mWorldSphere;
            ubosphere[index].mMat = mWorldSphere;
            ubosphere[index].nMat = glm::inverse(glm::transpose(ubosphere[index].mMat));
            DSsphere[index].map(currentImage, &ubosphere[index], sizeof(ubosphere[index]), 0);
#endif


        glm::mat4 mWorldTaxi =
                glm::translate(glm::mat4(1.0), taxiPos) *
                glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0));


        glm::mat4 mWorldCar1 =
                glm::translate(glm::mat4(1.0), carPos1)*
                glm::rotate(glm::mat4(1.0), steeringAngCar1, glm::vec3(0, 1, 0));
        glm::mat4 mWorldCar2 =
                glm::translate(glm::mat4(1.0), carPos2)*
                glm::rotate(glm::mat4(1.0), steeringAngCar2, glm::vec3(0, 1, 0));
        glm::mat4 mWorldCar3 =
                glm::translate(glm::mat4(1.0), carPos3)*
                glm::rotate(glm::mat4(1.0), steeringAngCar3, glm::vec3(0, 1, 0));
        glm::mat4 mWorldCar4 =
                glm::translate(glm::mat4(1.0), carPos4)*
                glm::rotate(glm::mat4(1.0), steeringAngCar4, glm::vec3(0, 1, 0));
        glm::mat4 mWorldCar5 =
                glm::translate(glm::mat4(1.0), carPos5)*
                glm::rotate(glm::mat4(1.0), steeringAngCar5, glm::vec3(0, 1, 0));
        glm::mat4 mWorldCar6 =
                glm::translate(glm::mat4(1.0), carPos6)*
                glm::rotate(glm::mat4(1.0), steeringAngCar6, glm::vec3(0, 1, 0));
        glm::mat4 mWorldCar7 =
                glm::translate(glm::mat4(1.0), carPos7)*
                glm::rotate(glm::mat4(1.0), steeringAngCar7, glm::vec3(0, 1, 0));
        glm::mat4 mWorldCar8 =
                glm::translate(glm::mat4(1.0), carPos8)*
                glm::rotate(glm::mat4(1.0), steeringAngCar8, glm::vec3(0, 1, 0));
        glm::mat4 mWorldCar9 =
                glm::translate(glm::mat4(1.0), carPos9)*
                glm::rotate(glm::mat4(1.0), steeringAngCar9, glm::vec3(0, 1, 0));


        uboTaxi.mvpMat = Prj * mView * mWorldTaxi;
        uboTaxi.mMat = glm::mat4(1.0f);
        uboTaxi.nMat = glm::inverse(glm::transpose(uboTaxi.mMat));
        DStaxi.map(currentImage, &uboTaxi, sizeof(uboTaxi), 0);
        //guboTaxi.lightDir = glm::vec3(cos(glm::radians(135.0f)) * cos(cTime * angTurnTimeFact), sin(glm::radians(135.0f)), cos(glm::radians(135.0f)) * sin(cTime * angTurnTimeFact));
        guboTaxi.lightDir = sunPos;
        guboTaxi.lightColor = glm::vec4(1.0f);
        guboTaxi.eyePos = camPos;
        guboTaxi.gamma = 128.0f;
        guboTaxi.metallic = 1.0f;
        DStaxi.map(currentImage, &guboTaxi, sizeof(guboTaxi), 2);

        uboCars[0].mvpMat = Prj * mView * mWorldCar1;
        uboCars[0].mMat = glm::mat4(1.0f);
        uboCars[0].nMat = glm::inverse(glm::transpose(uboCars[0].mMat));
        DScars[0].map(currentImage, &uboCars[0], sizeof(uboCars[0]), 0);
        guboCars[0].lightDir = sunPos;
        guboCars[0].lightColor = glm::vec4(1.0f);
        guboCars[0].eyePos = camPos;
        guboCars[0].gamma = 128.0f;
        guboCars[0].metallic = 1.0f;
        DScars[0].map(currentImage, &guboCars[0], sizeof(guboCars[0]), 2);

        uboCars[1].mvpMat = Prj * mView * mWorldCar2;
        uboCars[1].mMat = glm::mat4(1.0f);
        uboCars[1].nMat = glm::inverse(glm::transpose(uboCars[1].mMat));
        DScars[1].map(currentImage, &uboCars[1], sizeof(uboCars[1]), 0);
        guboCars[1].lightDir = sunPos;
        guboCars[1].lightColor = glm::vec4(1.0f);
        guboCars[1].eyePos = camPos;
        guboCars[1].gamma = 128.0f;
        guboCars[1].metallic = 1.0f;
        DScars[1].map(currentImage, &guboCars[1], sizeof(guboCars[1]), 2);

        uboCars[2].mvpMat = Prj * mView * mWorldCar3;
        uboCars[2].mMat = glm::mat4(1.0f);
        uboCars[2].nMat = glm::inverse(glm::transpose(uboCars[2].mMat));
        DScars[2].map(currentImage, &uboCars[2], sizeof(uboCars[2]), 0);
        guboCars[2].lightDir = sunPos;
        guboCars[2].lightColor = glm::vec4(1.0f);
        guboCars[2].eyePos = camPos;
        guboCars[2].gamma = 128.0f;
        guboCars[2].metallic = 1.0f;
        DScars[2].map(currentImage, &guboCars[2], sizeof(guboCars[2]), 2);

        uboCars[3].mvpMat = Prj * mView * mWorldCar4;
        uboCars[3].mMat = glm::mat4(1.0f);
        uboCars[3].nMat = glm::inverse(glm::transpose(uboCars[3].mMat));
        DScars[3].map(currentImage, &uboCars[3], sizeof(uboCars[3]), 0);
        guboCars[3].lightDir = sunPos;
        guboCars[3].lightColor = glm::vec4(1.0f);
        guboCars[3].eyePos = camPos;
        guboCars[3].gamma = 128.0f;
        guboCars[3].metallic = 1.0f;
        DScars[3].map(currentImage, &guboCars[3], sizeof(guboCars[3]), 2);

        uboCars[4].mvpMat = Prj * mView * mWorldCar5;
        uboCars[4].mMat = glm::mat4(1.0f);
        uboCars[4].nMat = glm::inverse(glm::transpose(uboCars[4].mMat));
        DScars[4].map(currentImage, &uboCars[4], sizeof(uboCars[4]), 0);
        guboCars[4].lightDir = sunPos;
        guboCars[4].lightColor = glm::vec4(1.0f);
        guboCars[4].eyePos = camPos;
        guboCars[4].gamma = 128.0f;
        guboCars[4].metallic = 1.0f;
        DScars[4].map(currentImage, &guboCars[4], sizeof(guboCars[4]), 2);

        uboCars[5].mvpMat = Prj * mView * mWorldCar6;
        uboCars[5].mMat = glm::mat4(1.0f);
        uboCars[5].nMat = glm::inverse(glm::transpose(uboCars[5].mMat));
        DScars[5].map(currentImage, &uboCars[5], sizeof(uboCars[5]), 0);
        guboCars[5].lightDir = sunPos;
        guboCars[5].lightColor = glm::vec4(1.0f);
        guboCars[5].eyePos = camPos;
        guboCars[5].gamma = 128.0f;
        guboCars[5].metallic = 1.0f;
        DScars[5].map(currentImage, &guboCars[5], sizeof(guboCars[5]), 2);

        uboCars[6].mvpMat = Prj * mView * mWorldCar7;
        uboCars[6].mMat = glm::mat4(1.0f);
        uboCars[6].nMat = glm::inverse(glm::transpose(uboCars[6].mMat));
        DScars[6].map(currentImage, &uboCars[6], sizeof(uboCars[6]), 0);
        guboCars[6].lightDir = sunPos;
        guboCars[6].lightColor = glm::vec4(1.0f);
        guboCars[6].eyePos = camPos;
        guboCars[6].gamma = 128.0f;
        guboCars[6].metallic = 1.0f;
        DScars[6].map(currentImage, &guboCars[6], sizeof(guboCars[6]), 2);

        uboCars[7].mvpMat = Prj * mView * mWorldCar8;
        uboCars[7].mMat = glm::mat4(1.0f);
        uboCars[7].nMat = glm::inverse(glm::transpose(uboCars[7].mMat));
        DScars[7].map(currentImage, &uboCars[7], sizeof(uboCars[7]), 0);
        guboCars[7].lightDir = sunPos;
        guboCars[7].lightColor = glm::vec4(1.0f);
        guboCars[7].eyePos = camPos;
        guboCars[7].gamma = 128.0f;
        guboCars[7].metallic = 1.0f;
        DScars[7].map(currentImage, &guboCars[7], sizeof(guboCars[7]), 2);

        uboCars[8].mvpMat = Prj * mView * mWorldCar9;
        uboCars[8].mMat = glm::mat4(1.0f);
        uboCars[8].nMat = glm::inverse(glm::transpose(uboCars[8].mMat));
        DScars[8].map(currentImage, &uboCars[8], sizeof(uboCars[8]), 0);
        guboCars[8].lightDir = sunPos;
        guboCars[8].lightColor = glm::vec4(1.0f);
        guboCars[8].eyePos = camPos;
        guboCars[8].gamma = 128.0f;
        guboCars[8].metallic = 1.0f;
        DScars[8].map(currentImage, &guboCars[8], sizeof(guboCars[8]), 2);

        glm::mat4 scaleMat = glm::translate(glm::mat4(1.0f), glm::vec3(40.0f, 20.0f, -75.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(195.0f, 50.0f, 195.0f));
        uboSky.mvpMat = Prj * mView * (scaleMat);
        uboSky.mMat = glm::mat4(1.0f);
        uboSky.nMat = glm::inverse(glm::transpose(uboSky.mMat));
        DSsky.map(currentImage, &uboSky, sizeof(uboSky), 0);
        //guboTaxi.lightDir = glm::vec3(cos(glm::radians(135.0f)) * cos(cTime * angTurnTimeFact), sin(glm::radians(135.0f)), cos(glm::radians(135.0f)) * sin(cTime * angTurnTimeFact));
        guboSky.lightDir = sunPos;
        guboSky.lightColor = glm::vec4(1.0f);
        guboSky.eyePos = camPos;
        guboSky.gamma = 128.0f;
        guboSky.metallic = 1.0f;
        DSsky.map(currentImage, &guboSky, sizeof(guboSky), 2);

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
            std::vector<glm::mat4> streetlightPositions;

            for(int k = 0; k < MESH; k++) {
                nlohmann::json TMjson = j["instances"][k]["transform"];
                for(int l = 0; l < 16; l++) {TMj[l] = TMjson[l];}
                mWorld=glm::mat4(TMj[0],TMj[4],TMj[8],TMj[12],TMj[1],TMj[5],TMj[9],TMj[13],TMj[2],TMj[6],TMj[10],TMj[14],TMj[3],TMj[7],TMj[11],TMj[15]);
                ubocity[k].mMat = glm::mat4(1);
                ubocity[k].nMat = glm::inverse(glm::transpose(ubocity[k].mMat));
                ubocity[k].mvpMat = Prj * mView * mWorld;
                DScity[k].map(currentImage, &ubocity[k], sizeof(ubocity[k]), 0);
                //gubocity[k].lightDir = glm::vec3(cos(glm::radians(135.0f)) * cos(cTime * angTurnTimeFact), sin(glm::radians(135.0f)), cos(glm::radians(135.0f)) * sin(cTime * angTurnTimeFact));
                gubocity[k].lightDir = sunPos;
                gubocity[k].lightColor = glm::vec4(1.0f);
                gubocity[k].eyePos = camPos;
                gubocity[k].gamma = 128.0f;
                gubocity[k].metallic = 0.1f;
                DScity[k].map(currentImage, &gubocity[k], sizeof(gubocity[k]), 2);

                // Check if the current model is a streetlight
                // X ==> verso del taxi
                // Y ==> verso l'alto
                // Z ==> verso destra
                std::string modelName = j["models"][k]["model"];
                if (modelName == "models/road_tile_1x1_001.mgcg") {
                    glm::mat4 position = glm::mat4(TMj[0],TMj[4],TMj[8],TMj[12],TMj[1],TMj[5],TMj[9],TMj[13],TMj[2],TMj[6],TMj[10],TMj[14],TMj[3],TMj[7],TMj[11],TMj[15]);
                    position = glm::translate(position, glm::vec3(3.75f, 4.25f, -0.75f)); // Adjust position
                    streetlightPositions.push_back(position);
                } else if (modelName == "models/road_tile_1x1_006.mgcg") {
                    glm::mat4 position = glm::mat4(TMj[0],TMj[4],TMj[8],TMj[12],TMj[1],TMj[5],TMj[9],TMj[13],TMj[2],TMj[6],TMj[10],TMj[14],TMj[3],TMj[7],TMj[11],TMj[15]);
                    position = glm::translate(position, glm::vec3(2.0f, 2.0f, 0.0f)); // Adjust position
                    streetlightPositions.push_back(position);
                }
                else if (modelName == "models/road_tile_1x1_008.mgcg") {
                    glm::mat4 position = glm::mat4(TMj[0],TMj[4],TMj[8],TMj[12],TMj[1],TMj[5],TMj[9],TMj[13],TMj[2],TMj[6],TMj[10],TMj[14],TMj[3],TMj[7],TMj[11],TMj[15]);
                    // position = glm::translate(position, glm::vec3(3.75f, 4.25f, -0.75f)); // Adjust position
                    streetlightPositions.push_back(position);
                }
            }

#if DEBUG
            for (size_t i = 0; i < streetlightPositions.size(); ++i) {
                    glm::mat4 mWorldSphere = streetlightPositions[i];
                    ubosphere[i].mvpMat = Prj * mView * mWorldSphere;
                    ubosphere[i].mMat = glm::mat4(1.0f);
                    ubosphere[i].nMat = glm::inverse(glm::transpose(ubosphere[i].mMat));
                    DSsphere[i].map(currentImage, &ubosphere[i], sizeof(ubosphere[i]), 0);
                }
#endif



        }catch (const nlohmann::json::exception& e) {
            std::cout << "[ EXCEPTION ]: " << e.what() << std::endl;
        }

        nlohmann::json js3;
        std::ifstream ifs3("models/people.json");
        if (!ifs3.is_open()) {
            std::cout << "[ ERROR ]: Scene file not found!" << std::endl;
            exit(-1);
        }
        try{
            json j3;
            ifs3>>j3;

            float TMj[16];

            for(int k = 0; k < PEOPLE; k++) {
                nlohmann::json TMjson = j3["instances"][k]["transform"];

                for(int l = 0; l < 16; l++) {TMj[l] = TMjson[l];}

                mWorld=glm::mat4(TMj[0],TMj[4],TMj[8],TMj[12],TMj[1],TMj[5],TMj[9],TMj[13],TMj[2],TMj[6],TMj[10],TMj[14],TMj[3],TMj[7],TMj[11],TMj[15]);
                uboPeople[k].mMat = glm::mat4(1);
                uboPeople[k].nMat = glm::inverse(glm::transpose(uboPeople[k].mMat));
                uboPeople[k].mvpMat = Prj * mView * mWorld;
                DSpeople[k].map(currentImage, &uboPeople[k], sizeof(uboPeople[k]), 0);
                //gubocity[k].lightDir = glm::vec3(cos(glm::radians(135.0f)) * cos(cTime * angTurnTimeFact), sin(glm::radians(135.0f)), cos(glm::radians(135.0f)) * sin(cTime * angTurnTimeFact));
                guboPeople[k].lightDir = sunPos;
                guboPeople[k].lightColor = glm::vec4(1.0f);
                guboPeople[k].eyePos = camPos;
                guboPeople[k].gamma = 128.0f;
                guboPeople[k].metallic = 0.1f;
                DSpeople[k].map(currentImage, &guboPeople[k], sizeof(guboPeople[k]), 2);

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
