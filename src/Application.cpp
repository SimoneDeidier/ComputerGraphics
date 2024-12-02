// This has been adapted from the Vulkan tutorial
#include "headers/Starter.hpp"
#include "headers/TextMaker.hpp"
#include <iostream>
#include <fstream>
#define MESH 209

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

    DescriptorSetLayout DSL,DSLcity, DSLsky;

    VertexDescriptor VD, VDcity, VDsky;

    Pipeline P, Pcity, Psky;

    TextMaker txt;

    Model  Mtaxi, Msky;
    Model Mcity[MESH];

    DescriptorSet DStaxi, DScity[MESH], DSsky;

    Texture Tcity, Tsky;


    UniformBufferObject uboTaxi, uboSky;
    UniformBufferObject ubocity[MESH];
    GlobalUniformBufferObject guboTaxi,guboSky;
    GlobalUniformBufferObject gubocity[MESH];

    // Other application parameters
    int currScene = 0;
	glm::vec3 camPos = glm::vec3(0.0, 1.5f, -5.0f); //initial pos of camera
    glm::vec3 camPosInPhotoMode;
    glm::vec3 taxiPos = glm::vec3(0.0, -0.2, 0.0); //initial pos of taxi
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
        uniformBlocksInPool =  (2 * MESH) + 2+2;
        texturesInPool = MESH + 1 +1+1; //city, taxi, text, sky
        setsInPool = MESH + 1 +1+1;

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

        P.init(this, &VD, "shaders/BaseVert.spv", "shaders/TaxiFrag.spv", {&DSL});
        Pcity.init(this, &VDcity, "shaders/BaseVert.spv", "shaders/TaxiFrag.spv", {&DSLcity});
        Pcity.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false);
        Psky.init(this, &VDsky, "shaders/SkyVert.spv", "shaders/SkyFrag.spv", {&DSLsky});
        Psky.setAdvancedFeatures(VK_COMPARE_OP_LESS, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, false); //todo cosa dovevamo fare quando telecamera dentro una stanza


        Mtaxi.init(this, &VD, "models/transport_purpose_003_transport_purpose_003.001.mgcg", MGCG );
        Msky.init(this, &VDsky, "models/Sphere.obj", OBJ);

        Tcity.init(this,"textures/Textures_City.png");
        Tsky.init(this, "textures/images.png");
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
    }

    void pipelinesAndDescriptorSetsInit() {

        P.create();
        Pcity.create();
        Psky.create();

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
        txt.pipelinesAndDescriptorSetsInit();	
    }

    void pipelinesAndDescriptorSetsCleanup() {
        P.cleanup();
        Pcity.cleanup();
        Psky.cleanup();

        DStaxi.cleanup();

        for(int i = 0; i < MESH; i++) {
            DScity[i].cleanup();
        }

        DSsky.cleanup();

        txt.pipelinesAndDescriptorSetsCleanup();
    }

    void localCleanup() {

        Tcity.cleanup();
        Tsky.cleanup();

        Mtaxi.cleanup();

        for(int i = 0; i < MESH; i++) {
            Mcity[i].cleanup();
        }

        Msky.cleanup();

        DSL.cleanup();
        DSLcity.cleanup();
        DSLsky.cleanup();

        P.destroy();
        Pcity.destroy();
        Psky.destroy();

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
        const float farPlane = 250.0f;
        glm::mat4 Prj = glm::perspective(glm::radians(45.0f), Ar, nearPlane, farPlane);
        Prj[1][1] *= -1; //Projection matrix


        glm::mat4 mWorld; //World matrix for city
        mWorld = glm::translate(glm::mat4(1), glm::vec3(0, 0, 3)) * glm::rotate(glm::mat4(1), glm::radians(180.0f), glm::vec3(0, 1, 0));


        //glm::vec3 sunPos = glm::vec3(5.5f, 30.0f, 7.5f);
        glm::vec3 sunPos = glm::vec3(cos(glm::radians(135.0f)) * cos(cTime * angTurnTimeFact), sin(glm::radians(135.0f)), cos(glm::radians(135.0f)) * sin(cTime * angTurnTimeFact));
        

        glm::mat4 mWorldTaxi =
            glm::translate(glm::mat4(1.0), taxiPos) *
            glm::rotate(glm::mat4(1.0), steeringAng, glm::vec3(0, 1, 0));


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

        glm::mat4 scaleMat = glm::translate(glm::mat4(1.0f), glm::vec3(40.0f, 20.0f, -75.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(180.0f, 50.0f, 180.0f));
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