#pragma once
#include <Windows.h>
#include <gl/GLU.h>

void drawSprite(GLuint texture, GLfloat xPositionLeft, GLfloat xPositionRight, GLfloat yPositionTop,
				GLfloat yPositionBottom, GLfloat u, GLfloat v, GLfloat xTextureCoord, GLfloat yTextureCoord,
				float depth);