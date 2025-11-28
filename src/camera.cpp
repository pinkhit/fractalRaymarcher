#include "camera.h"

// creates canonical camera
camera::camera(float w, float h)
{
	eye = glm::vec3(0.0f, 0.0f, 3.0f);
	lookAt = glm::vec3(0.0f, 0.0f, -1.0f);
	up = glm::vec3(0.0f, 1.0f, 0.0f);

	fov = 90.0f;

	resolution.x = w;
	resolution.y = h;
}

camera::~camera()
{

}

void camera::setUniforms(shader* pShader)
{
	pShader->setUniformV3("camPos", eye);
	pShader->setUniformV3("camLookAt", lookAt);
	pShader->setUniformV3("camUp", up);
	pShader->setUniformV2("resolution", resolution);
	pShader->setUniform1f("fov", fov);
}


glm::mat3 camera::rotationMat()
{
	glm::mat3 rotY = glm::mat3(glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0, 1, 0)));
	glm::mat3 rotX = glm::mat3(glm::rotate(glm::mat4(1.0f), pitch, glm::vec3(1, 0, 0)));

	glm::mat3 rotationMatrix = rotY * rotX;

	return rotationMatrix;
}

