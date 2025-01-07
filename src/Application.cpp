// This has been adapted from the Vulkan tutorial
#include "headers/Starter.hpp"
#include "headers/TextMaker.hpp"
#include <iostream>
#include <fstream>

#define MESH 210
#define CARS 9
#define SPHERES 2
#define STREET_LIGHT_COUNT 36
#define PEOPLE 45
#define TAXI_ELEMENTS 8

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

struct TaxiGUBO {
    alignas(16) glm::vec3 lightDir;
    alignas(16) glm::vec4 lightColor;
    //struct { alignas(16) glm::vec3 v; } rearLightPos[2];
    alignas(16) glm::vec3 rearLightRPos;
    alignas(16) glm::vec4 rearLightRCol;
    alignas(16) glm::vec3 rearLightLPos;
    alignas(16) glm::vec4 rearLightLCol;
    alignas(16) glm::vec3 eyePos;
    alignas(4) float gamma;
    alignas(4) float metallic;
};

struct SkyGUBO {
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

    Model  Mtaxi[TAXI_ELEMENTS], Msky, Mcars[CARS], Mpeople[PEOPLE];
    Model Mcity[MESH];

    DescriptorSet DStaxi[TAXI_ELEMENTS], DScity[MESH], DSsky, DScars[CARS], DSpeople[PEOPLE];

    Texture Tcity, Tsky, Tpeople, Ttaxy;

    UniformBufferObject uboTaxi[TAXI_ELEMENTS], uboSky, uboCars[CARS], ubocity[MESH], uboPeople[PEOPLE];
    GlobalUniformBufferObject guboCars[CARS], gubocity[MESH], guboPeople[PEOPLE];
    SkyGUBO guboSky;
    TaxiGUBO guboTaxi[TAXI_ELEMENTS];

    #if DEBUG
        DescriptorSetLayout DSLsphere;
        VertexDescriptor VDsphere;
        Pipeline Psphere;
        Model Msphere[SPHERES];
        DescriptorSet DSsphere[SPHERES];
        UniformBufferObject uboSphere[SPHERES];
    #endif

    // Other application parameters
    int currScene = 0;
    glm::vec3 camPos = glm::vec3(0.0, 1.5f, -5.0f); //initial pos of camera
    glm::vec3 camPosInPhotoMode;
    glm::vec3 taxiPos = glm::vec3(0.0, -0.2, 0.0); //initial pos of taxi

    glm::vec3 carPositions[CARS] = {glm::vec3(-72.0, -0.2, 36.0), //initial pos of car
                                 glm::vec3(5.0, -0.2, 36.0),
                                 glm::vec3(72.0, -0.2, 36.0),
                                 glm::vec3(-36.0, -0.2, -108.0),
                                 glm::vec3(36.0, -0.2, -108.0),
                                 glm::vec3(108.0, -0.2, -108.0),
                                 glm::vec3(-36.0, -0.2, -112.0),
                                 glm::vec3(36.0, -0.2, -112.0),
                                 glm::vec3(108.0, -0.2, -112.0)};


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
    std::vector<glm::vec3> wayPoints[CARS] = {wayPoints1, wayPoints2, wayPoints3, wayPoints4, wayPoints5, wayPoints6, wayPoints7, wayPoints8, wayPoints9};

    int currentPoints[CARS] = {0,0,0,0,0,0,0,0,0};
    float CamAlpha = 0.0f;
    float CamBeta = 0.0f;
    bool alreadyInPhotoMode = false;
    bool isNight = false;

    // Here you set the main application parameters
    void setWindowParameters() {

        windowWidth = 1920;
        windowHeight = 1080;
        windowTitle = "Computer graphics' project";
        windowResizable = GLFW_TRUE;
        initialBackgroundColor = {0.0f, 0.005f, 0.01f, 1.0f};

        // Descriptor pool sizes
        uniformBlocksInPool =  (2 * MESH) + 16+2+2*CARS+2*PEOPLE + (DEBUG ? 2 * SPHERES : 0);
        texturesInPool = MESH + 8 +1+1+CARS+PEOPLE; //city, taxi, text, sky, autonomous cars, people
        setsInPool = MESH + 8 +1+1+CARS+PEOPLE+ (DEBUG ? SPHERES : 0);

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

        P.init(this, &VD, "shaders/BaseVert.spv", "shaders/Taxi2Frag.spv", {&DSL});
        Pcity.init(this, &VDcity, "shaders/BaseVert.spv", "shaders/TaxiFrag.spv", {&DSLcity});
        Pcity.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
        Psky.init(this, &VDsky, "shaders/BaseVert.spv", "shaders/SkyFrag.spv", {&DSLsky});
        Psky.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false); //todo cosa dovevamo fare quando telecamera dentro una stanza
        Pcars.init(this, &VDcars, "shaders/BaseVert.spv", "shaders/TaxiFrag.spv", {&DSLcars});
        Ppeople.init(this, &VDpeople, "shaders/BaseVert.spv", "shaders/TaxiFrag.spv", {&DSLpeople});

        #if DEBUG
            Psphere.init(this, &VDsphere, "shaders/BaseVert.spv", "shaders/DEBUGFrag.spv", {&DSLsphere});
        #endif


        //Mtaxi.init(this, &VD, "models/transport_purpose_003_transport_purpose_003.001.mgcg", MGCG );
        Mtaxi[0].init(this, &VD, "models/Car_Hatch_C_DoorR.obj", OBJ );
        Mtaxi[1].init(this, &VD, "models/Car_Hatch_C_Extern.obj", OBJ );
        Mtaxi[2].init(this, &VD, "models/Car_Hatch_C_Intern_no-steer.obj", OBJ );
        Mtaxi[3].init(this, &VD, "models/Car_Hatch_C_Steer.obj", OBJ );
        Mtaxi[4].init(this, &VD, "models/Car_Hatch_C_WheelBL_Origin_at_center.obj", OBJ );
        Mtaxi[5].init(this, &VD, "models/Car_Hatch_C_WheelBR_Origin_at_center.obj", OBJ );
        Mtaxi[6].init(this, &VD, "models/Car_Hatch_C_WheelFL_Origin_at_center.obj", OBJ );
        Mtaxi[7].init(this, &VD, "models/Car_Hatch_C_WheelFR_Origin_at_center.obj", OBJ );
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
        Tpeople.init(this, "textures/Humano_01Business_01_Diffuse04.jpg");
        Ttaxy.init(this, "textures/VehiclePack_baseColor.png");

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
            exit(1);
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
            exit(1);
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

        for(int i = 0; i<8; i++){
            DStaxi[i].init(this, &DSL, {
                    {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                    {1, TEXTURE, 0, &Ttaxy},
                    {2, UNIFORM, sizeof(TaxiGUBO), nullptr}
            });
        }

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
                {2, UNIFORM, sizeof(SkyGUBO), nullptr}
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

        for(int i = 0; i < 8; i++) {
            DStaxi[i].cleanup();
        }

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
        Ttaxy.cleanup();

        for(int i = 0; i < 8; i++) {
            Mtaxi[i].cleanup();
        }

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

        for(int i = 0; i < 8; i++){
            DStaxi[i].bind(commandBuffer, P, 0, currentImage);
            Mtaxi[i].bind(commandBuffer);
            vkCmdDrawIndexed(commandBuffer,
                             static_cast<uint32_t>(Mtaxi[i].indices.size()), 1, 0, 0, 0);
        }


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

        glm::vec3 directions[CARS];
        for(int i = 0; i < CARS; i++) {
            directions[i] = glm::normalize(wayPoints[i][currentPoints[i]] - carPositions[i]);
        }

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

        static float steeringAngCars[CARS];
        for(int i = 0; i < CARS; i++) {
            carPositions[i] += directions[i] * speedCar * deltaT;
            if (glm::distance(carPositions[i], wayPoints[i][currentPoints[i]]) < 0.25f) {
                currentPoints[i] = (currentPoints[i] + 1) % wayPoints[i].size();
            }
            //steeringAngCars[i] = 0.0f;
            float targetSteering = atan2(directions[i].x, directions[i].z);
            //float angleDifference = targetSteering - steeringAngCars[i];
            //angleDifference = fmod(angleDifference + M_PI, 2.0f * M_PI) - M_PI;
            steeringAngCars[i]= fmod(targetSteering + M_PI, 2.0f * M_PI) - M_PI;
        }

        /* CODICE DI MARY
        
        carPos1 += direction1 * speedCar * deltaT;
        if (glm::distance(carPos1, wayPoints1[currentPoint1]) < 0.25f) {
            currentPoint1 = (currentPoint1 + 1) % wayPoints1.size();
        }
        static float steeringAngCar1 = 0.0f;
        float targetSteering1 = atan2(direction1.x, direction1.z);
        float angleDifference1 = targetSteering1 - steeringAngCar1;
        angleDifference1 = fmod(angleDifference1 + M_PI, 2.0f * M_PI) - M_PI;
        steeringAngCar1 += angleDifference1 * 0.1f;*/

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

        glm::vec3 sphereCenter = glm::vec3(40.0f, 20.0f, -75.0f);
        glm::vec3 sunPos = glm::vec3(40.0f + 200.0f * cos(cTime * angTurnTimeFact), // x
                                     20.0f + 55.0f * sin(cTime * angTurnTimeFact), // y
                                     -75.0f             // z (fisso)
        );

        //50.0f * sin(glm::radians(90.0f)
        // check when the sun is belowe the horizon
        isNight = (sunPos.y < 0.0f ? true : false);

        glm::mat4 mWorldTaxi =
                glm::translate(glm::mat4(1.0), taxiPos) *
                glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0));


        glm::mat4 mWorldCars[CARS];
        for(int i = 0; i < CARS; i++) {
            mWorldCars[i] = glm::translate(glm::mat4(1.0), carPositions[i]) *
                            glm::rotate(glm::mat4(1.0), steeringAngCars[i], glm::vec3(0, 1, 0));
        }

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
                for(int l = 0; l < 16; l++) {
                    TMj[l] = TMjson[l];
                }
                mWorld=glm::mat4(TMj[0],TMj[4],TMj[8],TMj[12],TMj[1],TMj[5],TMj[9],TMj[13],TMj[2],TMj[6],TMj[10],TMj[14],TMj[3],TMj[7],TMj[11],TMj[15]);
                ubocity[k].mMat = mWorld;
                ubocity[k].nMat = glm::inverse(glm::transpose(ubocity[k].mMat));
                ubocity[k].mvpMat = Prj * mView * mWorld;
                DScity[k].map(currentImage, &ubocity[k], sizeof(ubocity[k]), 0);
                gubocity[k].lightDir = sunPos;
                gubocity[k].lightColor = glm::vec4(1.0f);
                gubocity[k].eyePos = camPos;
                gubocity[k].gamma = 128.0f;
                gubocity[k].metallic = 0.1f;
                DScity[k].map(currentImage, &gubocity[k], sizeof(gubocity[k]), 2);
            }

        }
        catch (const nlohmann::json::exception& e) {
            std::cout << "[ EXCEPTION ]: " << e.what() << std::endl;
            exit(1);
        }

        for(int i=0; i<8; i++){
            uboTaxi[i].mvpMat = Prj * mView * mWorldTaxi;
            uboTaxi[i].mMat = mWorldTaxi;
            uboTaxi[i].nMat = glm::inverse(glm::transpose(uboTaxi[i].mMat));
            DStaxi[i].map(currentImage, &uboTaxi[i], sizeof(uboTaxi[i]), 0);
            guboTaxi[i].lightDir = sunPos;
            guboTaxi[i].lightColor = glm::vec4(1.0f);
            /*for(int j = 0; j < 2; j++) {
                glm::vec3 offset = (j == 0) ? glm::vec3(-0.5f, 0.5f, -0.75f) : glm::vec3(0.5f, 0.5f, -0.75f);
                glm::vec3 rearLightPos = glm::translate(mWorldTaxi, offset)[3];
                guboTaxi[i].rearLightPos[j].v = rearLightPos;
            }*/
            guboTaxi[i].rearLightRPos = glm::translate(mWorldTaxi, glm::vec3(-0.5f, 0.5f, -0.75f))[3];
            guboTaxi[i].rearLightLPos = glm::translate(mWorldTaxi, glm::vec3(0.5f, 0.5f, -0.75f))[3];
            if(isNight) {
                guboTaxi[i].rearLightRCol = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
                guboTaxi[i].rearLightLCol = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            }
            else {
                guboTaxi[i].rearLightRCol = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                guboTaxi[i].rearLightLCol = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            }
            guboTaxi[i].eyePos = camPos;
            guboTaxi[i].gamma = 128.0f;
            guboTaxi[i].metallic = 1.0f;
            DStaxi[i].map(currentImage, &guboTaxi[i], sizeof(guboTaxi[i]), 2);
        }

        #if DEBUG
            for (int i = 0; i < SPHERES; i++) {
                glm::vec3 offset = (i == 0) ? glm::vec3(-0.5f, 0.5f, -1.0f) : glm::vec3(0.5f, 0.5f, -1.0f);
                glm::mat4 mWorldSphere = glm::scale(glm::translate(mWorldTaxi, offset), glm::vec3(0.1f));
                uboSphere[i].mvpMat = Prj * mView * mWorldSphere;
                uboSphere[i].mMat = mWorldSphere;
                uboSphere[i].nMat = glm::inverse(glm::transpose(uboSphere[i].mMat));
                DSsphere[i].map(currentImage, &uboSphere[i], sizeof(uboSphere[i]), 0);
            }
        #endif

        for(int i = 0; i < CARS; i++) {
            uboCars[i].mvpMat = Prj * mView * mWorldCars[i];
            uboCars[i].mMat = mWorldCars[i];
            uboCars[i].nMat = glm::inverse(glm::transpose(uboCars[i].mMat));
            DScars[i].map(currentImage, &uboCars[i], sizeof(uboCars[i]), 0);
            guboCars[i].lightDir = sunPos;
            guboCars[i].lightColor = glm::vec4(1.0f);
            guboCars[i].eyePos = camPos;
            guboCars[i].gamma = 128.0f;
            guboCars[i].metallic = 1.0f;
            DScars[i].map(currentImage, &guboCars[i], sizeof(guboCars[i]), 2);
        }

        glm::mat4 scaleMat = glm::translate(glm::mat4(1.0f), glm::vec3(40.0f, 20.0f, -75.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(195.0f, 50.0f, 195.0f));
        uboSky.mvpMat = Prj * mView * (scaleMat);
        uboSky.mMat = scaleMat;
        uboSky.nMat = glm::inverse(glm::transpose(uboSky.mMat));
        DSsky.map(currentImage, &uboSky, sizeof(uboSky), 0);
        guboSky.lightDir = sunPos;
        guboSky.lightColor = glm::vec4(1.0f);
        guboSky.eyePos = camPos;
        guboSky.gamma = 128.0f;
        guboSky.metallic = 0.1f;
        DSsky.map(currentImage, &guboSky, sizeof(guboSky), 2);

        nlohmann::json js3;
        std::ifstream ifs3("models/people.json");
        if (!ifs3.is_open()) {
            std::cout << "[ ERROR ]: Scene file not found!" << std::endl;
            exit(1);
        }
        try{
            json j3;
            ifs3>>j3;

            float TMj[16];

            for(int k = 0; k < PEOPLE; k++) {
                nlohmann::json TMjson = j3["instances"][k]["transform"];

                for(int l = 0; l < 16; l++) {TMj[l] = TMjson[l];}

                mWorld=glm::mat4(TMj[0],TMj[4],TMj[8],TMj[12],TMj[1],TMj[5],TMj[9],TMj[13],TMj[2],TMj[6],TMj[10],TMj[14],TMj[3],TMj[7],TMj[11],TMj[15]);
                uboPeople[k].mMat = mWorld;
                uboPeople[k].nMat = glm::inverse(glm::transpose(uboPeople[k].mMat));
                uboPeople[k].mvpMat = Prj * mView * mWorld;
                DSpeople[k].map(currentImage, &uboPeople[k], sizeof(uboPeople[k]), 0);
                guboPeople[k].lightDir = sunPos;
                guboPeople[k].lightColor = glm::vec4(1.0f);
                guboPeople[k].eyePos = camPos;
                guboPeople[k].gamma = 128.0f;
                guboPeople[k].metallic = 0.1f;
                DSpeople[k].map(currentImage, &guboPeople[k], sizeof(guboPeople[k]), 2);

            }

        }catch (const nlohmann::json::exception& e) {
            std::cout << "[ EXCEPTION ]: " << e.what() << std::endl;
            exit(1);
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
