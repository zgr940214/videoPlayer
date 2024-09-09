#pragma once
#include "glad.h"
#include "glm.hpp"
#include <tuple>
#include <iostream>
#include <memory>

class Texture2D;
class Shader {
public:
	Shader();

	Shader(const char* filename);

	Shader(const char* vs, const char* fs, const char* gs = nullptr);

	Shader(Shader &shader);

	Shader(Shader && shader);
	
	~Shader();

	std::tuple<std::string, std::string, std::string> ParseShader(const char *); //vs fs gs written in same file
	std::string ReadShaderFile(const char *);

	Shader& Use();
	const Shader& Use() const;
	void UnUse() const;

	void BindTexture(std::string name, int slot, bool useShader=false) const;
	
	void    SetFloat    (const char *name, float value, bool useShader = false) const;
    void    SetInteger  (const char *name, int value, bool useShader = false) const;
    void    SetVector2f (const char *name, float x, float y, bool useShader = false) const;
    void    SetVector2f (const char *name, const glm::vec2 &value, bool useShader = false) const;
    void    SetVector3f (const char *name, float x, float y, float z, bool useShader = false) const;
    void    SetVector3f (const char *name, const glm::vec3 &value, bool useShader = false) const;
    void    SetVector4f (const char *name, float x, float y, float z, float w, bool useShader = false) const;
    void    SetVector4f (const char *name, const glm::vec4 &value, bool useShader = false) const;
    void    SetMatrix4  (const char *name, const glm::mat4 &matrix, bool useShader = false) const;

	Shader& operator ==(Shader &&other);
	Shader& operator ==(Shader &other);

public:
	enum class Type {
		VERTEX_SHADER = 0,
		FRAGMENT_SHADER,
		GEOMETRY_SHADER
	};

private:
	unsigned int m_render_id = -1;
	bool is_init = false;
};
