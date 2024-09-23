#include "gl_shader.hpp"
#include "gtc/type_ptr.hpp"
#include "gl_debug.hpp"
#include "gl_texture2D.hpp"

#include <fstream>
#include <sstream>

Shader::Shader() {};

Shader::Shader(Shader &shader) {
	this->m_render_id = shader.m_render_id;
	this->is_init = shader.is_init;
	shader.is_init = false;
}
Shader::Shader(Shader && shader) {
	this->m_render_id = shader.m_render_id;
	this->is_init = shader.is_init;
	shader.is_init = false;
}

Shader::Shader(const char *filename) {
	auto tuple = ParseShader(filename);
	auto vertexShaderSrc = std::get<0>(tuple);
	auto fragmentShaderSrc = std::get<1>(tuple);
	//auto geometryShaderSrc = std::get<2>(tuple);
	std::cout << vertexShaderSrc << "\n";
	std::cout << fragmentShaderSrc << "\n";
	unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
	//unsigned int gShader = glCreateShader(GL_GEOMETRY_SHADER);
	const char* vs_src = vertexShaderSrc.c_str();
	const char* fs_src = fragmentShaderSrc.c_str();
	//const char* gs_src = geometryShaderSrc.c_str();

	GLCALL(glShaderSource(vShader, 1, &vs_src, NULL));
	GLCALL(glShaderSource(fShader, 1, &fs_src, NULL));
	//GLCALL(glShaderSource(gShader, 1, &gs_src, NULL));
	GLCALL(glCompileShader(vShader));
	GLCALL(glCompileShader(fShader));
	//GLCALL(glCompileShader(gShader));

	int ret;
	char logInfo[512];
	glGetShaderiv(vShader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		glGetShaderInfoLog(vShader, 512, 0, logInfo);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << logInfo << std::endl;
	}

	glGetShaderiv(fShader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		glGetShaderInfoLog(fShader, 512, 0, logInfo);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << logInfo << std::endl;
	}

	// glGetShaderiv(gShader, GL_COMPILE_STATUS, &ret);
	// if (!ret) {
	// 	glGetShaderInfoLog(gShader, 512, 0, logInfo);
	// 	std::cout << "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED\n" << logInfo << std::endl;
	// }

	GLCALL(m_render_id = glCreateProgram());
	GLCALL(glAttachShader(m_render_id, vShader));
	GLCALL(glAttachShader(m_render_id, fShader));
	//GLCALL(glAttachShader(m_render_id, gShader));

	GLCALL(glLinkProgram(m_render_id));
	glGetProgramiv(m_render_id, GL_LINK_STATUS, &ret);
	if (!ret) {
		glGetProgramInfoLog(m_render_id, 512, NULL, logInfo);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << logInfo << std::endl;
	}
	
	
	is_init = true;

	GLCALL(glDeleteShader(vShader));
	GLCALL(glDeleteShader(fShader));
	//GLCALL(glDeleteShader(gShader));
};

Shader::Shader(const char* vs, const char* fs, const char* gs) {
	auto vertexShaderSrc = ReadShaderFile(vs);
	auto fragmentShaderSrc = ReadShaderFile(fs);
	unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
	unsigned int gShader = glCreateShader(GL_GEOMETRY_SHADER);
	const char* vs_src = vertexShaderSrc.c_str();
	const char* fs_src = fragmentShaderSrc.c_str();

	GLCALL(glShaderSource(vShader, 1, &vs_src, NULL));
	GLCALL(glShaderSource(fShader, 1, &fs_src, NULL));
	GLCALL(glCompileShader(vShader));
	GLCALL(glCompileShader(fShader));	

	int ret;
	char logInfo[512];
	glGetShaderiv(vShader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		glGetShaderInfoLog(vShader, 512, 0, logInfo);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << logInfo << std::endl;
	}

	glGetShaderiv(fShader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		glGetShaderInfoLog(fShader, 512, 0, logInfo);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << logInfo << std::endl;
	}
	GLCALL(m_render_id = glCreateProgram());
	GLCALL(glAttachShader(m_render_id, vShader));
	GLCALL(glAttachShader(m_render_id, fShader));

	if (gs != nullptr) {
		auto geometryShaderSrc = ReadShaderFile(gs);
		const char* gs_src = geometryShaderSrc.c_str();
		GLCALL(glShaderSource(gShader, 1, &gs_src, NULL));
		GLCALL(glCompileShader(gShader));

		glGetShaderiv(gShader, GL_COMPILE_STATUS, &ret);
		if (!ret) {
			glGetShaderInfoLog(gShader, 512, 0, logInfo);
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << logInfo << std::endl;
		}
		GLCALL(glAttachShader(m_render_id, gShader));
	}
	GLCALL(glLinkProgram(m_render_id));
	glGetProgramiv(m_render_id, GL_LINK_STATUS, &ret);
	if (!ret) {
		glGetProgramInfoLog(m_render_id, 512, NULL, logInfo);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << logInfo << std::endl;
	}

	is_init = true;

	GLCALL(glDeleteShader(vShader));
	GLCALL(glDeleteShader(fShader));
	GLCALL(glDeleteShader(gShader));
}


Shader::~Shader() {
	GLCALL(glDeleteProgram(m_render_id));
};

Shader& Shader::Use() {
	GLCALL(glUseProgram(m_render_id));
	return *this;
};

const Shader& Shader::Use() const {
	GLCALL(glUseProgram(m_render_id));
	return *this;
}

void Shader::UnUse() const{
	GLCALL(glUseProgram(0));
};

std::tuple<std::string, std::string, std::string> Shader::ParseShader(const char *filename) {
	std::ifstream in(filename);
	std::stringstream ss[3];
	std::string str;
	Type type = Type::NONE;

	while (!std::getline(in, str).eof()) {
		if (str.find("vertex shader") != std::string::npos) {
			type = Type::VERTEX_SHADER;
			continue;
		}
		else if (str.find("fragment shader") != std::string::npos) {
			type = Type::FRAGMENT_SHADER;
			continue;
		} else if (str.find("geometry shader") != std::string::npos) {
			type = Type::GEOMETRY_SHADER;
			continue;
		}
		if (type != Type::NONE)
			ss[static_cast<int>(type)] << str << "\n";
	
	}

	is_init = true;
	return { ss[0].str(), ss[1].str() ,ss[2].str()};
};

std::string Shader::ReadShaderFile(const char *glsl) {
	std::stringstream ss;
	std::ifstream f(glsl);
	ss << f.rdbuf();
	f.close();
	return ss.str();
}

void Shader::BindTexture(std::string name, int slot, bool useShader) const{
        if (useShader)
            this->Use();
        SetInteger(name.c_str(), slot);
    }

void Shader::SetFloat(const char *name, float value, bool useShader)const
{
    if (useShader)
        this->Use();
	GLCALL(auto location = glGetUniformLocation(this->m_render_id, name));
    GLCALL(glUniform1f(location, value));
}

void Shader::SetInteger(const char *name, int value, bool useShader)const
{
    if (useShader)
        this->Use();
	GLCALL(auto location = glGetUniformLocation(this->m_render_id, name));
    GLCALL(glUniform1i(location, value));
}

void Shader::SetVector2f(const char *name, float x, float y, bool useShader)const
{
    if (useShader)
        this->Use();
	GLCALL(auto location = glGetUniformLocation(this->m_render_id, name));
    GLCALL(glUniform2f(location, x, y));
}

void Shader::SetVector2f(const char *name, const glm::vec2 &value, bool useShader)const
{
    if (useShader)
        this->Use();
	GLCALL(auto location = glGetUniformLocation(this->m_render_id, name));
    GLCALL(glUniform2f(location, value.x, value.y));
}

void Shader::SetVector3f(const char *name, float x, float y, float z, bool useShader)const
{
    if (useShader)
        this->Use();
	GLCALL(auto location = glGetUniformLocation(this->m_render_id, name));
    GLCALL(glUniform3f(location, x, y, z));
}

void Shader::SetVector3f(const char *name, const glm::vec3 &value, bool useShader) const
{
    if (useShader)
        this->Use();
	GLCALL(auto location = glGetUniformLocation(this->m_render_id, name));
    GLCALL(glUniform3f(location, value.x, value.y, value.z));
}

void Shader::SetVector4f(const char *name, float x, float y, float z, float w, bool useShader)const 
{
    if (useShader)
        this->Use();
	GLCALL(auto location = glGetUniformLocation(this->m_render_id, name));
    GLCALL(glUniform4f(location, x, y, z, w));
}

void Shader::SetVector4f(const char *name, const glm::vec4 &value, bool useShader) const
{
    if (useShader)
        this->Use();
	GLCALL(auto location = glGetUniformLocation(this->m_render_id, name));
    GLCALL(glUniform4f(location, value.x, value.y, value.z, value.w));
}

void Shader::SetMatrix4(const char *name, const glm::mat4 &matrix, bool useShader) const 
{
    if (useShader)
        this->Use();

	GLCALL(auto location = glGetUniformLocation(this->m_render_id, name));
    GLCALL(glUniformMatrix4fv(location, 1, false, &matrix[0][0]));
}

Shader& Shader::operator ==(Shader &&other) {
	if (other.is_init) {
		this->m_render_id = other.m_render_id;
		this->is_init = true;
		other.is_init = false;
	}
		return *this;
}

Shader& Shader::operator ==(Shader &other) {
	if (other.is_init) {
		this->m_render_id = other.m_render_id;
		this->is_init = true;
		other.is_init = false;
	}
	return *this;
}


