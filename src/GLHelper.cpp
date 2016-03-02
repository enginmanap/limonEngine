//
// Created by Engin Manap on 10.02.2016.
//

#include "GLHelper.h"


GLuint GLHelper::createShader(GLenum eShaderType, const std::string &strShaderFile) {
    GLuint shader = glCreateShader(eShaderType);
    std::string shaderCode;
    std::ifstream shaderStream(strShaderFile.c_str(), std::ios::in);

    if(shaderStream.is_open()) {
        std::string Line = "";

        while(getline(shaderStream, Line))
            shaderCode += "\n" + Line;

        shaderStream.close();
    } else {
        std::cerr << strShaderFile.c_str() << " could not be read. Please ensure run directory if you used relative paths." << std::endl;
        getchar();
        return 0;
    }

    const char* shaderCodePtr = shaderCode.c_str();
    glShaderSource(shader, 1, &shaderCodePtr, NULL);

    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE)
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);
        const char *strShaderType = NULL;

        switch(eShaderType)
        {
            case GL_VERTEX_SHADER:
                strShaderType = "vertex";
                break;

            case GL_GEOMETRY_SHADER:
                strShaderType = "geometry";
                break;

            case GL_FRAGMENT_SHADER:
                strShaderType = "fragment";
                break;
        }

        std::cerr << strShaderType << " type shader " << strShaderFile.c_str() <<" could not be compiled:\n" << strInfoLog << std::endl;
        delete[] strInfoLog;

    }

    return shader;
}


GLuint GLHelper::createProgram(const std::vector<GLuint> &shaderList) {
    GLuint program = glCreateProgram();

    for(size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
        glAttachShader(program, shaderList[iLoop]);

    glLinkProgram(program);
    GLint status;
    glGetProgramiv (program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE)
    {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
        std::cerr << "Linking failed: \n" << strInfoLog << std::endl;
        delete[] strInfoLog;
    } else {
        std::cout << "Program compiled successfully" << std::endl;
    }

    for(size_t iLoop = 0; iLoop < shaderList.size(); iLoop++)
        glDetachShader(program, shaderList[iLoop]);

    return program;
}


GLuint GLHelper::initializeProgram(std::string vertexShaderFile, std::string fragmentShaderFile) {
    GLuint program;
    std::vector<GLuint> shaderList;

    shaderList.push_back(createShader(GL_VERTEX_SHADER, vertexShaderFile));
    shaderList.push_back(createShader(GL_FRAGMENT_SHADER, fragmentShaderFile));


    program = createProgram(shaderList);
    std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);
    return program;
}


GLHelper::GLHelper() {
    GLenum rev;
    glewExperimental = GL_TRUE;
    rev = glewInit();

    if (GLEW_OK != rev){
        std::cout << "GLEW init Error: " << glewGetErrorString(rev) << std::endl;
        exit(1);
    } else {
        std::cout << "GLEW Init: Success!" << std::endl;
    }

    gpuProgram = initializeProgram("./Data/Shaders/Star/vertex.shader","./Data/Shaders/Star/fragment.shader");
    glUseProgram(gpuProgram);

    transformMatrixLocation = glGetUniformLocation(gpuProgram, "worldTransformMatrix");
    cameraMatrixLocation = glGetUniformLocation(gpuProgram, "cameraTransformMatrix");
    glUseProgram(0);
    cameraTransform = glm::lookAt(glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,0.0f,-1.0f), glm::vec3(0.0f,1.0f,0.0f));

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    // Setup
    glDisable(GL_CULL_FACE);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDepthRange(0.0f, 1.0f);
}

void GLHelper::bufferVertexData(const GLfloat* vertexData, const GLuint vertexSize,
                      const GLuint* elementData, const GLuint elementSize,
                      GLuint& vao, GLuint& vbo, const GLuint attachPointer, GLuint& ebo){

    // Set up the element array buffer
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementSize, elementData, GL_STATIC_DRAW);

    // Set up the vertex attributes
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexSize, vertexData, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize, vertexData);
    glVertexAttribPointer(attachPointer, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(attachPointer);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindVertexArray(0);
}

void GLHelper::bufferVertexColor(const GLfloat* colorData, const GLuint ColorSize,
                       GLuint& vao, GLuint& vbo, const GLuint attachPointer){
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, ColorSize, colorData, GL_STATIC_DRAW);

    glBindVertexArray(vao);
    glVertexAttribPointer(attachPointer, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);
}

void GLHelper::bufferVertexTextureCoordinates(const GLfloat *coordinateData, const GLuint coordinateDataSize,
                                              GLuint &vao, GLuint &vbo, const GLuint attachPointer, GLuint &ebo) {
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, coordinateDataSize, coordinateData, GL_STATIC_DRAW);

    glBindVertexArray(vao);
    glVertexAttribPointer(attachPointer, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);

}

void GLHelper::setCamera(const glm::mat4& cameraTransform){
    this->cameraTransform = cameraTransform;
}


void GLHelper::render(const GLuint program, const GLuint vao, const GLuint ebo, const glm::mat4& modelMatrix, const GLuint elementCount) {
    if(program == 0) {
        //we don't allow no program rendering, I am not sure if it is possible either
        glUseProgram(gpuProgram);
    } else {
        glUseProgram(program);
        glUniform3f(3, cameraTransform[0][0], cameraTransform[0][1], cameraTransform[0][2] );
    }

    // Set up the model and projection matrix
    glm::mat4 projection_matrix(glm::frustum(-1.0f, 1.0f, -aspect, aspect, 1.0f, 500.0f));

    if(program == 0) {
        projection_matrix *= cameraTransform;
    } else {
        projection_matrix *=  glm::mat4(glm::mat3(cameraTransform));
    }

    glUniformMatrix4fv(cameraMatrixLocation, 1, GL_FALSE, glm::value_ptr(projection_matrix));

    // Set up for a glDrawElements call
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glUniformMatrix4fv(transformMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, NULL);

    glUseProgram(0);
    // DrawArraysInstanced
    //glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 1);
    // DrawElementsBaseVertex
    //glDrawElementsBaseVertex(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, NULL, 1);
}

GLHelper::~GLHelper() {
    glUseProgram(0);
    glDeleteProgram(gpuProgram);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void GLHelper::reshape(int height, int width) {
    glViewport(0, 0 , width, height);
    aspect = float(height) / float(width);
}

GLuint GLHelper::loadTexture(int height, int width, bool alpha, void *data) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    if(alpha){
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    }
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

void GLHelper::attachTexture(GLuint textureID){
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void GLHelper::attachCubeMap(GLuint cubeMapID){
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapID);
}

bool GLHelper::deleteTexture(GLuint textureID) {
    if(glIsTexture(textureID)) {
        glDeleteTextures(1, &textureID);
        return true;
    } else {
        return false;
    }
}

GLuint GLHelper::loadCubeMap(int height, int width, void *right, void *left, void *top, void *bottom, void *back,
                             void *front) {
    GLuint cubeMap;
    glGenTextures(1, &cubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, right);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, left);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, top);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, bottom);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, back);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, front);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return cubeMap;
}
