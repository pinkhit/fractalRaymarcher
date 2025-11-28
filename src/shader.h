#pragma once

#include "common.h"


static const std::string baseShaderPath = "shaders/";

struct ShaderSources
{
    std::string vertexShader;
    std::string fragShader;   
	std::string computeShader;
};

class shader
{
public:
	shader(const std::string& vertFile, const std::string& fragFile)
		: VF_ProgID(0), Comp_ProgID(0), vertSourceFile(vertFile), fragSourceFile(fragFile)
	{
		loadVertFrag(vertFile, fragFile);
		//loadCompute(compFile);
	}
	~shader();

	// calls create shader on vert, frag, then links program with the 2
	void loadVertFrag(const std::string& vertFile, const std::string& fragFile);
	// calls create shader on compute, then creates program for it
	void loadCompute(const std::string& compFile);

	unsigned int getVF_ID() const;   // to get shader ID
	void bindVF() const;
	void unbindVF() const;


	// uniforms
	//void setUniformMat4(const std::string& uniformName, glm::mat4 desiredMatrix) const;

	void setUniform1f(const std::string& uniformName, float desiredVal) const;
	void setUniform1i(const std::string& uniformName, int desiredVal) const;
	void setUniformV2(const std::string& uniformName, glm::vec2 desiredVec) const;
	void setUniformV3(const std::string& uniformName, glm::vec3 desiredVec) const;
	void setUniformV4(const std::string& uniformName, glm::vec4 desiredVec) const;
	void setUniformMat3(const std::string& uniformName, glm::mat3 desiredMatrix) const;

private:
	// private variables
	unsigned int VF_ProgID;
	unsigned int Comp_ProgID;
	const std::string vertSourceFile;   // file path to vert shader
	const std::string fragSourceFile;   // file path to frag shader
	const std::string computeSourceFile;   // file path to frag shader
	ShaderSources shaderSourceCode;    // store shader source code
	
	// private methods
	unsigned int createVFProgram();
	unsigned int createCompProgram();
};

