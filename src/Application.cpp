#include "headers/Starter.hpp"
#include "headers/TextMaker.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>

#define MINIAUDIO_IMPLEMENTATION
#include "headers/miniaudio.h"

#define MESH 210
#define CARS 9
#define STREET_LIGHT_COUNT 36
#define MAX_STREET_LIGHTS 5
#define PEOPLE 45
#define TAXI_ELEMENTS 8
#define TAXI_LIGHT_COUNT 4
#define PICKUP_COUNT 5
#define INGAME_SCENE_COUNT 2
#define GAMEMODE_COUNT 2
#define GRAPHICS_SETTINGS_COUNT 3
#define TAXI_COLL_PCOUNT 4
#define COLLISION_BOXES_COUNT 9

#define MIN_DISTANCE_TO_PICKUP 4.75f
#define COLLISION_SPHERE_RADIUS 0.75f
#define PICKUP_POINT_Y_OFFSET 2.0f
#define ARROW_Y_OFFSET 3.25f

struct UniformBufferObject {
    alignas(16) glm::mat4 mvpMat;
    alignas(16) glm::mat4 mMat;
    alignas(16) glm::mat4 nMat;
};

struct GlobalUniformBufferObject {
    alignas(16) glm::vec4 directLightPos;
    alignas(16) glm::vec4 directLightCol;
    alignas(16) glm::vec4 taxiLightPos[TAXI_LIGHT_COUNT];
    alignas(16) glm::vec4 frontLightCol;
    alignas(16) glm::vec4 rearLightCol;
    alignas(16) glm::vec4 frontLightDir;
    alignas(16) glm::vec4 frontLightCosines;
    alignas(16) glm::vec4 streetLightPos[MAX_STREET_LIGHTS];
    alignas(16) glm::vec4 streetLightCol;
    alignas(16) glm::vec4 streetLightDirection;
    alignas(16) glm::vec4 streetLightCosines;
    alignas(16) glm::vec4 pickupPointPos;
    alignas(16) glm::vec4 pickupPointCol;
    alignas(16) glm::vec4 eyePos;
    alignas(16) glm::vec4 gammaMetallicSettingsNight;
};

struct SkyGUBO {
	alignas(16) glm::vec4 lightDir;
    alignas(16) glm::vec4 lightColor;
	alignas(16) glm::vec4 eyePos;
    alignas(16) glm::vec4 gammaAndMetallic;
};

struct ArrowGUBO {
    alignas(16) glm::vec4 pickupPointPos;
    alignas(16) glm::vec4 pickupPointCol;
    alignas(16) glm::vec4 eyePos;
    alignas(16) glm::vec4 gammaAndMetallic;
};

// Vertex definition for 3D objects
struct Vertex {
    glm::vec3 pos;
    glm::vec2 UV;
    glm::vec3 normal;
};

// Vertex definition for 2D objects
struct TwoDimVertex {
    glm::vec3 pos;
    glm::vec2 UV;
};

// Struct to easily manage collision boxes
struct CollisionBox {
    float xMin;
    float xMax;
    float zMin;
    float zMax;
};

class Application : public BaseProject {

    public:

        // Game options setted by main menu
        int graphicsSettings = 2;
        bool endlessGameMode = false;
        ma_engine engine;
        ma_sound titleMusic;
        ma_sound ingameMusic;
        ma_sound idleEngineSound;
        ma_sound accelerationEngineSound;
        ma_sound pickupSound;
        ma_sound moneySound;
        ma_sound clacsonSound;
    
    protected:
        
        float Ar;

        DescriptorSetLayout DSLtaxi, DSLcity, DSLsky, DSLcars, DSLpeople, DSLtitle, DSLcontrols, DSLarrow, DSLendgame;

        VertexDescriptor VDtaxi, VDcity, VDsky, VDcars, VDpeople, VDtitle, VDcontrols, VDarrow, VDendgame;

        Pipeline Ptaxi, Pcity, Psky, Pcars, Ppeople, Ptitle, Pcontrols, Parrow, Pendgame;

        Model Mtaxi[TAXI_ELEMENTS], Msky, Mcars[CARS], Mpeople[PEOPLE], Mcity[MESH], Mtitle, Mcontrols, Marrow, Mendgame;

        DescriptorSet DStaxi[TAXI_ELEMENTS], DScity[MESH], DSsky, DScars[CARS], DSpeople[PEOPLE], DStitle, DScontrols, DSarrow, DSendgame;

        Texture Tcity, Tsky, Tpeople, Ttaxy, Ttitle, Tcontrols, Tendgame;

        UniformBufferObject uboTaxi[TAXI_ELEMENTS], uboSky, uboCars[CARS], uboCity[MESH], uboPeople[PEOPLE], uboArrow;
        GlobalUniformBufferObject guboTaxi[TAXI_ELEMENTS], guboCars[CARS], guboCity[MESH], guboPeople[PEOPLE];
        SkyGUBO guboSky;
        ArrowGUBO guboArrow;

        int currScene = -2; // Variables used for the scene management
        int lastSavedSceneValue;
        int currentPoints[CARS] = {0,0,0,0,0,0,0,0,0};  // Variable for the NPC cars
        int random_index = -1;  // Index used to choose randomically the person to pickup
        int collisionCounter = 0;   // Variable used to count on how many NPC care we are colliding
        int totDrivesCompleted = 0; // Variables used to count the number of drives completed
        float wheelRoll = 0.0f;
        float CamAlpha = 0.0f;
        float CamBeta = 0.0f;
        float money = 0.0f; // Variable used to store the money earned
        float wheelAndSteerAng = 0.0f;
        float openingDoorAngle = 0.0f;
        double pickupTime = 0.0;    // Variable used to time the pickup and dropoff

        // Boolean flag used in the code
		bool openDoor = false;
		bool closeDoor = false;
        bool alreadyInPhotoMode = false;
        bool isNight = false;
        bool drawTitle = true;
        bool drawControls = false;
        bool pickupPointSelected = false;
        bool pickedPassenger = false;
        bool endGame = false;
        bool inCollisionZone = false;

        glm::vec3 camPos = glm::vec3(0.0, 1.5f, -5.0f); // Initial pos of camera
        glm::vec3 camPosInPhotoMode;
        glm::vec3 taxiPos = glm::vec3(0.0f, -0.2f, 0.0f);   // Initial pos of taxi

        // Initial positions of the NPC cars
        glm::vec3 carPositions[CARS] = {glm::vec3(-72.0f, -0.2f, 36.0f),
                                    glm::vec3(5.0f, -0.2f, 36.0f),
                                    glm::vec3(72.0f, -0.2f, 36.0f),
                                    glm::vec3(-36.0f, -0.2f, -108.0f),
                                    glm::vec3(36.0f, -0.2f, -108.0f),
                                    glm::vec3(108.0f, -0.2f, -108.0f),
                                    glm::vec3(-36.0f, -0.2f, -112.0f),
                                    glm::vec3(36.0f, -0.2f, -112.0f),
                                    glm::vec3(108.0f, -0.2f, -112.0f)};

        // Waypoints for the NPC cars movements
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

        // Positions of the streetlights used by the shaders
        glm::vec3 streetlightPos[STREET_LIGHT_COUNT] = {glm::vec3(-1.65f, 9.3f, -22.2f),
                                                        glm::vec3(-1.65f, 9.3f, -94.2f),
                                                        glm::vec3(-1.65f, 9.3f, -166.2f),
                                                        glm::vec3(70.35f, 9.3f, -22.2f),
                                                        glm::vec3(70.35f, 9.3f, -94.2f),
                                                        glm::vec3(70.35f, 9.3f, -166.2f),
                                                        glm::vec3(-13.8f, 9.3f, -37.65f),
                                                        glm::vec3(58.2f, 9.3f, -37.65f),
                                                        glm::vec3(130.2f, 9.3f, -37.65f),
                                                        glm::vec3(-13.8f, 9.3f, -109.65f),
                                                        glm::vec3(58.2f, 9.3f, -109.65f),
                                                        glm::vec3(130.2f, 9.3f, -109.65f),
                                                        glm::vec3(-13.8f, 9.3f, 34.35f),
                                                        glm::vec3(58.2f, 9.3f, 34.35f),
                                                        glm::vec3(130.2f, 9.3f, 34.35f),
                                                        glm::vec3(-13.8f, 9.3f, -181.65f),
                                                        glm::vec3(58.2f, 9.3f, -181.65f),
                                                        glm::vec3(130.2f, 9.3f, -181.65f),
                                                        glm::vec3(-73.65f, 9.3f, -22.2f),
                                                        glm::vec3(-73.65f, 9.3f, -94.2f),
                                                        glm::vec3(-73.65f, 9.3f, -166.2f),
                                                        glm::vec3(142.35f, 9.3f, -22.2f),
                                                        glm::vec3(142.35f, 9.3f, -94.2f),
                                                        glm::vec3(142.35f, 9.3f, -166.2f),
                                                        glm::vec3(-11.4f, 9.3f, 38.4f),
                                                        glm::vec3(-74.4f, 9.3f, -47.4f),
                                                        glm::vec3(-74.4f, 9.3f, -119.4f),
                                                        glm::vec3(11.4f, 9.3f, -182.4f),
                                                        glm::vec3(83.4f, 9.3f, -182.4f),
                                                        glm::vec3(146.4f, 9.3f, -96.6f),
                                                        glm::vec3(146.4f, 9.3f, -24.6f),
                                                        glm::vec3(60.6f, 9.3f, 38.4f),
                                                        glm::vec3(-77.1f, 9.3f, 38.1f),
                                                        glm::vec3(-74.1f, 9.3f, -185.1f),
                                                        glm::vec3(149.1f, 9.3f, -182.1f),
                                                        glm::vec3(146.1f, 9.3f, 41.1f)};

        // Offsets of the collision points from the starting position of the taxi
        glm::vec3 taxiCollPOffsets[TAXI_COLL_PCOUNT] = {glm::vec3(-0.85f, 0.0f, -0.65f), // rear right
                                                                glm::vec3(0.85f, 0.0f, -0.65f), // rear left
                                                                glm::vec3(-0.85f, 0.0f, 2.7f), // front right
                                                                glm::vec3(0.85f, 0.0f, 2.7f)}; // front left
        glm::vec3 taxiCollisionPoints[TAXI_COLL_PCOUNT];

        // List of five persons to pickup (one choosen randomically)
        glm::vec4 pickupPoints[PICKUP_COUNT] = {glm::vec4(-79.0f, 0.0f, 0.0f, 0.0f),
                                                        glm::vec4(36.0f, 0.0f, -29.0f, 0.0f),
                                                        glm::vec4(26.0f, 0.0f, -115.0f, 0.0f),
                                                        glm::vec4(7.0f, 0.0f, -144.0f, 0.0f),
                                                        glm::vec4(151.0f, 0.0f, -144.0f, 0.0f)};

        // List of five dropoff points (one for each person)
        glm::vec4 dropoffPoints[PICKUP_COUNT] = {glm::vec4(95.69f, 0.0f, -187.0f, 0.0f),
                                                    glm::vec4(-80.0f, 0.0f, -184.0f, 0.0f),
                                                    glm::vec4(136.0f, 0.0f, -72.16f, 0.0f),
                                                    glm::vec4(107.84f, 0.0f, 44.0f, 0.0f),
                                                    glm::vec4(-7.0f, 0.0f, -46.13f, 0.0f)};

        // Some values used in the the majority of the shaders 
        glm::vec4 rearLightColor = glm::vec4(238.0f / 255.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 frontLightColor = glm::vec4(238.0f / 255.0f, 221.0f / 255.0f, 130.0f / 255.0f, 1.0f);
        glm::vec4 frontLightDirection = glm::vec4(0.0f, -1.0f * glm::abs(glm::sin(glm::radians(2.0f))), -1.0f * glm::abs(glm::cos(glm::radians(2.0f))), 0.0f);
        glm::vec4 frontLightCosines = glm::vec4(glm::abs(glm::cos(10.0f)), glm::abs(glm::cos(15.0f)), 0.0f, 0.0f);
        glm::vec4 sunCol = glm::vec4(253.0f / 255.0f, 251.0f / 255.0f, 211.0f / 255.0f, 1.0f);
        glm::vec4 streetLightCol = glm::vec4(255.0f / 255.0f, 230.0f / 255.0f, 146.0f / 255.0f, 1.0f);
        glm::vec4 streetLightDirection = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
        glm::vec4 streetLightCosines = glm::vec4(glm::abs(glm::cos(15.0f)), glm::abs(glm::cos(22.5f)), 0.0f, 0.0f);
        glm::vec4 pickupPointColor = glm::vec4(247.0f / 255.0f, 76.0f / 255.0f, 63.0f / 255.0f, 1.0f);
        glm::vec4 pickupPoint = glm::vec4(0.0f);
        glm::vec4 dropoffPoint = glm::vec4(0.0f);

        glm::mat4 mWorldCars[CARS];

        // Hash map used to check what people we don't want to draw (it has been picked up)
        // Easy working ==> when a person has been picked up, we set the value to false
        std::unordered_map<int, bool> drawPeople = {{3, true}, {7, true}, {35, true}, {37, true}, {44, true}};

        // Collision box of the city (external collision box)
        const CollisionBox externalCollisionBox = {
            .xMin = -78.0f,
            .xMax = 150.0f,
            .zMin = -186.0f,
            .zMax = 42.0f
        };

        // Collision boxes of the city (internal collision boxes)
        const CollisionBox internalCollisionBoxes[COLLISION_BOXES_COUNT] = {{.xMin = -66.0f,
                                                                            .xMax = -6.0f,
                                                                            .zMin = -174.0f,
                                                                            .zMax = -114.0f},
                                                                            {.xMin = 6.0f,
                                                                            .xMax = 66.0f,
                                                                            .zMin = -174.0f,
                                                                            .zMax = -114.0f},
                                                                            {.xMin = 78.0f,
                                                                            .xMax = 138.0f,
                                                                            .zMin = -174.0f,
                                                                            .zMax = -114.0f},
                                                                            {.xMin = -66.0f,
                                                                            .xMax = -6.0f,
                                                                            .zMin = -102.0f,
                                                                            .zMax = -42.0f},
                                                                            {.xMin = 6.0f,
                                                                            .xMax = 66.0f,
                                                                            .zMin = -102.0f,
                                                                            .zMax = -42.0f},
                                                                            {.xMin = 78.0f,
                                                                            .xMax = 138.0f,
                                                                            .zMin = -102.0f,
                                                                            .zMax = -42.0f},
                                                                            {.xMin = -66.0f,
                                                                            .xMax = -6.0f,
                                                                            .zMin = -30.0f,
                                                                            .zMax = 30.0f},
                                                                            {.xMin = 6.0f,
                                                                            .xMax = 66.0f,
                                                                            .zMin = -30.0f,
                                                                            .zMax = 30.0f},
                                                                            {.xMin = 78.0f,
                                                                            .xMax = 138.0f,
                                                                            .zMin = -30.0f,
                                                                            .zMax = 30.0f}};

        // Set the main application parameters
        void setWindowParameters() {

            // Window dimensions and title
            windowWidth = 1920;
            windowHeight = 1080;
            windowTitle = "TAXI DRIVER";
            windowResizable = GLFW_TRUE;
            initialBackgroundColor = {0.0f, 0.005f, 0.01f, 1.0f};

            // Descriptor pool sizes
            uniformBlocksInPool =  20 + (2 * MESH) + (2 * CARS) + (2 * PEOPLE);
            texturesInPool = 13 + MESH + CARS + PEOPLE;
            setsInPool = 13 + MESH + CARS + PEOPLE;

            Ar = (float)windowWidth / (float)windowHeight;

        }

        // Function on window resize
        void onWindowResize(int w, int h) {
            Ar = (float)w / (float)h;
        }

        // initialization of Descriptor Set Layouts, Vertex Descriptors, Pipelines, Models and Textures
        void localInit() {

            // Initialization of Descriptor Set Layouts
            DSLtaxi.init(this, {
                    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}, // Uniform Buffer Object
                    {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}, // Texture
                    {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS} // Global Uniform Buffer Object
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

            DSLtitle.init(this, {
                {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT} // Only texture
            });
            DSLcontrols.init(this, {
                {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
            });
            DSLarrow.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}, // UBO
                {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS} // Texture
            });
            DSLendgame.init(this, {
                {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
            });

            // Initialization of Vertex Descriptors
            VDtaxi.init(this, {
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
            
            VDtitle.init(this, {
                {0, sizeof(TwoDimVertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TwoDimVertex, pos),
                        sizeof(glm::vec3), POSITION},
                {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(TwoDimVertex, UV),
                        sizeof(glm::vec2), UV}
            });

            VDcontrols.init(this, {
                {0, sizeof(TwoDimVertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TwoDimVertex, pos),
                        sizeof(glm::vec3), POSITION},
                {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(TwoDimVertex, UV),
                        sizeof(glm::vec2), UV}
            });
            
            VDarrow.init(this, {
                {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                        sizeof(glm::vec3), POSITION},
                {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                        sizeof(glm::vec2), UV},
                {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal),
                        sizeof(glm::vec3), NORMAL}
            });

            VDendgame.init(this, {
                {0, sizeof(TwoDimVertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TwoDimVertex, pos),
                        sizeof(glm::vec3), POSITION},
                {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(TwoDimVertex, UV),
                        sizeof(glm::vec2), UV}
            });

            // Initialization of Pipelines
            Ptaxi.init(this, &VDtaxi, "shaders/BaseVert.spv", "shaders/BaseFrag.spv", {&DSLtaxi});
            Pcity.init(this, &VDcity, "shaders/BaseVert.spv", "shaders/BaseFrag.spv", {&DSLcity});
            // Deactivate culling for the city pipeline (when enabled the models are broken)
            Pcity.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
            Psky.init(this, &VDsky, "shaders/BaseVert.spv", "shaders/SkyFrag.spv", {&DSLsky});
            // Deactivate culling for the sky pipeline (render the skybox from the inside)
            Psky.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
            Pcars.init(this, &VDcars, "shaders/BaseVert.spv", "shaders/BaseFrag.spv", {&DSLcars});
            Ppeople.init(this, &VDpeople, "shaders/BaseVert.spv", "shaders/BaseFrag.spv", {&DSLpeople});
            Ptitle.init(this, &VDtitle, "shaders/TwoDimVert.spv", "shaders/TwoDimFrag.spv", {&DSLtitle});
            // Setting for 2D rendering pipeline
            Ptitle.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
            Pcontrols.init(this, &VDcontrols, "shaders/TwoDimVert.spv", "shaders/TwoDimFrag.spv", {&DSLcontrols});
            Pcontrols.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
            Parrow.init(this, &VDarrow, "shaders/BaseVert.spv", "shaders/ArrowFrag.spv", {&DSLarrow});
            Pendgame.init(this, &VDendgame, "shaders/TwoDimVert.spv", "shaders/TwoDimFrag.spv", {&DSLendgame});
            Pendgame.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);

            // Initialization of Models
            Mtaxi[0].init(this, &VDtaxi, "models/Car_Hatch_C_Door.obj", OBJ);
            Mtaxi[1].init(this, &VDtaxi, "models/Car_Hatch_C_Extern.obj", OBJ);
            Mtaxi[2].init(this, &VDtaxi, "models/Car_Hatch_C_Intern_no-steer.obj", OBJ);
            Mtaxi[3].init(this, &VDtaxi, "models/TruckBodySteeringWheelO.mgcg", MGCG);
            Mtaxi[4].init(this, &VDtaxi, "models/Car_Hatch_C_Wheel.obj", OBJ);
            Mtaxi[5].init(this, &VDtaxi, "models/Car_Hatch_C_Wheel.obj", OBJ);
            Mtaxi[6].init(this, &VDtaxi, "models/Car_Hatch_C_Wheel.obj", OBJ);
			Mtaxi[7].init(this, &VDtaxi, "models/Car_Hatch_C_Wheel.obj", OBJ);
            Msky.init(this, &VDsky, "models/Sphere2.obj", OBJ);
            Mcars[0].init(this, &VDcars, "models/transport_cool_001_transport_cool_001.001.mgcg", MGCG);
            Mcars[1].init(this, &VDcars, "models/transport_cool_003_transport_cool_003.001.mgcg", MGCG);
            Mcars[2].init(this, &VDcars, "models/transport_cool_004_transport_cool_004.001.mgcg", MGCG);
            Mcars[3].init(this, &VDcars, "models/transport_cool_010_transport_cool_010.001.mgcg", MGCG);
            Mcars[4].init(this, &VDcars, "models/transport_jeep_001_transport_jeep_001.001.mgcg", MGCG);
            Mcars[5].init(this, &VDcars, "models/transport_jeep_010_transport_jeep_010.001.mgcg", MGCG);
            Mcars[6].init(this, &VDcars, "models/transport_cool_001_transport_cool_001.001.mgcg", MGCG);
            Mcars[7].init(this, &VDcars, "models/transport_cool_004_transport_cool_004.001.mgcg", MGCG);
            Mcars[8].init(this, &VDcars, "models/transport_cool_010_transport_cool_010.001.mgcg", MGCG);

            std::vector<TwoDimVertex> vertices = {{{-1.0, -1.0, 0.9f}, {0.0f, 0.0f}},
                    {{-1.0, 1.0, 0.9f}, {0.0f, 1.0f}},
                    {{ 1.0,-1.0, 0.9f}, {1.0f, 0.0f}},
                    {{ 1.0, 1.0, 0.9f}, {1.0f, 1.0f}}};
            Mtitle.vertices = std::vector<unsigned char>((unsigned char*)vertices.data(), (unsigned char*)vertices.data() + sizeof(TwoDimVertex) * vertices.size());
            Mtitle.indices = {0, 1, 2, 1, 3, 2};
            Mtitle.initMesh(this, &VDtitle);
            Mcontrols.vertices = std::vector<unsigned char>((unsigned char*)vertices.data(), (unsigned char*)vertices.data() + sizeof(TwoDimVertex) * vertices.size());
            Mcontrols.indices = {0, 1, 2, 1, 3, 2};
            Mcontrols.initMesh(this, &VDcontrols);
            Mendgame.vertices = std::vector<unsigned char>((unsigned char*)vertices.data(), (unsigned char*)vertices.data() + sizeof(TwoDimVertex) * vertices.size());
            Mendgame.indices = {0, 1, 2, 1, 3, 2};
            Mendgame.initMesh(this, &VDendgame);

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

            Marrow.init(this, &VDarrow, "models/simple arrow.obj", OBJ);

            // Initialization of Textures
            Tcity.init(this,"textures/Textures_City.png");
            Tsky.init(this, "textures/images.png");
            Tpeople.init(this, "textures/Humano_01Business_01_Diffuse04.jpg");
            Ttaxy.init(this, "textures/VehiclePack_baseColor.png");
            Ttitle.init(this, "textures/title.jpg");
            Tcontrols.init(this, "textures/controls.jpg");
            Tendgame.init(this, "textures/endgame.jpg");

        }

        // Initialization of Pipelines and Descriptor Sets
        void pipelinesAndDescriptorSetsInit() {

            // Creation of the Pipelines
            Ptaxi.create();
            Pcity.create();
            Psky.create();
            Pcars.create();
            Ppeople.create();
            Ptitle.create();
            Pcontrols.create();
            Parrow.create();
            Pendgame.create();

            // Initialization of the Descriptor Sets
            for(int i = 0; i<8; i++){
                DStaxi[i].init(this, &DSLtaxi, {
                        {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                        {1, TEXTURE, 0, &Ttaxy},
                        {2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr}
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

            DStitle.init(this, &DSLtitle, {
                {0, TEXTURE, 0, &Ttitle}
            });

            DScontrols.init(this, &DSLcontrols, {
                {0, TEXTURE, 0, &Tcontrols}
            });

            DSarrow.init(this, &DSLarrow, {
                {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                {1, UNIFORM, sizeof(ArrowGUBO), nullptr}
            });

            DSendgame.init(this, &DSLendgame, {
                {0, TEXTURE, 0, &Tendgame}
            });
        }

        // Cleanup of Pipelines and Descriptor Sets
        void pipelinesAndDescriptorSetsCleanup() {

            // Cleanup of the Pipelines
            Ptaxi.cleanup();
            Pcity.cleanup();
            Psky.cleanup();
            Pcars.cleanup();
            Ppeople.cleanup();
            Ptitle.cleanup();
            Pcontrols.cleanup();
            Parrow.cleanup();
            Pendgame.cleanup();

            // Cleanup of the Descriptor Sets
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

            DStitle.cleanup();
            DScontrols.cleanup();
            DSarrow.cleanup();
            DSendgame.cleanup();

        }

        // Cleanup of Textures, Models, Descruot Set Layouts and Pipelines
        void localCleanup() {

            // Cleanup of Textures
            Tcity.cleanup();
            Tsky.cleanup();
            Tpeople.cleanup();
            Ttaxy.cleanup();
            Ttitle.cleanup();
            Tcontrols.cleanup();
            Tendgame.cleanup();

            // Cleanup of Models
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
            Mtitle.cleanup();
            Mcontrols.cleanup();
            Marrow.cleanup();
            Mendgame.cleanup();

            // Cleanup of Descriptor Set Layouts
            DSLtaxi.cleanup();
            DSLcity.cleanup();
            DSLsky.cleanup();
            DSLcars.cleanup();
            DSLpeople.cleanup();
            DSLtitle.cleanup();
            DSLcontrols.cleanup();
            DSLarrow.cleanup();
            DSLendgame.cleanup();

            // Cleanup of Pipelines and Descriptor Sets
            Ptaxi.destroy();
            Pcity.destroy();
            Psky.destroy();
            Pcars.destroy();
            Ppeople.destroy();
            Ptitle.destroy();
            Pcontrols.destroy();
            Parrow.destroy();
            Pendgame.destroy();

        }

        // Binding of the Pipelines, Descriptor Sets and Models to the command buffer
        void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

            // If we are not drawing a 2D scene:
            if(!drawTitle && !drawControls && !endGame) {
                // Bind the Pipelines, Descriptor Sets, Models and draw the scene
                Ptaxi.bind(commandBuffer);

                for(int i = 0; i < 8; i++){
                    DStaxi[i].bind(commandBuffer, Ptaxi, 0, currentImage);
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
                    // Check for the person to not be drawn (picked up)
                    if((i == 3 || i == 7 || i == 35 || i == 37 || i == 44) && !drawPeople[i]) {
                        // do nothing ==> do not draw it
                    }
                    else {
                        DSpeople[i].bind(commandBuffer, Ppeople, 0, currentImage);
                        Mpeople[i].bind(commandBuffer);
                        vkCmdDrawIndexed(commandBuffer,
                                        static_cast<uint32_t>(Mpeople[i].indices.size()), 1, 0, 0, 0);
                    }
                }

                Parrow.bind(commandBuffer);
                DSarrow.bind(commandBuffer, Parrow, 0, currentImage);
                Marrow.bind(commandBuffer);
                vkCmdDrawIndexed(commandBuffer,
                                static_cast<uint32_t>(Marrow.indices.size()), 1, 0, 0, 0);

            }
            // Else bind the right Pipeline, Descriptor Set and Model for the 2D scene
            else if(drawTitle) {
                Ptitle.bind(commandBuffer);
                DStitle.bind(commandBuffer, Ptitle, 0, currentImage);
                Mtitle.bind(commandBuffer);
                vkCmdDrawIndexed(commandBuffer,
                                static_cast<uint32_t>(Mtitle.indices.size()), 1, 0, 0, 0);
            }
            else if(drawControls) {
                Pcontrols.bind(commandBuffer);
                DScontrols.bind(commandBuffer, Pcontrols, 0, currentImage);
                Mcontrols.bind(commandBuffer);
                vkCmdDrawIndexed(commandBuffer,
                                static_cast<uint32_t>(Mcontrols.indices.size()), 1, 0, 0, 0);
            }
            else if(endGame) {
                Pendgame.bind(commandBuffer);
                DSendgame.bind(commandBuffer, Pendgame, 0, currentImage);
                Mendgame.bind(commandBuffer);
                vkCmdDrawIndexed(commandBuffer,
                                static_cast<uint32_t>(Mendgame.indices.size()), 1, 0, 0, 0);
            }

        }

        // Main application loop
        void updateUniformBuffer(uint32_t currentImage) {

            static bool debounce = false;
            static int curDebounce = 0;

            static bool autoTime = true;
            static float cTime = 0.0f;
            const float turnTime = 72.0f;
            const float angTurnTimeFact = 2.0f * M_PI / turnTime;
            static float CamPitch = glm::radians(20.0f);
            static float CamYaw = M_PI;
            static float CamRoll = 0.0f;

            glm::vec3 directions[CARS];

            for(int i = 0; i < CARS; i++) {
                directions[i] = glm::normalize(wayPoints[i][currentPoints[i]] - carPositions[i]);
            }

            float speedCar= 4.0f;
            float speed = 0.0f;

            // Standard procedure to quit when the ESC key is pressed
            if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
                // Check if a sound is playing and stop it, then uninitialize it
                if(ma_sound_is_playing(&titleMusic)) ma_sound_stop(&titleMusic);
                ma_sound_uninit(&titleMusic);
                if(ma_sound_is_playing(&ingameMusic)) ma_sound_stop(&ingameMusic);
                ma_sound_uninit(&ingameMusic);
                if(ma_sound_is_playing(&idleEngineSound)) ma_sound_stop(&idleEngineSound);
                ma_sound_uninit(&idleEngineSound);
                if(ma_sound_is_playing(&accelerationEngineSound)) ma_sound_stop(&accelerationEngineSound);
                ma_sound_uninit(&accelerationEngineSound);
                if(ma_sound_is_playing(&pickupSound)) ma_sound_stop(&pickupSound);
                ma_sound_uninit(&pickupSound);
                if(ma_sound_is_playing(&moneySound)) ma_sound_stop(&moneySound);
                ma_sound_uninit(&moneySound);
                if(ma_sound_is_playing(&clacsonSound)) ma_sound_stop(&clacsonSound);
                ma_sound_uninit(&clacsonSound);

                // Uninitialize the sound engine
                ma_engine_uninit(&engine);

                // Case of endless game mode: print the final score
                if(endlessGameMode) {
                    std::cout << "\n\n\n\t------------- FINAL SCORE -----------\n" << std::endl;
                    std::cout << "\tTotal drives completed:\t" << totDrivesCompleted << std::endl;
                    std::cout << "\tTotal earnings:        \t" << money << " $"<< std::endl;
                    std::cout << "\n\t------------- FINAL SCORE -------------" << std::endl;
                }

                // Quit the application
                glfwSetWindowShouldClose(window, GL_TRUE);
            }

            // Check if the space key is pressed to change the scene
            if(glfwGetKey(window, GLFW_KEY_SPACE) && currScene != 3) {
                if(!debounce) {
                    debounce = true;
                    curDebounce = GLFW_KEY_SPACE;
                    /* Increment the scene counter and rebuild the pipeline
                     * Scenes:
                     * -2: Title scene
                     * -1: Controls scene
                     *  0: Third person view
                     *  1: First person view
                     *  2: Photo mode
                     *  3: End game scene
                     */
                    currScene = (currScene + 1) % INGAME_SCENE_COUNT;
                    if(currScene != -2) {
                        drawTitle = false;
                    }
                    if(currScene == -1) {
                        drawControls = true;
                    }
                    if(currScene != -1) {
                        drawControls = false;
                    }
                    RebuildPipeline();
                }
            } else {
                if((curDebounce == GLFW_KEY_SPACE) && debounce) {
                    debounce = false;
                    curDebounce = 0;
                }
            }

            // Check if the P key is pressed to change the scene to photo mode
            if (glfwGetKey(window, GLFW_KEY_P) && currScene != 3) {
                if (!debounce) {
                    debounce = true;
                    curDebounce = GLFW_KEY_P;
					// Check if I'm already in photo mode
					if (currScene == 2) {
                        currScene = lastSavedSceneValue;
					}
                    else {
						lastSavedSceneValue = currScene;
						currScene = 2;
                    }
                    RebuildPipeline();
                }
            }
            else {
                if ((curDebounce == GLFW_KEY_P) && debounce) {
                    debounce = false;
                    curDebounce = 0;
                }
            }
            
            float deltaT;
            glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
            bool fire = false;
            getSixAxis(deltaT, m, r, fire);

            if (autoTime) {
                cTime += deltaT;
                cTime = (cTime > turnTime) ? (cTime - turnTime) : cTime;
            }

            static float steeringAngCars[CARS];
            // Update the position of the NPC cars only if not in photo mode
            if(currScene != 2) {
                for(int i = 0; i < CARS; i++) {
                    // Update the position of the NPC cars
                    carPositions[i] += directions[i] * speedCar * deltaT;
                    // Update the target to the next waypoint when the car approaches one
                    if (glm::distance(carPositions[i], wayPoints[i][currentPoints[i]]) < 0.25f) {
                        currentPoints[i] = (currentPoints[i] + 1) % wayPoints[i].size();
                    }
                    float targetSteering = atan2(directions[i].x, directions[i].z);
                    steeringAngCars[i]= fmod(targetSteering + M_PI, 2.0f * M_PI) - M_PI;
                }
            }

            static float steeringAng = 0.0f;
            float oldSteeringAng = steeringAng;
            glm::mat4 mView;

            // If the scene is the title or control and the title muisc is not playing, start it
            if((currScene == -2 || currScene == -1) && !ma_sound_is_playing(&titleMusic)) {
                    ma_sound_start(&titleMusic);
            }
            else {
                // If the scene is not the title and the control and the title music is playing, stop it and start the in-game music
                if(currScene != -2 && currScene != -1 && ma_sound_is_playing(&titleMusic) && !ma_sound_is_playing(&ingameMusic)) {
                    ma_sound_stop(&titleMusic);
                    ma_sound_start(&ingameMusic);
                }
                // If the scene is first/third person view
                if(currScene == 0 || currScene == 1) {
                    alreadyInPhotoMode = false;
                    // Third person view or first person
                    const float steeringSpeed = glm::radians(45.0f);
                    const float moveSpeed = 7.5f;

                    static float currentSpeed = 0.0f;
                    float targetSpeed = moveSpeed * -m.z;
                    // Adjust this value to control the damping effect
                    const float dampingFactor = 3.0f;
                    float speedDifference = targetSpeed - currentSpeed;
                    if (fabs(speedDifference) < 0.01f) {
                        currentSpeed = targetSpeed;
                    } else {
                        currentSpeed += speedDifference * dampingFactor * deltaT;
                    }
                    if(!openDoor && !closeDoor) {
                        speed = currentSpeed * deltaT;
                    }
                    wheelRoll -= currentSpeed;       
                    speed = (abs(speed) < 0.01f) ? 0.0f : speed;
                    oldSteeringAng = steeringAng;
                    steeringAng += (speed >= 0 ? -m.x : m.x) * steeringSpeed * deltaT;
                    if (steeringAng == oldSteeringAng) {
                        wheelAndSteerAng = (wheelAndSteerAng < 0.0f ? wheelAndSteerAng + 0.25f : (wheelAndSteerAng > 0.0f ? wheelAndSteerAng - 0.25f : wheelAndSteerAng));
                    }
                    else if (steeringAng > oldSteeringAng) {
                        wheelAndSteerAng = (wheelAndSteerAng > -1.5f ? wheelAndSteerAng - 0.25f : wheelAndSteerAng);
                    }
                    else {
                        wheelAndSteerAng = (wheelAndSteerAng < 1.5f ? wheelAndSteerAng + 0.25f : wheelAndSteerAng);
                    }
                    if (speed == 0.0f) {
						steeringAng = oldSteeringAng;
                    }
                    
                    // Set the four collision points of the taxi (four corners of the car)
                    for(int i = 0; i < TAXI_COLL_PCOUNT; i++) {
                        taxiCollisionPoints[i] = glm::translate(glm::rotate(glm::translate(glm::mat4(1.0f), taxiPos), steeringAng, glm::vec3(0.0f, 1.0f, 0.0f)), taxiCollPOffsets[i])[3];
                    }

                    // Boolean variable to check if the taxi is in collision with one of the collision boxes
                    bool collisionCheck = checkCollision(taxiCollisionPoints, TAXI_COLL_PCOUNT, externalCollisionBox, true);
                    for(int i = 0; i < COLLISION_BOXES_COUNT; i++) {
                        collisionCheck = collisionCheck && checkCollision(taxiCollisionPoints, TAXI_COLL_PCOUNT, internalCollisionBoxes[i], false);
                    }
                    // If the taxi is not in collision, update the position
                    if(collisionCheck) {
                        taxiPos = taxiPos + glm::vec3(speed * sin(steeringAng), 0.0f, speed * cos(steeringAng));
                    }

                    // If the taxi is running
                    if (speed != 0.0f) {
                        // If the idle engine sound is playing, stop it and reset the acceleration engine sound
                        if(ma_sound_is_playing(&idleEngineSound)) {
                            ma_sound_stop(&idleEngineSound);
                            ma_sound_seek_to_pcm_frame(&accelerationEngineSound, 0);
                        }
                        // Start the acceleration engine sound
                        ma_sound_start(&accelerationEngineSound);
                    }
                    else {
                        // Else check if the acceleration engine sound is playing, stop it and reset the idle engine sound
                        if(ma_sound_is_playing(&accelerationEngineSound)) {
                            ma_sound_stop(&accelerationEngineSound);
                            ma_sound_seek_to_pcm_frame(&idleEngineSound, 0);
                        }
                        // Start the idle engine sound
                        ma_sound_start(&idleEngineSound);
                    }

                    // Calculate how much the taxi has turned
                    float actualTurn = steeringAng - oldSteeringAng;
                    // If the rotation is not 0, update the front light direction
                    if(actualTurn != 0.0f) {
                        frontLightDirection = glm::vec4(glm::rotate(glm::mat4(1.0), actualTurn, glm::vec3(0.0f, 1.0f, 0.0f)) * frontLightDirection);
                    }

                    // If we are in the third person view
                    if (currScene == 0) {

                        // Update camera position for lookAt view, keeping into account the current SteeringAng
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
                    }
                    // Else if we are in the first person view
                    else {
                        const float ROT_SPEED = glm::radians(120.0f);
                        CamYaw -= ROT_SPEED * deltaT * r.y;
                        CamPitch -= ROT_SPEED * deltaT * r.x;
                        CamRoll -= ROT_SPEED * deltaT * r.z;
                        CamYaw = (CamYaw < M_PI_2 ? M_PI_2 : (CamYaw > 1.5 * M_PI ? 1.5 * M_PI : CamYaw));
                        CamPitch = (CamPitch < -0.25 * M_PI ? -0.25 * M_PI : (CamPitch > 0.25 * M_PI ? 0.25 * M_PI : CamPitch));
                        CamRoll = (CamRoll < -M_PI ? -M_PI : (CamRoll > M_PI ? M_PI : CamRoll));

                        glm::vec3 camOffset(0.35f, 1.05f, 0.7f);
                        glm::vec3 rotatedCamOffset = glm::vec3(
                            glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0)) * glm::vec4(camOffset, 1.0)
                        );
                        camPos = taxiPos + rotatedCamOffset;
                        // TODO top ma magari guardare dritto quando si passa in prima persona sarebbe carino
                        mView=
                            glm::rotate(glm::mat4(1.0f), -CamRoll, glm::vec3(0, 0, 1)) *
                            glm::rotate(glm::mat4(1.0f), -CamPitch, glm::vec3(1, 0, 0)) *
                            glm::rotate(glm::mat4(1.0f), -CamYaw - steeringAng, glm::vec3(0, 1, 0)) *
                            glm::translate(glm::mat4(1.0f), -camPos);

                    }
                }
                // Else if we are in photo mode
                else if(currScene == 2) {

                    if(!alreadyInPhotoMode) {
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

                const float nearPlane = 0.1f;
                const float farPlane = 375.0f;
                glm::mat4 Prj = glm::perspective(glm::radians(45.0f), Ar, nearPlane, farPlane);
                //Projection matrix
                Prj[1][1] *= -1;


                //World matrix for city
                glm::mat4 mWorld;
                mWorld = glm::translate(glm::mat4(1), glm::vec3(0, 0, 3)) * glm::rotate(glm::mat4(1), glm::radians(180.0f), glm::vec3(0, 1, 0));

                // Set the center and the scale (radius) of the sky box sphere
                glm::vec3 sphereCenter = glm::vec3(40.0f, 0.0f, -75.0f);
                glm::vec3 sphereScale = glm::vec3(180.0f);
                // Offset from the sky box sphere
                float sunOffset = 10.0f;
                // Set the sun position rotating around the X and Y axis, Z position is fixed
                glm::vec3 sunPos = glm::vec3(sphereCenter.x + (sphereScale.x - sunOffset) * cos(cTime * angTurnTimeFact), // x
                                            sphereCenter.y + (sphereScale.x - sunOffset) * sin(cTime * angTurnTimeFact), // y
                                            sphereCenter.z);

                // Check when the sun is below the horizon ==> set the night to true
                isNight = (sunPos.y < 0.0f ? true : false);

                // If we have to open the door
                if(openDoor) {
                    // Update the angle of the door
                    openingDoorAngle += 5.0f;
                    // Check when the door reaches the maximum angle
                    if (openingDoorAngle >= 69.0f) {
                        openDoor = false;
                        // Start the animation to close the door
                        closeDoor = true;
                    }
                }
                // If we have to close the door ==> same as open but reverted
                else {
                    openingDoorAngle -= 5.0f;
                    if (openingDoorAngle <= 0.0f) {
                        openingDoorAngle = 0.0f;
                        closeDoor = false;
                    }
                }
                
                glm::mat4 mWorldTaxi[8];

                // Taxi's world matrix 
                mWorldTaxi[1] = mWorldTaxi[2] =
                glm::translate(glm::mat4(1.0), taxiPos) *
                glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0));

				glm::vec3 offsets[6] = {
					glm::vec3(-0.65f, 0.23f, 2.05f), //FR wheel
					glm::vec3(0.65f, 0.23f, 2.05f), //FL wheel
					glm::vec3(-0.65f, 0.2f, -0.1f), //BR wheel
					glm::vec3(0.65f, 0.2f, -0.1f), //BL wheel
					glm::vec3(0.45f, 0.75f, 1.5f), //steer
					glm::vec3(-0.742f, 0.695f, 1.6f) //door
				};
                glm::vec3 rotatedOffSet[6], finalWorldPos[6];
                for (int i = 0; i < 6; i++) {
					rotatedOffSet[i] = glm::vec3(glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0)) * glm::vec4(offsets[i], 1.0));
					finalWorldPos[i] = taxiPos + rotatedOffSet[i];
                }
                // Wheel's world matrix
				mWorldTaxi[4] =  //FR wheel
                    glm::translate(glm::mat4(1.0), finalWorldPos[0]) *
                    glm::rotate(glm::mat4(1.0), steeringAng - glm::radians(wheelAndSteerAng*15), glm::vec3(0, 1, 0)) *
					glm::rotate(glm::mat4(1.0), wheelRoll, glm::vec3(1, 0, 0)) * //when I accelerate the wheel should spin
                    glm::rotate(glm::mat4(1.0), glm::radians(180.0f), glm::vec3(0, 0, 1)); //the wheel was facing left
				mWorldTaxi[5] =  //FL wheel
                    glm::translate(glm::mat4(1.0), finalWorldPos[1]) *
                    glm::rotate(glm::mat4(1.0), steeringAng - glm::radians(wheelAndSteerAng * 15), glm::vec3(0, 1, 0)) *
                    glm::rotate(glm::mat4(1.0), wheelRoll, glm::vec3(1, 0, 0)); //when I accelerate the wheel should spin
				mWorldTaxi[6] = //BR wheel
                    glm::translate(glm::mat4(1.0), finalWorldPos[2]) *
                    glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0)) *
                    glm::rotate(glm::mat4(1.0), wheelRoll, glm::vec3(1, 0, 0)) *
                    glm::rotate(glm::mat4(1.0), glm::radians(180.0f), glm::vec3(0, 0, 1));
				mWorldTaxi[7] = //BL wheel
                    glm::translate(glm::mat4(1.0), finalWorldPos[3]) *
                    glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0)) *
                    glm::rotate(glm::mat4(1.0), wheelRoll, glm::vec3(1, 0, 0)); //when I accelerate the wheel should spin
				mWorldTaxi[3] = //steer
                    glm::translate(glm::mat4(1.0), finalWorldPos[4]) *
                    glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0)) *
                    glm::rotate(glm::mat4(1.0), wheelAndSteerAng, glm::vec3(0, 0, 1));
				mWorldTaxi[0] = //door
					glm::translate(glm::mat4(1.0), finalWorldPos[5]) *
					glm::rotate(glm::mat4(1.0), steeringAng + glm::radians(openingDoorAngle), glm::vec3(0, 1, 0));
                 

                // Set the position where there will be the taxi lights (point for back, spot for front)
                glm::vec4 taxiLightPos[TAXI_LIGHT_COUNT] = {glm::translate(mWorldTaxi[1], glm::vec3(-0.5f, 0.5f, -0.75f))[3], // rear right
                                                            glm::translate(mWorldTaxi[1], glm::vec3(0.5f, 0.5f, -0.75f))[3], // rear left
                                                            glm::translate(mWorldTaxi[1], glm::vec3(-0.6f, 0.6f, 2.6f))[3], // front right
                                                            glm::translate(mWorldTaxi[1], glm::vec3(0.6f, 0.6f, 2.6f))[3]}; // front left
                
                // If we are not in photo mode, update the position of the NPC cars
                if(currScene != 2) {
                    for(int i = 0; i < CARS; i++) {
                        mWorldCars[i] = glm::translate(glm::mat4(1.0), carPositions[i]) *
                                        glm::rotate(glm::mat4(1.0), steeringAngCars[i], glm::vec3(0, 1, 0));
                    }
                }

                // If we don't have already selected a random person to pick up
                if(!pickupPointSelected) {
                    // Randomly select an index for the pickup person point [0 - 4]
                    random_index = rand() % PICKUP_COUNT;
                    // Get the position of the randomly selected person
                    pickupPoint = pickupPoints[random_index];
                    // Ste the flag to true to not choose another one
                    pickupPointSelected = true;
                }

                // If the taxi is close to the person to pick up, we have not already picked up the person and we are not moving
                if(glm::distance(glm::vec3(pickupPoint), taxiPos) < MIN_DISTANCE_TO_PICKUP && !pickedPassenger && speed == 0.0f) {
                    // Get the hash map index of the selected person
                    int map_index = ((random_index == 0) ? 3 : ((random_index == 1) ? 7 : ((random_index == 2) ? 35 : ((random_index == 3) ? 37 : 44))));
                    // Set the value in the hash map to false ==> when the pipeline is rebuilded, we will not draw it
                    drawPeople[map_index] = false;
                    // Set the flag to true and start the animation to open the door
                    pickedPassenger = true;
                    openDoor = true;
                    // Rebuild the pipeline to not draw the picked up person
                    RebuildPipeline();
                    // Reset the sound of the pickup and start it
                    if(ma_sound_at_end(&pickupSound)) ma_sound_seek_to_pcm_frame(&pickupSound, 0);
                    ma_sound_start(&pickupSound);
                    // Get the position of the point where to take the person
                    dropoffPoint = dropoffPoints[random_index];
                    // Save the time of the pickup to calculate the income at the end
                    pickupTime = time(NULL);
                }

                // If the taxi is close to the dropoff point, we have already picked up the person and we are not moving
                if(glm::distance(glm::vec3(dropoffPoint), taxiPos) < MIN_DISTANCE_TO_PICKUP && pickedPassenger && speed == 0.0f) {
                    int map_index = ((random_index == 0) ? 3 : ((random_index == 1) ? 7 : ((random_index == 2) ? 35 : ((random_index == 3) ? 37 : 44))));
                    drawPeople[map_index] = true;
                    pickedPassenger = false;
                    openDoor = true;
                    pickupPointSelected = false;
                    RebuildPipeline();
                    if(ma_sound_at_end(&moneySound)) ma_sound_seek_to_pcm_frame(&moneySound, 0);
                    ma_sound_start(&moneySound);
                    money += (time(NULL) - pickupTime) * (isNight ? 7.9f : 4.1f);
                    if(!endlessGameMode) {
                        currScene = 3;
                        endGame = true;
                        RebuildPipeline();
                        if(ma_sound_is_playing(&idleEngineSound)) {
                            ma_sound_stop(&idleEngineSound);
                        }
                        if(ma_sound_is_playing(&accelerationEngineSound)) {
                            ma_sound_stop(&accelerationEngineSound);
                        }
                        std::cout << "\n\n\n\t--------- FINAL SCORE ---------\n" << std::endl;
                        std::cout << "\tTotal earnings: " << money << " $"<< std::endl;
                        std::cout << "\n\t--------- FINAL SCORE ---------" << std::endl;
                    }
                    else {
                        totDrivesCompleted++;
                    }
                }

                nlohmann::json js;
                std::ifstream ifs2("models/city.json");
                if (!ifs2.is_open()) {
                    std::cout << "[ ERROR ]: Scene file not found!" << std::endl;
                    exit(-1);
                }
                try {
                    json j;
                    ifs2>>j;

                    float TMj[16];

                    for(int k = 0; k < MESH; k++) {

                        nlohmann::json TMjson = j["instances"][k]["transform"];
                        for(int l = 0; l < 16; l++) {
                            TMj[l] = TMjson[l];
                        }
                        mWorld=glm::mat4(TMj[0],TMj[4],TMj[8],TMj[12],TMj[1],TMj[5],TMj[9],TMj[13],TMj[2],TMj[6],TMj[10],TMj[14],TMj[3],TMj[7],TMj[11],TMj[15]);
                        uboCity[k].mMat = mWorld;
                        uboCity[k].nMat = glm::inverse(glm::transpose(uboCity[k].mMat));
                        uboCity[k].mvpMat = Prj * mView * mWorld;
                        DScity[k].map(currentImage, &uboCity[k], sizeof(uboCity[k]), 0);
                        guboCity[k].directLightPos = glm::vec4(sunPos, 1.0f);
                        for(int i = 0; i < TAXI_LIGHT_COUNT; i++) {
                            guboCity[k].taxiLightPos[i] = taxiLightPos[i];
                        }
                        guboCity[k].directLightCol = sunCol;
                        guboCity[k].rearLightCol = rearLightColor;
                        guboCity[k].frontLightCol = frontLightColor;
                        guboCity[k].frontLightDir = frontLightDirection;
                        guboCity[k].frontLightCosines = frontLightCosines;
                        std::unordered_map<float, glm::vec3> distancesToPositions;
                        std::vector<float> distances;
                        float dist = 0.0f;
                        for(int i = 0; i < STREET_LIGHT_COUNT; i++) {
                            dist = glm::distance(streetlightPos[i], glm::vec3(mWorld[3]));
                            distances.push_back(dist);
                            distancesToPositions[dist] = streetlightPos[i];
                        }
                        std::sort(distances.begin(), distances.end());
                        for(int i = 0; i < MAX_STREET_LIGHTS; i++) {
                            guboCity[k].streetLightPos[i] = glm::vec4(distancesToPositions[distances[i]], 1.0f);
                        }
                        guboCity[k].streetLightCol = streetLightCol;
                        guboCity[k].streetLightDirection = streetLightDirection;
                        guboCity[k].streetLightCosines = streetLightCosines;
                        guboCity[k].pickupPointPos = (!pickedPassenger ? glm::vec4(pickupPoint.x, PICKUP_POINT_Y_OFFSET, pickupPoint.z, pickupPoint.w) : glm::vec4(dropoffPoint.x, PICKUP_POINT_Y_OFFSET, dropoffPoint.z, dropoffPoint.w));
                        guboCity[k].pickupPointCol = pickupPointColor;
                        guboCity[k].eyePos = glm::vec4(camPos, 1.0f);
                        guboCity[k].gammaMetallicSettingsNight = glm::vec4(128.0f, 0.1f, float(graphicsSettings), (isNight ? 1.0f : 0.0f));
                        DScity[k].map(currentImage, &guboCity[k], sizeof(guboCity[k]), 2);
                    }

                }
                catch (const nlohmann::json::exception& e) {
                    std::cout << "[ EXCEPTION ]: " << e.what() << std::endl;
                    exit(1);
                }

                for(int i=0; i<8; i++){
                    uboTaxi[i].mMat = mWorldTaxi[i];
                    uboTaxi[i].nMat = glm::inverse(glm::transpose(uboTaxi[i].mMat));
                    uboTaxi[i].mvpMat = Prj * mView * uboTaxi[i].mMat;
                    DStaxi[i].map(currentImage, &uboTaxi[i], sizeof(uboTaxi[i]), 0);

                    guboTaxi[i].directLightPos = glm::vec4(sunPos, 1.0f);
                    for(int j = 0; j < TAXI_LIGHT_COUNT; j++) {
                        guboTaxi[i].taxiLightPos[j] = taxiLightPos[j];
                    }
                    guboTaxi[i].directLightCol = sunCol;
                    guboTaxi[i].rearLightCol = rearLightColor;
                    guboTaxi[i].frontLightCol = frontLightColor;
                    guboTaxi[i].frontLightDir = frontLightDirection;
                    guboTaxi[i].frontLightCosines = frontLightCosines;
                    std::unordered_map<float, glm::vec3> distancesToPositions;
                    std::vector<float> distances;
                    float dist = 0.0f;
                    for(int j = 0; j < STREET_LIGHT_COUNT; j++) {
                        dist = glm::distance(streetlightPos[j], glm::vec3(mWorldTaxi[1][3]));
                        distances.push_back(dist);
                        distancesToPositions[dist] = streetlightPos[j];
                    }
                    std::sort(distances.begin(), distances.end());
                    for(int j = 0; j < MAX_STREET_LIGHTS; j++) {
                        guboTaxi[i].streetLightPos[j] = glm::vec4(distancesToPositions[distances[j]], 1.0f);
                    }
                    guboTaxi[i].streetLightCol = streetLightCol;
                    guboTaxi[i].streetLightDirection = streetLightDirection;
                    guboTaxi[i].streetLightCosines = streetLightCosines;
                    guboTaxi[i].pickupPointPos = (!pickedPassenger ? glm::vec4(pickupPoint.x, PICKUP_POINT_Y_OFFSET, pickupPoint.z, pickupPoint.w) : glm::vec4(dropoffPoint.x, PICKUP_POINT_Y_OFFSET, dropoffPoint.z, dropoffPoint.w));
                    guboTaxi[i].pickupPointCol = pickupPointColor;
                    guboTaxi[i].eyePos = glm::vec4(camPos, 1.0f);
                    guboTaxi[i].gammaMetallicSettingsNight = glm::vec4(128.0f, 1.0f, float(graphicsSettings), (isNight ? 1.0f : 0.0f));
                    DStaxi[i].map(currentImage, &guboTaxi[i], sizeof(guboTaxi[i]), 2);
                }

                glm::vec4 taxiCollisionSphereCenter = glm::translate(mWorldTaxi[1], glm::vec3(0.0f, 0.0f, 1.0f))[3];
                

                for(int i = 0; i < CARS; i++) {
                    uboCars[i].mvpMat = Prj * mView * mWorldCars[i];
                    uboCars[i].mMat = mWorldCars[i];
                    uboCars[i].nMat = glm::inverse(glm::transpose(uboCars[i].mMat));
                    DScars[i].map(currentImage, &uboCars[i], sizeof(uboCars[i]), 0);
                    guboCars[i].directLightPos = glm::vec4(sunPos, 1.0f);
                    for(int j = 0; j < TAXI_LIGHT_COUNT; j++) {
                        guboCars[i].taxiLightPos[j] = taxiLightPos[j];
                    }
                    guboCars[i].directLightCol = sunCol;
                    guboCars[i].rearLightCol = rearLightColor;
                    guboCars[i].frontLightCol = frontLightColor;
                    guboCars[i].frontLightDir = frontLightDirection;
                    guboCars[i].frontLightCosines = frontLightCosines;
                    std::unordered_map<float, glm::vec3> distancesToPositions;
                    std::vector<float> distances;
                    float dist = 0.0f;
                    for(int j = 0; j < STREET_LIGHT_COUNT; j++) {
                        dist = glm::distance(streetlightPos[j], glm::vec3(mWorldCars[i][3]));
                        distances.push_back(dist);
                        distancesToPositions[dist] = streetlightPos[j];
                    }
                    std::sort(distances.begin(), distances.end());
                    for(int j = 0; j < MAX_STREET_LIGHTS; j++) {
                        guboCars[i].streetLightPos[j] = glm::vec4(distancesToPositions[distances[j]], 1.0f);
                    }
                    guboCars[i].streetLightCol = streetLightCol;
                    guboCars[i].streetLightDirection = streetLightDirection;
                    guboCars[i].streetLightCosines = streetLightCosines;
                    guboCars[i].pickupPointPos = (!pickedPassenger ? glm::vec4(pickupPoint.x, PICKUP_POINT_Y_OFFSET, pickupPoint.z, pickupPoint.w) : glm::vec4(dropoffPoint.x, PICKUP_POINT_Y_OFFSET, dropoffPoint.z, dropoffPoint.w));
                    guboCars[i].pickupPointCol = pickupPointColor;
                    guboCars[i].eyePos = glm::vec4(camPos, 1.0f);
                    guboCars[i].gammaMetallicSettingsNight = glm::vec4(128.0f, 1.0f, float(graphicsSettings), (isNight ? 1.0f : 0.0f));
                    DScars[i].map(currentImage, &guboCars[i], sizeof(guboCars[i]), 2);
                }

                glm::vec4 carCollisionSphereCenter = glm::vec4(0.0f);
                collisionCounter = 0;
                for(int i = 0; i < CARS; i++) {
                    carCollisionSphereCenter = mWorldCars[i][3];
                    if(glm::distance(glm::vec3(taxiCollisionSphereCenter), glm::vec3(carCollisionSphereCenter)) < 2 * COLLISION_SPHERE_RADIUS) {
                        collisionCounter++;
                    }
                }
                if(collisionCounter > 0 && !inCollisionZone) {
                    inCollisionZone = true;
                    money -= 100.0f;
                    if(ma_sound_at_end(&clacsonSound)) ma_sound_seek_to_pcm_frame(&clacsonSound, 0);
                    ma_sound_start(&clacsonSound);
                }
                else if(collisionCounter == 0 && inCollisionZone) {
                    inCollisionZone = false;
                }


                glm::mat4 scaleMat = glm::translate(glm::mat4(1.0f), sphereCenter) * glm::scale(glm::mat4(1.0f), sphereScale);
                uboSky.mvpMat = Prj * mView * (scaleMat);
                uboSky.mMat = scaleMat;
                uboSky.nMat = glm::inverse(glm::transpose(uboSky.mMat));
                DSsky.map(currentImage, &uboSky, sizeof(uboSky), 0);
                guboSky.lightDir = glm::vec4(sunPos, 1.0f);
                guboSky.lightColor = glm::vec4(1.0f);
                guboSky.eyePos = glm::vec4(camPos, 1.0f);
                guboSky.gammaAndMetallic = glm::vec4(128.0f, 0.1f, 0.0f, 0.0f);
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
                        guboPeople[k].directLightPos = glm::vec4(sunPos, 1.0f);
                        for(int j = 0; j < TAXI_LIGHT_COUNT; j++) {
                            guboPeople[k].taxiLightPos[j] = taxiLightPos[j];
                        }
                        guboPeople[k].directLightCol = sunCol;
                        guboPeople[k].rearLightCol = rearLightColor;
                        guboPeople[k].frontLightCol = frontLightColor;
                        guboPeople[k].frontLightDir = frontLightDirection;
                        guboPeople[k].frontLightCosines = frontLightCosines;
                        std::unordered_map<float, glm::vec3> distancesToPositions;
                        std::vector<float> distances;
                        float dist = 0.0f;
                        for(int i = 0; i < STREET_LIGHT_COUNT; i++) {
                            dist = glm::distance(streetlightPos[i], glm::vec3(mWorld[3]));
                            distances.push_back(dist);
                            distancesToPositions[dist] = streetlightPos[i];
                        }
                        std::sort(distances.begin(), distances.end());
                        for(int i = 0; i < MAX_STREET_LIGHTS; i++) {
                            guboPeople[k].streetLightPos[i] = glm::vec4(distancesToPositions[distances[i]], 1.0f);
                        }
                        guboPeople[k].streetLightCol = streetLightCol;
                        guboPeople[k].streetLightDirection = streetLightDirection;
                        guboPeople[k].streetLightCosines = streetLightCosines;
                        guboPeople[k].pickupPointPos = (!pickedPassenger ? glm::vec4(pickupPoint.x, PICKUP_POINT_Y_OFFSET, pickupPoint.z, pickupPoint.w) : glm::vec4(dropoffPoint.x, PICKUP_POINT_Y_OFFSET, dropoffPoint.z, dropoffPoint.w));
                        guboPeople[k].pickupPointCol = pickupPointColor;
                        guboPeople[k].eyePos = glm::vec4(camPos, 1.0f);
                        guboPeople[k].gammaMetallicSettingsNight = glm::vec4(128.0f, 0.1f, float(graphicsSettings), (isNight ? 1.0f : 0.0f));
                        DSpeople[k].map(currentImage, &guboPeople[k], sizeof(guboPeople[k]), 2);
                    }

                }
                catch (const nlohmann::json::exception& e) {
                    std::cout << "[ EXCEPTION ]: " << e.what() << std::endl;
                    exit(1);
                }

                glm::vec3 arrowPosition = (!pickedPassenger ? glm::vec3(pickupPoint.x, ARROW_Y_OFFSET + (glm::cos(cTime) / 4.0f), pickupPoint.z) : glm::vec3(dropoffPoint.x, ARROW_Y_OFFSET + (glm::cos(cTime) / 4.0f), dropoffPoint.z));
                glm::mat4 mWorldArrow = glm::rotate(glm::rotate(glm::translate(glm::mat4(1.0), arrowPosition), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)), glm::radians(10.0f) * cTime, glm::vec3(0.0f, 1.0f, 0.0f));
                uboArrow.mvpMat = Prj * mView * mWorldArrow;
                uboArrow.mMat = mWorldArrow;
                uboArrow.nMat = glm::inverse(glm::transpose(uboArrow.mMat));
                DSarrow.map(currentImage, &uboArrow, sizeof(uboArrow), 0);
                guboArrow.pickupPointPos = (!pickedPassenger ? glm::vec4(pickupPoint.x, PICKUP_POINT_Y_OFFSET, pickupPoint.z, pickupPoint.w) : glm::vec4(dropoffPoint.x, PICKUP_POINT_Y_OFFSET, dropoffPoint.z, dropoffPoint.w));
                guboArrow.pickupPointCol = pickupPointColor;
                guboArrow.eyePos = glm::vec4(camPos, 1.0f);
                guboArrow.gammaAndMetallic = glm::vec4(128.0f, 1.0f, 0.0f, 0.0f);
                DSarrow.map(currentImage, &guboArrow, sizeof(guboArrow), 1);

            }
        }

        bool checkCollision(glm::vec3 *points, int dim, CollisionBox collBox, bool ext) {
            if(ext) {
                for(int i = 0; i < dim; i++) {
                    if(points[i].x <= collBox.xMin || points[i].x >= collBox.xMax || points[i].z <= collBox.zMin || points[i].z >= collBox.zMax) {
                        return false;
                    }
                }
            }
            else {
                for(int i = 0; i < dim; i++) {
                    if((points[i].x >= collBox.xMin && points[i].x <= collBox.xMax) && (points[i].z >= collBox.zMin && points[i].z <= collBox.zMax)) {
                        return false;
                    }
                }
            }
            return true;
        }

};


// This is the main: probably you do not need to touch this!
int main(int argc, char* argv[]) {

    Application app;

    int choose = 0;
    int oldChoose = 0;
    int gameMode = 0;
    const char* gameModes[GAMEMODE_COUNT] = {"Arcade", "Endless"};
    int graphicSetting = 2;
    const char* gSettings[GRAPHICS_SETTINGS_COUNT] = {"Low", "Medium", "High"};
    float musicVolume = 25.0f;
    float soundVolume = 100.0f;
    
    std::ifstream f("files/logo.txt");
    if (f.is_open()) {
        std::cout << f.rdbuf();
    }
    do {
        std::cout << "--------- MAIN MENU ---------\n" << std::endl;
        std::cout << "1 - Start the game" << std::endl;
        std::cout << "2 - Settings" << std::endl;
        std::cout << "3 - Exit" << std::endl;
        std::cout << "\nChoosing: ";
        std::cin >> choose;
        switch(choose) {
            case 2: {
                oldChoose = choose;
                do {
                    std::cout << std::fixed;
                    std::cout << std::setprecision(2);
                    std::cout << "\n--------- SETTINGS ---------\n" << std::endl;
                    std::cout << "1 - Game mode:          " << "\t< " << gameModes[gameMode] << " >" << std::endl;
                    std::cout << "2 - Graphics settings:  " << "\t< " << gSettings[graphicSetting]  << " > " << std::endl;
                    std::cout << "3 - Music volume:       " << "\t< " << musicVolume << " >" << std::endl;
                    std::cout << "4 - Sound and FX volume:" << "\t< " << soundVolume << " >" << std::endl;
                    std::cout << "5 - Back" << std::endl;
                    std::cout << "\nChoosing: ";
                    std::cin >> choose;
                    switch(choose) {
                        case 1: {
                            gameMode = (gameMode + 1) % GAMEMODE_COUNT;
                            break;
                        }
                        case 2: {
                            graphicSetting = (graphicSetting + 1) % GRAPHICS_SETTINGS_COUNT;
                            break;
                        }
                        case 3: {
                            do {
                                std::cout << "Enter the music volume [0.0 - 100.0]: ";
                                std::cin >> musicVolume;
                            } while(musicVolume < 0.0f || musicVolume > 100.0f);
                            break;
                        }
                        case 4: {
                            do {
                                std::cout << "Enter the sound and FX volume [0.0 - 100.0]: ";
                                std::cin >> soundVolume;
                            } while(soundVolume < 0.0f || soundVolume > 100.0f);
                            break;
                        }
                        case 5:
                            break;
                        default: 
                            break;
                    }
                } while(choose != 5);
                choose = oldChoose;
                break;
            }
            case 3: {
                std::cout << "Closing the sofware..." << std::endl;
                return EXIT_SUCCESS;
            }
            default:
                break;
        }
    } while(choose == 2);

    app.graphicsSettings = graphicSetting;
    app.endlessGameMode = (gameMode == 1);

    std::cout << "[ SOUND ]: Initializing sound resources..." << std::endl;
    // initialize the miniaudio engine
    ma_result result = ma_engine_init(NULL, &app.engine);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize miniaudio engine!");
    }
    // initialize title music
    result = ma_sound_init_from_file(&app.engine, "audios/title.mp3", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.titleMusic);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize title sound!");
    }
    ma_sound_set_looping(&app.titleMusic, MA_TRUE);
    ma_sound_set_volume(&app.titleMusic, musicVolume / 100.0f);
    // initialize in-game music
    result = ma_sound_init_from_file(&app.engine, "audios/ingame.mp3", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.ingameMusic);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize ingame sound!");
    }
    ma_sound_set_looping(&app.ingameMusic, MA_TRUE);
    ma_sound_set_volume(&app.ingameMusic, musicVolume / 100.0f);
    // initialize idle engine sound
    result = ma_sound_init_from_file(&app.engine, "audios/idle.wav", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.idleEngineSound);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize idle engine sound!");
    }
    ma_sound_set_looping(&app.idleEngineSound, MA_TRUE);
    ma_sound_set_volume(&app.idleEngineSound, soundVolume / 100.0f + 1.0f);
    // initialize accelearion engine sound
    result = ma_sound_init_from_file(&app.engine, "audios/acceleration.wav", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.accelerationEngineSound);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize acceleration engine sound!");
    }
    ma_sound_set_looping(&app.accelerationEngineSound, MA_TRUE);
    ma_sound_set_volume(&app.accelerationEngineSound, soundVolume / 100.0f + 0.5f);
    // initialize pickup sound
    result = ma_sound_init_from_file(&app.engine, "audios/pickup.wav", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.pickupSound);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize pickup sound!");
    }
    ma_sound_set_volume(&app.pickupSound, soundVolume / 100.0f + 1.0f);
    // initialize money sound
    result = ma_sound_init_from_file(&app.engine, "audios/money.wav", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.moneySound);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize money sound!");
    }
    ma_sound_set_volume(&app.moneySound, soundVolume / 100.0f + 2.0f);
    // intialize clacson sound
    result = ma_sound_init_from_file(&app.engine, "audios/clacson.wav", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.clacsonSound);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize clacson sound!");
    }
    ma_sound_set_volume(&app.clacsonSound, soundVolume / 100.0f - 0.5f);
    std::cout << "[ SOUND ]: Sound resources initialized!" << std::endl;

    srand(time(NULL));

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "[ EXCEPTION ]:" << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}