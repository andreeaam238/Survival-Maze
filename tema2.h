#pragma once

#include "components/simple_scene.h"
#include "lab_m1/tema2/lab_camera.h"
#include <iostream>
#include <stack>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>


namespace m1
{
	constexpr auto ROAD = 0;
	constexpr auto WALL = 1;

	class Tema2 : public gfxc::SimpleScene
	{
	public:
		Tema2();
		~Tema2();

		void Init()
			override;

		struct ViewportSpace {
			ViewportSpace() : x(0), y(0), width(1), height(1) {}

			ViewportSpace(int x, int y, int width, int height)
				: x(x), y(y), width(width), height(height) {}

			int x;
			int y;
			int width;
			int height;
		};

		struct Enemy {
			double x;
			double y;
			double initialX;
			double initialY;
			int render = 0;
		};

		struct Bullet {
			float x;
			float y;
			float z;
			float speed;
			int render = 0;
			glm::vec3 forward;
		};

	private:
		void FrameStart()
			override;
		void Update(float deltaTimeSeconds)
			override;
		void FrameEnd()
			override;

		void RenderMesh(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix);

		void OnInputUpdate(float deltaTime, int mods) override;
		void OnKeyPress(int key, int mods) override;
		void OnKeyRelease(int key, int mods) override;
		void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
		void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
		void OnWindowResize(int width, int height) override;
		void RenderSimpleMesh(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix,
			const glm::vec3& color, bool die = false);
		void RenderSimpleMesh(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix,
			Texture2D* texture1 = NULL, Texture2D* texture2 = NULL);
		void SetViewportArea(const ViewportSpace& viewSpace,
			glm::vec3 colorColor = glm::vec3(0), bool clear = true);

		// Maze Functions.
		void backtrackMaze(int x, int y);
		void buildMaze();
		bool hasEscaped();
		bool checkPosition(int x, int y);

		// Colission Functions.
		bool intersect(glm::vec3 sphere, glm::vec3 boxmin, glm::vec3 boxmax, float radius);
		bool intersect(glm::vec3 sphere, glm::vec3 other, float radius1, float radius2);
		bool checkPlayerWallCollision();

		// Element Generator and Render Functions.
		void generateEnemies();
		void renderPlayer();
		void renderEnemies(float deltaTimeSeconds);
		void renderMaze();
		void renderBullets(float deltaTimeSeconds);
		void renderHUD(float deltaTimeSeconds);

	protected:
		// TODO(student): If you need any other class variables, define them here.
		ViewportSpace viewSpace;
		implemented::Camera* camera;
		glm::mat4 projectionMatrix;
		bool renderCameraTarget;
		bool perspective;
		float zNear, zFar, fov;

		// Camera Offset.
		glm::vec3 positionOffset = glm::vec3(0.0f, 8.5f, -5.0f);
		glm::vec3 centerOffset = glm::vec3(0.0f, 1.0f, 1.0f);
		glm::vec3 up = glm::vec3(0.0f, 2.0f, 0.0f);

		std::unordered_map < std::string, Texture2D* > mapTextures;

		// Map.
		int** map;
		int initialX, initialY;
		int rows, columns;
		glm::vec3 initialPosition;
		double wallSize, tolerance;

		// Enemies.
		std::vector <Enemy> enemies;
		bool movePlusX = true, moveMinusX = false, movePlusY = false, moveMinusY = false;

		// Player.
		int hits = 0;
		float rotateAngle = 0.0f;
		bool thirdPerson = true;

		// Time.
		std::vector <Bullet> bullets;
		float bulletLaunchingTime, interval;
		float timeToEscape = 120.0f;
	};
}   // namespace m1
