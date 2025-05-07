#include <Windows.h>
#include <gl/GLU.h>


/// @brief draws the given sprite based on the given parameters
/// @params 
void drawSprite(GLuint texture, GLfloat xPositionLeft, GLfloat xPositionRight, GLfloat yPositionTop,
				GLfloat yPositionBottom, GLfloat u, GLfloat v, GLfloat xTextureCoord, GLfloat yTextureCoord,
				float depth)
{
	// draw the Ui elements
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBegin(GL_TRIANGLE_STRIP);
	{
		// draw the textured quad as a tristrip
		glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);

		// TL
		glTexCoord2f(xTextureCoord, yTextureCoord);
		glVertex3f(xPositionLeft, yPositionTop, depth);

		// BL
		glTexCoord2f(xTextureCoord, yTextureCoord - v);
		glVertex3f(xPositionLeft, yPositionBottom, depth);

		// TR
		glTexCoord2f(xTextureCoord + u, yTextureCoord);
		glVertex3f(xPositionRight, yPositionTop, depth);

		// BR
		glTexCoord2f(xTextureCoord + u, yTextureCoord - v);
		glVertex3f(xPositionRight, yPositionBottom, depth);
	}
	glEnd();
}