#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// game state
bool gameWon = false;

glm::vec3 charPosition(0.0f, 1.5f, 19.5f); // try values until it looks like the entrance

// reset game state
void resetGame()
{
    // Reset character to start
    charPosition = glm::vec3(0.0f, 1.5f, 19.5f);
    camera.Position = charPosition + glm::vec3(0.0f, 0.5f, 2.25f);
    gameWon = false;
}

// render win screen
void renderWinScreen(GLFWwindow* window, bool printed)
{
    // Set up orthographic 2D view
    glClearColor(0.0f, 0.0f, 0.0f, 0.7f); // dim background
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw simple text
    if (!printed) {
        std::cout << "YOU WIN!" << std::endl;

        /*renderText("Press [R] to Replay", 300.0f, 250.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        renderText("Press [ESC] to Quit", 300.0f, 200.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));*/
        std::cout << "Press [R] to Replay" << std::endl;
        std::cout << "Press [ESC] to Quit" << std::endl;
    }

    // Check for replay/quit input
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        resetGame(); // function we'll add below
    }
    else if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

// maze grid settings
const float maze_grids = 17.0f;
const float maze_grid_size = 2.3f;
std::vector<std::vector<int>> mazeGrid = {
    {1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1}, // 0
    {1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1}, // 1
    {1,0,1,1,1,1,0,1,1,1,0,1,0,1,1,0,1}, // 2
    {1,1,1,0,0,0,0,0,1,0,0,1,0,0,1,0,1}, // 3
    {1,0,0,0,1,1,1,1,1,0,1,1,0,1,1,0,1}, // 4
    {1,1,1,1,1,0,0,0,0,0,0,0,0,1,0,0,1}, // 5
    {1,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1}, // 6
    {1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1}, // 7
    {1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1}, // 8
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}, // 9
    {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1}, // 10
    {1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,1}, // 11
    {1,0,1,1,0,1,1,1,1,1,1,0,1,1,1,1,1}, // 12
    {1,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,1}, // 13
    {1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,1}, // 14
    {1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1}, // 15
    {1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1} // 16
};

bool isWallAt(const glm::vec3& position, const std::vector<std::vector<int>>& mazeGrid, float cellSize)
{
    float mazeOffsetX = -19.55f; 
    float mazeOffsetZ = -18.95f;

    int gridZ = static_cast<int>(floor((position.z - mazeOffsetZ) / cellSize));
    int gridX = static_cast<int>(floor((position.x - mazeOffsetX) / cellSize));

    if (gridZ < 0 || gridZ >= (int)mazeGrid.size() ||
        gridX < 0 || gridX >= (int)mazeGrid[0].size())
        return true;

	std::cout << "Grid Position: (" << gridX << ", " << gridZ << ") Value: " << mazeGrid[gridZ][gridX] << std::endl;

    return mazeGrid[gridZ][gridX] == 1;
}

// Raycast from start -> end, returning how far we can go before hitting a wall
float cameraRaycast(const glm::vec3& start, const glm::vec3& end,
    const std::vector<std::vector<int>>& mazeGrid,
    float cellSize)
{
    glm::vec3 dir = end - start;
    float totalDist = glm::length(dir);
    if (totalDist < 0.001f) return totalDist;

    dir = glm::normalize(dir);
    float step = 0.1f;
    float dist = 0.0f;

    while (dist < totalDist) {
        glm::vec3 samplePoint = start + dir * dist;
        if (isWallAt(samplePoint, mazeGrid, cellSize)) {
            // Collision! back up a bit
            return glm::max(dist - 0.2f, 0.0f);
        }
        dist += step;
    }
    return totalDist; // clear line
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("1.model_loading.vs", "1.model_loading.fs");

    // load models
    // -----------
    // Model ourModel(FileSystem::getPath("resources/objects/backpack/backpack.obj"));
    Model charModel(FileSystem::getPath("resources/objects/chibi-char/obj_export/char.obj"));
    Model mazeModel(FileSystem::getPath("resources/objects/maze-grass/obj_export/maze_grass.obj"));

    // --- Light setup ---
    ourShader.use();
    ourShader.setVec3("light.ambient", glm::vec3(0.10f));  // soft ambient
    ourShader.setVec3("light.diffuse", glm::vec3(1.0f, 0.9f, 0.7f)); // warm light
    ourShader.setVec3("light.specular", glm::vec3(0.5f));

    // Material setup (same for all models)
    ourShader.setVec3("material.ambient", glm::vec3(1.0f));
    ourShader.setVec3("material.diffuse", glm::vec3(1.0f));
    ourShader.setVec3("material.specular", glm::vec3(0.5f));

    // Camera position
    ourShader.setVec3("viewPos", camera.Position);
    
    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	bool printed = false;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

		// If game won, skip rendering character and show message
        if (gameWon)
        {
            renderWinScreen(window, printed);
            printed = true;
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // Draw Maze
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // Maze position
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Maze rotation
        model = glm::scale(model, glm::vec3(1.0f)); // Maze scale
        ourShader.setMat4("model", model);
        ourShader.setFloat("material.shininess", 4.0f);
        mazeModel.Draw(ourShader);

        float yaw = glm::radians(camera.Yaw);
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), -yaw, glm::vec3(0.0f, 1.0f, 0.0f));

        // Draw Character
        glm::vec3 frontDir = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
        glm::vec3 lightOffset = glm::vec3(0.0f, 0.8f, 0.0f) - frontDir * 0.3f;
        ourShader.setVec3("light.position", charPosition + lightOffset);
        glm::mat4 charModelMat = glm::mat4(1.0f);
		//charModelMat = glm::rotate(charModelMat, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Adjust initial orientation
        charModelMat = glm::translate(charModelMat, charPosition) * rotation; // Place character in maze
        //charModelMat = glm::rotate(charModelMat, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        charModelMat = glm::scale(charModelMat, glm::vec3(1.25f)); // Adjust size if needed
        ourShader.setMat4("model", charModelMat);
        ourShader.setFloat("material.shininess", 16.0f);
        charModel.Draw(ourShader);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    glm::vec3 moveDir(0.0f);
    float speed = 2.5f * deltaTime;
	bool moved = false;

    // Determine movement direction using camera's orientation
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        moveDir += camera.Front;
	    moved = true;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        moveDir -= camera.Front;
        moved = true;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        moveDir -= camera.Right;
        moved = true;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        moveDir += camera.Right;
        moved = true;

	//if (!moved) return;

    // Normalize to avoid faster diagonal movement
    if (glm::length(moveDir) > 0.0f)
        moveDir = glm::normalize(moveDir);

    float radius = 0.3f;
    glm::vec3 nextPos = charPosition + moveDir * speed;
    bool blocked =
        isWallAt(nextPos + glm::vec3(radius, 0, 0), mazeGrid, maze_grid_size) ||
        isWallAt(nextPos - glm::vec3(radius, 0, 0), mazeGrid, maze_grid_size) ||
        isWallAt(nextPos + glm::vec3(0, 0, radius), mazeGrid, maze_grid_size) ||
        isWallAt(nextPos - glm::vec3(0, 0, radius), mazeGrid, maze_grid_size);

    if (!blocked)
        charPosition += moveDir * speed;

    // Update camera to stay behind character
    glm::vec3 offset(0.0f, 0.45f, 2.25f); // height and distance
    glm::vec3 desiredCamPos = charPosition - camera.Front * offset.z + glm::vec3(0.0f, offset.y, 0.0f);

    glm::vec3 correctedCamPos = desiredCamPos;

    // Perform raycast to check if something blocks the camera
    float hitDist = cameraRaycast(charPosition + glm::vec3(0.0f, offset.y, 0.0f), desiredCamPos, mazeGrid, maze_grid_size);

    // If blocked, move camera closer
    if (hitDist < glm::length(desiredCamPos - charPosition))
    {
        glm::vec3 dir = glm::normalize(desiredCamPos - charPosition);
        glm::vec3 newCamPos = charPosition + dir * hitDist;
        camera.Position = newCamPos;
    }
    else
    {
        camera.Position = desiredCamPos;
    }

    // Check win condition
    int playerGridX = static_cast<int>((charPosition.x + (mazeGrid[0].size() * maze_grid_size) / 2.0f) / maze_grid_size);
    int playerGridZ = static_cast<int>((charPosition.z + (mazeGrid.size() * maze_grid_size) / 2.0f) / maze_grid_size);

    if (playerGridX == 8 && playerGridZ == 0)
    {
        gameWon = true;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
