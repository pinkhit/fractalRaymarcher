#include "shader.h"

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

static std::string ParseShader(const std::string file)
{
    if (file.empty())
        return file;

    std::string shaderText;
    std::string line;

    std::ifstream fileStream(file);
    if (!fileStream)
    {
        return std::string();
    }

    while (true)
    {
        std::getline(fileStream, line);

        // read in shader
        shaderText.append(line);
        shaderText.append("\n");

        if (fileStream.eof())
            break;
    }

    if (fileStream.bad())
    {
        return std::string();
    }

    return shaderText;
}


void shader::loadVertFrag(const std::string& vertFile, const std::string& fragFile)
{
    if (!vertFile.empty())
    {
        shaderSourceCode.vertexShader = ParseShader(vertFile);
        if (shaderSourceCode.vertexShader.empty())
        {
            std::cerr << "error reading vertex  shader" << std::endl;
            return;
        }
    }
    else
    {
        std::cerr << "invalid vert file name" << std::endl;
        return;
    }

    if (!fragFile.empty())
    {
        shaderSourceCode.fragShader = ParseShader(fragFile);
        if (shaderSourceCode.fragShader.empty())
        { 
            std::cerr << "error reading fragment shader  " << std::endl;
            return;
        }
    }
    else
    {
        std::cerr << "invalid frag file name" << std::endl;
        return;
    }


    VF_ProgID = createVFProgram();
    if (VF_ProgID)
    {
        std::cout << "Vertex & Fragment program created with ID " << VF_ProgID << std::endl;
    }
    else
    {
        std::cerr << "Failed to create shader program from: " << vertSourceFile << ", " << fragSourceFile << "\n";
        return;
    }
}

void shader::loadCompute(const std::string& compFile)
{
    if (!compFile.empty())
    {
        shaderSourceCode.computeShader = ParseShader(compFile);
        if (shaderSourceCode.computeShader.empty())
        {
            std::cerr << "Failed to read compute file: " << compFile << "\n";
            return;
        }

    }
    else
    {
        std::cerr << "invalid compute file name" << std::endl;
        return;
    }

    Comp_ProgID = createCompProgram();
    if (Comp_ProgID)
    {
        std::cout << "Compute program created with ID " << Comp_ProgID << std::endl;
    }
    else
    {
        std::cerr << "Failed to create shader program from: " << compFile << "\n";
    }
}

shader::~shader()
{
    if (glIsProgram(VF_ProgID))
        glDeleteProgram(VF_ProgID);

    if (glIsProgram(Comp_ProgID))
        glDeleteProgram(Comp_ProgID);
}

unsigned int shader::getVF_ID() const
{
    return VF_ProgID;
}

void shader::bindVF() const
{
    glUseProgram(VF_ProgID);
}

void shader::unbindVF() const
{
    glUseProgram(0);
}

static unsigned int CreateShader(GLenum type, std::string& source, const std::string& filename)
{
    if (source.empty() || filename.empty())
        return 0;

    unsigned int shader_id = glCreateShader(type);
    const char* source_ptr = source.c_str();
    glShaderSource(shader_id, 1, &source_ptr, nullptr);
    glCompileShader(shader_id);

    int success = 0;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        int length = 0;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);

        char* error_message = static_cast<char*>(calloc(1, length + 1));
        if (!error_message) return 0;
        glGetShaderInfoLog(shader_id, length, &length, error_message);

        std::cout << "shader failed to compile" << std::endl;
        std::cout << error_message << std::endl;

        free(error_message);

        return 0;
    }

    return shader_id;
}


static unsigned int LinkProgramVF(unsigned vertShader, unsigned fragShader)
{
    unsigned int program = glCreateProgram();

    if (vertShader) glAttachShader(program, vertShader);
    if (fragShader) glAttachShader(program, fragShader);

    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        int length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, ' ');
        glGetProgramInfoLog(program, length, nullptr, log.data());
        std::cerr << "Program linking failed:\n" << log << std::endl;
        glDeleteProgram(program);
        return 0;
    }

    // delete shaders after linking
    if (vertShader) glDeleteShader(vertShader);
    if (fragShader) glDeleteShader(fragShader);

    return program;
}

static unsigned int LinkProgramComp(unsigned computeShader)
{
    unsigned int program = glCreateProgram();

    if (computeShader) glAttachShader(program, computeShader);

    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        int length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string log(length, ' ');
        glGetProgramInfoLog(program, length, nullptr, log.data());
        std::cerr << "Program linking failed:\n" << log << std::endl;
        glDeleteProgram(program);
        return 0;
    }

    // delete shaders after linking
    if (computeShader) glDeleteShader(computeShader);

    return program;
}

unsigned int shader::createVFProgram()
{
    unsigned int vert = 0, frag = 0, comp = 0;

    if (!shaderSourceCode.vertexShader.empty())
        vert = CreateShader(GL_VERTEX_SHADER, shaderSourceCode.vertexShader, vertSourceFile);
    if (!shaderSourceCode.fragShader.empty())
        frag = CreateShader(GL_FRAGMENT_SHADER, shaderSourceCode.fragShader, fragSourceFile);

    return LinkProgramVF(vert, frag);
}

unsigned int shader::createCompProgram()
{
    unsigned int comp = 0;
    if (!shaderSourceCode.computeShader.empty())
        comp = CreateShader(GL_COMPUTE_SHADER, shaderSourceCode.computeShader, computeSourceFile);
    return LinkProgramComp(comp);
}


void shader::setUniform1f(const std::string& uniformName, float desiredVal) const
{
    int uniformLocation = glGetUniformLocation(VF_ProgID, uniformName.c_str());
    if (uniformLocation != -1)
    {
        glUniform1f(uniformLocation, desiredVal);
    }
    else
        std::cerr << "invalid uniform: " << uniformName << std::endl;

}

void shader::setUniform1i(const std::string& uniformName, int desiredVal) const
{
    int uniformLocation = glGetUniformLocation(VF_ProgID, uniformName.c_str());
    if (uniformLocation != -1)
    {
        glUniform1i(uniformLocation, desiredVal);
    }
    else
        std::cerr << "invalid uniform: " << uniformName << std::endl;
}


void shader::setUniformV2(const std::string& uniformName, glm::vec2 desiredVec) const
{
    int uniformLocation = glGetUniformLocation(VF_ProgID, uniformName.c_str());
    if (uniformLocation != -1)
    {
        glUniform2fv(uniformLocation, 1, glm::value_ptr(desiredVec));
    }
    else
        std::cerr << "invalid uniform: " << uniformName << std::endl;
}

void shader::setUniformV3(const std::string& uniformName, glm::vec3 desiredVec) const
{
    int uniformLocation = glGetUniformLocation(VF_ProgID, uniformName.c_str());
    if (uniformLocation != -1)
    {
        glUniform3fv(uniformLocation, 1, glm::value_ptr(desiredVec));
    }
    else
        std::cerr << "invalid uniform: " << uniformName << std::endl;
}

void shader::setUniformV4(const std::string& uniformName, glm::vec4 desiredVec) const
{
    int uniformLocation = glGetUniformLocation(VF_ProgID, uniformName.c_str());
    if (uniformLocation != -1)
    {
        glUniform4fv(uniformLocation, 1, glm::value_ptr(desiredVec));
    }
    else
        std::cerr << "invalid uniform: " << uniformName << std::endl;
}

void shader::setUniformMat3(const std::string& uniformName, glm::mat3 desiredMatrix) const
{
    // get the location of uniform
    int uniformLocation = glGetUniformLocation(VF_ProgID, uniformName.c_str());
    // upload the matrix to the shader
    if (uniformLocation != -1)
        glUniformMatrix3fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(desiredMatrix));
}

//void shader::setUniformMat4(const std::string& uniformName, glm::mat4 desiredMatrix) const
//{
//    // get the location of uniform
//    int uniformLocation = glGetUniformLocation(progID, uniformName.c_str());
//    // upload the matrix to the shader
//    if (uniformLocation != -1)
//        glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(desiredMatrix));
//}

