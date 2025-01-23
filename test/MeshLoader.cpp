#include "Starter.hpp"

struct UniformBlock {
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
};

struct GlobalUniformBufferObject {
	alignas(16) glm::vec4 lightPos;
	alignas(16) glm::vec4 lightCol;
	alignas(16) glm::vec4 eyePos;
};

struct Vertex {
	glm::vec3 pos;
	glm::vec2 UV;
	glm::vec3 normal;
};

class MeshLoader : public BaseProject {
	
	protected:

		float Ar;

		DescriptorSetLayout DSL;

		VertexDescriptor VD;

		Pipeline P;

		Model<Vertex> M1;

		DescriptorSet DS1;

		Texture T1;
		
		UniformBlock ubo1;
		GlobalUniformBufferObject gubo1;

	void setWindowParameters() {
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Mesh Loader";
		windowResizable = GLFW_TRUE;
		initialBackgroundColor = {0.0f, 0.005f, 0.01f, 1.0f};
		
		uniformBlocksInPool = 2;
		texturesInPool = 1;
		setsInPool = 1;
		
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

		VD.init(this, {
			{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
		}, {
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION},
			{0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV), sizeof(glm::vec2), UV},
			{0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal), sizeof(glm::vec3), NORMAL}
		});

		P.init(this, &VD, "shaders/Vert.spv", "shaders/Frag.spv", {&DSL});

		M1.init(this, &VD, "models/road_tile_1x1_001.mgcg", MGCG);
		T1.init(this, "textures/Textures_City.png");
	}
	
	void pipelinesAndDescriptorSetsInit() {
		P.create();

		DS1.init(this, &DSL, {
			{0, UNIFORM, sizeof(UniformBlock), nullptr},
			{1, TEXTURE, 0, &T1},
			{2, UNIFORM, sizeof(GlobalUniformBufferObject), nullptr}
		});
	}

	void pipelinesAndDescriptorSetsCleanup() {
		P.cleanup();
		DS1.cleanup();
	}

	void localCleanup() {
		T1.cleanup();
		M1.cleanup();
		DSL.cleanup();
		P.destroy();        
	}
	
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
		P.bind(commandBuffer);
		DS1.bind(commandBuffer, P, 0, currentImage);
		M1.bind(commandBuffer);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(M1.indices.size()), 1, 0, 0, 0);
	}

	void updateUniformBuffer(uint32_t currentImage) {
		if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		
		float deltaT;
		glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
		bool fire = false;
		getSixAxis(deltaT, m, r, fire);

		static float cTime = 0.0f;
		cTime += deltaT;
		cTime = (cTime > 72.0f) ? (cTime - 72.0f) : cTime;
		const float aTTF = 2.0f * M_PI / 72.0f;

		const float FOVy = glm::radians(90.0f);
		const float nearPlane = 0.1f;
		const float farPlane = 100.0f;
		
		glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
		Prj[1][1] *= -1;
		glm::vec3 camTarget = glm::vec3(0, 0, 0);
		glm::vec3 camPos = camTarget + glm::vec3(6, 3, 10) / 2.0f;
		glm::mat4 View = glm::lookAt(camPos, camTarget, glm::vec3(0, 1, 0));

		glm::mat4 World;

		World = glm::translate(glm::mat4(1.0f), glm::vec3(-3, -10, -5));
		ubo1.mvpMat = Prj * View * World;
		ubo1.mMat = World;
		ubo1.nMat = glm::inverse(glm::transpose(ubo1.mMat));
		DS1.map(currentImage, &ubo1, sizeof(ubo1), 0);
		gubo1.lightPos = glm::vec4(15.0f * glm::cos(cTime * aTTF), 15.0f * glm::sin(cTime * aTTF), 0.0f, 1.0f);
		gubo1.lightCol = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		gubo1.eyePos = glm::vec4(camPos, 1.0f);
		DS1.map(currentImage, &gubo1, sizeof(gubo1), 2);
	}    
};

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