// ============================================================================
// INCLUDES & DRIVER EXPORTS
// ============================================================================
#include "gameClients.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem.h>
#include <shader.h>
#include <camera.h>
#include <animator.h>
#include <model.h>
#include <gbuffer.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <future>
#include <chrono>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
extern "C" {
    _declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

// ============================================================================
// CONSTANTS & ENUMS
// ============================================================================
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

const float WORLD_SIZE_X = 300.0f;
const float WORLD_SIZE_Z = 300.0f;
const float HALF_SIZE_X = WORLD_SIZE_X / 2.0f;
const float HALF_SIZE_Z = WORLD_SIZE_Z / 2.0f;
const float project_max = 100.0f;

const unsigned int TREE_AMOUNT = 200;
const unsigned int GRASS_AMOUNT = 1000000;
const unsigned int MUTANT_AMOUNT = 35;

const int GRID_SIZE = 60;
const float CELL_SIZE = WORLD_SIZE_X / GRID_SIZE;

const float PLAYER_SIZE = 1.5f;
const float ENEMY_SIZE = 3.5f;

enum PlayerState { IDLE, RUNNING, ATTACK, SKILL, ULTI };
enum mutantState { MUTANT_IDLE, MUTANT_RUNNING, MUTANT_ATTACK, MUTANT_DEAD, MUTANT_SKILL, MUTANT_ULTI };

// ============================================================================
// STRUCTS & DATA TYPES
// ============================================================================
struct AABB2D {
    glm::vec2 minBound;
    glm::vec2 maxBound;
};

struct PathNode {
    int x, z;
    int g, h, f;
    PathNode* parent;
};

struct ComparePathNodes {
    bool operator()(PathNode* a, PathNode* b) { return a->f > b->f; }
};

struct TerrainMesh {
    unsigned int VAO, VBO, EBO;
    size_t indexCount;
};

// Enemy Entity
struct Mutant {
    int hp = 60;
    int maxHp = 60;
    int mutantAtk = 15;
    bool isDead = false;
    float deathTimer = 0.0f;
    bool shouldRemove = false;
    float attackCooldown = 0.0f;

    glm::vec3 position{0.0f};
    float yaw = 0.0f;
    float speed = 4.0f;
    mutantState state = MUTANT_IDLE;
    float stateTimer = 0.0f;

    std::vector<glm::vec3> path;
    int currentPathNode = 0;
    glm::vec3 targetDestination{0.0f};
    float pathRecalculateTimer = 0.0f;

    Animator idleAnimator;
    Animator runAnimator;
    Animator attackAnimator;
    Animator dyingAnimator;
    Animator* currentAnimator = nullptr;

    Mutant(Animation* idle, Animation* run, Animation* mutantAtk, Animation* die)
        : idleAnimator(idle), runAnimator(run), attackAnimator(mutantAtk), dyingAnimator(die) {
        currentAnimator = &idleAnimator;
    }

    void ApplyDamage(int dmg) {
        if (isDead) return;
        hp -= dmg;
        if (hp <= 0) {
            hp = 0;
            isDead = true;
            state = MUTANT_DEAD;
        }
    }

    void Update(float dt, const glm::vec3& targetPlayerPos);
    void FollowPath(float dt, const glm::vec3& targetPlayerPos);

    glm::mat4 GetModelMatrix(float scaleSize) const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        return glm::scale(model, glm::vec3(scaleSize));
    }
};

// ============================================================================
// GLOBAL STATE
// ============================================================================
Camera camera(glm::vec3(0.0f, 3.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

static std::string activeUsername = "";
static float playerYaw = 0.0f;
glm::vec3 playerPos = glm::vec3(0.0f);
float playerSpeed = 8.0f;
int playerHp = 100;
int playerMaxHp = 100;
int playerAtk = 15;
int playerScore = 0;

PlayerState currentState = IDLE;
std::vector<AABB2D> treeColliders;
bool obstacleGrid[GRID_SIZE][GRID_SIZE] = { false };

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
GLFWwindow* InitWindow();
TerrainMesh CreateTerrainMesh(int width, int height);
void SetupInstancedMatrixAttributes(const std::vector<Mesh>& meshes, unsigned int buffer);
void BindMeshTextures(const Mesh& mesh, const Shader& shader);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

bool CheckTreeCollision(const glm::vec3& newPos, float entityRadius);
glm::ivec2 WorldToGrid(const glm::vec3& worldPos);
glm::vec3 GridToWorld(int gx, int gz);
std::vector<glm::vec3> FindAStarPath(glm::vec3 startPos, glm::vec3 targetPos);

void SaveAndLogout(bool& isLoggedIn, const std::string& username, const glm::vec3& pos, float yaw, int hp, int score);
void RenderAuthInterface(bool& isLoggedIn, glm::vec3& outPlayerPos, float& outPlayerYaw, int& outPlayerHp, int& outPlayerScore);

// ============================================================================
// MAIN APPLICATION ENTRY POINT
// ============================================================================
int main()
{
    // 1. Initialize Subsystems (Winsock, GLFW, GLAD)
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "Failed to initialize Winsock." << std::endl;
            return -1;
        }
    #endif

    GLFWwindow* window = InitWindow();
    if (!window) return -1;

    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_FRAMEBUFFER_SRGB);


    // 2. Load Shaders & Assets
    Shader treeShader("shaders/treeShader.vert", "shaders/treeShader.frag");
    Shader grassShader("shaders/grassShader.vert", "shaders/grassShader.frag");
    Shader terrainShader("shaders/terrainShader.vert", "shaders/terrainShader.frag");
    Shader playerShader("shaders/playerShader.vert", "shaders/playerShader.frag");

    GLint boneMatrixLoc = glGetUniformLocation(playerShader.ID, "finalBoneMatrices[0]");

    Shader lightingShader("shaders/lightingShader.vert", "shaders/lightingShader.frag");

    // Screen Quad VAO/VBO for lighting pass
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    Model tree("assets/realistic_tree.glb", ENV);
    Model grass("assets/grass.glb", ENV);

    // Player Models & Animations
    Model idleplayer("assets/Player/idle/idle.dae", CHARR);
    Animation idleAnimation("assets/Player/idle/idle.dae", &idleplayer);
    Animator idleAnimator(&idleAnimation);

    Model runPlayer("assets/Player/run/run.dae", CHARR);
    Animation runAnimation("assets/Player/run/run.dae", &runPlayer);
    Animator runAnimator(&runAnimation);

    Model stopPlayer("assets/Player/run_to_stop/run_to_stop.dae", CHARR);
    Animation stopAnimation("assets/Player/run_to_stop/run_to_stop.dae", &stopPlayer);
    Animator stopAnimator(&stopAnimation);

    Model player_basic_atk("assets/Player/basic_atk/basic_atk.dae", CHARR);
    Animation basic_atkAnimation("assets/Player/basic_atk/basic_atk.dae", &player_basic_atk);
    Animator basic_atkAnimator(&basic_atkAnimation);

    Model player_skill("assets/Player/skill/skill.dae", CHARR);
    Animation skillAnimation("assets/Player/skill/skill.dae", &player_skill);
    Animator skillAnimator(&skillAnimation);

    Model player_ulti("assets/Player/ulti/ulti.dae", CHARR);
    Animation ultiAnimation("assets/Player/ulti/ulti.dae", &player_ulti);
    Animator ultiAnimator(&ultiAnimation);

    // Enemy Models & Animations
    Model idleMutant("assets/Mutant/mutantIdle/mutantIdle.dae", CHARR);
    Animation idleMutantAnimation("assets/Mutant/mutantIdle/mutantIdle.dae", &idleMutant);
    Animator idleMutantAnimator(&idleMutantAnimation);

    Model runMutant("assets/Mutant/mutantRun/mutantRun.dae", CHARR);
    Animation runMutantAnimation("assets/Mutant/mutantRun/mutantRun.dae", &runMutant);
    Animator runMutantAnimator(&runMutantAnimation);

    Model mutantAttack("assets/Mutant/mutantAttack/mutantAttack.dae", CHARR);
    Animation mutantAttackAnimation("assets/Mutant/mutantAttack/mutantAttack.dae", &mutantAttack);
    Animator mutantAttackAnimator(&mutantAttackAnimation);

    Model mutantDying("assets/Mutant/mutantDying/mutantDying.dae", CHARR);
    Animation mutantDyingAnimation("assets/Mutant/mutantDying/mutantDying.dae", &mutantDying);
    Animator mutantDyingAnimator(&mutantDyingAnimation);

    // 3. Initialize Game World & Dynamic Instances
    std::vector<Mutant> mutants;
    mutants.reserve(MUTANT_AMOUNT);
    for (unsigned int i = 0; i < MUTANT_AMOUNT; i++) {
        Mutant m(&idleMutantAnimation, &runMutantAnimation, &mutantAttackAnimation, &mutantDyingAnimation);
        float randX = (static_cast<float>(rand()) / RAND_MAX * WORLD_SIZE_X) - HALF_SIZE_X;
        float randZ = (static_cast<float>(rand()) / RAND_MAX * WORLD_SIZE_Z) - HALF_SIZE_Z;
        m.position = glm::vec3(randX, 0.0f, randZ);
        m.yaw = glm::radians(static_cast<float>(rand() % 360));
        mutants.push_back(m);
    }

    // Instanced Trees Setup
    std::vector<glm::mat4> treeMatrices(TREE_AMOUNT);
    srand(static_cast<unsigned int>(glfwGetTime()));
    for (unsigned int i = 0; i < TREE_AMOUNT; i++) {
        glm::mat4 model = glm::mat4(1.0f);
        float x = (static_cast<float>(rand()) / RAND_MAX * WORLD_SIZE_X) - HALF_SIZE_X;
        float z = (static_cast<float>(rand()) / RAND_MAX * WORLD_SIZE_Z) - HALF_SIZE_Z;
        model = glm::translate(model, glm::vec3(x, -0.5f, z));
        float scale = 0.01f + (static_cast<float>(rand()) / RAND_MAX) * 0.01f;
        model = glm::scale(model, glm::vec3(scale));
        model = glm::rotate(model, glm::radians(static_cast<float>(rand() % 360)), glm::vec3(0.0f, 1.0f, 0.0f));
        treeMatrices[i] = model;
    }

    unsigned int treeBuffer;
    glGenBuffers(1, &treeBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, treeBuffer);
    glBufferData(GL_ARRAY_BUFFER, TREE_AMOUNT * sizeof(glm::mat4), treeMatrices.data(), GL_STATIC_DRAW);
    SetupInstancedMatrixAttributes(tree.meshes, treeBuffer);

    // Tree Colliders & Obstacle Grid Setup
    float treeTrunkRadius = 1.2f;
    for (const auto& mat : treeMatrices) {
        glm::vec3 pos = glm::vec3(mat[3]);
        treeColliders.push_back({ glm::vec2(pos.x - treeTrunkRadius, pos.z - treeTrunkRadius),
                                  glm::vec2(pos.x + treeTrunkRadius, pos.z + treeTrunkRadius) });
    }

    for (int x = 0; x < GRID_SIZE; x++) {
        for (int z = 0; z < GRID_SIZE; z++) {
            glm::vec3 cellPos((x * CELL_SIZE) - HALF_SIZE_X + (CELL_SIZE * 0.5f), 0.0f, (z * CELL_SIZE) - HALF_SIZE_Z + (CELL_SIZE * 0.5f));
            if (CheckTreeCollision(cellPos, CELL_SIZE * 0.5f)) obstacleGrid[x][z] = true;
        }
    }

    // Instanced Grass Setup
    std::vector<glm::mat4> grassMatrices(GRASS_AMOUNT);
    for (unsigned int i = 0; i < GRASS_AMOUNT; i++) {
        glm::mat4 model = glm::mat4(1.0f);
        float x = (static_cast<float>(rand()) / RAND_MAX * WORLD_SIZE_X) - HALF_SIZE_X;
        float z = (static_cast<float>(rand()) / RAND_MAX * WORLD_SIZE_Z) - HALF_SIZE_Z;
        float density = sin(x * 0.02f) * cos(z * 0.02f) + sin(x * 0.05f + z * 0.05f) * 0.5f;
        if (density < 0.1f && (static_cast<float>(rand()) / RAND_MAX) > 0.3f) {
            x += sin(x) * 5.0f; z += cos(z) * 5.0f;
        }
        model = glm::translate(model, glm::vec3(x, 0.3f, z));
        float scale = 0.01f + (static_cast<float>(rand()) / RAND_MAX) * 0.01f;
        model = glm::scale(model, glm::vec3(scale));
        model = glm::rotate(model, glm::radians(static_cast<float>(rand() % 360)), glm::vec3(0.0f, 1.0f, 0.0f));
        grassMatrices[i] = model;
    }

    unsigned int grassBuffer;
    glGenBuffers(1, &grassBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, grassBuffer);
    glBufferData(GL_ARRAY_BUFFER, GRASS_AMOUNT * sizeof(glm::mat4), grassMatrices.data(), GL_DYNAMIC_DRAW);
    SetupInstancedMatrixAttributes(grass.meshes, grassBuffer);

    // Terrain Initialization
    const int TERRAIN_WIDTH = 6, TERRAIN_HEIGHT = 6;
    TerrainMesh terrain = CreateTerrainMesh(TERRAIN_WIDTH, TERRAIN_HEIGHT);
    unsigned int grassTexture = TextureFromFile("grass.png", "assets/textures");
    unsigned int sandTexture  = TextureFromFile("sand.png", "assets/textures");

    terrainShader.use();
    terrainShader.setInt("grassTexture", 0);
    terrainShader.setInt("sandTexture", 1);

    // --- G-BUFFER INITIALIZATION ---
    GBuffer gBuffer;
    gBuffer.Init(SCR_WIDTH, SCR_HEIGHT);
    // ------------------------------------------

    // 4. GUI Setup
    bool isLoggedIn = false;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    auto start = std::chrono::high_resolution_clock::now();
    int totalFramesCounter = 0;

    // 5. Main Render Loop
    while (!glfwWindowShouldClose(window))
    {
        totalFramesCounter++;
        float currentFrame = static_cast<float>(glfwGetTime());

        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input & Cursor Management
        if (!isLoggedIn || playerHp <= 0 || glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        glClearColor(0.02f, 0.02f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (!isLoggedIn) {
            RenderAuthInterface(isLoggedIn, playerPos, playerYaw, playerHp, playerScore);
        } else {
            processInput(window);

            // Background Heartbeat Autosave
            static float heartbeatTimer = 0.0f;
            static std::future<void> heartbeatSaveTask;
            heartbeatTimer += deltaTime;
            if (heartbeatTimer >= 1.0f && playerHp > 0) {
                heartbeatTimer = 0.0f;
                if (!heartbeatSaveTask.valid() || heartbeatSaveTask.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    std::string userCopy = activeUsername; glm::vec3 posCopy = playerPos;
                    float yawCopy = playerYaw; int hpCopy = playerHp, scoreCopy = playerScore;
                    heartbeatSaveTask = std::async(std::launch::async, [userCopy, posCopy, yawCopy, hpCopy, scoreCopy]() {
                        saveGameToServer(userCopy, posCopy.x, posCopy.y, posCopy.z, yawCopy, hpCopy, scoreCopy);
                    });
                }
            }

            // Animation Timers & State Flow
            static float actionTimer = 0.0f;
            static PlayerState previousState = IDLE;
            static bool damageDealtThisAction = false;

            if (currentState != previousState) {
                actionTimer = 0.0f;
                if (currentState == ATTACK)     basic_atkAnimator.Reset();
                else if (currentState == SKILL) skillAnimator.Reset();
                else if (currentState == ULTI)  ultiAnimator.Reset();
                previousState = currentState;
            }

            if (currentState == ATTACK || currentState == SKILL || currentState == ULTI) {
                actionTimer += deltaTime;
                float targetDuration = (currentState == ATTACK) ? basic_atkAnimator.GetDurationInSeconds() :
                                       (currentState == SKILL)  ? skillAnimator.GetDurationInSeconds() : ultiAnimator.GetDurationInSeconds();
                if (actionTimer >= targetDuration) {
                    actionTimer = 0.0f; currentState = IDLE; previousState = IDLE; damageDealtThisAction = false;
                }
            }

            // Update only the active player animator
            Animator* activePlayerAnimator = (currentState == RUNNING) ? &runAnimator :
                                            (currentState == ATTACK)  ? &basic_atkAnimator :
                                            (currentState == SKILL)   ? &skillAnimator :
                                            (currentState == ULTI)    ? &ultiAnimator : &idleAnimator;
            activePlayerAnimator->UpdateAnimation(deltaTime);

            // Player HP Regeneration
            if (playerHp > 0) {
                static float regenTimer = 0.0f;
                regenTimer += deltaTime;
                if (regenTimer >= 3.0f) {
                    if (playerHp < playerMaxHp) playerHp = glm::min(playerMaxHp, playerHp + 5);
                    regenTimer = 0.0f;
                }
            }

            // Combat & Damage Resolution
            if ((currentState == ATTACK || currentState == SKILL || currentState == ULTI) && !damageDealtThisAction) {
                int mult = (currentState == SKILL) ? 2 : (currentState == ULTI) ? 4 : 1;
                for (auto& m : mutants) {
                    if (!m.isDead && glm::distance(playerPos, m.position) <= 5.0f) {
                        m.ApplyDamage(playerAtk * mult);
                        if (m.isDead) playerScore += 100;
                    }
                }
                damageDealtThisAction = true;
            }

            for (auto& m : mutants) {
                if (m.state == MUTANT_ATTACK && !m.isDead) {
                    m.attackCooldown += deltaTime;
                    if (m.attackCooldown >= 1.0f) {
                        playerHp = glm::max(0, playerHp - m.mutantAtk);
                        m.attackCooldown = 0.0f;
                    }
                }
            }

            // Game UI Overlay
            if (playerHp <= 0) {
                ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                ImGui::Begin("Game Over", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "YOU DIED"); ImGui::Separator();
                ImGui::Text("Score : %d", playerScore); ImGui::Spacing();
                if (ImGui::Button("Restart", ImVec2(120, 0))) {
                    playerHp = playerMaxHp; playerScore = 0; playerPos = glm::vec3(0.0f); currentState = IDLE;
                }
                ImGui::SameLine();
                if (ImGui::Button("Log Out", ImVec2(120, 0))) {
                    SaveAndLogout(isLoggedIn, activeUsername, playerPos, playerYaw, playerHp, playerScore);
                    playerHp = playerMaxHp; playerScore = 0; playerPos = glm::vec3(0.0f); currentState = IDLE;
                }
                ImGui::End();
            } else {
                ImGui::Begin("Player Status", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
                ImGui::Text("Health: %d / %d", playerHp, playerMaxHp); ImGui::ProgressBar((float)playerHp / (float)playerMaxHp);
                ImGui::Text("Score: %d", playerScore); ImGui::Separator();
                ImGui::Text("Engaged Enemies");
                int engagedCount = 0;
                for (size_t i = 0; i < mutants.size(); i++) {
                    if (!mutants[i].isDead && glm::distance(playerPos, mutants[i].position) <= 15.0f) {
                        engagedCount++; ImGui::PushID(static_cast<int>(i));
                        ImGui::Text("Mutant #%d HP: %d / %d", engagedCount, mutants[i].hp, mutants[i].maxHp);
                        ImGui::ProgressBar((float)mutants[i].hp / (float)mutants[i].maxHp); ImGui::PopID();
                    }
                }
                if (engagedCount == 0) ImGui::TextDisabled("No enemies nearby");
                ImGui::Separator();
                if (ImGui::Button("Log Out & Save")) SaveAndLogout(isLoggedIn, activeUsername, playerPos, playerYaw, playerHp, playerScore);
                ImGui::End();
            }

            // 3D Scene Rendering
            glm::vec3 sunDirection = glm::normalize(glm::vec3(-0.2f, -1.0f, -0.3f));


            // Update enemy state and pathfinding in the logic phase
            mutants.erase(std::remove_if(mutants.begin(), mutants.end(), [](const Mutant& m) { return m.shouldRemove; }), mutants.end());
            for (auto& mutant : mutants) {
                mutant.Update(deltaTime, playerPos);
            }

            // ==========================================
            // --- 1. GEOMETRY PASS ---
            // ==========================================
            gBuffer.BindForWriting();
            glDisable(GL_BLEND); // Ensure blending is OFF for G-Buffer pass
            glEnable(GL_DEPTH_TEST);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear G-Buffer with zero
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            camera.Position = playerPos - (camera.Front * 5.0f) + glm::vec3(0.0f, 2.0f, 0.0f);
            if (camera.Position.y < 0.5f) camera.Position.y = 0.5f;

            glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, project_max);
            glm::mat4 view = camera.GetViewMatrix();

            // Render Terrain
            terrainShader.use();
            terrainShader.setMat4("projection", projection); terrainShader.setMat4("view", view);
            glm::mat4 terrainModelMat = glm::translate(glm::mat4(1.0f), glm::vec3(-HALF_SIZE_X, 0.0f, -HALF_SIZE_Z));
            terrainModelMat = glm::scale(terrainModelMat, glm::vec3(WORLD_SIZE_X / (TERRAIN_WIDTH - 1), 1.0f, WORLD_SIZE_Z / (TERRAIN_HEIGHT - 1)));
            terrainShader.setMat4("model", terrainModelMat);

            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, grassTexture);
            glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, sandTexture);
            glBindVertexArray(terrain.VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(terrain.indexCount), GL_UNSIGNED_INT, 0);

            // Render Player
            playerShader.use();
            playerShader.setMat4("projection", projection); playerShader.setMat4("view", view);

            Model* activeModel = (currentState == RUNNING) ? &runPlayer : (currentState == ATTACK) ? &player_basic_atk :
                                 (currentState == SKILL)   ? &player_skill : (currentState == ULTI) ? &player_ulti : &idleplayer;
            Animator* activeAnimator = (currentState == RUNNING) ? &runAnimator : (currentState == ATTACK) ? &basic_atkAnimator :
                                       (currentState == SKILL)   ? &skillAnimator : (currentState == ULTI) ? &ultiAnimator : &idleAnimator;

            auto transforms = activeAnimator->GetFinalBoneMatrices();
            glUniformMatrix4fv(boneMatrixLoc, static_cast<GLsizei>(transforms.size()), GL_FALSE, glm::value_ptr(transforms[0]));

            glm::mat4 playerModelMat = glm::translate(glm::mat4(1.0f), playerPos);
            playerModelMat = glm::rotate(playerModelMat, playerYaw, glm::vec3(0.0f, 1.0f, 0.0f));
            playerShader.setMat4("model", glm::scale(playerModelMat, glm::vec3(PLAYER_SIZE)));
            if (playerHp > 0) activeModel->Draw(playerShader);

            // Render Enemy Mutants (Draw pass only)
            for (auto& mutant : mutants) {
                auto enemyTransforms = mutant.currentAnimator->GetFinalBoneMatrices();
                glUniformMatrix4fv(boneMatrixLoc, static_cast<GLsizei>(enemyTransforms.size()), GL_FALSE, glm::value_ptr(enemyTransforms[0]));
                playerShader.setMat4("model", mutant.GetModelMatrix(ENEMY_SIZE));

                if (mutant.state == MUTANT_RUNNING)     runMutant.Draw(playerShader);
                else if (mutant.state == MUTANT_ATTACK) mutantAttack.Draw(playerShader);
                else if (mutant.state == MUTANT_DEAD)   mutantDying.Draw(playerShader);
                else                                    idleMutant.Draw(playerShader);
            }

            // Render Trees
            glDisable(GL_CULL_FACE);
            treeShader.use();
            treeShader.setMat4("projection", projection); treeShader.setMat4("view", view);

            for (auto& mesh : tree.meshes) {
                BindMeshTextures(mesh, treeShader);
                glBindVertexArray(mesh.VAO);
                glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, 0, TREE_AMOUNT);
            }

            // Distance Cull Grass Instances
            static glm::vec3 lastCullPos(9999.0f);
            static std::vector<glm::mat4> visibleGrass;

            // Only recalculate if player moved more than 0.5 units
            if (glm::distance2(playerPos, lastCullPos) > 0.25f) {
                lastCullPos = playerPos;
                visibleGrass.clear();
                const float maxDistSq = 75.0f * 75.0f;

                for (const auto& mat : grassMatrices) {
                    glm::vec3 diff = playerPos - glm::vec3(mat[3]);
                    if (glm::dot(diff, diff) <= maxDistSq) {
                        visibleGrass.push_back(mat);
                    }
                }

                glBindBuffer(GL_ARRAY_BUFFER, grassBuffer);
                glBufferData(GL_ARRAY_BUFFER, GRASS_AMOUNT * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
                glBufferSubData(GL_ARRAY_BUFFER, 0, visibleGrass.size() * sizeof(glm::mat4), visibleGrass.data());
            }

            // Render Grass into G-Buffer
            glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            grassShader.use();
            grassShader.setMat4("projection", projection); grassShader.setMat4("view", view);
            grassShader.setFloat("time", currentFrame);

            for (auto& mesh : grass.meshes) {
                BindMeshTextures(mesh, grassShader);
                glBindVertexArray(mesh.VAO);
                glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(visibleGrass.size()));
            }

            glEnable(GL_CULL_FACE);

            // Unbind G-Buffer back to default window framebuffer
            GBuffer::Unbind();

            // ==========================================
            // --- 2. LIGHTING PASS ---
            // ==========================================
            glClearColor(0.02f, 0.02f, 0.08f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            lightingShader.use();
            gBuffer.BindTexturesForReading(0);
            lightingShader.setInt("gPosition", 0);
            lightingShader.setInt("gNormal", 1);
            lightingShader.setInt("gAlbedoSpec", 2);
            lightingShader.setVec3("lightDir", sunDirection);
            lightingShader.setVec3("viewPos", camera.Position);

            // Render Screen-space Quad
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);

            // ==========================================
            // --- 3. BLIT DEPTH FOR FORWARD/UI PASSES ---
            // ==========================================
            glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.fbo);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

        }
        // ==========================================
        // --- 5. UI PASS ---
        // ==========================================
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
    float totalSeconds = static_cast<float>(duration) / 1000.0f;
    std::cout << "Average Frame Performance: " << (static_cast<float>(totalFramesCounter) / totalSeconds) << " FPS" << std::endl;

    #ifdef _WIN32
        WSACleanup();
    #endif
    glfwTerminate();
    return 0;
}

// ============================================================================
// INITIALIZATION HELPERS
// ============================================================================
GLFWwindow* InitWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "V_IT_PROJECT(2026)", monitor, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return nullptr;
    }
    return window;
}

void SetupInstancedMatrixAttributes(const std::vector<Mesh>& meshes, unsigned int buffer) {
    for (const auto& mesh : meshes) {
        glBindVertexArray(mesh.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        for (unsigned int i = 0; i < 4; i++) {
            glEnableVertexAttribArray(7 + i);
            glVertexAttribPointer(7 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(glm::vec4)));
            glVertexAttribDivisor(7 + i, 1);
        }
        glBindVertexArray(0);
    }
}

void BindMeshTextures(const Mesh& mesh, const Shader& shader) {
    unsigned int diffuseNr = 1, specularNr = 1;
    for (unsigned int j = 0; j < mesh.textures.size(); j++) {
        glActiveTexture(GL_TEXTURE0 + j);
        std::string name = mesh.textures[j].type;
        std::string number = (name == "texture_diffuse") ? std::to_string(diffuseNr++) : std::to_string(specularNr++);
        shader.setInt((name + number).c_str(), j);
        glBindTexture(GL_TEXTURE_2D, mesh.textures[j].id);
    }
}

TerrainMesh CreateTerrainMesh(int width, int height) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            vertices.push_back((float)x); vertices.push_back(0.0f); vertices.push_back((float)z);
            vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
            vertices.push_back(((float)x / width) * 20.0f); vertices.push_back(((float)z / height) * 20.0f);
        }
    }

    for (int z = 0; z < height - 1; z++) {
        for (int x = 0; x < width - 1; x++) {
            int start = z * width + x;
            indices.push_back(start); indices.push_back(start + width); indices.push_back(start + 1);
            indices.push_back(start + 1); indices.push_back(start + width); indices.push_back(start + width + 1);
        }
    }

    TerrainMesh mesh;
    mesh.indexCount = indices.size();
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    return mesh;
}
// ============================================================================
// GAME LOGIC & PATHFINDING
// ============================================================================
void Mutant::Update(float dt, const glm::vec3& targetPlayerPos) {
    if (isDead) {
        currentAnimator = &dyingAnimator;
        if (currentAnimator) currentAnimator->UpdateAnimation(dt);
        deathTimer += dt;
        if (deathTimer >= 2.5f) shouldRemove = true;
        return;
    }

    float distToPlayer = glm::distance(position, targetPlayerPos);
    switch (state) {
        case MUTANT_IDLE:
            currentAnimator = &idleAnimator;
            if (distToPlayer < 15.0f) state = MUTANT_RUNNING;
            break;
        case MUTANT_RUNNING:
            currentAnimator = &runAnimator;
            pathRecalculateTimer += dt;
            if (pathRecalculateTimer >= 0.5f) {
                path = FindAStarPath(position, targetPlayerPos);
                pathRecalculateTimer = 0.0f;
            }
            FollowPath(dt, targetPlayerPos);
            if (distToPlayer <= 2.0f) state = MUTANT_ATTACK;
            else if (distToPlayer > 20.0f) state = MUTANT_IDLE;
            break;
        case MUTANT_ATTACK:
            currentAnimator = &attackAnimator;
            stateTimer += dt;
            if (stateTimer >= attackAnimator.GetDurationInSeconds()) {
                stateTimer = 0.0f; state = MUTANT_RUNNING;
            }
            break;
        case MUTANT_DEAD:
        case MUTANT_ULTI:
            currentAnimator = &dyingAnimator;
            break;
    }
    if (currentAnimator) currentAnimator->UpdateAnimation(dt);
}

void Mutant::FollowPath(float dt, const glm::vec3& targetPlayerPos) {
    glm::vec3 target = (!path.empty() && path.size() > 1) ? path[path.size() - 2] : targetPlayerPos;
    glm::vec3 dir = target - position;
    dir.y = 0.0f;

    if (glm::length(dir) > 0.1f) {
        dir = glm::normalize(dir);
        glm::vec3 nextPos = position + dir * speed * dt;
        if (!CheckTreeCollision(nextPos, 1.0f)) position = nextPos;
        yaw = glm::atan(dir.x, dir.z);
    }
}

bool CheckTreeCollision(const glm::vec3& newPos, float entityRadius) {
    for (const auto& box : treeColliders) {
        float closestX = glm::clamp(newPos.x, box.minBound.x, box.maxBound.x);
        float closestZ = glm::clamp(newPos.z, box.minBound.y, box.maxBound.y);
        float distX = newPos.x - closestX, distZ = newPos.z - closestZ;
        if ((distX * distX + distZ * distZ) < (entityRadius * entityRadius)) return true;
    }
    return false;
}

glm::ivec2 WorldToGrid(const glm::vec3& worldPos) {
    return glm::ivec2(
        glm::clamp(static_cast<int>((worldPos.x + HALF_SIZE_X) / CELL_SIZE), 0, GRID_SIZE - 1),
        glm::clamp(static_cast<int>((worldPos.z + HALF_SIZE_Z) / CELL_SIZE), 0, GRID_SIZE - 1)
    );
}

glm::vec3 GridToWorld(int gx, int gz) {
    return glm::vec3((gx * CELL_SIZE) - HALF_SIZE_X + (CELL_SIZE * 0.5f), 0.0f, (gz * CELL_SIZE) - HALF_SIZE_Z + (CELL_SIZE * 0.5f));
}

// Fast A* Pathfinding without heap allocation
std::vector<glm::vec3> FindAStarPath(glm::vec3 startPos, glm::vec3 targetPos) {
    std::vector<glm::vec3> path;
    glm::ivec2 s = WorldToGrid(startPos), t = WorldToGrid(targetPos);
    if (s == t) return path;

    // Fixed-size flat buffer on the stack/static memory (No 'new' allocations)
    static PathNode nodePool[GRID_SIZE][GRID_SIZE];
    bool closed[GRID_SIZE][GRID_SIZE] = { false };
    bool openSet[GRID_SIZE][GRID_SIZE] = { false };

    auto heuristic = [](int x1, int z1, int x2, int z2) { return std::abs(x1 - x2) + std::abs(z1 - z2); };

    std::priority_queue<PathNode*, std::vector<PathNode*>, ComparePathNodes> open;

    nodePool[s.y][s.x] = { s.x, s.y, 0, heuristic(s.x, s.y, t.x, t.y), 0, nullptr };
    nodePool[s.y][s.x].f = nodePool[s.y][s.x].h;

    open.push(&nodePool[s.y][s.x]);
    openSet[s.y][s.x] = true;

    int dx[4] = { 1, -1, 0, 0 };
    int dz[4] = { 0, 0, 1, -1 };

    PathNode* targetNode = nullptr;

    while (!open.empty()) {
        PathNode* cur = open.top();
        open.pop();

        if (cur->x == t.x && cur->z == t.y) { targetNode = cur; break; }
        closed[cur->z][cur->x] = true;

        for (int i = 0; i < 4; i++) {
            int nx = cur->x + dx[i], nz = cur->z + dz[i];
            if (nx < 0 || nz < 0 || nx >= GRID_SIZE || nz >= GRID_SIZE || obstacleGrid[nx][nz] || closed[nz][nx]) continue;

            int newG = cur->g + 1;
            if (!openSet[nz][nx]) {
                nodePool[nz][nx] = { nx, nz, newG, heuristic(nx, nz, t.x, t.y), 0, cur };
                nodePool[nz][nx].f = newG + nodePool[nz][nx].h;
                open.push(&nodePool[nz][nx]);
                openSet[nz][nx] = true;
            } else if (newG < nodePool[nz][nx].g) {
                nodePool[nz][nx].g = newG;
                nodePool[nz][nx].f = newG + nodePool[nz][nx].h;
                nodePool[nz][nx].parent = cur;
            }
        }
    }

    if (targetNode) {
        PathNode* temp = targetNode;
        while (temp) { path.push_back(GridToWorld(temp->x, temp->z)); temp = temp->parent; }
    }

    return path;
}

// ============================================================================
// INPUT & CALLBACKS
// ============================================================================
void processInput(GLFWwindow* window) {
    if (playerHp <= 0) return;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (currentState == ATTACK || currentState == SKILL || currentState == ULTI) return;

    float velocity = playerSpeed * deltaTime;
    glm::vec3 moveForward = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
    glm::vec3 moveRight   = glm::normalize(glm::vec3(camera.Right.x, 0.0f, camera.Right.z));
    glm::vec3 moveDir     = glm::vec3(0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += moveForward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= moveForward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= moveRight;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += moveRight;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) currentState = ATTACK;
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) currentState = SKILL;
    else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) currentState = ULTI;
    else if (glm::length(moveDir) > 0.0f) {
        moveDir = glm::normalize(moveDir);
        glm::vec3 nextPos = playerPos + moveDir * velocity;
        if (!CheckTreeCollision(nextPos, PLAYER_SIZE * 0.4f)) playerPos = nextPos;
        playerYaw = glm::atan(moveDir.x, moveDir.z);
        currentState = RUNNING;
    } else {
        currentState = IDLE;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn), ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX, yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) { camera.ProcessMouseScroll(static_cast<float>(yoffset)); }

// ============================================================================
// UI & AUTHENTICATION
// ============================================================================
void SaveAndLogout(bool& isLoggedIn, const std::string& username, const glm::vec3& pos, float yaw, int hp, int score) {
    saveGameToServer(username, pos.x, pos.y, pos.z, yaw, hp, score);
    isLoggedIn = false;
}

void RenderAuthInterface(bool& isLoggedIn, glm::vec3& outPlayerPos, float& outPlayerYaw, int& outPlayerHp, int& outPlayerScore) {
    static bool isSignUpPage = false;
    static char username[64] = "", password[64] = "", confirmPassword[64] = "";
    static std::string errorMessage = "", successMessage = "";

    auto ClearForm = []() {
        memset(username, 0, sizeof(username)); memset(password, 0, sizeof(password)); memset(confirmPassword, 0, sizeof(confirmPassword));
        errorMessage.clear(); successMessage.clear();
    };

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(350.0f, isSignUpPage ? 360.0f : 300.0f), ImGuiCond_Always);

    ImGui::Begin("Authentication", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(isSignUpPage ? "Create Account" : "Account Login").x) * 0.5f);
    ImGui::TextUnformatted(isSignUpPage ? "Create Account" : "Account Login");
    ImGui::Separator(); ImGui::Spacing();

    ImGui::Text("Username"); ImGui::SetNextItemWidth(-1.0f); ImGui::InputText("##user", username, IM_ARRAYSIZE(username));
    ImGui::Text("Password"); ImGui::SetNextItemWidth(-1.0f); ImGui::InputText("##pass", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);

    if (isSignUpPage) {
        ImGui::Text("Confirm Password"); ImGui::SetNextItemWidth(-1.0f); ImGui::InputText("##confirm_pass", confirmPassword, IM_ARRAYSIZE(confirmPassword), ImGuiInputTextFlags_Password);
    }
    ImGui::Spacing();

    bool enterPressed = ImGui::IsKeyPressed(ImGuiKey_Enter);
    if (isSignUpPage) {
        if (ImGui::Button("Register", ImVec2(-1, 0)) || enterPressed) {
            if (username[0] == '\0' || password[0] == '\0') errorMessage = "Fields cannot be empty.";
            else if (strcmp(password, confirmPassword) != 0) errorMessage = "Passwords do not match.";
            else if (registerPlayer(username, password)) { ClearForm(); isSignUpPage = false; successMessage = "Account created successfully! Please login."; }
            else { errorMessage = "Username already taken or server error."; successMessage.clear(); }
        }
        if (ImGui::Button("Already have an account? Log In", ImVec2(-1, 0))) { ClearForm(); isSignUpPage = false; }
    } else {
        if (ImGui::Button("Sign In", ImVec2(-1, 0)) || enterPressed) {
            if (username[0] == '\0' || password[0] == '\0') errorMessage = "Please enter your credentials.";
            else {
                std::string serverError; PlayerData loadedData;
                if (loginPlayer(username, password, loadedData, serverError)) {
                    activeUsername = username;
                    outPlayerPos = glm::vec3(loadedData.posX, 0.0f, loadedData.posZ);
                    outPlayerYaw = loadedData.yaw; outPlayerHp = loadedData.hp; outPlayerScore = loadedData.score;
                    successMessage = "Login successful!"; errorMessage.clear(); ClearForm(); isLoggedIn = true;
                } else { errorMessage = serverError; successMessage.clear(); }
            }
        }
        if (ImGui::Button("Need an account? Sign Up", ImVec2(-1, 0))) { ClearForm(); isSignUpPage = true; }
    }

    if (!errorMessage.empty()) ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", errorMessage.c_str());
    if (!successMessage.empty()) ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", successMessage.c_str());
    ImGui::End();
}
