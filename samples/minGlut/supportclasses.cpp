#include "GLG3D/glheaders.h"
#include "glut.h"
#include "supportclasses.h"
#include "G3D/Vector2.h"

using G3D::Vector2;

int Image::PPMGammaCorrect(float radiance) const {
    // Note that the PPM gamma is fixed at 2.2
    return int(pow(std::min(1.0f, std::max(0.0f, radiance * m_exposureConstant)), 1.0f / 2.2f) * 255.0f);
}


void Image::save(const std::string& filename) const {
    FILE* file = fopen(filename.c_str(), "wt");
    fprintf(file, "P3 %d %d 255\n", m_width, m_height);                                                    
    for (int y = 0; y < m_height; ++y) {
        fprintf(file, "\n# y = %d\n", y);                                                                  
        for (int x = 0; x < m_width; ++x) {
            const Color3& c(get(x, y));
            fprintf(file, "%d %d %d\n", PPMGammaCorrect(c.r), PPMGammaCorrect(c.g), PPMGammaCorrect(c.b));
        }
    }
    fclose(file);
}


static void quitOnEscape(unsigned char key, int x, int y) {
    if (key == 27) { ::exit(0); }
}


static void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    // Draw a full-screen quad of the image
    glDrawArrays(GL_QUADS, 0, 4);
    glutSwapBuffers();
}


void Image::display(float deviceGamma) const {
    int argc = 1;
    char* argv[] = {"supportclasses"};
    
    // Initialize OpenGL
    glutInit(&argc, argv);
    glutInitWindowSize(m_width, m_height);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutCreateWindow("G3D");
    glutKeyboardFunc(&quitOnEscape);
    glutDisplayFunc(&render);

    // Initialize OpenGL extensions
    glewInit();

    // Set the color scale applied as textures are uploaded to be the exposure constant
    glMatrixMode(GL_COLOR);
    glLoadIdentity();
    glScalef(m_exposureConstant, m_exposureConstant, m_exposureConstant);

    // Create a gamma correction color table for texture load
    std::vector<Color3> gammaTable(256);
    for (unsigned int i = 0; i < gammaTable.size(); ++i) {
        gammaTable[i] = (Color3::white() * i / (gammaTable.size() - 1.0f)).pow(1.0f / deviceGamma);
    }
    glColorTable(GL_POST_COLOR_MATRIX_COLOR_TABLE, GL_RGB, (GLsizei)gammaTable.size(), GL_RGB, GL_FLOAT, &gammaTable[0]);
    glEnable(GL_POST_COLOR_MATRIX_COLOR_TABLE);
    
    // Create a texture, upload our image, and bind it (assume a
    // version of GL that supports NPOT textures)
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_FLOAT, &m_data[0]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glEnable(GL_TEXTURE_2D);

    // The vertices of a 2D quad mesh containing a single CCW square
    static const Vector2 corner[] = {Vector2(0,0), Vector2(0,1), Vector2(1,1), Vector2(1,0)};

    // Bind the quad mesh as the active geometry
    glVertexPointer(2, GL_FLOAT, 0, corner);
    glTexCoordPointer(2, GL_FLOAT, 0, corner);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    // Set orthographic projection that stretches the unit square to the
    // dimensions of the image
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 1, 0, 0, 2);
    glutMainLoop();
}
