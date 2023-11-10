#pragma once

#include <GL\glew.h>
#include "Common.h"
#include <vector>

class Mesh
{
public:
	Mesh();

	void initialise();
	void draw();

	~Mesh();

protected:
	std::vector<Vertex> _vertices;
	std::vector<GLuint> _indices;

private:
	GLuint _VAO, _VBO, _IBO;
	GLsizei _indexCount;

	void clear();
};

