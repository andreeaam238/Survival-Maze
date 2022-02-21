#include "lab_m1/tema2/tema2.h"

using namespace std;
using namespace m1;


/*
 *  To find out more about `FrameStart`, `Update`, `FrameEnd`
 *  and the order in which they are called, see `world.cpp`.
 */


Tema2::Tema2() {
	fov = RADIANS(60);
	zNear = 0.01f;
	zFar = 200.0f;

	rows = 28;
	columns = 28;
	wallSize = 20.0f;
	tolerance = 0.001f;

	buildMaze();
	generateEnemies();

	initialPosition = glm::vec3(initialX * (wallSize + tolerance), wallSize / 1.5f, initialY * (wallSize + tolerance));

	bulletLaunchingTime = 0.0f;
	interval = 0.5f;
}


Tema2::~Tema2() {
}


void Tema2::Init() {
	renderCameraTarget = false;

	camera = new implemented::Camera();
	camera->Set(initialPosition + positionOffset, initialPosition + centerOffset, up);

	const string sourceTextureDir = PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "tema2", "textures");

	{
		Texture2D* texture = new Texture2D();
		texture->Load2D(PATH_JOIN(sourceTextureDir, "wall.jpg").c_str(), GL_REPEAT);
		mapTextures["wall"] = texture;
	}

	{
		Texture2D* texture = new Texture2D();
		texture->Load2D(PATH_JOIN(sourceTextureDir, "pavement.jpg").c_str(), GL_REPEAT);
		mapTextures["pavement"] = texture;
	}

	{
		Texture2D* texture = new Texture2D();
		texture->Load2D(PATH_JOIN(sourceTextureDir, "winner.jpg").c_str(), GL_REPEAT);
		mapTextures["winner"] = texture;
	}

	{
		Mesh* mesh = new Mesh("box");
		mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "box.obj");
		meshes[mesh->GetMeshID()] = mesh;
	}

	{
		Mesh* mesh = new Mesh("sphere");
		mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "sphere.obj");
		meshes[mesh->GetMeshID()] = mesh;
	}

	// TODO(student): After you implement the changing of the projection
	// parameters, remove hardcodings of these parameters
	projectionMatrix = glm::perspective(fov, window->props.aspectRatio, zNear, zFar);

	// Create a shader program for drawing face polygon with the color of the normal
	{
		Shader* shader = new Shader("LabShader");
		shader->AddShader(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "tema2", "shaders", "VertexShader.glsl"),
			GL_VERTEX_SHADER);
		shader->AddShader(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "tema2", "shaders", "FragmentShader.glsl"),
			GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	{
		Shader* shader = new Shader("ObjectShader");
		shader->AddShader(
			PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "tema2", "shaders", "ObjectVertexShader.glsl"),
			GL_VERTEX_SHADER);
		shader->AddShader(
			PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "tema2", "shaders", "ObjectFragmentShader.glsl"),
			GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
}


// Create the Enemies.
void Tema2::generateEnemies() {
	std::vector <std::pair<int, int>> freeCells;

	// Keep the ROAD squares that are not an exit of the Maze.
	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < columns; ++j) {
			if (i == initialX && j == initialY) {
				continue;
			}

			if (map[i][j] == ROAD && ((i != 0 && i != rows - 1) && (j != 0 && j != columns - 1))) {
				freeCells.push_back(std::make_pair(i, j));
			}
		}
	}

	// Keep random free positions for spawning enemies.
	int noEnemies = rand() % freeCells.size();
	std::random_shuffle(std::begin(freeCells), std::end(freeCells));

	// Create the Enemies.
	for (int i = 0; i < noEnemies; ++i) {
		Enemy enemy;

		enemy.initialX = enemy.x = freeCells.at(i).first;
		enemy.initialY = enemy.y = freeCells.at(i).second;

		enemy.x *= (wallSize + tolerance);
		enemy.x -= wallSize / 4.0f;
		enemy.initialX *= (wallSize + tolerance);

		enemy.y *= (wallSize + tolerance);
		enemy.y -= wallSize / 4.0f;
		enemy.initialY *= (wallSize + tolerance);

		enemies.push_back(enemy);
	}
}


// Check if a Road can be built at the current Position.
bool Tema2::checkPosition(int i, int j) {
	if (i < 0 || i >= rows || j < 0 || j >= columns) {
		return false;
	}

	if (map[i][j] != WALL) {
		return false;
	}

	return true;
}


// Backtracking Generator Maze Algorithm
void Tema2::backtrackMaze(int x, int y) {
	// Current cell.
	map[x][y] = ROAD;

	// Movement directions: North, South, West and East.
	int deltaX[] = { 0, 0, 1, -1 };
	int deltaY[] = { -1, 1, 0, 0 };

	// Shuffling the movement directions.
	auto seed = std::rand();

	std::srand(seed);
	std::random_shuffle(std::begin(deltaX), std::end(deltaX));

	std::srand(seed);
	std::random_shuffle(std::begin(deltaY), std::end(deltaY));

	// Randomly choose a wall at the current cell and open a passage through it to 
	// any random, unvisited, adjacent cell.
	for (int i = 0; i < 4; ++i) {
		int p = x + 2 * deltaX[i];
		int q = y + 2 * deltaY[i];
		if (checkPosition(p, q)) {
			map[p - deltaX[i]][q - deltaY[i]] = ROAD;

			// This is now the current cell.
			backtrackMaze(p, q);
		}
	}
}


void Tema2::buildMaze() {
	// Allocate memory for the Maze and initialise it only with Walls.
	map = new
		int* [rows];
	for (int i = 0; i < rows; ++i) {
		map[i] = new
			int[columns];

		for (int j = 0; j < columns; ++j) {
			map[i][j] = WALL;
		}
	}

	// Randomly choose a starting cell which is not a Marginal Cell.
	srand(time(NULL));
	initialX = std::rand() % rows;
	initialY = std::rand() % columns;

	if (initialX == 0) {
		initialX++;
	}
	else if (initialX == rows - 1) {
		initialX--;
	}

	if (initialY == 0) {
		initialY++;
	}
	else if (initialY == columns - 1) {
		initialY--;
	}

	// Generate Maze from the Starting Cell.
	backtrackMaze(initialX, initialY);

	// Check if the Player can Escape from the Maze.
	bool canEscape = false;

	// Left and Right Margins Check.
	for (int i = 0; i < rows; ++i) {
		if (map[i][0] == ROAD || map[i][columns - 1] == ROAD) {
			canEscape = true;
			break;
		}
	}

	// Upper and Lower Margins Check.
	if (!canEscape) {
		for (int i = 0; i < columns; ++i) {
			if (map[0][i] == ROAD || map[rows - 1][i] == ROAD) {
				canEscape = true;
				break;
			}
		}
	}

	// If there isn't any way out (usually happens when both the number of rows 
	// and the number of columns are uneven, remove the marginal walls.
	if (!canEscape) {
		for (int i = 0; i < rows; ++i) {
			map[i][0] = map[i][columns - 1] = ROAD;
		}

		for (int i = 0; i < columns; ++i) {
			map[0][i] = map[rows - 1][i] = ROAD;
		}
	}
}


void Tema2::SetViewportArea(const ViewportSpace& viewSpace, glm::vec3 colorColor,
	bool clear) {
	glViewport(viewSpace.x, viewSpace.y, viewSpace.width, viewSpace.height);

	glEnable(GL_SCISSOR_TEST);
	glScissor(viewSpace.x, viewSpace.y, viewSpace.width, viewSpace.height);

	// Clears the color buffer (using the previously set color) and depth buffer
	glClearColor(colorColor.r, colorColor.g, colorColor.b, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
}


void Tema2::FrameStart() {
	// Clears the color buffer (using the previously set color) and depth buffer
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Hide Mouse Pointer.
	window->HidePointer();

	glm::ivec2 resolution = window->GetResolution();
	// Sets the screen area where to draw
	glViewport(0, 0, resolution.x, resolution.y);
}


// Sphere - AABB Collision - implemented as explained on 
// developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection
bool Tema2::intersect(glm::vec3 sphere, glm::vec3 boxmin, glm::vec3 boxmax, float radius) {
	// Get box closest point to sphere center by clamping.
	float x = max(boxmin.x, min(sphere.x, boxmax.x));
	float y = max(boxmin.y, min(sphere.y, boxmax.y));
	float z = max(boxmin.z, min(sphere.z, boxmax.z));

	// Calculate the distance between the AABB's closest point and the sphere's center.
	float distance = sqrt((x - sphere.x) * (x - sphere.x) +
		(y - sphere.y) * (y - sphere.y) +
		(z - sphere.z) * (z - sphere.z));

	// Check if it is less than or equal to the sphere's radius.
	return distance < radius;
}


// Sphere - Sphere Collision - implemented as explained on 
// developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection
bool Tema2::intersect(glm::vec3 sphere, glm::vec3 other, float radius1, float radius2) {
	// Compute the distance between the spheres radii.
	float distance = sqrt((sphere.x - other.x) * (sphere.x - other.x) +
		(sphere.y - other.y) * (sphere.y - other.y) +
		(sphere.z - other.z) * (sphere.z - other.z));

	// Check if the distance between the sphere's centers is less than or equal to 
	// the sum of their radii.
	return distance < (radius1 + radius2);
}


// Check if the Player has escaped the Maze (he has reached any of the Maze's margins).
bool Tema2::hasEscaped() {
	// Upper and Lower Margins.
	for (int i = 0; i < rows; ++i) {
		if (map[i][0] == ROAD &&
			camera->GetTargetPosition().x >= i * (wallSize + tolerance) - wallSize / 2 &&
			camera->GetTargetPosition().z >= -wallSize / 2 &&
			camera->GetTargetPosition().x <= i * (wallSize + tolerance) + wallSize / 2 &&
			camera->GetTargetPosition().z <= wallSize / 2) {
			return true;
			printf("Congratulations! You have escaped the maze!\n");
			exit(0);
		}

		if (map[i][columns - 1] == ROAD &&
			camera->GetTargetPosition().x >= i * (wallSize + tolerance) - wallSize / 2 &&
			camera->GetTargetPosition().z >= (double(columns) - 1) * (wallSize + tolerance) - wallSize / 2 &&
			camera->GetTargetPosition().x <= i * (wallSize + tolerance) + wallSize / 2 &&
			camera->GetTargetPosition().z <= (double(columns) - 1) * (wallSize + tolerance) + wallSize / 2) {
			return true;

		}
	}

	// Left and Right Margins.
	for (int i = 0; i < columns; ++i) {
		if (map[0][i] == ROAD &&
			camera->GetTargetPosition().x >= -wallSize / 2 &&
			camera->GetTargetPosition().z >= i * (wallSize + tolerance) - wallSize / 2 &&
			camera->GetTargetPosition().x <= wallSize / 2 &&
			camera->GetTargetPosition().z <= i * (wallSize + tolerance) + wallSize / 2) {
			return true;
		}

		if (map[rows - 1][i] == ROAD &&
			camera->GetTargetPosition().x >= (double(rows) - 1) * (wallSize + tolerance) - wallSize / 2 &&
			camera->GetTargetPosition().z >= i * (wallSize + tolerance) - wallSize / 2 &&
			camera->GetTargetPosition().x <= (double(rows) - 1) * (wallSize + tolerance) + wallSize / 2 &&
			camera->GetTargetPosition().z <= i * (wallSize + tolerance) + wallSize / 2) {
			return true;
		}
	}

	return false;
}


// Render the Player reusing the model matrices in order to keep the 3d construction as a one piece.
void Tema2::renderPlayer() {
	// Head.
	glm::mat4 modelMatrix = glm::mat4(1);
	modelMatrix = glm::translate(modelMatrix, camera->GetTargetPosition());
	modelMatrix = glm::rotate(modelMatrix, glm::radians(rotateAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(wallSize / 10.0f, wallSize / 10.0f, wallSize / 10.0f));
	RenderSimpleMesh(meshes["box"], shaders["ObjectShader"], modelMatrix, glm::vec3(1.0f, 0.8f, 0.8f));

	// Torso.
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0, -wallSize / 11.0f, 0));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(wallSize / 12.0f, wallSize / 7.0f, wallSize / 18.0f));
	RenderSimpleMesh(meshes["box"], shaders["ObjectShader"], modelMatrix, glm::vec3(0.5f, 0.8f, 0.8f));

	// Arms.
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-wallSize / 25.71f, 0.17f, 0));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(wallSize / 36.0f, 0.65f, 0.8f));
	RenderSimpleMesh(meshes["box"], shaders["ObjectShader"], modelMatrix, glm::vec3(0.69f, 0.4f, 0.69f));

	modelMatrix = glm::translate(modelMatrix, glm::vec3(3.65f * wallSize / 26.0f, 0, 0));
	RenderSimpleMesh(meshes["box"], shaders["ObjectShader"], modelMatrix, glm::vec3(0.69f, 0.4f, 0.69f));

	// Hands.
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0, -wallSize / 26.0f, 0));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(1, 0.55f, 0.8f));
	RenderSimpleMesh(meshes["box"], shaders["ObjectShader"], modelMatrix, glm::vec3(1.0f, 0.8f, 0.8f));

	modelMatrix = glm::translate(modelMatrix, glm::vec3(-3.65f * wallSize / 26.0f, 0, 0));
	RenderSimpleMesh(meshes["box"], shaders["ObjectShader"], modelMatrix, glm::vec3(1.0f, 0.8f, 0.8f));

	// Legs.
	modelMatrix = glm::translate(modelMatrix, glm::vec3(1.21f * wallSize / 26.0f, -wallSize / 12.0f, 0));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(wallSize / 24.0f, wallSize / 8.5f, wallSize / 13.0f));
	RenderSimpleMesh(meshes["box"], shaders["ObjectShader"], modelMatrix, glm::vec3(0.5f, 0.1f, 0.3f));

	modelMatrix = glm::translate(modelMatrix, glm::vec3(1.46f * wallSize / 26.0f, 0, 0));
	RenderSimpleMesh(meshes["box"], shaders["ObjectShader"], modelMatrix, glm::vec3(0.5f, 0.1f, 0.3f));
}


// Render Enemies.
void Tema2::renderEnemies(float deltaTimeSeconds) {
	glm::mat4 modelMatrix;
	double enemySpeed = 5.0f;

	for (int i = 0; i < enemies.size(); ++i) {
		Enemy enemy = enemies.at(i);
		if (enemies.at(i).render == 20) {
			enemies.erase(enemies.begin() + i);
		}
		else if (enemies.at(i).render != 0) {
			modelMatrix = glm::mat4(1);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(enemy.x, wallSize / 2.5f - 2.0f, enemy.y));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(wallSize / 2.5f, wallSize / 2.5f, wallSize / 2.5f));
			RenderSimpleMesh(meshes["sphere"], shaders["ObjectShader"], modelMatrix, glm::vec3(0.81f, 0.05f, 0.05f),
				true);
			enemies.at(i).render++;
		}
		else {
			// Check Enemy - Player Collision.
			bool collision = intersect(glm::vec3(enemy.x, wallSize / 2.5f - 2.0f, enemy.y),
				camera->GetTargetPosition() - 0.9f,
				camera->GetTargetPosition() + 0.9f,
				wallSize / 2.5f);
			if (collision) {
				modelMatrix = glm::mat4(1);
				modelMatrix = glm::translate(modelMatrix, glm::vec3(enemy.x, wallSize / 2.5f - 2.0f, enemy.y));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(wallSize / 2.5f, wallSize / 2.5f, wallSize / 2.5f));
				RenderSimpleMesh(meshes["sphere"], shaders["ObjectShader"], modelMatrix, glm::vec3(0.81f, 0.05f, 0.05f),
					true);
				enemies.at(i).render++;
				hits++;

				// Game Over - Player has 0 Lives left.
				if (hits == 3) {
					printf("Game Over! You've lost!\n");
					exit(0);
				}
			}
			else {
				modelMatrix = glm::mat4(1);
				modelMatrix = glm::translate(modelMatrix, glm::vec3(enemy.x, wallSize / 2.5f - 2.0f, enemy.y));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(wallSize / 2.5f, wallSize / 2.5f, wallSize / 2.5f));
				RenderSimpleMesh(meshes["sphere"], shaders["ObjectShader"], modelMatrix,
					glm::vec3(0.81f, 0.05f, 0.05f));

				// Move Enemy on square trajectory.
				if (movePlusX) {
					enemies.at(i).x += enemySpeed * deltaTimeSeconds;

					if (enemies.at(i).x >= enemy.initialX + wallSize / 4.0f) {
						enemies.at(i).x = enemy.initialX + wallSize / 4.0f;
						movePlusX = false;
						movePlusY = true;
					}
				}
				else if (moveMinusX) {
					enemies.at(i).x -= enemySpeed * deltaTimeSeconds;

					if (enemies.at(i).x <= enemy.initialX - wallSize / 4.0f) {
						enemies.at(i).x = enemy.initialX - wallSize / 4.0f;
						moveMinusX = false;
						moveMinusY = true;
					}
				}
				else if (movePlusY) {
					enemies.at(i).y += enemySpeed * deltaTimeSeconds;

					if (enemies.at(i).y >= enemy.initialY + wallSize / 4.0f) {
						enemies.at(i).y = enemy.initialY + wallSize / 4.0f;
						moveMinusX = true;
						movePlusY = false;
					}
				}
				else if (moveMinusY) {
					enemies.at(i).y -= enemySpeed * deltaTimeSeconds;

					if (enemies.at(i).y <= enemy.initialY - wallSize / 4.0f) {
						enemies.at(i).y = enemy.initialY - wallSize / 4.0f;
						moveMinusY = false;
						movePlusX = true;
					}
				}
			}
		}

	}
}


// Render Maze.
void Tema2::renderMaze() {
	glm::mat4 modelMatrix;
	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < columns; ++j) {
			if (map[i][j] == ROAD) {
				modelMatrix = glm::mat4(1);
				modelMatrix = glm::translate(modelMatrix,
					glm::vec3(i * (wallSize + tolerance), 1.0f, j * (wallSize + tolerance)));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(wallSize, 2.0f, wallSize));

				// If it's a marginal Road the Texture will be Grass.
				if (i == 0 || j == 0 || i == rows - 1 || j == columns - 1) {
					RenderSimpleMesh(meshes["box"], shaders["LabShader"], modelMatrix, mapTextures["winner"]);
				}
				else {
					// Otherwise it will be Pavement.
					RenderSimpleMesh(meshes["box"], shaders["LabShader"], modelMatrix, mapTextures["pavement"]);
				}
			}
			else if (map[i][j] == WALL) {
				modelMatrix = glm::mat4(1);
				modelMatrix = glm::translate(modelMatrix, glm::vec3(i * (wallSize + tolerance), wallSize / 2,
					j * (wallSize + tolerance)));
				modelMatrix = glm::scale(modelMatrix, glm::vec3(wallSize, wallSize, wallSize));
				RenderSimpleMesh(meshes["box"], shaders["LabShader"], modelMatrix, mapTextures["wall"]);
			}
		}
	}
}


// Render Bullets.
void Tema2::renderBullets(float deltaTimeSeconds) {
	glm::mat4 modelMatrix;

	for (int i = 0; i < bullets.size(); i++) {
		bool collision = false;

		// Check Bullet - Enemy Collision.
		for (int j = 0; j < enemies.size(); ++j) {
			if (enemies.at(j).render == 0) {
				collision = intersect(glm::vec3(bullets[i].x, bullets[i].y, bullets[i].z),
					glm::vec3(enemies[j].x, wallSize / 2.5f - 2.0f, enemies[j].y), 2.0f,
					wallSize / 9.0f);
				if (collision) {
					enemies.at(j).render++;

					glm::mat4 modelMatrix = glm::mat4(1);
					modelMatrix = glm::translate(modelMatrix,
						glm::vec3(enemies[j].x, wallSize / 2.5f - 2.0f, enemies[j].y));
					modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));
					RenderSimpleMesh(meshes["sphere"], shaders["ObjectShader"], modelMatrix,
						glm::vec3(1.0f, 1.0f, 0.0f));
					break;
				}
			}

			if (!collision) {
				for (int p = 0; p < rows; ++p) {
					for (int q = 0; q < columns; ++q) {
						// Check Bullet - Road Collision.
						if (map[p][q] == ROAD) {
							collision =
								intersect(glm::vec3(bullets.at(i).x, bullets.at(i).y, bullets.at(i).z),
									glm::vec3(p * (wallSize + tolerance) - wallSize / 2,
										1.0f, q * (wallSize + tolerance) - wallSize / 2),
									glm::vec3(p * (wallSize + tolerance) + wallSize / 2,
										1.0f, q * (wallSize + tolerance) + wallSize / 2),
									2.0f);
							if (collision) {
								glm::mat4 modelMatrix = glm::mat4(1);
								modelMatrix = glm::translate(modelMatrix,
									glm::vec3(p * (wallSize + tolerance), 1.0f,
										q * (wallSize + tolerance)));
								modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));
								RenderSimpleMesh(meshes["sphere"], shaders["ObjectShader"], modelMatrix,
									glm::vec3(1.0f, 1.0f, 0.0f));
								break;
							}
						}
						// Check Bullet - Road Collision.
						else if (map[i][j] == WALL) {
							collision =
								intersect(glm::vec3(bullets.at(i).x, bullets.at(i).y, bullets.at(i).z),
									glm::vec3(p * (wallSize + tolerance) - wallSize / 2,
										0.0f, q * (wallSize + tolerance) - wallSize / 2),
									glm::vec3(p * (wallSize + tolerance) + wallSize / 2,
										wallSize, q * (wallSize + tolerance) + wallSize / 2),
									2.0f);
							if (collision) {
								glm::mat4 modelMatrix = glm::mat4(1);
								modelMatrix = glm::translate(modelMatrix,
									glm::vec3(p * (wallSize + tolerance), wallSize / 2,
										q * (wallSize + tolerance)));
								modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));
								RenderSimpleMesh(meshes["sphere"], shaders["ObjectShader"], modelMatrix,
									glm::vec3(1.0f, 1.0f, 0.0f));
								break;
							}
						}
					}

					if (collision) {
						break;
					}
				}
			}
		}

		if (collision || bullets.at(i).render == 50) {
			bullets.erase(bullets.begin() + i);
			continue;
		}
		else {
			// Draw the Bullet
			glm::mat4 modelMatrix = glm::mat4(1);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(bullets[i].x, bullets[i].y, bullets[i].z));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));
			RenderSimpleMesh(meshes["sphere"], shaders["ObjectShader"], modelMatrix, glm::vec3(1.0f, 1.0f, 0.0f));

			// Compute the Bullets's new coordinates according to it's angle and speed
			bullets[i].x += glm::vec3(bullets[i].forward * bullets[i].speed * deltaTimeSeconds).x;
			bullets[i].y += glm::vec3(bullets[i].forward * bullets[i].speed * deltaTimeSeconds).y;
			bullets[i].z += glm::vec3(bullets[i].forward * bullets[i].speed * deltaTimeSeconds).z;
			bullets.at(i).render += 1;
		}
	}
}


// Render HUD.
void Tema2::renderHUD(float deltaTimeSeconds) {
	// Health Bar.
	glm::ivec2 resolution = window->GetResolution();
	viewSpace = ViewportSpace(20, resolution.y * 19 / 20 - 20, resolution.x / 4 + 20, resolution.y / 20);
	SetViewportArea(viewSpace, glm::vec3(1.0f, 1.0f, 1.0f), true);

	float remainingHealth = hits == 0 ? resolution.x / 4 : resolution.x / 4 - hits * resolution.x / 12;
	viewSpace = ViewportSpace(20, resolution.y * 19 / 20 - 20, remainingHealth + 20, resolution.y / 20);
	SetViewportArea(viewSpace, glm::vec3(1.0f, 0.0f, 0.0f), true);

	// Time Bar.
	viewSpace = ViewportSpace(resolution.x * 3 / 4 - 20, resolution.y * 19 / 20 - 20, resolution.x / 4,
		resolution.y / 20);
	SetViewportArea(viewSpace, glm::vec3(1.0f, 1.0f, 1.0f), true);

	float step = resolution.x / (4 * timeToEscape);
	float timeBarLeft = resolution.x * 3 / 4 - 20 + Engine::GetElapsedTime() * step;
	float timeBarRight = resolution.x - timeBarLeft - 19;
	viewSpace = ViewportSpace(timeBarLeft, resolution.y * 19 / 20 - 20, timeBarRight, resolution.y / 20);
	SetViewportArea(viewSpace, glm::vec3(0.06f, 1.0f, 0.0f), true);
}


void Tema2::Update(float deltaTimeSeconds) {
	// Check if the Time is Over.
	if (Engine::GetElapsedTime() >= timeToEscape) {
		printf("Game Over! You've lost!\n");
		exit(0);
	}

	// Render the Maze.
	renderMaze();

	// Render the Bullets.
	renderBullets(deltaTimeSeconds);

	// Check if the Player has Escaped the Maze.
	if (hasEscaped() == true) {
		printf("Congratulations! You have escaped the maze!\n");
		exit(0);
	}

	// Render the Player if the Third Person Mode is On.
	if (thirdPerson) {
		renderPlayer();
	}

	// Render the Enemies.
	renderEnemies(deltaTimeSeconds);

	// Render the camera target. This is useful for understanding where
	// the rotation point is, when moving in third-person camera mode.
	if (renderCameraTarget) {
		glm::mat4 modelMatrix = glm::mat4(1);
		modelMatrix = glm::translate(modelMatrix, camera->GetTargetPosition());
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.2f));
		RenderMesh(meshes["sphere"], shaders["VertexNormal"], modelMatrix);
	}

	// Render the HUD.
	renderHUD(deltaTimeSeconds);
}


void Tema2::FrameEnd() {
	// DrawCoordinateSystem(camera->GetViewMatrix(), projectionMatrix);
}


void Tema2::RenderMesh(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix)
{
	if (!mesh || !shader || !shader->program)
		return;

	// Render an object using the specified shader and the specified position
	shader->Use();

	glUniformMatrix4fv(shader->loc_view_matrix, 1, GL_FALSE, glm::value_ptr(camera->GetViewMatrix()));
	glUniformMatrix4fv(shader->loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniformMatrix4fv(shader->loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));

	mesh->Render();
}


void Tema2::RenderSimpleMesh(Mesh* mesh, Shader* shader, const glm::mat4& modelMatrix,
	Texture2D* texture1, Texture2D* texture2)
{
	if (!mesh || !shader || !shader->GetProgramID())
		return;

	// Render an object using the specified shader and the specified position
	glUseProgram(shader->program);

	// Bind model matrix
	GLint loc_model_matrix = glGetUniformLocation(shader->program, "Model");
	glUniformMatrix4fv(loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));

	// Bind view matrix
	glm::mat4 viewMatrix = camera->GetViewMatrix();
	int loc_view_matrix = glGetUniformLocation(shader->program, "View");
	glUniformMatrix4fv(loc_view_matrix, 1, GL_FALSE, glm::value_ptr(viewMatrix));

	// Bind projection matrix
	glm::mat4 projectionnMatrix = projectionMatrix;
	int loc_projection_matrix = glGetUniformLocation(shader->program, "Projection");
	glUniformMatrix4fv(loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(projectionnMatrix));

	if (texture1)
	{
		// TODO(student): Do these:

		// - activate texture location 0
		glActiveTexture(GL_TEXTURE0);

		// - bind the texture1 ID
		glBindTexture(GL_TEXTURE_2D, texture1->GetTextureID());

		// - send theuniform value
		glUniform1i(glGetUniformLocation(shader->program, "texture_1"), 0);
	}

	if (texture2)
	{
		// TODO(student): Do these:
		// - activate texture location 1
		glActiveTexture(GL_TEXTURE1);

		// - bind the texture2 ID
		glBindTexture(GL_TEXTURE_2D, texture2->GetTextureID());

		// - send the uniform value
		glUniform1i(glGetUniformLocation(shader->program, "texture_2"), 1);
	}

	// Draw the object
	glBindVertexArray(mesh->GetBuffers()->m_VAO);
	glDrawElements(mesh->GetDrawMode(), static_cast<int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
}


void Tema2::RenderSimpleMesh(Mesh* mesh, Shader* shader,
	const glm::mat4& modelMatrix, const glm::vec3& color, bool die)
{
	if (!mesh || !shader || !shader->GetProgramID())
		return;

	// Render an object using the specified shader and the specified position
	glUseProgram(shader->program);

	// Set shader uniforms for light & material properties
	GLint loc_object_color = glGetUniformLocation(shader->program, "object_color");
	glUniform3fv(loc_object_color, 1, glm::value_ptr(color));

	// Bind model matrix
	GLint loc_model_matrix = glGetUniformLocation(shader->program, "Model");
	glUniformMatrix4fv(loc_model_matrix, 1, GL_FALSE, glm::value_ptr(modelMatrix));

	// Bind view matrix
	glm::mat4 viewMatrix = camera->GetViewMatrix();
	int loc_view_matrix = glGetUniformLocation(shader->program, "View");
	glUniformMatrix4fv(loc_view_matrix, 1, GL_FALSE, glm::value_ptr(viewMatrix));

	// Bind projection matrix
	glm::mat4 projectionnMatrix = projectionMatrix;
	int loc_projection_matrix = glGetUniformLocation(shader->program, "Projection");
	glUniformMatrix4fv(loc_projection_matrix, 1, GL_FALSE, glm::value_ptr(projectionnMatrix));

	// TODO(student): Set any other shader uniforms that you need
	int loc_time = glGetUniformLocation(shader->program, "time");
	if (mesh == meshes["sphere"] && die == true) {
		glUniform1f(loc_time, (float)Engine::GetElapsedTime());
	}
	else {
		glUniform1f(loc_time, -1.0f);
	}

	// Draw the object
	glBindVertexArray(mesh->GetBuffers()->m_VAO);
	glDrawElements(mesh->GetDrawMode(), static_cast<int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
}


// Player - Wall Collision
bool Tema2::checkPlayerWallCollision() {
	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < columns; ++j) {
			if (map[i][j] == WALL) {
				// Player - Wall Collision.
				if (camera->GetTargetPosition().x < i * (wallSize + tolerance) + wallSize / 2 + 4.0f &&
					camera->GetTargetPosition().x > i * (wallSize + tolerance) - wallSize / 2 - 4.0f &&
					camera->GetTargetPosition().z > j * (wallSize + tolerance) - wallSize / 2 - 4.0f &&
					camera->GetTargetPosition().z < j * (wallSize + tolerance) + wallSize / 2 + 4.0f) {
					return true;
				}

				// Camera - Wall Collision (in order not to look through the Walls).
				if (!thirdPerson) {
					if (camera->position.x < i * (wallSize + tolerance) + wallSize / 2 + 4.0f &&
						camera->position.x > i * (wallSize + tolerance) - wallSize / 2 - 4.0f &&
						camera->position.z > j * (wallSize + tolerance) - wallSize / 2 - 4.0f &&
						camera->position.z < j * (wallSize + tolerance) + wallSize / 2 + 4.0f) {
						return true;
					}
				}
				else {
					if (camera->position.x < i * (wallSize + tolerance) + wallSize / 2 &&
						camera->position.x > i * (wallSize + tolerance) - wallSize / 2 &&
						camera->position.z > j * (wallSize + tolerance) - wallSize / 2 &&
						camera->position.z < j * (wallSize + tolerance) + wallSize / 2) {
						return true;
					}
				}
			}
		}
	}
	return false;
}


/*
 *  These are callback functions. To find more about callbacks and
 *  how they behave, see `input_controller.h`.
 */


void Tema2::OnInputUpdate(float deltaTime, int mods) {
	float cameraSpeed = 10.0f;

	if (window->KeyHold(GLFW_KEY_W)) {
		// TODO(student): Translate the camera forward
		camera->MoveForward(cameraSpeed * deltaTime);

		if (checkPlayerWallCollision()) {
			camera->MoveForward(-cameraSpeed * deltaTime);
		}
	}

	if (window->KeyHold(GLFW_KEY_A)) {
		// TODO(student): Translate the camera to the left
		camera->TranslateRight(-cameraSpeed * deltaTime);

		if (checkPlayerWallCollision()) {
			camera->TranslateRight(cameraSpeed * deltaTime);
		}
	}

	if (window->KeyHold(GLFW_KEY_S)) {
		// TODO(student): Translate the camera backward
		camera->MoveForward(-cameraSpeed * deltaTime);

		if (checkPlayerWallCollision()) {
			camera->MoveForward(cameraSpeed * deltaTime);
		}
	}

	if (window->KeyHold(GLFW_KEY_D)) {
		// TODO(student): Translate the camera to the right
		camera->TranslateRight(cameraSpeed * deltaTime);

		if (checkPlayerWallCollision()) {
			camera->TranslateRight(-cameraSpeed * deltaTime);
		}
	}
}


void Tema2::OnKeyPress(int key, int mods) {
	// Change View Mode: First Person or Third Person.
	if (key == GLFW_KEY_LEFT_CONTROL) {
		thirdPerson = !thirdPerson;
		renderCameraTarget = !renderCameraTarget;

		if (thirdPerson) {
			camera->MoveForward(-5.0f);
		}
		else {
			camera->MoveForward(5.0f);
		}
	}

}


void Tema2::OnKeyRelease(int key, int mods) {
	// Add key release event
}


void Tema2::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) {
	// Add mouse move event

	float sensivityOX = 0.007f;
	float sensivityOY = 0.007f;

	// Angle for rotating the Player in the direction the Camera is Facing.
	rotateAngle -= deltaX * 0.4f;

	// TODO(student): Rotate the camera around OX and OY using `deltaX` and 
	// `deltaY`. Use the sensitivity variables for setting up the rotation speed.
	camera->RotateThirdPerson_OX(-sensivityOX * deltaY);
	if (checkPlayerWallCollision()) {
		camera->RotateThirdPerson_OX(sensivityOX * deltaY);
	}

	camera->RotateThirdPerson_OY(-sensivityOY * deltaX);
	if (checkPlayerWallCollision()) {
		camera->RotateThirdPerson_OY(sensivityOY * deltaX);
		rotateAngle += deltaX * 0.4f;
	}
}


void Tema2::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) {
	// Add mouse button press event

	// Check if the Player wants to Lauch a Bullet and the minimum Time between 
	// two Bullet launches has paased.
	if (window->MouseHold(GLFW_MOUSE_BUTTON_LEFT) && !thirdPerson &&
		Engine::GetElapsedTime() - bulletLaunchingTime >= interval) {
		Bullet bullet;

		// The Bullet's info
		bullet.speed = 150.0f;
		bullet.x = camera->GetTargetPosition().x - 0.2f;
		bullet.y = camera->GetTargetPosition().y - 0.2f;
		bullet.z = camera->GetTargetPosition().z - 0.2f;
		bullet.forward = camera->forward;

		// Add the Bullet to the Bullet Vector and Update the Last Time a Bullet has 
		// been launched.
		bullets.push_back(bullet);
		bulletLaunchingTime = Engine::GetElapsedTime();
	}
}


void Tema2::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) {
	// Add mouse button release event
}


void Tema2::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) {
}


void Tema2::OnWindowResize(int width, int height) {
}
