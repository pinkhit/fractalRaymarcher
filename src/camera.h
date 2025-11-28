#pragma once

#include "common.h"

#include "shader.h"

class camera
{
public:
	camera(float w, float h); // canonical cam
	~camera();

	void updateResolution(float w, float h) { resolution.x = w; resolution.y = h; };

	void setUniforms(shader* pShader);
	glm::mat3 rotationMat();

	glm::vec2 getResolution() const { return resolution; };


	float yaw;
	float pitch;
	float roll;
	float speed;

	float fov;
private:
	glm::vec3 eye;
	glm::vec3 lookAt;
	glm::vec3 up;

	glm::vec2 resolution;

};