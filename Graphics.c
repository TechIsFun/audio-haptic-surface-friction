#include "Graphics.h"

GLUquadricObj* cone;
GLfloat w2, d2, h2;

/**
 * Draw a surface
 * @param w width
 * @param d depth
 * @param h height
 * @param texturized true if the surface has a texture
 */
void drawSurface(const GLfloat w, const GLfloat d, const GLfloat h, const GLboolean texturized)
{
	w2 = w/2;
	d2 = d/2;
	h2 = h/2;
	glPushMatrix();
	glColor3f(.8f,.8f,.9f);
	#if(defined(_TEXTURE))
	if (texturized) glBindTexture(GL_TEXTURE_2D,textures[0]);
	#endif
	glBegin(GL_QUAD_STRIP);
	#if(defined(_TEXTURE))
	if(texturized)
	{
		glNormal3f(0.0f,0.0f,1.0f);
		glTexCoord2f(1.0,0.0);
		glVertex3f(w/2,-h/2,d/2);
		glTexCoord2f(1.0,1.0);
		glVertex3f(w/2,h/2,d/2);
		glTexCoord2f(0.0,0.0);
		glVertex3f(-w/2,-h/2,d/2);
		glTexCoord2f(0.0f,1.0f);
		glVertex3f(-w/2,h/2,d/2);
	
		glNormal3f(-1.0f,0.0f,0.0f);
		glTexCoord2f(1.0,0.0);
		glVertex3f(-w/2,-h/2,-d/2);
		glTexCoord2f(1.0,1.0);
		glVertex3f(-w/2,h/2,-d/2);
		
		glNormal3f(0.0f,0.0f,-1.0f);
		glTexCoord2f(0.0,0.0);
		glVertex3f(w/2,-h/2,-d/2);
		glTexCoord2f(0.0,1.0);
		glVertex3f(w/2,h/2,-d/2);
		
		glNormal3f(1.0f,0.0f,0.0f);
		glTexCoord2f(1.0,0.0);
		glVertex3f(w/2,-h/2,d/2);
		glTexCoord2f(1.0,1.0);
		glVertex3f(w/2,h/2,d/2);
	}
	else
	#endif
	{
		glNormal3f(0.0f,0.0f,1.0f);
		glVertex3f(w2,-h2,d2);
		glVertex3f(w2,h2,d2);
		glVertex3f(-w2,-h2,d2);
		glVertex3f(-w2,h2,d2);
	
		glNormal3f(-1.0f,0.0f,0.0f);
		glVertex3f(-w2,-h2,-d2);
		glVertex3f(-w2,h2,-d2);
		
		glNormal3f(0.0f,0.0f,-1.0f);
		glVertex3f(w2,-h2,-d2);
		glVertex3f(w2,h2,-d2);
		
		glNormal3f(1.0f,0.0f,0.0f);
		glVertex3f(w2,-h2,d2);
		glVertex3f(w2,h2,d2);

	}
	glEnd();
	glBegin(GL_QUADS);
	#if(defined(_TEXTURE))
	if(texturized)
	{
		glNormal3f(0.0f,1.0f,0.0f);
		glTexCoord2f(0.0,0.0);
		glVertex3f(-w/2,h/2,d/2);
		glTexCoord2f(1.0,0.0);
		glVertex3f(w/2,h/2,d/2);
		glTexCoord2f(1.0,1.0);
		glVertex3f(w/2,h/2,-d/2);
		glTexCoord2f(0.0,1.0);
		glVertex3f(-w/2,h/2,-d/2);

		glNormal3f(0.0f,-1.0f,0.0f);
		glTexCoord2f(1.0,1.0);
		glVertex3f(w/2,-h/2,-d/2);
		glTexCoord2f(1.0,0.0);
		glVertex3f(w/2,-h/2,d/2);
		glTexCoord2f(0.0,0.0);
		glVertex3f(-w/2,-h/2,d/2);
		glTexCoord2f(0.0,1.0);
		glVertex3f(-w/2,-h/2,-d/2);
	}
	else
	#endif
	{
		glNormal3f(0.0f,1.0f,0.0f);
		glVertex3f(-w2,h2,d2);
		glVertex3f(w2,h2,d2);
		glVertex3f(w2,h2,-d2);
		glVertex3f(-w2,h2,-d2);

		glNormal3f(0.0f,-1.0f,0.0f);
		glVertex3f(w2,-h2,-d2);
		glVertex3f(w2,-h2,d2);
		glVertex3f(-w2,-h2,d2);
		glVertex3f(-w2,-h2,-d2);

		//Bordo rosso
		/**
		glColor3f(1.0f, .0f, .0f);
		glNormal3f(0.0f,1.0f,0.0f);
		glVertex3f(-w2+10,h2+1,d2);
		glVertex3f(-w2+20,h2+1,d2);
		glVertex3f(-w2+20,h2+1,-d2);
		glVertex3f(-w2+10,h2+1,-d2);

		glVertex3f(w2-10,h2+1,d2);
		glVertex3f(w2-20,h2+1,d2);
		glVertex3f(w2-20,h2+1,-d2);
		glVertex3f(w2-10,h2+1,-d2);

		glVertex3f(w2,h2+1,d2-10);
		glVertex3f(w2,h2+1,d2-20);
		glVertex3f(-w2,h2+1,d2-10);
		glVertex3f(-w2,h2+1,d2-20);
		glColor3f(.8f,.8f,.9f);
		**/

	}
	glEnd();
	glPopMatrix();
}

/**
 * Draw a cone
 * @param texturized true if the cone has a texture
 * @param nShadow determine the color of the cursor
 */
void drawCone(const GLboolean texturized, const GLint nShadow)
{
	//glShadeModel(GL_SMOOTH);
	glPushMatrix();
	//glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, light0_ambient);
	glTranslatef(0.0f,20.0f,20.0f);
	cone = gluNewQuadric();
	gluQuadricDrawStyle(cone,GLU_FILL);
	gluQuadricNormals(cone,GLU_FLAT);
	gluQuadricOrientation(cone,GLU_OUTSIDE);
	if(nShadow == 0) { glColor4f(.1f, .1f, .9f, 1.0f); }
    else glColor4f(.4f, .4f, .4f, .4f);
	#if(defined(_TEXTURE))
	if (texturized)
	{
		gluQuadricTexture(cone,GL_TRUE);
		glBindTexture(GL_TEXTURE_2D,textures[0]);
	}
	#endif
	//gluCylinder(cone,10.0,0,20.0,50,50);
	//gluCylinder(cone,0,1,2,30,30);
	gluSphere(cone,10,30,30);
	gluDeleteQuadric(cone);
	glPopMatrix();
}

