#include <iostream>
#include <algorithm>	

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Window.h"
#include "Sphere.h"
#include "Box.h"
#include "Skybox.h"
#include "Program.h"
#include "UniformBuffer.h"
#include "Texture.h"
#include "Cubemap.h"
#include "Camera.h"
#include "Common.h"

const GLint WIDTH = 1920;
const GLint HEIGHT = 1080;
const GLfloat SPHERE_SCALE = 0.6;

glm::vec3 get_ray_from_mouse(glm::vec2 cursor, glm::mat4 projection, glm::mat4 view);
glm::vec3 ray_sphere(glm::vec3 ray_origin_wor, glm::vec3 ray_direction_wor, glm::vec3 sphere_centre_wor, GLfloat sphere_radius, bool& intersection);

int main()
{
	Window mainWindow = Window(WIDTH, HEIGHT);
	mainWindow.initialise();

	Program mainProgram = Program();
	mainProgram.createFromFiles("main.vert", "main.frag");

	Program cubemapProgram = Program();
	cubemapProgram.createFromFiles("skybox.vert", "skybox.frag");

	UniformBuffer UBO = UniformBuffer(mainProgram.getID());
	UBO.genVertexBuffers();
	UBO.genFragmentBuffers();
	
	Sphere sphere = Sphere(1.0f);
	sphere.initialise();
	Texture earthTexture = Texture();
	earthTexture.initialise("textures/map.jpg");

	Cubemap cubemap = Cubemap();
	std::vector<std::string> faces;
	faces.push_back("textures/cubemap/px.png");
	faces.push_back("textures/cubemap/nx.png");
	faces.push_back("textures/cubemap/py.png");
	faces.push_back("textures/cubemap/ny.png");
	faces.push_back("textures/cubemap/pz.png");
	faces.push_back("textures/cubemap/nz.png");
	cubemap.initialise(faces);
	Skybox skybox = Skybox(20.0f);
	skybox.initialise();

	// Control
	const char* msg;
	bool useTexture = true;
	bool wireFrame = false;
	bool dragStartFlag = false;
	glm::vec3 prevMouseRayVector = glm::vec3(0.0f, 1.0f, 0.0f);

	// Model
	glm::mat4 model(1.0f);
	glm::vec3 translation = { 0.0f, 0.0f, 0.0f };
	glm::vec3 scaling = glm::vec3(SPHERE_SCALE);
	glm::quat modelQuaternions = glm::angleAxis(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));

	// Projection
	GLfloat aspectRatio = (GLfloat)mainWindow.getBufferWidth() / mainWindow.getBufferHeight();
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

	// View
	Camera camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3(0.0f, 0.0f, -1.0f));

	// Material
	Material material;
	material.ambient = 0.1f;
	material.shininess = 1.0f;
	material.diffuse = 1.0f;
	material.specular = 1.0f;
	
	// Light
	Light light;
	light.position = { 0.0f, 0.0f, 2.0f };
	light.direction = { 0.0f, 0.0f, -1.0f };
	light.strength = 1.0;
	light.fallOffStart = 0.0f;
	light.fallOffEnd = 10.0f;
	light.spotPower = 1.0f;
	light.isDirectional = 1;
	light.isPoint = 0;
	light.isSpot = 0;
	light.useBlinnPhong = true;


	while (!mainWindow.getShouldClose())
	{
		// Get + Handle user input events
		glfwPollEvents();

		// Pick	
		bool intersection = false;
		if (mainWindow.getMouseLeft())
		{
			glm::vec3 ray_wor = get_ray_from_mouse(mainWindow.getCursor(), projection, camera.calculateViewMatrix());
			glm::vec3 pickPoint = ray_sphere(camera.getPosition(), ray_wor, translation, SPHERE_SCALE, intersection);
			if (intersection)
			{
				if (dragStartFlag) {
					dragStartFlag = false;
					prevMouseRayVector = glm::normalize(pickPoint - translation);
				}
				else {
					glm::vec3 currentMouseRayVector = glm::normalize(pickPoint - translation);
					GLfloat theta = glm::acos(glm::dot(prevMouseRayVector, currentMouseRayVector));
					if (theta > 0.001f)
					{
						glm::vec3 axis = glm::normalize(glm::cross(prevMouseRayVector, currentMouseRayVector));
						modelQuaternions = glm::angleAxis(theta, axis) * modelQuaternions;
						prevMouseRayVector = glm::normalize(pickPoint - translation);
					}
				}
			}		
		}
		else
		{
			dragStartFlag = true;
		}
	
		glPolygonMode(GL_FRONT_AND_BACK, wireFrame ? GL_LINE : GL_FILL);
		glEnable(GL_CULL_FACE);
		mainWindow.clear(0.0f, 0.0f, 0.0f, 1.0f);

		// Model
		model = glm::mat4(1.0f);
		model = glm::translate(model, translation);
		model = glm::scale(model, scaling);
		model = model * glm::toMat4(modelQuaternions);

		// Start to draw main objects
		mainProgram.use();
		UBO.bindFragmentBuffers(useTexture, camera.getPosition(), material, light);

		// Draw sphere
		UBO.bindVertexBuffers(model, projection, camera.calculateViewMatrix());
		earthTexture.use();
		sphere.draw();

		// Draw skybox
		glDepthMask(GL_FALSE);
		cubemapProgram.use();
		cubemap.use();
		skybox.draw();
		glDepthMask(GL_TRUE);

		// Initialise
		glUseProgram(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		mainWindow.swapBuffers();
	}
}

// https://github.com/capnramses/antons_opengl_tutorials_book/blob/master/07_ray_picking/main.cpp
glm::vec3 get_ray_from_mouse(glm::vec2 cursor, glm::mat4 projection, glm::mat4 view) {
	// screen space (viewport coordinates)
	GLfloat x = cursor.x * 2.0f / WIDTH - 1.0f;
	GLfloat y = 1.0f - cursor.y * 2.0f / HEIGHT;
	GLfloat z = 1.0f;
	// normalised device space
	glm::vec3 ray_nds = glm::vec3(x, y, z);
	// clip space
	glm::vec4 ray_clip = glm::vec4(ray_nds[0], ray_nds[1], -1.0, 1.0);
	// eye space
	glm::vec4 ray_eye = glm::inverse(projection) * ray_clip;
	ray_eye = glm::vec4(ray_eye[0], ray_eye[1], -1.0, 0.0);
	// world space
	glm::vec3 ray_wor = glm::vec3(inverse(view) * ray_eye);
	ray_wor = glm::normalize(ray_wor);
	return ray_wor;
}

// https://github.com/capnramses/antons_opengl_tutorials_book/blob/master/07_ray_picking/main.cpp
glm::vec3 ray_sphere(glm::vec3 ray_origin_wor, glm::vec3 ray_direction_wor, glm::vec3 sphere_centre_wor, GLfloat sphere_radius, bool& intersection) {
	// work out components of quadratic
	glm::vec3 dist_to_sphere = ray_origin_wor - sphere_centre_wor;
	GLfloat b = glm::dot(ray_direction_wor, dist_to_sphere);
	GLfloat c = glm::dot(dist_to_sphere, dist_to_sphere) - sphere_radius * sphere_radius;
	GLfloat b_squared_minus_c = b * b - c;

	// check for "imaginary" answer. == ray completely misses sphere
	if (b_squared_minus_c < 0.0f) {
		intersection = false;
		return glm::vec3(0.0f);
	}

	// check for ray hitting twice (in and out of the sphere)
	if (b_squared_minus_c > 0.0f) {
		// get the 2 intersection distances along ray
		GLfloat t_a = -b + sqrt(b_squared_minus_c);
		GLfloat t_b = -b - sqrt(b_squared_minus_c);

		// if behind viewer, throw one or both away
		if (t_a < 0.0) {
			if (t_b < 0.0) {
				intersection = false;
				return glm::vec3(0.0f);
			}
		}
		else if (t_b < 0.0) {
			return ray_origin_wor + ray_direction_wor * t_a;
		}
		else {
			intersection = true;
			return ray_origin_wor + ray_direction_wor * t_b;
		}
	}

	// check for ray hitting once (skimming the surface)
	if (b_squared_minus_c == 0.0f) {
		// if behind viewer, throw away
		GLfloat t = -b + sqrt(b_squared_minus_c);
		if (t < 0.0f) {
			intersection = false;
			return glm::vec3(0.0f);
		}
		intersection = true;
		return ray_origin_wor + ray_direction_wor * t;
	}

	// note: could also check if ray origin is inside sphere radius
	intersection = false;
	return glm::vec3(0.0f);
}
