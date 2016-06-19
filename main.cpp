// the OpenGL version include also includes all previous versions
// Build note: Due to a minefield of preprocessor build flags, the gl_load.hpp must come after 
// the version include.
// Build note: Do NOT mistakenly include _int_gl_4_4.h.  That one doesn't define OpenGL stuff 
// first.
// Build note: Also need to link opengl32.lib (unknown directory, but VS seems to know where it 
// is, so don't put in an "Additional Library Directories" entry for it).
// Build note: Also need to link glload/lib/glloadD.lib.
#include "glload/include/glload/gl_4_4.h"
#include "glload/include/glload/gl_load.hpp"

// Build note: Must be included after OpenGL code (in this case, glload).
// Build note: Also need to link freeglut/lib/freeglutD.lib.  However, the linker will try to 
// find "freeglut.lib" (note the lack of "D") instead unless the following preprocessor 
// directives are set either here or in the source-building command line (VS has a
// "Preprocessor" section under "C/C++" for preprocessor definitions).
// Build note: Also need to link winmm.lib (VS seems to know where it is, so don't put in an 
// "Additional Library Directories" entry).
#define FREEGLUT_STATIC
#define _LIB
#define FREEGLUT_LIB_PRAGMAS 0
#include "freeglut/include/GL/freeglut.h"

// this linking approach is very useful for portable, crude, barebones demo code, but it is 
// better to link through the project building properties
#pragma comment(lib, "glload/lib/glloadD.lib")
#pragma comment(lib, "opengl32.lib")            // needed for glload::LoadFunctions()
#pragma comment(lib, "freeglut/lib/freeglutD.lib")
#ifdef WIN32
#pragma comment(lib, "winmm.lib")               // Windows-specific; freeglut needs it
#endif

// for making program from shader collection
#include <string>
#include <fstream>
#include <sstream>

// for printf(...)
#include <stdio.h>

#define DEBUG

// these should really be encapsulated off in some structure somewhere, but for the sake of this
// barebones demo, keep them here
GLint gUniformTextureLocation;
GLuint gVaoId;
GLuint gTextureId;

/*-----------------------------------------------------------------------------------------------
Description:
    This is some kind of assistant debugger function that is called at startup that I found while
    doing some tutorial awhile back.  I don't know what it does, but I keep it around.
Parameters:
    Unknown.  The function pointer is provided to glDebugMessageCallbackARB(...), and that
    function is responsible for calling this one as it sees fit.
Returns:    None
Exception:  Safe
Creator:
    John Cox (2014)
-----------------------------------------------------------------------------------------------*/
void APIENTRY DebugFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* message, const GLvoid* userParam)
{
    std::string srcName;
    switch (source)
    {
    case GL_DEBUG_SOURCE_API_ARB: srcName = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: srcName = "Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: srcName = "Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: srcName = "Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION_ARB: srcName = "Application"; break;
    case GL_DEBUG_SOURCE_OTHER_ARB: srcName = "Other"; break;
    }

    std::string errorType;
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR_ARB: errorType = "Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: errorType = "Deprecated Functionality"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: errorType = "Undefined Behavior"; break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB: errorType = "Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB: errorType = "Performance"; break;
    case GL_DEBUG_TYPE_OTHER_ARB: errorType = "Other"; break;
    }

    std::string typeSeverity;
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH_ARB: typeSeverity = "High"; break;
    case GL_DEBUG_SEVERITY_MEDIUM_ARB: typeSeverity = "Medium"; break;
    case GL_DEBUG_SEVERITY_LOW_ARB: typeSeverity = "Low"; break;
    }

    printf("%s from %s,\t%s priority\nMessage: %s\n",
        errorType.c_str(), srcName.c_str(), typeSeverity.c_str(), message);
}

/*-----------------------------------------------------------------------------------------------
Description:
    Encapsulates the creation of a texture.  It tries to cover all the basics and be as self-
    contained as possible, only returning a texture ID when it is finished.
Parameters: None
Returns:    
    The OpenGL ID of the texture that was created.
Exception:  Safe
Creator:
    John Cox (2-24-2016)
-----------------------------------------------------------------------------------------------*/
GLuint CreateTexture()
{
    // create a 2D texture buffer
    GLuint textureId;
    glGenTextures(1, &textureId);

    // bind the generated texture (it's a buffer, but I'll keep calling it "texture") to the 
    // OpenGL texture unit 0
    // Note: This is default behavior, which is perhaps why many tutorials skip it, but this 
    // program is for posterity, so I am trying to be thorough.
    //??why does making a mismatch between the one in here and the one in display() not cause a 
    // problem??
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // these are some kind of standard texture settings for how to magnify it (detail when 
    // zooming in), "minify" it (detail when zooming out), and set tiling.
    // Note: I don't know how these work or what they do in detail, but they seem to be common
    // in a few texture tutorials.
    //??why wouldn't the program work at all without the "min filter"? it'll work without the
    // other glTexParameteri(...) settings, but not without the "min filter"??
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   // "S" is texture X axis
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);   // "T" is texture Y axis
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   // "zoom in"
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   // "zoom out"

    //??do this? http://www.gamedev.net/page/resources/_/technical/opengl/opengl-texture-mapping-an-introduction-r947??)
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // create a texture out of RGB values
    // Note: The function glTexImage2D(...) is how texture data is sent to the GPU.  The 
    // documentation (http://docs.gl/gl4/glTexImage2D) specifies what the individual arguments 
    // mean.  
    // void glTexImage2D(GLenum target, // making a 2D texture, so use GL_TEXTURE_2D
    //     GLint level,                 // somekind of "level of detail" thing; leave at 0
    //     GLint internalFormat,        // how we want OpenGL to store the data we provide
    //     GLsizei width,               // how many texels wide
    //     GLsizei height,              // texels tall (for GL_TEXTURE_2D; GL_TEXTURE_1D different)
    //     GLint border,                // documentation says to only use 0 (probably legacy)
    //     GLenum format,               // the format of the data that we provide
    //     GLenum type,                 // integer, unsigned byte, float, etc.
    //     const GLvoid * data);        // pointer to data
    // 
    // I have decided that I will create a 2D texture of RGB values.  The fragment shader's idea 
    // of color is 0.0-1.0, so I will make each color channel (R, G, or B) as a float.  Other 
    // format options are available if someone wants to get very specific about how they store 
    // their texture data (ex: GL_RGBA32F is RGBA (RGB + alpha channel) with 32bits per channel,
    // which might come in handy if the programmer was concerned that a user's "float" might not
    // be 32bits), but for this demo, I will keep things relatively simple.
    struct texel
    {
        // do NOT define any methods or else the texel construction loop will have to change
        //texel() {}
        GLfloat r;
        GLfloat g;
        GLfloat b;
        GLfloat a;  // alpha
    };

    // glTexImage2D(...) will take a pointer to the data, but not a pointer to pointer, so 2D
    // arrays are not an option and the 2D texture data must be crammed into a 1D array
    // Note: The documentation for glTexImage2D(...) says this about the order of the contents:
    // "The first element corresponds to the lower left corner of the texture image. Subsequent 
    // elements progress left-to-right through the remaining texels in the lowest row of the 
    // texture image, and then in successively higher rows of the texture image. The final 
    // element corresponds to the upper right corner of the texture image."
    const unsigned int MAX_TEXEL_ROWS = 64;
    const unsigned int TEXELS_PER_ROW = 64;
    texel crudeTextureArr[MAX_TEXEL_ROWS * TEXELS_PER_ROW];
    for (size_t rowCounter = 0; rowCounter < MAX_TEXEL_ROWS; rowCounter++)
    {
        for (size_t colCounter = 0; colCounter < TEXELS_PER_ROW; colCounter++)
        {
            // for the sake of this demo, the bottom third will be red, the middle third green, 
            // and the top third blue.

            // this array-style assignment is only possible when the struct has no methods 
            // Note: Even a constructor that takes nothing and does nothing will prevent this.
            texel t = { 0.0f, 0.0f, 0.0f, 0.0f };
            if (rowCounter < (MAX_TEXEL_ROWS / 3))
            {
                // bottom third, so red
                t = { 1.0f, 0.0f, 0.0f, 1.0f };
            }
            else if (rowCounter < ((2 * MAX_TEXEL_ROWS) / 3))
            {
                // middle third, so green
                t = { 0.0f, 1.0f, 0.0f, 1.0f };
            }
            else
            {
                // top third, so blue
                t = { 0.0f, 0.0f, 1.0f, 1.0f };
            }

            // jam the data into the array
            crudeTextureArr[(rowCounter * TEXELS_PER_ROW) + colCounter] = t;
        }
    }

    // ??when to use this??
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    // finally, upload the texture to the GPU
    GLint level = 0;                    // some kind of "detail" thing (leave at 0)
    GLenum type = GL_FLOAT;             // type is float
    GLenum format = GL_RGBA;            // texel data provided as 4-float sets
    GLint internalFormat = GL_RGBA;     // store the texel as 4-float sets
    GLsizei width = TEXELS_PER_ROW;
    GLsizei height = MAX_TEXEL_ROWS;
    GLint border = 0;                   // documentation says 0 (must be legacy)
    glTexImage2D(GL_TEXTURE_2D, level, internalFormat, width, height, border, format, type, crudeTextureArr);

    // clean up bindings
    glBindTexture(GL_TEXTURE_2D, 0);

    // give back a handle to what was created
    return textureId;
}

/*-----------------------------------------------------------------------------------------------
Description:
    Encapsulates the creation of vertices, including the texture coordinates of each vertex.  It 
    tries to cover all the basics and be as self-contained as possible, only returning a VAO ID 
    when it is finished.
Parameters: None
Returns:
    The OpenGL ID of the VAO that was created.
Exception:  Safe
Creator:
    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
GLuint CreateGeometry()
{
    // center the triangle on the texture, whose texture coordinates are[0, 1]
    // Note: This doesn't strictly need to be static because the gl*Data(...) functions (called 
    // later in this same function) will send it off to the GPU, and then it won't be needed in 
    // system memory.
    GLfloat localVerts[] =
    {
        -0.5f, -0.5f, -1.0f,        // (pos) left bottom corner
        +0.0f, +0.0f,               // texel at bottom left of texture

        +0.5f, -0.5f, -1.0f,        // (pos) right bottom corner
        +1.0f, 0.0f,                // texel at bottom right of texture

        +0.0f, +0.5f, -1.0f,        // (pos) center top
        +0.5f, +1.0f,               // texel at top center of texture
    };

    // create a vertex array and bind it to the context (it is expected that glload's "init" 
    // function has already been called, which sets up the OpenGL context)
    GLuint vertexArrayObjectId = 0;
    glGenVertexArrays(1, &vertexArrayObjectId);
    glBindVertexArray(vertexArrayObjectId);

    // create vertex data
    // Note: If you need to later call glDeleteBuffers(...) to clean up the data, keep 
    // the vertex buffer ID around.  This barebones code will rely on program end for
    // cleanup, and the bound VAO will handle future binding/unbinding, so the ID will 
    // be created just long enough to use it.
    GLuint vertBufId = 0;
    glGenBuffers(1, &vertBufId);
    glBindBuffer(GL_ARRAY_BUFFER, vertBufId);  // ??check for bad number first??

    // send data to GPU
    // Note: If data already exists and you know exactly which byte in the array to stick the 
    // new data, use glBufferSubData(...)
    glBufferData(GL_ARRAY_BUFFER, sizeof(localVerts), localVerts, GL_STATIC_DRAW);

    // set vertex array data to describe the byte pattern
    // Note: The byte pattern in that single float array is arranged such that it can be split 
    // into three distinct patterns.  Vertex Attribute Arrays let the patterns be specified.
    // Starting at float 0, the pattern repeats every 9 floats (36 bytes) until end of the 
    // buffer.  OpenGL already knows the data size from glBufferData(...).
    unsigned int vertexArrayIndex = 0;
    unsigned int bufferStartByteOffset = 0;
    const unsigned int NUM_THINGS = 3;          // 3 floats per vertex array index
    const unsigned int BYTES_PER_THING = 12;    // 3 floats at 4 bytes per
    const unsigned int BYTES_PER_VERT = 20;     // attribute pattern repeats every 5 floats

    // position attribute (3 floats (12 bytes) per)
    // Note: 3 floats (12 bytes) per attribute,
    glEnableVertexAttribArray(vertexArrayIndex);
    glVertexAttribPointer(vertexArrayIndex, 3, GL_FLOAT, GL_FALSE, BYTES_PER_VERT, 
        (void *)bufferStartByteOffset);
    bufferStartByteOffset += 12;   // 12 bytes after attribute start

    // texture coord (2 floats (8 bytes) per)
    // Note: Don't forget (as I did) to increment to the next attribute or the attribute in the 
    // vertex shader will be a zero vector (that is, won't get set).
    vertexArrayIndex++; 
    glEnableVertexAttribArray(vertexArrayIndex);
    glVertexAttribPointer(vertexArrayIndex, 2, GL_FLOAT, GL_FALSE, BYTES_PER_VERT, 
        (void *)bufferStartByteOffset);

    // index data
    // Note: In order to draw, OpenGL needs point data.  Triangles always need three points 
    // specified, so rather than sending in repeat vertices, index data allows the user to 
    // repeat index data instead (always in sets of 3), which is computationally less demanding 
    // than sending the entire vertex to the GPU multiple times.  The vertices were previously 
    // described by the vertex attribute arrays and associated pointers, saying that each vertex 
    // was composed of three things with their own byte patterns.
    GLushort localIndices[]
    {
        0, 1, 2,
    };

    // like "vertex buffer ID", keep this around if you need to alter the data or clean it up 
    // properly before program end
    GLuint elemArrBufId = 0;
    glGenBuffers(1, &elemArrBufId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemArrBufId);  // ??check for bad number first??
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(localIndices), localIndices, GL_STATIC_DRAW);

    // clean up bindings
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // all good, so return the VAO ID
    return vertexArrayObjectId;
}

/*-----------------------------------------------------------------------------------------------
Description:
    Encapsulates the creation of an OpenGL GPU program, including the compilation and linking of 
    shaders.  It tries to cover all the basics and the error reporting and is as self-contained 
    as possible, only returning a program ID when it is finished.
Parameters: None
Returns:
    The OpenGL ID of the GPU program.
Exception:  Safe
Creator:
    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
GLuint CreateProgram()
{
    // hard-coded ignoring possible errors like a boss

    // load up the vertex shader and compile it
    // Note: After retrieving the file's contents, dump the stringstream's contents into a 
    // single std::string.  Do this because, in order to provide the data for shader 
    // compilation, pointers are needed.  The std::string that the stringstream::str() function 
    // returns is a copy of the data, not a reference or pointer to it, so it will go bad as 
    // soon as the std::string object disappears.  To deal with it, copy the data into a 
    // temporary string.
    std::ifstream shaderFile("shader.vert");
    std::stringstream shaderData;
    shaderData << shaderFile.rdbuf();
    shaderFile.close();
    std::string tempFileContents = shaderData.str();
    GLuint vertShaderId = glCreateShader(GL_VERTEX_SHADER);
    const GLchar *vertBytes[] = { tempFileContents.c_str() };
    const GLint vertStrLengths[] = { (int)tempFileContents.length() };
    glShaderSource(vertShaderId, 1, vertBytes, vertStrLengths);
    glCompileShader(vertShaderId);
    // alternately (if you are willing to include and link in glutil, boost, and glm), call 
    // glutil::CompileShader(GL_VERTEX_SHADER, shaderData.str());

    GLint isCompiled = 0;
    glGetShaderiv(vertShaderId, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
        GLchar errLog[128];
        GLsizei *logLen = 0;
        glGetShaderInfoLog(vertShaderId, 128, logLen, errLog);
        printf("vertex shader failed: '%s'\n", errLog);
        glDeleteShader(vertShaderId);
        return 0;
    }

    // load up the fragment shader and compiler it
    shaderFile.open("shader.frag");
    shaderData.str(std::string());      // because stringstream::clear() only clears error flags
    shaderData.clear();                 // clear any error flags that may have popped up
    shaderData << shaderFile.rdbuf();
    shaderFile.close();
    tempFileContents = shaderData.str();
    GLuint fragShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *fragBytes[] = { tempFileContents.c_str() };
    const GLint fragStrLengths[] = { (int)tempFileContents.length() };
    glShaderSource(fragShaderId, 1, fragBytes, fragStrLengths);
    glCompileShader(fragShaderId);

    glGetShaderiv(fragShaderId, GL_COMPILE_STATUS, &isCompiled);
    if (isCompiled == GL_FALSE)
    {
        GLchar errLog[128];
        GLsizei *logLen = 0;
        glGetShaderInfoLog(fragShaderId, 128, logLen, errLog);
        printf("fragment shader failed: '%s'\n", errLog);
        glDeleteShader(vertShaderId);
        glDeleteShader(fragShaderId);
        return 0;
    }

    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertShaderId);
    glAttachShader(programId, fragShaderId);
    glLinkProgram(programId);

    // the program contains binary, linked versions of the shaders, so clean up the compile 
    // objects
    // Note: Shader objects need to be un-linked before they can be deleted.  This is ok because
    // the program safely contains the shaders in binary form.
    glDetachShader(programId, vertShaderId);
    glDetachShader(programId, fragShaderId);
    glDeleteShader(vertShaderId);
    glDeleteShader(fragShaderId);

    // check if the program was built ok
    GLint isLinked = 0;
    glGetProgramiv(programId, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        printf("program didn't compile\n");
        glDeleteProgram(programId);
        return 0;
    }

    // done here
    return programId;
}

/*-----------------------------------------------------------------------------------------------
Description:
    This is the rendering function.  It tells OpenGL to clear out some color and depth buffers,
    to set up the data to draw, to draw than stuff, and to report any errors that it came across.
    This is not a user-called function.

    This function is registered with glutDisplayFunc(...) during glut's initialization.
Parameters: None
Returns:    None
Exception:  Safe
Creator:
    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
void display()
{
    // clear existing data
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // set up the data to draw
    glUniform1i(gUniformTextureLocation, 0);
    glBindVertexArray(gVaoId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId);

    // do the thing
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0);

    // tell the GPU to swap out the displayed buffer with the one that was just rendered
    glutSwapBuffers();

    // tell glut to call this display() function again on the next iteration of the main loop
    // Note: https://www.opengl.org/discussion_boards/showthread.php/168717-I-dont-understand-what-glutPostRedisplay()-does
    glutPostRedisplay();

    // clean up bindings
    // Note: This is just good practice, but in reality the bindings can be left as they were 
    // and re-bound on each new call to this rendering function.
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/*-----------------------------------------------------------------------------------------------
Description:
    Tell's OpenGL to resize the viewport based on the arguments provided.  This is not a 
    user-called function.

    This function is registered with glutReshapeFunc(...) during glut's initialization.
Parameters: 
    w   The width of the window in pixels.
    h   The height of the window in pixels.
Returns:    None
Exception:  Safe
Creator:
    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
}

/*-----------------------------------------------------------------------------------------------
Description:
    Executes some kind of command when the user presses a key on the keyboard.  This is not a 
    user-called function.

    This function is registered with glutKeyboardFunc(...) during glut's initialization.
Parameters: 
    key     The ASCII code of the key that was pressed (ex: ESC key is 27)
    x       ??
    y       ??
Returns:    None
Exception:  Safe
Creator:
    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 27:
    {
        // ESC key
        glutLeaveMainLoop();
        return;
    }
    default:
        break;
    }
}

/*-----------------------------------------------------------------------------------------------
Description:
    ??what does it do? I picked it up awhile back and haven't changed it??
Parameters:
    displayMode     ??
    width           ??
    height          ??
Returns:    
    ??what??
Exception:  Safe
Creator:
    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
unsigned int defaults(unsigned int displayMode, int &width, int &height) 
{ 
    return displayMode; 
}

/*-----------------------------------------------------------------------------------------------
Description:
    Governs window creation, the initial OpenGL configuration (face culling, depth mask, even
    though this is a 2D demo and that stuff won't be of concern), the creation of geometry, and
    the creation of a texture.
Parameters:
    argc    (From main(...)) The number of char * items in argv.  For glut's initialization.
    argv    (From main(...)) A collection of argument strings.  For glut's initialization.
Returns:
    False if something went wrong during initialization, otherwise true;
Exception:  Safe
Creator:
    John Cox (3-7-2016)
-----------------------------------------------------------------------------------------------*/
bool init(int argc, char *argv[])
{
    glutInit(&argc, argv);

    // I don't know what this is doing, but it has been working, so I'll leave it be for now
    int windowWidth = 500;  // square 500x500 pixels
    int windowHeight = 500;
    unsigned int displayMode = GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;
    displayMode = defaults(displayMode, windowWidth, windowHeight);
    glutInitDisplayMode(displayMode);

    // create the window
    // ??does this have to be done AFTER glutInitDisplayMode(...)??
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(300, 200);   // X = 0 is screen left, Y = 0 is screen top
    int window = glutCreateWindow(argv[0]);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);

    // OpenGL 4.3 was where internal debugging was enabled, freeing the user from having to call
    // glGetError() and analyzing it after every single OpenGL call, complete with surrounding it
    // with #ifdef DEBUG ... #endif blocks
    // Note: https://blog.nobel-joergensen.com/2013/02/17/debugging-opengl-part-2-using-gldebugmessagecallback/
    int glMajorVersion = 4;
    int glMinorVersion = 4;
    glutInitContextVersion(glMajorVersion, glMinorVersion);
    glutInitContextProfile(GLUT_CORE_PROFILE);
#ifdef DEBUG
    glutInitContextFlags(GLUT_DEBUG);   // if enabled, 
#endif

                                        // glload must load AFTER glut loads the context
    glload::LoadTest glLoadGood = glload::LoadFunctions();
    if (!glLoadGood)    // apparently it has an overload for "bool type"
    {
        printf("glload::LoadFunctions() failed\n");
        return false;
    }
    else if (!glload::IsVersionGEQ(glMajorVersion, glMinorVersion))
    {
        // the "is version" check is an "is at least version" check
        printf("Your OpenGL version is %i, %i. You must have at least OpenGL %i.%i to run this tutorial.\n",
            glload::GetMajorVersion(), glload::GetMinorVersion(), glMajorVersion, glMinorVersion);
        glutDestroyWindow(window);
        return 0;
    }
    else if (glext_ARB_debug_output)
    {
        // condition will be true if GLUT_DEBUG is a context flag
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageCallbackARB(DebugFunc, (void*)15);
    }

    // these OpenGL initializations are for 3D stuff, where depth matters and multiple shapes can
    // be "on top" of each other relative to the most distant thing rendered, and this barebones 
    // code is only for 2D stuff, but initialize them anyway as good practice (??bad idea? only 
    // use these once 3D becomes a thing??)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);

    GLuint programId = CreateProgram();
    glUseProgram(programId);
    gUniformTextureLocation = glGetUniformLocation(programId, "tex");
    if (gUniformTextureLocation == -1)
    {
        fprintf(stderr, "Could not bind uniform %s\n", "tex");
        //??throw a fit or continue??
    }

    // create the vertices for the geometry (and the texture coordinates that go with each 
    // vertex) and the texture that will be used to color it
    // Note: The VAO and texture will be bound at render time (see display()).
    gVaoId = CreateGeometry();
    gTextureId = CreateTexture();

    // all went well
    return true;
}

/*-----------------------------------------------------------------------------------------------
Description:
    Program start and end.
Parameters: 
    argc    The number of strings in argv.
    argv    A pointer to an array of null-terminated, C-style strings.
Returns:
    0 if program ended well, which it always does or it crashes outright, so returning 0 is fune
Exception:  Safe
Creator:
    John Cox (2-13-2016)
-----------------------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
//    glutInit(&argc, argv);
//
//    int width = 500;
//    int height = 500;
//    unsigned int displayMode = GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH | GLUT_STENCIL;
//    displayMode = defaults(displayMode, width, height);
//
//    glutInitDisplayMode(displayMode);
//    glutInitContextVersion(4, 4);
//    glutInitContextProfile(GLUT_CORE_PROFILE);
//#ifdef DEBUG
//    glutInitContextFlags(GLUT_DEBUG);
//#endif
//    glutInitWindowSize(width, height);
//    glutInitWindowPosition(300, 200);
//    int window = glutCreateWindow(argv[0]);
//
//    glload::LoadTest glLoadGood = glload::LoadFunctions();
//    // ??check return value??
//
//    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
//
//    // the "is version" check is an "is at least version" check
//    if (!glload::IsVersionGEQ(3, 3))
//    {
//        printf("Your OpenGL version is %i, %i. You must have at least OpenGL 3.3 to run this tutorial.\n",
//            glload::GetMajorVersion(), glload::GetMinorVersion());
//        glutDestroyWindow(window);
//        return 0;
//    }
//
//    if (glext_ARB_debug_output)
//    {
//        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
//        glDebugMessageCallbackARB(DebugFunc, (void*)15);
//    }
//
//    init();
    if (!init(argc, argv))
    {
        // bad initialization; it will take care of it's own error reporting
        return 1;
    }


    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMainLoop();

    return 0;
}