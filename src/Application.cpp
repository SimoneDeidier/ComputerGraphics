#include "headers/Starter.hpp"
#include <iostream>

#define MINIAUDIO_IMPLEMENTATION
#include "headers/miniaudio.h"  // Miniaudio library (used to play sounds)

#define MESH 210    // Number of models in the city json
#define CARS 9  // Number of autonomus cars
#define STREET_LIGHT_COUNT 36   // Number of streetlights in the city
#define MAX_STREET_LIGHTS 5 // Number of people in the city
#define PEOPLE 45   // Number of people in the city
#define TAXI_ELEMENTS 8 // Number of elements in the taxi model
#define TAXI_LIGHT_COUNT 4  // Number of lights in the taxi
#define PICKUP_COUNT 5  // Number of people that the player can pickup
#define INGAME_SCENE_COUNT 2    // Number of active scenes in the game
#define GAMEMODE_COUNT 2    // Number of game modes
#define GRAPHICS_SETTINGS_COUNT 3   // Number of graphics settings
#define TAXI_COLL_PCOUNT 4  // Number of collision points of the taxi
#define COLLISION_BOXES_COUNT 9 // Number of internal collision boxes of the city
#define TAXI_ELEMENTS_W_OFFSETS_C 6 // Number of elements in the taxi model with offsets

#define MIN_DISTANCE_TO_PICKUP 4.75f    // Minimum distance to pickup a person
#define COLLISION_SPHERE_RADIUS 0.75f   // Radius of the collision sphere between taxi and cars
#define PICKUP_POINT_Y_OFFSET 2.0f  // Y offset for the pickup point light
#define ARROW_Y_OFFSET 3.25f    // Y offset for the pickup point arrow

// One type of UBO used by the majority of the shaders
struct UniformBufferObject {
    alignas(16) glm::mat4 mvpMat;   // Model-view-projection matrix
    alignas(16) glm::mat4 mMat; // Model matrix
    alignas(16) glm::mat4 nMat; // Normal matrix
};

/* Global GUBO used by the majority of the shaders.
 * It contains all the parameters that are equally used by the majority of the shaders.
 */
struct GlobalUniformBufferObject {
    alignas(16) glm::vec4 directLightPos; // Position of the sun
    alignas(16) glm::vec4 directLightCol;   // Color of the sun
    alignas(16) glm::vec4 taxiLightPos[TAXI_LIGHT_COUNT];   // Position of the taxi lights
    alignas(16) glm::vec4 frontLightCol;    // Color of the front light
    alignas(16) glm::vec4 rearLightCol; // Color of the rear light
    alignas(16) glm::vec4 frontLightDir;    // Direction of the front light (SPOTLIGHTS)
    alignas(16) glm::vec4 frontLightCosines;    // Cosines of the front light (SPOTLIGHTS)
    alignas(16) glm::vec4 streetLightCol;   // Color of the streetlights
    alignas(16) glm::vec4 streetLightDirection; // Direction of the streetlights (SPOTLIGHTS)
    alignas(16) glm::vec4 streetLightCosines;   // Cosines of the streetlights (SPOTLIGHTS)
    alignas(16) glm::vec4 pickupPointPos;   // Position of the pickup point
    alignas(16) glm::vec4 pickupPointCol;   // Color of the pickup point
    alignas(16) glm::vec4 eyePos;   // Position of the camera
    alignas(16) glm::vec4 settingsAndNight;  // Vector containing some values:
    /* settingsAndNight.x ==> graphics settings value (low - medium - high)
     * settingsAndNight.y ==> boolean flag for the night (1 if it's night, 0 otherwise)
     */
};

/* "Local" GUBO used by the majority of the shaders.
 * There will be one different LUBO for each Descriptor Set.
 * It contains some "local" parameters that are dependent from the model.
 */
struct LocalGUBO {
    alignas(16) glm::vec4 streetLightPos[MAX_STREET_LIGHTS]; // Position of the 5 streetlights to count during the BRDF calculation
    alignas(16) glm::vec4 gammaAndMetallic; // Vector containing some values:
    /* gammaAndMetallic.x ==> gamma value (BRDF)
     * gammaAndMetallic.y ==> metallic value (BRDF)
     */
};

// GUBO used for the the skybox shader
struct SkyGUBO {
    alignas(16) glm::vec4 directLightPos; // Position of the sun
    alignas(16) glm::vec4 directLightCol;   // Color of the sun
    alignas(16) glm::vec4 eyePos;   // Position of the camera
    alignas(16) glm::vec4 gammaAndMetallic; // Vector containing gamma and metallic values
};

// GUBO used for the arrow shader, it only counts for the pickup light during the BRDF calculation
struct ArrowGUBO {
    alignas(16) glm::vec4 pickupPointPos;   // Position of the pickup point (POINT LIGHT)
    alignas(16) glm::vec4 pickupPointCol;   // Color of the pickup point
    alignas(16) glm::vec4 eyePos;   // Position of the camera
    alignas(16) glm::vec4 gammaAndMetallic; // Vector containing gamma and metallic values
};

// Vertex definition for 3D objects
struct Vertex {
    glm::vec3 pos;  // Position
    glm::vec2 UV;   // UV coordinates
    glm::vec3 normal;   // Normal
};

// Vertex definition for 2D objects
struct TwoDimVertex {
    glm::vec3 pos;  // Position
    glm::vec2 UV;   // UV coordinates
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

        // Game options setted by main menu:
        int graphicsSettings = 2;   // 0 = low, 1 = medium, 2 = high
        bool endlessGameMode = false;   // True if the endless game mode is selected
        ma_engine engine;   // Miniaudio engine (used to play sounds)
        ma_sound titleMusic;    // Sound used in the title screen
        ma_sound ingameMusic;   // Sound used in the game
        ma_sound idleEngineSound;   // Sound used when the taxi is idle
        ma_sound accelerationEngineSound;   // Sound used when the taxi is accelerating
        ma_sound pickupSound;   // Sound used when the taxi picks up a person
        ma_sound moneySound;    // Sound used when the taxi earns money
        ma_sound clacsonSound;  // Sound used when the taxi collides with a car
    
    protected:
        
        float Ar;

        // Vertex Descrpitors: just two, one for 3D objects and one for 2D objects
        VertexDescriptor VDthreeDim, VDtwoDim;

        // Pipelines: one for each kind of object
        Pipeline Ptaxi, Pcity, PskyBox, Pcars, Ppeople, PtwoDim, Parrow;

        // Descriptor Set Layouts: a global DSL and one for each kind of object
        DescriptorSetLayout DSLglobal, DSLpeople, DSLtaxi, DSLcars, DSLcity, DSLskyBox, DSLtwoDim, DSLarrow;

        // Descriptor Sets: a global DS and one for each type of object (MODEL)
        DescriptorSet DSglobal, DSpeople[PEOPLE], DStaxi[TAXI_ELEMENTS], DScars[CARS], DScity[MESH], DSskyBox, DStwoDim, DSarrow;

        // Models: one for each type of object
        Model Mtaxi[TAXI_ELEMENTS], MskyBox, Mcars[CARS], Mpeople[PEOPLE], Mcity[MESH], MtwoDim, Marrow;

        // Textures:
        Texture Tcity, TskyBox, Tpeople, Ttaxi, Ttitle, Tcontrols, Tendgame;

        // Uniform Buffers: one for each type of object
        UniformBufferObject uboTaxi[TAXI_ELEMENTS], uboSkyBox, uboCars[CARS], uboCity[MESH], uboPeople[PEOPLE], uboArrow;

        // Global Uniform Buffer Object (one for all the shaders)
        GlobalUniformBufferObject globalGUBO;

        // Local Uniform Buffers: one for each type of object
        LocalGUBO guboTaxi[TAXI_ELEMENTS], guboCars[CARS], guboCity[MESH], guboPeople[PEOPLE];

        // Local GUBO for the skybox shader
        SkyGUBO guboSkyBox;

        // Unique GUBO for the arrow shader
        ArrowGUBO guboArrow;

        int currScene = -2; // Variables used for the scene management
        int lastSavedSceneValue;    // Variable used to save the last scene value
        int currentPoints[CARS] = {0,0,0,0,0,0,0,0,0};  // Variable for the NPC cars
        int random_index = -1;  // Index used to choose randomically the person to pickup
        int collisionCounter = 0;   // Variable used to count on how many NPC cars we are colliding
        int totDrivesCompleted = 0; // Variable used to count the number of drives completed
        int twoDimTexture = 0;  // Index variable used to choose the 2D texture to draw
        float wheelRoll = 0.0f; 
        float CamAlpha = 0.0f;  
        float CamBeta = 0.0f;
        float money = 0.0f; // Variable used to store the money earned
        float wheelAndSteerAng = 0.0f;
        float openingDoorAngle = 0.0f;
        double pickupTime = 0.0;    // Variable used to time the pickup and dropoff

        // Boolean flag used in the code
		bool openDoor = false;  // True when the animation of door opening has to start
		bool closeDoor = false; // True when the animation of door closing has to start
        bool alreadyInPhotoMode = false;    // True when the player is in the photo mode
        bool isNight = false;   // True when is nihgt in the game
        bool drawTwoDimPlane = true;    // True when we have to draw the 2D plane
        bool pickupPointSelected = false;   // True when the pickup point has been selected
        bool pickedPassenger = false;   // True when the passenger has been picked up
        bool inCollisionZone = false;   // True when the taxi is in the collision zone

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

        // Positions of the streetlights (used by the shaders)
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
        glm::vec3 taxiCollPOffsets[TAXI_COLL_PCOUNT] = {glm::vec3(-0.85f, 0.0f, -0.65f),
                                                                glm::vec3(0.85f, 0.0f, -0.65f),
                                                                glm::vec3(-0.85f, 0.0f, 2.7f),
                                                                glm::vec3(0.85f, 0.0f, 2.7f)};
        
        // Collision points of the taxi
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

        // Some constant values used in the the majority of the shaders:
        glm::vec4 rearLightColor = glm::vec4(238.0f / 255.0f, 0.0f, 0.0f, 1.0f);    // Color of the rear light
        glm::vec4 frontLightColor = glm::vec4(238.0f / 255.0f, 221.0f / 255.0f, 130.0f / 255.0f, 1.0f);   // Color of the front light
        // Direction of the spot lights ==> angle of 2 degrees
        // - On Y ==> -|cos(2°)|
        // - On Z ==> -|sin(2°)|
        glm::vec4 frontLightDirection = glm::vec4(0.0f, -1.0f * glm::abs(glm::sin(glm::radians(2.0f))), -1.0f * glm::abs(glm::cos(glm::radians(2.0f))), 0.0f);  // Direction of the front light (SPOTLIGHTS)
        // Cosines for the inner and outer angles of the spot light (inner constant, inner-outer decreasing)
        // Inner angle: 10°, outer angle: 15°
        glm::vec4 frontLightCosines = glm::vec4(glm::abs(glm::cos(10.0f)), glm::abs(glm::cos(15.0f)), 0.0f, 0.0f);  // Cosines of the front light (SPOTLIGHTS)
        glm::vec4 sunCol = glm::vec4(253.0f / 255.0f, 251.0f / 255.0f, 211.0f / 255.0f, 1.0f);  // Color of the sun
        glm::vec4 streetLightCol = glm::vec4(255.0f / 255.0f, 230.0f / 255.0f, 146.0f / 255.0f, 1.0f);  // Color of the streetlights
        // Direction of the spot lights ==> towards the terrain (-1 on Y)
        glm::vec4 streetLightDirection = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);    // Direction of the streetlights (SPOTLIGHTS)
        // Cosines for the inner and outer angles of the spot light (inner constant, inner-outer decreasing)
        // Inner angle: 15°, outer angle: 22.5°
        glm::vec4 streetLightCosines = glm::vec4(glm::abs(glm::cos(15.0f)), glm::abs(glm::cos(22.5f)), 0.0f, 0.0f);   // Cosines of the streetlights (SPOTLIGHTS)
        glm::vec4 pickupPointColor = glm::vec4(247.0f / 255.0f, 76.0f / 255.0f, 63.0f / 255.0f, 1.0f);  // Color of the pickup point
        glm::vec4 pickupPoint = glm::vec4(0.0f);    // Position of the pickup point
        glm::vec4 dropoffPoint = glm::vec4(0.0f);   // Position of the dropoff point

        // World matrices of the NPCs cars
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

            // Descriptor pool sizes:
            // 2 uniforms (UBO and GUBO) for: taxi, city, NPCs, people, skybox and arrow, plus one Global GUBO
            uniformBlocksInPool =  (2 * TAXI_ELEMENTS) + (2 * MESH) + (2 * CARS) + (2 * PEOPLE) + 2 + 2 + 1;
            // One texture for each model (taxi, city, NPCs and people)
            // Plus one for the skybox and three for the 2D plane
            texturesInPool = TAXI_ELEMENTS + MESH + CARS + PEOPLE + 1 + 3;
            // One set for each model (taxi, city, NPCs and people)
            // Plus one for the skybox, one for the 2D plane, one for the arrow and one for the Global GUBO
            setsInPool = TAXI_ELEMENTS + MESH + CARS + PEOPLE + 1 + 1 + 1 + 1;

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
                    {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS} // Local GUBO
            });
            DSLcity.init(this, {
                    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
                    {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
                    {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}
            });
            DSLskyBox.init(this, {
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
            DSLglobal.init(this, {
                    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS} // Global GUBO
            });
            DSLtwoDim.init(this, {
                {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT} // Only texture
            });
            DSLarrow.init(this, {
                {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS}, // UBO
                {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS} // GUBO
            });

            // Initialization of Vertex Descriptors
            VDthreeDim.init(this, {
                    {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
                                    sizeof(glm::vec3), POSITION},
                            {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
                                    sizeof(glm::vec2), UV},
                            {0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal),
                                    sizeof(glm::vec3), NORMAL}
                    });
            VDtwoDim.init(this, {
                {0, sizeof(TwoDimVertex), VK_VERTEX_INPUT_RATE_VERTEX}
            }, {
                {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TwoDimVertex, pos),
                        sizeof(glm::vec3), POSITION},
                {0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(TwoDimVertex, UV),
                        sizeof(glm::vec2), UV}
            });

            // Initialization of Pipelines:
            // Each pipeline, except for the SkyBox, 2D and Arrow ones, has two DSL: one for the global values and one for the local ones
            Ptaxi.init(this, &VDthreeDim, "shaders/BaseVert.spv", "shaders/BaseFrag.spv", {&DSLtaxi, &DSLglobal});
            Pcity.init(this, &VDthreeDim, "shaders/BaseVert.spv", "shaders/BaseFrag.spv", {&DSLcity, &DSLglobal});
            Ppeople.init(this, &VDthreeDim, "shaders/BaseVert.spv", "shaders/BaseFrag.spv", {&DSLpeople, &DSLglobal});
            Pcars.init(this, &VDthreeDim, "shaders/BaseVert.spv", "shaders/BaseFrag.spv", {&DSLcars, &DSLglobal});
            PskyBox.init(this, &VDthreeDim, "shaders/BaseVert.spv", "shaders/SkyFrag.spv", {&DSLskyBox});
            // Deactivate culling for the sky pipeline (render the skybox from the inside)
            PskyBox.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
            PtwoDim.init(this, &VDtwoDim, "shaders/TwoDimVert.spv", "shaders/TwoDimFrag.spv", {&DSLtwoDim});
            // Settings for 2D rendering pipeline
            PtwoDim.setAdvancedFeatures(VK_COMPARE_OP_LESS_OR_EQUAL, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
            Parrow.init(this, &VDthreeDim, "shaders/BaseVert.spv", "shaders/ArrowFrag.spv", {&DSLarrow});

            std::cout << "[ LOADING ]: -------------------------------------------------" << std::endl;
            std::cout << "[ LOADING ]: Loading models:\t\t[                    ]" << std::endl;

            // Initialization of Models
            Mtaxi[0].init(this, &VDthreeDim, "models/Car_Hatch_C_Door.obj", OBJ);
            Mtaxi[1].init(this, &VDthreeDim, "models/Car_Hatch_C_Extern.obj", OBJ);
            Mtaxi[2].init(this, &VDthreeDim, "models/Car_Hatch_C_Intern_no-steer.obj", OBJ);
            Mtaxi[3].init(this, &VDthreeDim, "models/TruckBodySteeringWheelO.mgcg", MGCG);
            Mtaxi[4].init(this, &VDthreeDim, "models/Car_Hatch_C_Wheel.obj", OBJ);
            Mtaxi[5].init(this, &VDthreeDim, "models/Car_Hatch_C_Wheel.obj", OBJ);
            Mtaxi[6].init(this, &VDthreeDim, "models/Car_Hatch_C_Wheel.obj", OBJ);
			Mtaxi[7].init(this, &VDthreeDim, "models/Car_Hatch_C_Wheel.obj", OBJ);
            MskyBox.init(this, &VDthreeDim, "models/Sphere2.obj", OBJ);
            Mcars[0].init(this, &VDthreeDim, "models/transport_cool_001_transport_cool_001.001.mgcg", MGCG);
            Mcars[1].init(this, &VDthreeDim, "models/transport_cool_003_transport_cool_003.001.mgcg", MGCG);
            Mcars[2].init(this, &VDthreeDim, "models/transport_cool_004_transport_cool_004.001.mgcg", MGCG);
            Mcars[3].init(this, &VDthreeDim, "models/transport_cool_010_transport_cool_010.001.mgcg", MGCG);
            Mcars[4].init(this, &VDthreeDim, "models/transport_jeep_001_transport_jeep_001.001.mgcg", MGCG);
            Mcars[5].init(this, &VDthreeDim, "models/transport_jeep_010_transport_jeep_010.001.mgcg", MGCG);
            Mcars[6].init(this, &VDthreeDim, "models/transport_cool_001_transport_cool_001.001.mgcg", MGCG);
            Mcars[7].init(this, &VDthreeDim, "models/transport_cool_004_transport_cool_004.001.mgcg", MGCG);
            Mcars[8].init(this, &VDthreeDim, "models/transport_cool_010_transport_cool_010.001.mgcg", MGCG);

            // Initialization of the 2D plane
            // Vector of TwoDimVertex, each element has the position and the UV coordinates
            std::vector<TwoDimVertex> vertices = {{{-1.0, -1.0, 0.9f}, {0.0f, 0.0f}},
                    {{-1.0, 1.0, 0.9f}, {0.0f, 1.0f}},
                    {{ 1.0,-1.0, 0.9f}, {1.0f, 0.0f}},
                    {{ 1.0, 1.0, 0.9f}, {1.0f, 1.0f}}};
            // Set the vertices in the model
            MtwoDim.vertices = std::vector<unsigned char>((unsigned char*)vertices.data(), (unsigned char*)vertices.data() + sizeof(TwoDimVertex) * vertices.size());
            // Set the indices of the model (two triangles)
            MtwoDim.indices = {0, 1, 2, 1, 3, 2};
            MtwoDim.initMesh(this, &VDtwoDim);

            std::cout << "[ LOADING ]: Loading models:\t\t[=====               ]" << std::endl;

            // Initialization of the models of the city (from json)
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

                    Mcity[k].init(this, &VDthreeDim, modelPath, (format[0] == 'O') ? OBJ : ((format[0] == 'G') ? GLTF : MGCG));

                }
            }catch (const nlohmann::json::exception& e) {
                std::cout << "[ EXCEPTION ]: " << e.what() << std::endl;
                exit(1);
            }

            std::cout << "[ LOADING ]: Loading models:\t\t[==========          ]" << std::endl;

            // Initialization of people's models (from json)
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

                    Mpeople[k].init(this, &VDthreeDim, modelPath, (format[0] == 'O') ? OBJ : ((format[0] == 'G') ? GLTF : MGCG));

                }
            }catch (const nlohmann::json::exception& e) {
                std::cout << "[ EXCEPTION ]: " << e.what() << std::endl;
                exit(1);
            }

            Marrow.init(this, &VDthreeDim, "models/simple arrow.obj", OBJ);

            // Initialization of Textures
            Tcity.init(this,"textures/city.png");
            TskyBox.init(this, "textures/skybox.png");
            Tpeople.init(this, "textures/person.jpg");
            Ttaxi.init(this, "textures/taxi.png");
            Ttitle.init(this, "textures/title.jpg");
            Tcontrols.init(this, "textures/controls.jpg");
            Tendgame.init(this, "textures/endgame.jpg");

            std::cout << "[ LOADING ]: Loading models:\t\t[====================]" << std::endl;
            std::cout << "[ LOADING ]: Loading completed!" << std::endl;

        }

        // Initialization of Pipelines and Descriptor Sets
        void pipelinesAndDescriptorSetsInit() {

            // Creation of the Pipelines
            Ptaxi.create();
            Pcity.create();
            Ppeople.create();
            Pcars.create();
            PskyBox.create();
            PtwoDim.create();
            Parrow.create();

            // Initialization of the Descriptor Sets
            DSglobal.init(this, &DSLglobal, {
                    {0, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr} // Global GUBO
            });

            for(int i = 0; i < TAXI_ELEMENTS; i++){
                DStaxi[i].init(this, &DSLtaxi, {
                        {0, UNIFORM, sizeof(UniformBufferObject), nullptr}, // Uniform Buffer Object
                        {1, TEXTURE, 0, &Ttaxi},    // Texture
                        {2, UNIFORM, sizeof(LocalGUBO), nullptr}    // Local GUBO
                });
            }

            for(int i = 0; i < MESH; i++) {
                DScity[i].init(this, &DSLcity, {
                        {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                        {1, TEXTURE, 0, &Tcity},
                        {2, UNIFORM, sizeof(LocalGUBO), nullptr}
                });
            }

            DSskyBox.init(this, &DSLskyBox, {
                    {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                    {1, TEXTURE, 0, &TskyBox},
                    {2, UNIFORM, sizeof(SkyGUBO), nullptr}
            });

            for(int i = 0; i < CARS; i++) {
                DScars[i].init(this, &DSLcars, {
                        {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                        {1, TEXTURE, 0, &Tcity},
                        {2, UNIFORM, sizeof(LocalGUBO), nullptr}
                });
            }

            for(int i = 0; i < PEOPLE; i++) {
                DSpeople[i].init(this, &DSLpeople, {
                        {0, UNIFORM, sizeof(UniformBufferObject), nullptr},
                        {1, TEXTURE, 0, &Tpeople},
                        {2, UNIFORM, sizeof(LocalGUBO), nullptr}
                });
            }

            DStwoDim.init(this, &DSLtwoDim, {
                // When initializing the Descriptor Set for the 2D plane, we need to pass the texture
                // Check the value of twoDimTexture to know which texture to pass (0 = title, 1 = controls, 2 = endgame)
                {0, TEXTURE, 0, (twoDimTexture == 0 ? &Ttitle : (twoDimTexture == 1 ? &Tcontrols : &Tendgame))}
            });

            DSarrow.init(this, &DSLarrow, {
                {0, UNIFORM, sizeof(UniformBufferObject), nullptr}, // UBO
                {1, UNIFORM, sizeof(ArrowGUBO), nullptr}    // Texture
            });
        }

        // Cleanup of Pipelines and Descriptor Sets
        void pipelinesAndDescriptorSetsCleanup() {

            // Cleanup of the Pipelines
            Ptaxi.cleanup();
            Pcity.cleanup();
            Ppeople.cleanup();
            Pcars.cleanup();
            PskyBox.cleanup();
            PtwoDim.cleanup();
            Parrow.cleanup();

            // Cleanup of the Descriptor Sets
            DSglobal.cleanup();

            for(int i = 0; i < TAXI_ELEMENTS; i++) {
                DStaxi[i].cleanup();
            }

            for(int i = 0; i < MESH; i++) {
                DScity[i].cleanup();
            }

            DSskyBox.cleanup();

            for(int i = 0; i < CARS; i++) {
                DScars[i].cleanup();
            }

            for(int i = 0; i < PEOPLE; i++) {
                DSpeople[i].cleanup();
            }

            DStwoDim.cleanup();
            DSarrow.cleanup();

        }

        // Cleanup of Textures, Models, Descruot Set Layouts and Pipelines
        void localCleanup() {

            // Cleanup of Textures
            Tcity.cleanup();
            TskyBox.cleanup();
            Tpeople.cleanup();
            Ttaxi.cleanup();
            Ttitle.cleanup();
            Tcontrols.cleanup();
            Tendgame.cleanup();

            // Cleanup of Models
            for(int i = 0; i < TAXI_ELEMENTS; i++) {
                Mtaxi[i].cleanup();
            }

            for(int i = 0; i < MESH; i++) {
                Mcity[i].cleanup();
            }

            MskyBox.cleanup();

            for(int i = 0; i < CARS; i++) {
                Mcars[i].cleanup();
            }
            for(int i = 0; i < PEOPLE; i++) {
                Mpeople[i].cleanup();
            }
            MtwoDim.cleanup();
            Marrow.cleanup();

            // Cleanup of Descriptor Set Layouts
            DSLglobal.cleanup();
            DSLtaxi.cleanup();
            DSLcity.cleanup();
            DSLskyBox.cleanup();
            DSLcars.cleanup();
            DSLpeople.cleanup();
            DSLtwoDim.cleanup();
            DSLarrow.cleanup();

            // Cleanup of Pipelines and Descriptor Sets
            Ptaxi.destroy();
            Pcity.destroy();
            Ppeople.destroy();
            Pcars.destroy();
            PskyBox.destroy();
            PtwoDim.destroy();
            Parrow.destroy();

        }

        // Binding of the Pipelines, Descriptor Sets and Models to the command buffer
        void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

            // If we are not drawing a 2D scene:
            if(!drawTwoDimPlane) {

                Ptaxi.bind(commandBuffer);  // Bind the taxi Pipeline

                // Bind the Global Descriptor Set in the set = 1 of the taxi Pipeline (just the GUBO)
                DSglobal.bind(commandBuffer, Ptaxi, 1, currentImage);
                for(int i = 0; i < TAXI_ELEMENTS; i++){
                    // Bind the "Local" Descriptor Sets in the set = 0 (UBO, texture and Local GUBO)
                    DStaxi[i].bind(commandBuffer, Ptaxi, 0, currentImage);
                    Mtaxi[i].bind(commandBuffer);
                    vkCmdDrawIndexed(commandBuffer,
                                    static_cast<uint32_t>(Mtaxi[i].indices.size()), 1, 0, 0, 0);
                }

                Pcity.bind(commandBuffer);  // Bind the city Pipeline

                // Bind the Global Descriptor Set in the set = 1 of the city Pipeline (just the GUBO)
                DSglobal.bind(commandBuffer, Pcity, 1, currentImage);
                for(int i = 0; i < MESH; i++) {
                    // Bind the "Local" Descriptor Sets in the set = 0 (UBO, texture and Local GUBO)
                    DScity[i].bind(commandBuffer, Pcity, 0, currentImage);
                    Mcity[i].bind(commandBuffer);
                    vkCmdDrawIndexed(commandBuffer,
                                    static_cast<uint32_t>(Mcity[i].indices.size()), 1, 0, 0, 0);
                }

                PskyBox.bind(commandBuffer);    // Bind the skybox Pipeline

                // Bind the SkyBox Descriptor Set in the set = 0 of the skybox Pipeline (UBO, texture and Local GUBO)
                DSskyBox.bind(commandBuffer, PskyBox, 0, currentImage);
                MskyBox.bind(commandBuffer);
                vkCmdDrawIndexed(commandBuffer,
                                static_cast<uint32_t>(MskyBox.indices.size()), 1, 0, 0, 0);

                Pcars.bind(commandBuffer);  // Bind the cars Pipeline

                // Bind the Global Descriptor Set in the set = 1 of the cars Pipeline (just the GUBO)
                DSglobal.bind(commandBuffer, Pcars, 1, currentImage);
                for(int i = 0; i < CARS; i++) {
                    // Bind the "Local" Descriptor Sets in the set = 0 (UBO, texture and Local GUBO)
                    DScars[i].bind(commandBuffer, Pcars, 0, currentImage);
                    Mcars[i].bind(commandBuffer);
                    vkCmdDrawIndexed(commandBuffer,
                                    static_cast<uint32_t>(Mcars[i].indices.size()), 1, 0, 0, 0);
                }

                Ppeople.bind(commandBuffer);    // Bind the people Pipeline

                // Bind the Global Descriptor Set in the set = 1 of the people Pipeline (just the GUBO)
                DSglobal.bind(commandBuffer, Ppeople, 1, currentImage);
                for(int i = 0; i < PEOPLE; i++) {
                    // Check for the person to not be drawn (picked up)
                    if((i == 3 || i == 7 || i == 35 || i == 37 || i == 44) && !drawPeople[i]) {
                        // do nothing ==> do not draw it
                    }
                    else {
                        // Bind the "Local" Descriptor Sets in the set = 0 (UBO, texture and Local GUBO)
                        DSpeople[i].bind(commandBuffer, Ppeople, 0, currentImage);
                        Mpeople[i].bind(commandBuffer);
                        vkCmdDrawIndexed(commandBuffer,
                                        static_cast<uint32_t>(Mpeople[i].indices.size()), 1, 0, 0, 0);
                    }
                }

                Parrow.bind(commandBuffer);   // Bind the arrow Pipeline
                DSarrow.bind(commandBuffer, Parrow, 0, currentImage);   // For the arrow just bind his DS
                Marrow.bind(commandBuffer);
                vkCmdDrawIndexed(commandBuffer,
                                static_cast<uint32_t>(Marrow.indices.size()), 1, 0, 0, 0);

            }
            // Else bind the Pipeline, Descriptor Set and Model for the 2D scene
            else {
                PtwoDim.bind(commandBuffer);    // Bind the 2D Pipeline
                DStwoDim.bind(commandBuffer, PtwoDim, 0, currentImage);   // Bind the 2D DS
                MtwoDim.bind(commandBuffer);
                vkCmdDrawIndexed(commandBuffer,
                                static_cast<uint32_t>(MtwoDim.indices.size()), 1, 0, 0, 0);
            }

        }

        // Main application loop
        void updateUniformBuffer(uint32_t currentImage) {

            static bool debounce = false;
            static int curDebounce = 0;

            static bool autoTime = true;    // Boolean set to true to auto update the time
            static float cTime = 0.0f;  // Current time
            const float turnTime = 72.0f;   // Time to turn 360° (cyclic timer)
            const float angTurnTimeFact = 2.0f * M_PI / turnTime;   // Factor to convert the time in angle
            // Initial values for the camera (first person view)
            static float CamPitch = glm::radians(0.0f);
            static float CamYaw = M_PI;
            static float CamRoll = 0.0f;
            // Initial values for the camera (third person view)
            static float camOffsetAngle = 0.0f;

            // Vector with all the directions of the cars
            glm::vec3 directions[CARS];

            // Computation of the direction for each car (point to reach - pos of the car)
            for(int i = 0; i < CARS; i++) {
                directions[i] = glm::normalize(wayPoints[i][currentPoints[i]] - carPositions[i]);
            }

            // Speed for NPC cars
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
                    // When we are in first person view, we want to go back to the third person view and so on...
                    currScene = (currScene + 1) % INGAME_SCENE_COUNT;
                    // When changed to third person view, reset the camera to initial position
                    if(currScene == 0) {
                        camOffsetAngle = 0.0f;  // Reset the camera offset angle
                    }
                    // When changed to first person view, reset the camera to initial position
                    if(currScene == 1) {
                        CamPitch = glm::radians(0.0f);  // Set the pitch to 0
                        CamYaw = M_PI;  // Set the yaw to PI
                        CamRoll = 0.0f; // Set the roll to 0
                    }
                    // Set it to true if we are not in the first, third person or photo view
                    drawTwoDimPlane = (currScene < 0) || (currScene == 3); 
                    // Set the value of the texture to show in the 2D plane
                    twoDimTexture = (currScene == -2 ? 0 : (currScene == -1 ? 1 : 2));
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
					// If I'm already in photo mode
					if (currScene == 2) {
                        // Exit the photo mode and return to the last saved scene
                        currScene = lastSavedSceneValue;
					}
                    else {
                        // Save the scene I am leaving
						lastSavedSceneValue = currScene;
                        // Enter in photo mode
						currScene = 2;
                        // Check if the sounds of the taxi are playing and stop them
                        if(ma_sound_is_playing(&idleEngineSound)) {
                            ma_sound_stop(&idleEngineSound);
                        }
                        if(ma_sound_is_playing(&accelerationEngineSound)) {
                            ma_sound_stop(&accelerationEngineSound);
                        }
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

            // Integration with the timers and the controllers
            float deltaT;
            glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
            bool fire = false;
            // Get user inputs
            getSixAxis(deltaT, m, r, fire);

            if (autoTime) {
                cTime += deltaT;    // Update the time
                // If the time is greater than the turn time, subtract the turn time (make it cyclic)
                cTime = (cTime > turnTime) ? (cTime - turnTime) : cTime;    
            }

            static float steeringAngCars[CARS];
            // Update the position of the NPC cars only if not in photo mode
            if(currScene != 2) {
                for(int i = 0; i < CARS; i++) {
                    // Update the position of the NPC cars
                    carPositions[i] += directions[i] * speedCar * deltaT;
                    // If the distance between the car's current position and the current waypoint is less than 0.25
                    // Update the current waypoint index to point to the next waypoint
                    if (glm::distance(carPositions[i], wayPoints[i][currentPoints[i]]) < 0.25f) {
                        currentPoints[i] = (currentPoints[i] + 1) % wayPoints[i].size();
                    }

                    // Compute the steering angle towards the next waypoint
                    float targetSteering = atan2(directions[i].x, directions[i].z);
                    // Normalize the steering angle between -π and π
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
                    // Max speed of the taxi
                    const float moveSpeed = 7.5f;

                    static float currentSpeed = 0.0f;
                    float targetSpeed = moveSpeed * -m.z;
                    // Adjust this value to control the damping effect
                    const float dampingFactor = 3.0f;
                    float speedDifference = targetSpeed - currentSpeed;
                    // If the difference between the targetSpeed and the current speed is small ==> current speed become equal to the targetSpeed
                    if (fabs(speedDifference) < 0.01f) {
                        currentSpeed = targetSpeed;
                    } else {
                        // Otherwise speed gradually change
                        currentSpeed += speedDifference * dampingFactor * deltaT;
                    }
                    // If I am not opening/closing the door I update the speed
                    if(!openDoor && !closeDoor) {
                        speed = currentSpeed * deltaT;
                    }
                    wheelRoll -= currentSpeed;
                    // If the speed is very small it is forced to 0
                    speed = (abs(speed) < 0.01f) ? 0.0f : speed;
                    // Store the current steering angle before updating it
                    oldSteeringAng = steeringAng;
                    // Adjust the steering angle based on player input
                    steeringAng += (speed >= 0 ? -m.x : m.x) * steeringSpeed * deltaT;
                    // If the steering angle has not changed, gradually reset the wheel alignment
                    if (steeringAng == oldSteeringAng) {
                        wheelAndSteerAng = (wheelAndSteerAng < 0.0f ? wheelAndSteerAng + 0.25f : (wheelAndSteerAng > 0.0f ? wheelAndSteerAng - 0.25f : wheelAndSteerAng));
                    }
                    else if (steeringAng > oldSteeringAng) {
                        // If the new steering angle is greater than the old one, limit the wheel rotation
                        wheelAndSteerAng = (wheelAndSteerAng > -1.5f ? wheelAndSteerAng - 0.25f : wheelAndSteerAng);
                    }
                    else {
                        // If the new steering angle is smaller than the old one, limit the wheel rotation in the other direction
                        wheelAndSteerAng = (wheelAndSteerAng < 1.5f ? wheelAndSteerAng + 0.25f : wheelAndSteerAng);
                    }
                    if (speed == 0.0f) {
						steeringAng = oldSteeringAng;
                    }
                    
                    // Set the four collision points of the taxi (four corners of the car)
                    for(int i = 0; i < TAXI_COLL_PCOUNT; i++) {
                        taxiCollisionPoints[i] = glm::translate(glm::rotate(glm::translate(glm::mat4(1.0f), taxiPos), steeringAng, glm::vec3(0.0f, 1.0f, 0.0f)), taxiCollPOffsets[i])[3];
                    }

                    // Boolean variable: set it to true if the taxi is in collision with the external collision box
                    bool collisionCheck = checkCollision(taxiCollisionPoints, TAXI_COLL_PCOUNT, externalCollisionBox, true);
                    for(int i = 0; i < COLLISION_BOXES_COUNT; i++) {
                        // For each internal collision box, check if the taxi is in collision with it and do the logical AND
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
                        // Define the camera rotation speed
                        const float ROT_SPEED = glm::radians(120.0f);
                        // Rotation y axis of the camera based on user input
                        CamYaw -= ROT_SPEED * deltaT * r.y;
                        // Rotation x axis of the camera based on user input
                        CamPitch -= ROT_SPEED * deltaT * r.x;
                        // Rotation z axis of the camera based on user input
                        CamRoll -= ROT_SPEED * deltaT * r.z;
                        // Limit the yaw (Y axis rotation) between (PI / 2) and (3 * PI / 2)
                        CamYaw = (CamYaw < M_PI_2 ? M_PI_2 : (CamYaw > 1.5 * M_PI ? 1.5 * M_PI : CamYaw));
                        // Limit the pitch (X axis rotation) between (-PI / 4) and (PI / 4)
                        CamPitch = (CamPitch < -0.25 * M_PI ? -0.25 * M_PI : (CamPitch > 0.25 * M_PI ? 0.25 * M_PI : CamPitch));
                        // Limit the roll (Z axis rotation) between (-PI) and (PI)
                        CamRoll = (CamRoll < -M_PI ? -M_PI : (CamRoll > M_PI ? M_PI : CamRoll));

                        // Define an offset for the camera position relative to the taxi
                        glm::vec3 camOffset(0.35f, 1.05f, 0.7f);
                        // Compute camera offset based on the steering angle
                        glm::vec3 rotatedCamOffset = glm::vec3(
                            glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0)) * glm::vec4(camOffset, 1.0)
                        );
                        //Update the position of the camera
                        camPos = taxiPos + rotatedCamOffset;
                        // Build the final view matrix by applying rotations and translation:
                        mView=
                            glm::rotate(glm::mat4(1.0f), -CamRoll, glm::vec3(0, 0, 1)) *
                            glm::rotate(glm::mat4(1.0f), -CamPitch, glm::vec3(1, 0, 0)) *
                            glm::rotate(glm::mat4(1.0f), -CamYaw - steeringAng, glm::vec3(0, 1, 0)) *
                            glm::translate(glm::mat4(1.0f), -camPos);

                    }
                }
                // Else if we are in photo mode
                else if(currScene == 2) {

                    // Check if we are entering photo mode for the first time
                    if(!alreadyInPhotoMode) {
                        // Save the current camera position before entering photo mode
                        camPosInPhotoMode = camPos;
                        alreadyInPhotoMode = true;
                    }
                    // Define the camera rotation speed
                    const float ROT_SPEED2 = glm::radians(240.0f);
                    // Define the camera speed
                    const float MOVE_SPEED2 = 7.5f;

                    // Rotate the camera around the Y-axis based on user input
                    CamAlpha = CamAlpha - ROT_SPEED2 * deltaT * r.y;
                    // Rotate the camera around the X-axis based on user input
                    CamBeta = CamBeta - ROT_SPEED2 * deltaT * r.x;
                    // Limitation of the rotation around X-axis
                    CamBeta = CamBeta < glm::radians(-90.0f) ? glm::radians(-90.0f) :
                            (CamBeta > glm::radians(90.0f) ? glm::radians(90.0f) : CamBeta);


                    glm::vec3 ux = glm::rotate(glm::mat4(1.0f), CamAlpha, glm::vec3(0, 1, 0)) * glm::vec4(1, 0, 0, 1);
                    glm::vec3 uz = glm::rotate(glm::mat4(1.0f), CamAlpha, glm::vec3(0, 1, 0)) * glm::vec4(0, 0, -1, 1);
                    camPosInPhotoMode = camPosInPhotoMode + MOVE_SPEED2 * m.x * ux * deltaT;
                    camPosInPhotoMode = camPosInPhotoMode + MOVE_SPEED2 * m.y * glm::vec3(0, 1, 0) * deltaT;
                    camPosInPhotoMode = camPosInPhotoMode + MOVE_SPEED2 * -m.z * uz * deltaT;

                    // Build the final view matrix by applying rotations and translation:
                    mView = glm::rotate(glm::mat4(1.0), -CamBeta, glm::vec3(1, 0, 0)) *
                            glm::rotate(glm::mat4(1.0), -CamAlpha, glm::vec3(0, 1, 0)) *
                            glm::translate(glm::mat4(1.0), -camPosInPhotoMode);


                }

                const float nearPlane = 0.1f;   // Near plane
                const float farPlane = 375.0f;  // Far plane
                glm::mat4 Prj = glm::perspective(glm::radians(45.0f), Ar, nearPlane, farPlane);
                // Projection matrix
                Prj[1][1] *= -1;


                // World matrix for the city
                glm::mat4 mWorld  = glm::translate(glm::mat4(1), glm::vec3(0, 0, 3)) * glm::rotate(glm::mat4(1), glm::radians(180.0f), glm::vec3(0, 1, 0));

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
                
                // Taxi's world matrix (one for each model of the taxi)
                glm::mat4 mWorldTaxi[8];

                // Set the the matrixes of the intern and extern of the taxi model
                mWorldTaxi[1] = mWorldTaxi[2] =
                    glm::translate(glm::mat4(1.0), taxiPos) *
                    glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0));

                // Vector with the offsets of the other taxi's elements
				glm::vec3 offsets[6] = {
					glm::vec3(-0.65f, 0.23f, 2.05f), // Front right wheel
					glm::vec3(0.65f, 0.23f, 2.05f), // Front left wheel
					glm::vec3(-0.65f, 0.2f, -0.1f), // Rear right wheel
					glm::vec3(0.65f, 0.2f, -0.1f), // Rear left wheel
					glm::vec3(0.45f, 0.75f, 1.5f), // Steering wheel
					glm::vec3(-0.742f, 0.695f, 1.6f) // Door (rotating one)
				};

                // Compute the final position of the other taxi's elements
                glm::vec3 rotatedOffsets[TAXI_ELEMENTS_W_OFFSETS_C], finalWorldPos[TAXI_ELEMENTS_W_OFFSETS_C];
                for (int i = 0; i < TAXI_ELEMENTS_W_OFFSETS_C; i++) {
                    // Rotate the offsets based on the steering angle
					rotatedOffsets[i] = glm::vec3(glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0)) * glm::vec4(offsets[i], 1.0));
                    // Compute the final position of the elements
					finalWorldPos[i] = taxiPos + rotatedOffsets[i];
                }

                // Setting the world matrix for the other taxi's elements
				mWorldTaxi[4] =  // Front right wheel
                    glm::translate(glm::mat4(1.0), finalWorldPos[0]) *
                    glm::rotate(glm::mat4(1.0), steeringAng - glm::radians(wheelAndSteerAng*15), glm::vec3(0, 1, 0)) *
					glm::rotate(glm::mat4(1.0), wheelRoll, glm::vec3(1, 0, 0)) * //when I accelerate the wheel should spin
                    glm::rotate(glm::mat4(1.0), glm::radians(180.0f), glm::vec3(0, 0, 1)); //the wheel was facing left
				mWorldTaxi[5] =  // Front left wheel
                    glm::translate(glm::mat4(1.0), finalWorldPos[1]) *
                    glm::rotate(glm::mat4(1.0), steeringAng - glm::radians(wheelAndSteerAng * 15), glm::vec3(0, 1, 0)) *
                    glm::rotate(glm::mat4(1.0), wheelRoll, glm::vec3(1, 0, 0)); //when I accelerate the wheel should spin
				mWorldTaxi[6] = // Rear right wheel
                    glm::translate(glm::mat4(1.0), finalWorldPos[2]) *
                    glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0)) *
                    glm::rotate(glm::mat4(1.0), wheelRoll, glm::vec3(1, 0, 0)) *
                    glm::rotate(glm::mat4(1.0), glm::radians(180.0f), glm::vec3(0, 0, 1));
				mWorldTaxi[7] = // Rear left wheel
                    glm::translate(glm::mat4(1.0), finalWorldPos[3]) *
                    glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0)) *
                    glm::rotate(glm::mat4(1.0), wheelRoll, glm::vec3(1, 0, 0)); //when I accelerate the wheel should spin
				mWorldTaxi[3] = // Steering wheel
                    glm::translate(glm::mat4(1.0), finalWorldPos[4]) *
                    glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0)) *
                    glm::rotate(glm::mat4(1.0), wheelAndSteerAng, glm::vec3(0, 0, 1));
				mWorldTaxi[0] = // Door (rotating one)
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
                    // Get the hash map index of the selected person
                    int map_index = ((random_index == 0) ? 3 : ((random_index == 1) ? 7 : ((random_index == 2) ? 35 : ((random_index == 3) ? 37 : 44))));
                    // Set the value in the hash map to true ==> when the pipeline is rebuilded, we will draw it
                    drawPeople[map_index] = true;
                    // Set the flag to false and start the animation to close the door
                    pickedPassenger = false;
                    openDoor = true;
                    // Set to false the flag to say that we have to choose a new person to pick up
                    pickupPointSelected = false;
                    // Rebuild the pipeline to draw the dropped off person
                    RebuildPipeline();
                    // Reset the sound of the money and start it
                    if(ma_sound_at_end(&moneySound)) ma_sound_seek_to_pcm_frame(&moneySound, 0);
                    ma_sound_start(&moneySound);
                    // Calculate the income of the drive: time passed multiplied by the rate that is higher if it is night
                    money += (time(NULL) - pickupTime) * (isNight ? 7.9f : 4.1f);
                    // If we are not in the endless game mode
                    if(!endlessGameMode) {
                        currScene = 3;  // Set the scene to the end game scene
                        drawTwoDimPlane = true; // Set the flag to draw the 2D plane
                        twoDimTexture = 2;  // Set the texture index for the end game scene
                        RebuildPipeline();
                        // Stop the taxi's sounds
                        if(ma_sound_is_playing(&idleEngineSound)) {
                            ma_sound_stop(&idleEngineSound);
                        }
                        if(ma_sound_is_playing(&accelerationEngineSound)) {
                            ma_sound_stop(&accelerationEngineSound);
                        }
                        // Print the final score
                        std::cout << "\n\n\n\t--------- FINAL SCORE ---------\n" << std::endl;
                        std::cout << "\tTotal earnings: " << money << " $"<< std::endl;
                        std::cout << "\n\t--------- FINAL SCORE ---------" << std::endl;
                    }
                    else {
                        totDrivesCompleted++;   // Else if we are in the endless game mode, increment the number of drives completed
                    }
                }

                // SETTING OF THE PARAMETERS FOR THE GLOABL GUBO
                globalGUBO.directLightPos = glm::vec4(sunPos, 1.0f);    // Set the sun position
                for(int i = 0; i < TAXI_LIGHT_COUNT; i++) {
                    globalGUBO.taxiLightPos[i] = taxiLightPos[i];   // Set the taxi lights positions
                }
                globalGUBO.directLightCol = sunCol; // Set the sun color
                globalGUBO.rearLightCol = rearLightColor;   // Set the rear light color
                globalGUBO.frontLightCol = frontLightColor; // Set the front light color
                globalGUBO.frontLightDir = frontLightDirection; // Set the front light direction
                globalGUBO.frontLightCosines = frontLightCosines;   // Set the front light cosines
                globalGUBO.streetLightCol = streetLightCol; // Set the street light color
                globalGUBO.streetLightDirection = streetLightDirection; // Set the street light direction
                globalGUBO.streetLightCosines = streetLightCosines; // Set the street light cosines
                // Set the pickup point position (if we have already picked up the person, set the dropoff point)
                globalGUBO.pickupPointPos = (!pickedPassenger ? glm::vec4(pickupPoint.x, PICKUP_POINT_Y_OFFSET, pickupPoint.z, pickupPoint.w) : glm::vec4(dropoffPoint.x, PICKUP_POINT_Y_OFFSET, dropoffPoint.z, dropoffPoint.w));
                globalGUBO.pickupPointCol = pickupPointColor;   // Set the pickup point color
                globalGUBO.eyePos = glm::vec4(camPos, 1.0f);    // Set the camera position
                globalGUBO.settingsAndNight = glm::vec4(float(graphicsSettings), (isNight ? 1.0f : 0.0f), 0.0f, 0.0f);  // Set the graphics settings and if it is night
                DSglobal.map(currentImage, &globalGUBO, sizeof(globalGUBO), 0); // Map the global GUBO to the descriptor set

                // Read the "city.json" file to configure and update the city's mesh instances
                nlohmann::json js;
                std::ifstream ifs2("models/city.json"); // Open the JSON file
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
                        // Set the world matrix for the city element
                        mWorld=glm::mat4(TMj[0],TMj[4],TMj[8],TMj[12],TMj[1],TMj[5],TMj[9],TMj[13],TMj[2],TMj[6],TMj[10],TMj[14],TMj[3],TMj[7],TMj[11],TMj[15]);
                        uboCity[k].mMat = mWorld;   // Set the model matrix
                        uboCity[k].nMat = glm::inverse(glm::transpose(uboCity[k].mMat));    // Set the normal matrix
                        uboCity[k].mvpMat = Prj * mView * mWorld;   // Set the MVP matrix
                        DScity[k].map(currentImage, &uboCity[k], sizeof(uboCity[k]), 0); 
                        // Hash map used to take the 5 positions of the street lights closest to the city element 
                        std::unordered_map<float, glm::vec3> distancesToPositions;
                        std::vector<float> distances;   // Vector used to store the distances
                        float dist = 0.0f;
                        // For each street light:
                        for(int i = 0; i < STREET_LIGHT_COUNT; i++) {
                            // Calculate the distance between the city element and the street light
                            dist = glm::distance(streetlightPos[i], glm::vec3(mWorld[3]));
                            // Store the distance in the vector
                            distances.push_back(dist);
                            // Store the position of the street light in the hash map using the distance as key
                            distancesToPositions[dist] = streetlightPos[i];
                        }
                        // Sort the distances vector in ascending order
                        std::sort(distances.begin(), distances.end());
                        // Set in the "Local" GUBO the positions of the 5 closest street lights using the distances as keys
                        for(int i = 0; i < MAX_STREET_LIGHTS; i++) {
                            guboCity[k].streetLightPos[i] = glm::vec4(distancesToPositions[distances[i]], 1.0f);
                        }
                        // Set the gamma and metallic values
                        guboCity[k].gammaAndMetallic = glm::vec4(128.0f, 0.1f, 0.0f, 0.0f);
                        // Map the "Local" GUBO to the descriptor set
                        DScity[k].map(currentImage, &guboCity[k], sizeof(guboCity[k]), 2);
                    }

                }
                catch (const nlohmann::json::exception& e) {
                    std::cout << "[ EXCEPTION ]: " << e.what() << std::endl;
                    exit(1);
                }

                // For each mesh of the taxi
                for(int i=0; i<8; i++){
                    uboTaxi[i].mMat = mWorldTaxi[i];    // Set the model matrix
                    uboTaxi[i].nMat = glm::inverse(glm::transpose(uboTaxi[i].mMat));    // Set the normal matrix
                    uboTaxi[i].mvpMat = Prj * mView * uboTaxi[i].mMat;  // Set the MVP matrix
                    DStaxi[i].map(currentImage, &uboTaxi[i], sizeof(uboTaxi[i]), 0);    // Map the UBO to the descriptor set
                    // Hash map used to take the 5 positions of the street lights closest to the taxi element
                    std::unordered_map<float, glm::vec3> distancesToPositions;
                    std::vector<float> distances;   // Vector used to store the distances
                    float dist = 0.0f;  // Distance variable
                    // For each street light:
                    for(int j = 0; j < STREET_LIGHT_COUNT; j++) {
                        // Calculate the distance between the taxi element and the street light
                        dist = glm::distance(streetlightPos[j], glm::vec3(mWorldTaxi[1][3]));
                        // Store the distance in the vector
                        distances.push_back(dist);
                        // Store the position of the street light in the hash map using the distance as key
                        distancesToPositions[dist] = streetlightPos[j];
                    }
                    // Sort the distances vector in ascending order
                    std::sort(distances.begin(), distances.end());
                    // Set in the "Local" GUBO the positions of the 5 closest street lights using the distances as keys
                    for(int j = 0; j < MAX_STREET_LIGHTS; j++) {
                        guboTaxi[i].streetLightPos[j] = glm::vec4(distancesToPositions[distances[j]], 1.0f);
                    }
                    // Set the gamma and metallic values
                    guboTaxi[i].gammaAndMetallic = glm::vec4(128.0f, 1.0f, 0.0f, 0.0f);
                    // Map the "Local" GUBO to the descriptor set
                    DStaxi[i].map(currentImage, &guboTaxi[i], sizeof(guboTaxi[i]), 2);
                }

                // Set the position of the taxi's collision sphere center (used for collision with NPCs)
                glm::vec4 taxiCollisionSphereCenter = glm::translate(mWorldTaxi[1], glm::vec3(0.0f, 0.0f, 1.0f))[3];

                // For each NPC car
                for(int i = 0; i < CARS; i++) {
                    uboCars[i].mvpMat = Prj * mView * mWorldCars[i];    // Set the MVP matrix
                    uboCars[i].mMat = mWorldCars[i];    // Set the model matrix
                    uboCars[i].nMat = glm::inverse(glm::transpose(uboCars[i].mMat));    // Set the normal matrix
                    DScars[i].map(currentImage, &uboCars[i], sizeof(uboCars[i]), 0);    // Map the UBO to the descriptor set
                    // Hash map used to take the 5 positions of the street lights closest to the NPC car element
                    std::unordered_map<float, glm::vec3> distancesToPositions;
                    std::vector<float> distances;   // Vector used to store the distances
                    float dist = 0.0f;  // Distance variable
                    // For each street light:
                    for(int j = 0; j < STREET_LIGHT_COUNT; j++) {
                        // Calculate the distance between the NPC car element and the street light
                        dist = glm::distance(streetlightPos[j], glm::vec3(mWorldCars[i][3]));
                        // Store the distance in the vector
                        distances.push_back(dist);
                        // Store the position of the street light in the hash map using the distance as key
                        distancesToPositions[dist] = streetlightPos[j];
                    }
                    // Sort the distances vector in ascending order
                    std::sort(distances.begin(), distances.end());
                    // Set in the "Local" GUBO the positions of the 5 closest street lights using the distances as keys
                    for(int j = 0; j < MAX_STREET_LIGHTS; j++) {
                        guboCars[i].streetLightPos[j] = glm::vec4(distancesToPositions[distances[j]], 1.0f);
                    }
                    // Set the gamma and metallic values
                    guboCars[i].gammaAndMetallic = glm::vec4(128.0f, 1.0f, 0.0f, 0.0f);
                    // Map the "Local" GUBO to the descriptor set
                    DScars[i].map(currentImage, &guboCars[i], sizeof(guboCars[i]), 2);
                }

                glm::vec4 carCollisionSphereCenter = glm::vec4(0.0f);
                // Counter to check on how many cars the taxi is colliding
                collisionCounter = 0;
                // For each NPC car
                for(int i = 0; i < CARS; i++) {
                    // Set the center of the collision sphere for the NPC car as the center of the car model
                    carCollisionSphereCenter = mWorldCars[i][3];
                    // Check if the two collision spheres are colliding (dist < 2 * radius)
                    if(glm::distance(glm::vec3(taxiCollisionSphereCenter), glm::vec3(carCollisionSphereCenter)) < 2 * COLLISION_SPHERE_RADIUS) {
                        // If so, increment the collision counter
                        collisionCounter++;
                    }
                }
                // If the taxi is colliding with at least one NPC car and it wasn't already colliding
                if(collisionCounter > 0 && !inCollisionZone) {
                    inCollisionZone = true; // Set the flag to true
                    money -= 100.0f;    // Decrement the money by 100
                    // Reset the sound of the clacson and start it
                    if(ma_sound_at_end(&clacsonSound)) ma_sound_seek_to_pcm_frame(&clacsonSound, 0);
                    ma_sound_start(&clacsonSound);
                }
                // Else if the taxi is not colliding with any NPC car and it was colliding
                else if(collisionCounter == 0 && inCollisionZone) {
                    inCollisionZone = false;    // Set the flag to false
                }

                // Set the sky box's center and scale (translate and scale the sky box sphere)
                glm::mat4 scaleMat = glm::translate(glm::mat4(1.0f), sphereCenter) * glm::scale(glm::mat4(1.0f), sphereScale);
                uboSkyBox.mvpMat = Prj * mView * (scaleMat);    // Set the MVP matrix
                uboSkyBox.mMat = scaleMat;  // Set the model matrix
                uboSkyBox.nMat = glm::inverse(glm::transpose(uboSkyBox.mMat));  // Set the normal matrix
                DSskyBox.map(currentImage, &uboSkyBox, sizeof(uboSkyBox), 0);   // Map the UBO to the descriptor set
                guboSkyBox.directLightPos = glm::vec4(sunPos, 1.0f);    // Set the sun position
                guboSkyBox.directLightCol = sunCol; // Set the sun color
                guboSkyBox.eyePos = glm::vec4(camPos, 1.0f);    // Set the camera position
                guboSkyBox.gammaAndMetallic = glm::vec4(128.0f, 0.1f, 0.0f, 0.0f);  // Set the gamma and metallic values
                DSskyBox.map(currentImage, &guboSkyBox, sizeof(guboSkyBox), 2);  // Map the "Local" GUBO to the descriptor set

                // Read the "people.json" file to configure and update the people's mesh instances
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

                        // Set the world matrix for the people element
                        mWorld=glm::mat4(TMj[0],TMj[4],TMj[8],TMj[12],TMj[1],TMj[5],TMj[9],TMj[13],TMj[2],TMj[6],TMj[10],TMj[14],TMj[3],TMj[7],TMj[11],TMj[15]);
                        uboPeople[k].mMat = mWorld; // Set the model matrix
                        uboPeople[k].nMat = glm::inverse(glm::transpose(uboPeople[k].mMat));    // Set the normal matrix
                        uboPeople[k].mvpMat = Prj * mView * mWorld;  // Set the MVP matrix
                        DSpeople[k].map(currentImage, &uboPeople[k], sizeof(uboPeople[k]), 0);  // Map the UBO to the descriptor set
                        // Hash map used to take the 5 positions of the street lights closest to the people element
                        std::unordered_map<float, glm::vec3> distancesToPositions;
                        std::vector<float> distances;   // Vector used to store the distances
                        float dist = 0.0f;  // Distance variable
                        // For each street light:
                        for(int i = 0; i < STREET_LIGHT_COUNT; i++) {
                            // Calculate the distance between the people element and the street light
                            dist = glm::distance(streetlightPos[i], glm::vec3(mWorld[3]));
                            // Store the distance in the vector
                            distances.push_back(dist);
                            // Store the position of the street light in the hash map using the distance as key
                            distancesToPositions[dist] = streetlightPos[i];
                        }
                        // Sort the distances vector in ascending order
                        std::sort(distances.begin(), distances.end());
                        // Set in the "Local" GUBO the positions of the 5 closest street lights using the distances as keys
                        for(int i = 0; i < MAX_STREET_LIGHTS; i++) {
                            guboPeople[k].streetLightPos[i] = glm::vec4(distancesToPositions[distances[i]], 1.0f);
                        }
                        // Set the gamma and metallic values
                        guboPeople[k].gammaAndMetallic = glm::vec4(128.0f, 0.1f, 0.0f, 0.0f);
                        // Map the "Local" GUBO to the descriptor set
                        DSpeople[k].map(currentImage, &guboPeople[k], sizeof(guboPeople[k]), 2);
                    }

                }
                catch (const nlohmann::json::exception& e) {
                    std::cout << "[ EXCEPTION ]: " << e.what() << std::endl;
                    exit(1);
                }

                // Set the position of the arrow (if we have already picked up the person, set the dropoff point)
                // The arrow will move up and down with a sinusoidal movement
                glm::vec3 arrowPosition = (!pickedPassenger ? glm::vec3(pickupPoint.x, ARROW_Y_OFFSET + (glm::cos(cTime) / 4.0f), pickupPoint.z) : glm::vec3(dropoffPoint.x, ARROW_Y_OFFSET + (glm::cos(cTime) / 4.0f), dropoffPoint.z));
                // Set the world matrix for the arrow translating it to the position and rotating it around the Z axis
                // The arrow will also rotate around the Y axis with a turn factor of 10 degrees per tick
                glm::mat4 mWorldArrow = glm::rotate(glm::rotate(glm::translate(glm::mat4(1.0), arrowPosition), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)), glm::radians(10.0f) * cTime, glm::vec3(0.0f, 1.0f, 0.0f));
                uboArrow.mvpMat = Prj * mView * mWorldArrow;    // Set the MVP matrix
                uboArrow.mMat = mWorldArrow;    // Set the model matrix
                uboArrow.nMat = glm::inverse(glm::transpose(uboArrow.mMat));    // Set the normal matrix
                DSarrow.map(currentImage, &uboArrow, sizeof(uboArrow), 0);  // Map the UBO to the descriptor set
                // Set the position of the arrow's pickup point (if we have already picked up the person, set the dropoff point)
                guboArrow.pickupPointPos = (!pickedPassenger ? glm::vec4(pickupPoint.x, PICKUP_POINT_Y_OFFSET, pickupPoint.z, pickupPoint.w) : glm::vec4(dropoffPoint.x, PICKUP_POINT_Y_OFFSET, dropoffPoint.z, dropoffPoint.w));
                guboArrow.pickupPointCol = pickupPointColor;    // Set the pickup point color (POINTLIGHT)
                guboArrow.eyePos = glm::vec4(camPos, 1.0f); // Set the camera position
                guboArrow.gammaAndMetallic = glm::vec4(128.0f, 1.0f, 0.0f, 0.0f);   // Set the gamma and metallic values
                DSarrow.map(currentImage, &guboArrow, sizeof(guboArrow), 1);    // Map the GUBO to the descriptor set

            }
        }

        // Helper function to check if a vector of points is inside a collision box
        bool checkCollision(glm::vec3 *points, int dim, CollisionBox collBox, bool ext) {
            // If we are checking for external collision (the model is inside the collision box and cannot exit)
            if(ext) {
                for(int i = 0; i < dim; i++) {
                    // Check the x and z coordinates of the points and in case return false
                    if(points[i].x <= collBox.xMin || points[i].x >= collBox.xMax || points[i].z <= collBox.zMin || points[i].z >= collBox.zMax) {
                        return false;
                    }
                }
            }
            // Else we are checking for internal collision (the model is outside the collision box and cannot enter)
            else {
                for(int i = 0; i < dim; i++) {
                    // Check the x and z coordinates of the points and in case return false
                    if((points[i].x >= collBox.xMin && points[i].x <= collBox.xMax) && (points[i].z >= collBox.zMin && points[i].z <= collBox.zMax)) {
                        return false;
                    }
                }
            }
            return true;
        }

};

int main(int argc, char* argv[]) {

    Application app;    // Create the application object

    int choose = 0;
    int oldChoose = 0;
    int gameMode = 0;
    const char* gameModes[GAMEMODE_COUNT] = {"Arcade", "Endless"};
    int graphicSetting = 2;
    const char* gSettings[GRAPHICS_SETTINGS_COUNT] = {"Low", "Medium", "High"};
    float musicVolume = 25.0f;
    float soundVolume = 100.0f;
    
    std::ifstream f("files/logo.txt");  // Load the file with the logo for the CLI
    if (f.is_open()) {
        std::cout << f.rdbuf(); // Print the logo
    }
    // Print the main menu
    do {
        std::cout << "--------- MAIN MENU ---------\n" << std::endl;
        std::cout << "1 - Start the game" << std::endl;
        std::cout << "2 - Settings" << std::endl;
        std::cout << "3 - Exit" << std::endl;
        std::cout << "\nChoosing: ";
        std::cin >> choose; // Get the user's choice
        switch(choose) {
            case 2: {   // If the user chooses the settings
                oldChoose = choose; // Save the choice
                // Print the settings menu
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
                    std::cin >> choose; // Get the user's choice
                    switch(choose) {
                        case 1: {   // If the user chooses the game mode
                            gameMode = (gameMode + 1) % GAMEMODE_COUNT; // Change the game mode from arcade to endless and vice versa
                            break;
                        }
                        case 2: {   // If the user chooses the graphic settings
                            graphicSetting = (graphicSetting + 1) % GRAPHICS_SETTINGS_COUNT;    // Change the graphic settings (low, medium, high)
                            break;
                        }
                        case 3: {   // If the user chooses the music volume
                            do {
                                std::cout << "Enter the music volume [0.0 - 100.0]: ";
                                std::cin >> musicVolume;    // Get the music volume
                            } while(musicVolume < 0.0f || musicVolume > 100.0f);    
                            break;
                        }
                        case 4: {   // If the user chooses the sound and FX volume
                            do {
                                std::cout << "Enter the sound and FX volume [0.0 - 100.0]: ";
                                std::cin >> soundVolume;    // Get the sound and FX volume
                            } while(soundVolume < 0.0f || soundVolume > 100.0f);
                            break;
                        }
                        case 5:
                            break;
                        default: 
                            break;
                    }
                } while(choose != 5);
                choose = oldChoose; // Go back to the main menu
                break;
            }
            case 3: {   // If the user chooses to exit
                std::cout << "Closing the sofware..." << std::endl;
                return EXIT_SUCCESS;
            }
            default:
                break;
        }
    } while(choose == 2);   // If the user chooses the settings, go back to the main menu

    app.graphicsSettings = graphicSetting;  // Set the graphic settings
    app.endlessGameMode = (gameMode == 1);  // Set the game mode (arcade or endless)

    std::cout << "[ LOADING ]: Loading sound resources:\t[                    ]" << std::endl;
    // Initialize the miniaudio engine (used for the sound)
    ma_result result = ma_engine_init(NULL, &app.engine);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize miniaudio engine!");
    }
    // Initialize title music
    result = ma_sound_init_from_file(&app.engine, "audios/title.mp3", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.titleMusic);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize title sound!");
    }
    ma_sound_set_looping(&app.titleMusic, MA_TRUE); // Set the title music to loop
    ma_sound_set_volume(&app.titleMusic, musicVolume / 100.0f); // Set the volume of the title music
    // Initialize in-game music
    result = ma_sound_init_from_file(&app.engine, "audios/ingame.mp3", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.ingameMusic);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize ingame sound!");
    }
    ma_sound_set_looping(&app.ingameMusic, MA_TRUE);    // Set the in-game music to loop
    ma_sound_set_volume(&app.ingameMusic, musicVolume / 100.0f);    // Set the volume of the in-game music
    // Initialize idle engine sound
    result = ma_sound_init_from_file(&app.engine, "audios/idle.wav", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.idleEngineSound);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize idle engine sound!");
    }
    ma_sound_set_looping(&app.idleEngineSound, MA_TRUE);    // Set the idle engine sound to loop
    ma_sound_set_volume(&app.idleEngineSound, 2.0f * soundVolume / 100.0f);   // Set the volume of the idle engine sound
    // Initialize accelearion engine sound
    result = ma_sound_init_from_file(&app.engine, "audios/acceleration.wav", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.accelerationEngineSound);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize acceleration engine sound!");
    }
    ma_sound_set_looping(&app.accelerationEngineSound, MA_TRUE);    // Set the acceleration engine sound to loop
    ma_sound_set_volume(&app.accelerationEngineSound, 1.5f * soundVolume / 100.0f);  // Set the volume of the acceleration engine sound
    // Initialize pickup sound
    result = ma_sound_init_from_file(&app.engine, "audios/pickup.wav", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.pickupSound);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize pickup sound!");
    }
    ma_sound_set_volume(&app.pickupSound, 2.0f * soundVolume / 100.0f);   // Set the volume of the pickup sound
    // Initialize money sound
    result = ma_sound_init_from_file(&app.engine, "audios/money.wav", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.moneySound);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize money sound!");
    }
    ma_sound_set_volume(&app.moneySound, 3.0f * soundVolume / 100.0f);  // Set the volume of the money sound
    // Intialize clacson sound
    result = ma_sound_init_from_file(&app.engine, "audios/clacson.wav", MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, NULL, NULL, &app.clacsonSound);
    if(result != MA_SUCCESS) {
        throw std::runtime_error("[ ERROR ]: Failed to initialize clacson sound!");
    }
    ma_sound_set_volume(&app.clacsonSound, 0.25f * soundVolume / 100.0f);    // Set the volume of the clacson sound
    std::cout << "[ LOADING ]: Loading sound resources:\t[====================]" << std::endl;

    srand(time(NULL));  // Initialize the random seed

    try {
        app.run();  // Run the application
    } catch (const std::exception& e) {
        std::cerr << "[ EXCEPTION ]:" << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}