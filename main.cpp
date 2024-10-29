// This code:
// Boids simulation using OpenGL, GLFW & GLEW
// UI made using Dear Imgui
// Code owned by 

#define GLEW_BUILD
#define GLFW_DLL

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <numeric>
using namespace std;

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GLEW/glew.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "sprite_renderer.h"
#include "resource_manager.h"
#include "animation_object.h"

int screen_width, screen_height;
SpriteRenderer  *Renderer;
glm::dvec2 MOUSE_POS;

vector<AnimationObject> BOIDS;
// glm::vec2 BOID_POS_SUM;
// glm::vec2 BOID_VEL_SUM;

// Boid constants
const int BOID_NUM = 100;
glm::vec2 BOID_SIZE(16.0f, 16.0f);
float BASE_BOID_COHESION = 30;
float BASE_BOID_SEPARATION = 6;
float BASE_BOID_ALIGNMENT = 1;

// Boid options (changeable in real time through imgui menu)
float SCALE_BOID_COHESION = 5.0;
float SCALE_BOID_SEPARATION = 5.0;
float SCALE_BOID_ALIGNMENT = 1.5;
float TURN_FACTOR_FLAT = 2.0;
float VIEW_RANGE = 150;
float SPEED_SCALAR = 1.0;
int vel_lim = BOID_SIZE.x*2;
bool follow_mouse = false;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
static void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the window dimensions always;
    glViewport(0, 0, width, height);
}

glm::vec2 Scalar_Div(glm::vec2 Dividend, float Divisor) {
    if (Divisor != 0) return Dividend / glm::vec2 (Divisor, Divisor);
    return Dividend;
}

glm::vec2 get_init_boid_pos() {
    // initialises position of a boid
    // position is a random off-screen location limited by 240 units away
    int x, y;
    int posneg[2] = {1, -1};
    int neg_x = posneg[rand() % 2];
    int neg_y = posneg[rand() % 2];
    if (neg_x > 0) {
        x = screen_width + rand() % 240 + 1;
    }
    else {
        x = - (rand() % 240 + 1);
    }
    if (neg_y > 0) {
        y = screen_height + rand() % 240 + 1;
    }
    else {
        y = - (rand() % 240 + 1);
    }
    glm::vec2 boid_pos = {x, y};
    // cout << boid_pos.x << " " << boid_pos.y << "\n";
    return boid_pos;
}

glm::vec2 get_init_boid_vel() {
    // initialises velocity of a boid
    // velocity is completely random under vel_lim
    int posneg[2] = {1, -1};
    int neg_x = posneg[rand() % 2];
    int neg_y = posneg[rand() % 2];
    return glm::vec2 {neg_x*rand() % vel_lim, neg_y*rand() % vel_lim};
}

// void sum_boid_prop() { 
//     // sums boid properties of position + velocity and stores them in global variables to avoid recomputation
//     glm::vec2 pos_res, vel_res;
//     for (int i = 0; i < BOIDS.size(); i++) {
//         pos_res += BOIDS[i].Position;
//         vel_res += BOIDS[i].Velocity;
//     }
//     BOID_POS_SUM = pos_res;
//     BOID_VEL_SUM = vel_res;
// }

// brute force to implement view range
vector<vector<bool>> boid_neighbours;
vector<pair<glm::vec2, glm::vec2>> boid_prop;

void find_boid_neighbours(int ix) {
    
    for (int j = 0; j < BOIDS.size(); j++) {
        if (ix != j && glm::length(BOIDS[ix].Position-BOIDS[j].Position) < VIEW_RANGE) {
            boid_neighbours[ix][j] = true;
        }
        else {
            boid_neighbours[ix][j] = false;
        }
        // cout << boid_neighbours[ix][j] << "\n";
    }
}

void sum_boid_prop_within_range(int ix) {
    glm::vec2 pos_res = {0, 0};
    glm::vec2 vel_res = {0, 0};
    find_boid_neighbours(ix);

    for (int j = 0; j < boid_neighbours[ix].size(); j++) {
        if (boid_neighbours[ix][j] == true) {
            pos_res += BOIDS[j].Position;
            vel_res += BOIDS[j].Velocity;
        }
    }
    boid_prop[ix].first = pos_res;
    boid_prop[ix].second = vel_res;
}

glm::vec2 bound_boid_pos(int ix, glm::vec2 res_v) {
    int bind_margin = 250;
    if (BOIDS[ix].Position.x < bind_margin) {
        res_v.x = TURN_FACTOR_FLAT;
    }
    else if (BOIDS[ix].Position.x > screen_width-bind_margin) {
        res_v.x = -TURN_FACTOR_FLAT;
    }
    if (BOIDS[ix].Position.y < bind_margin) {
        res_v.y = TURN_FACTOR_FLAT;
    }
    else if (BOIDS[ix].Position.y > screen_height-bind_margin) {
        res_v.y = -TURN_FACTOR_FLAT;
    }
    return res_v;
}

glm::vec2 update_boid_pos(int ix) {
    // Returns vector to change position of the boid at index ix in BOIDS vector
    glm::vec2 res_v, BOID_POS_SUM, BOID_VEL_SUM;

    sum_boid_prop_within_range(ix);
    BOID_POS_SUM = boid_prop[ix].first;
    BOID_VEL_SUM = boid_prop[ix].second;
    int n_neighbours = accumulate(boid_neighbours[ix].begin(), boid_neighbours[ix].end(), 0);
    /////////////////////////////////////////////////////////////////////////////
    // Movement is based on 3 main rules, with additional misc conditions
    glm::vec2 v1, v2, v3;
    // rule 1 Cohesion: tries to move towards perceived centre of the flock (avg pos of all other boids within range)
    v1 = Scalar_Div(BOID_POS_SUM, n_neighbours);
    //    vector [Perceived Centre] = vector [Sum Neighbour Boid Centres] / number of neighbours
    v1 = Scalar_Div((v1 - BOIDS[ix].Position), BASE_BOID_COHESION*(1/SCALE_BOID_COHESION));
    //    vector [Cur Boid->Perceived Centre] = (vector [Origin->Perceived Centre] - vector [Origin->Cur Boid])

    // rule 2 Separation: tries to keep small distance away from other boids
    for (int i = 0; i < BOIDS.size(); i++) {
        glm::vec2 displacement = BOIDS[i].Position - BOIDS[ix].Position;
        double dist = glm::length(displacement);
        if (dist < BASE_BOID_SEPARATION*SCALE_BOID_SEPARATION) {
            // if distance is less than X units, move away from that boid as far as you already are
            v2 -= displacement;
        }
    }
    // rule 3 Alignment: tries to match velocity with near boids
    v3 = Scalar_Div(BOID_VEL_SUM, n_neighbours);
    v3 = Scalar_Div(v3 - BOIDS[ix].Velocity, BASE_BOID_ALIGNMENT*SCALE_BOID_ALIGNMENT);
    
    // Sum together vectors from the 3 main rules
    res_v = v1+v2+v3;

    /////////////////////////////////////////////////////////////////////////////
    // Additional rules:

    // rule A: following mouse cursor
    if (follow_mouse) {
        glm::vec2 vA = (glm::vec2(MOUSE_POS) - BOIDS[ix].Position) / glm::vec2(40, 40);
        res_v += vA;
    }

    // rule B: bound position of boid softly (redirect velocity into screen bounds)
    res_v = bound_boid_pos(ix, res_v);

    // cout << res_v.x << " " << res_v.y << "\n";
    return res_v;
}

// Main code
int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        cout << "GLFW did not initialise properly" << "\n";
        return 1;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Setup GLFW window
    screen_width = 1600;
    screen_height = 900;
    GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "Boids simulation ", nullptr, nullptr);
    if (window == nullptr) {
        cout << "failed to create window" << "\n";
        return 1;
    }
        
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // glfwSwapInterval(1); // Enable vsync
    
    if ( GLEW_OK != glewInit() )
    {
        cout << "GLEW did not initialise properly" << "\n";
        return false;
    }
    // Load shaders
    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.fs", nullptr, "sprite");
    // configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(screen_width), 
        static_cast<float>(screen_height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
    // TODO: make it so you can switch between textures
    ResourceManager::LoadTexture("textures\\space_cat.png", true, "space_cat");

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Animation settings
    int sprite_selected = 0;
    string sprite_options[2] = {"space_cat", "borg"};
    ImVec4 clear_color = ImVec4(0.55f, 0.00f, 0.00f, 0.70f);
    
    // Init boid positions
    glm::vec2 empty_vec2 = {0, 0};
    for (int i = 0; i < BOID_NUM; i++) {
        boid_prop.push_back(make_pair(empty_vec2, empty_vec2));
        boid_neighbours.push_back(vector<bool>(BOID_NUM, false));

        glm:: vec2 boidPos = get_init_boid_pos();
        // cout << boidPos.x << " " << boidPos.y << "\n";
        AnimationObject curBoid = AnimationObject(boidPos, BOID_SIZE, ResourceManager::GetTexture("space_cat"));
        curBoid.Velocity = get_init_boid_vel();
        BOIDS.push_back(curBoid);
    }
    // add buffer just in case
    boid_prop.push_back(make_pair(empty_vec2, empty_vec2));
    boid_neighbours.push_back(vector<bool>(BOID_NUM, false));

    // Setup OpenGL view
    glViewport(0, 0, screen_width, screen_height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ////////////////////////////////////
    // temp code to control fps - taken from stack overflow
    constexpr float MAX_FPS = 60.f;
    constexpr float FRAME_TIME = 1.f / MAX_FPS;

    float last_time = glfwGetTime();
    float last_update_time = glfwGetTime();
    float accumulated_time = 0.f;
    ////////////////////////////////////
    
    // Main rendering loop
    while (!glfwWindowShouldClose(window))
    {
        float current_time = glfwGetTime();
        float dt = current_time - last_time;
        last_time = current_time;
        accumulated_time += dt;

        glfwPollEvents();
        processInput(window);

        // Update stored mouse position
        glfwGetCursorPos(window, &MOUSE_POS.x, &MOUSE_POS.y);
        // Update sum of boid pos + vel (outdated)
        // sum_boid_prop();

        if (accumulated_time >= FRAME_TIME)
        {
            accumulated_time = 0.0f;
            // Update boid positions
            for (int i = 0; i < BOID_NUM; i++) {
                BOIDS[i].Velocity += update_boid_pos(i);
                // scale velocity
                BOIDS[i].Velocity *= glm::vec2 {SPEED_SCALAR, SPEED_SCALAR};
                // limit velocity
                if (glm::length(BOIDS[i].Velocity) > vel_lim) {
                    BOIDS[i].Velocity = BOIDS[i].Velocity / glm::length(BOIDS[i].Velocity) * glm::vec2(vel_lim, vel_lim);
                }
                BOIDS[i].Position += BOIDS[i].Velocity;
                if (BOIDS[i].Velocity.x > 0) {
                    BOIDS[i].Rotation = 1.5;
                }
                else {
                    BOIDS[i].Rotation = -1.5;
                }
                
        }
        
        // OpenGL rendering
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        
        for (int i = 0; i < BOID_NUM; i++) {
            BOIDS[i].Draw(*Renderer);
        }

        // ImGui rendering
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // SETTINGS MENU WINDOW
        // purpose: to modify sprites, speed, and whether boids should follow mouse
        ImGui::Begin("Settings");
        ImVec2 display_size = ImGui::GetIO().DisplaySize;
        ImGui::SetWindowSize(ImVec2(display_size.x/3, display_size.y/2));
        ImGui::SetWindowFontScale(2.0);
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_NoResize;

        // sprite options
        if (ImGui::TreeNode("Sprite")) {
            for (int n = 0; n < 2; n++)
            {
                char buf[32];
                sprintf(buf, sprite_options[n].c_str(), n);
                if (ImGui::Selectable(buf, sprite_selected == n))
                    sprite_selected = n;
            }
            ImGui::TreePop();
        }
        // boid speed slider
        ImGui::Text("Speed");
        ImGui::SameLine();
        ImGui::SliderFloat("##", &SPEED_SCALAR, 0.5f, 2.0f);

        // BOID OPTIONS
        ImGui::Text("COHESION  ");
        ImGui::SameLine();
        ImGui::SliderFloat("#1", &SCALE_BOID_COHESION, 0.1f, 10.0f);

        ImGui::Text("SEPARATION");
        ImGui::SameLine();
        ImGui::SliderFloat("#2", &SCALE_BOID_SEPARATION, 0.1f, 10.0f);

        ImGui::Text("ALIGNMENT ");
        ImGui::SameLine();
        ImGui::SliderFloat("#3", &SCALE_BOID_ALIGNMENT, 0.1f, 3.0f);
        // problem might be imgui returning too precise of a number/float
        ImGui::Text("TURN FACTOR");
        ImGui::SameLine();
        ImGui::SliderFloat("#4", &TURN_FACTOR_FLAT, 1.0f, 12.0f);

        ImGui::Text("VIEW RANGE");
        ImGui::SameLine();
        ImGui::SliderFloat("#5", &VIEW_RANGE, 100.0f, 800.0f);

        // follow mouse tickbox
        ImGui::Checkbox("Follow mouse", &follow_mouse);

        ImGui::Text("Application average %.3f ms/frame\n(%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        // Rendering frame
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        }
    }
    
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
