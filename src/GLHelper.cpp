//
// Created by Engin Manap on 10.02.2016.
//

#include "GLHelper.h"
#include "GLSLProgram.h"


GLuint GLHelper::createShader(GLenum eShaderType, const std::string &strShaderFile) {
    GLuint shader = glCreateShader(eShaderType);
    std::string shaderCode;
    std::ifstream shaderStream(strShaderFile.c_str(), std::ios::in);

    if (shaderStream.is_open()) {
        std::string Line;

        while (getline(shaderStream, Line))
            shaderCode += "\n" + Line;

        shaderStream.close();
    } else {
        std::cerr << strShaderFile.c_str() <<
        " could not be read. Please ensure run directory if you used relative paths." << std::endl;
        getchar();
        return 0;
    }

    const char *shaderCodePtr = shaderCode.c_str();
    glShaderSource(shader, 1, &shaderCodePtr, nullptr);

    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE) {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetShaderInfoLog(shader, infoLogLength, nullptr, strInfoLog);
        const char *strShaderType = nullptr;

        switch (eShaderType) {
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

        std::cerr << strShaderType << " type shader " << strShaderFile.c_str() << " could not be compiled:\n" <<
        strInfoLog << std::endl;
        delete[] strInfoLog;

    }
    checkErrors("createShader");
    return shader;
}


GLuint GLHelper::createProgram(const std::vector<GLuint> &shaderList) {
    GLuint program = glCreateProgram();

    for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++) {
        glAttachShader(program, shaderList[iLoop]);
    }

    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE) {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetProgramInfoLog(program, infoLogLength, nullptr, strInfoLog);
        std::cerr << "Linking failed: \n" << strInfoLog << std::endl;
        delete[] strInfoLog;
    } else {
        std::cout << "Program compiled successfully" << std::endl;
    }

    for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++) {
        glDetachShader(program, shaderList[iLoop]);
    }

    checkErrors("createProgram");
    return program;
}


GLuint GLHelper::initializeProgram(const std::string &vertexShaderFile, const std::string &geometryShaderFile, const std::string &fragmentShaderFile,
                                   std::map<std::string, Uniform *> &uniformMap) {
    GLuint program;
    std::vector<GLuint> shaderList;
    checkErrors("before create shaders");
    shaderList.push_back(createShader(GL_VERTEX_SHADER, vertexShaderFile));
    if(!geometryShaderFile.empty()){
        shaderList.push_back(createShader(GL_GEOMETRY_SHADER, geometryShaderFile));
    }
    shaderList.push_back(createShader(GL_FRAGMENT_SHADER, fragmentShaderFile));


    program = createProgram(shaderList);
    std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);

    fillUniformMap(program, uniformMap);
    attachUBOs(program);

    checkErrors("initializeProgram");
    return program;
}

void GLHelper::fillUniformMap(const GLuint program, std::map<std::string, GLHelper::Uniform *> &uniformMap) const {
    GLint i;
    GLint count;

    GLint size; // size of the variable
    GLenum type; // type of the variable (float, vec3 or mat4, etc)

    GLint maxLength;

    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

    GLchar name[maxLength]; // variable name in GLSL
    GLsizei length; // name length

    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
    //std::cout << "Active Uniforms:" << count << std::endl;

    uint32_t uniformLocation;
    for (i = 0; i < count; i++)
    {
        glGetActiveUniform(program, (GLuint)i, maxLength, &length, &size, &type, name);
        uniformLocation = glGetUniformLocation(program, name);

        //std::cout << "Uniform " << i << " Location: " << uniformLocation << " Type: " << type << " Name: " << name << std::endl;
        uniformMap[name] = new Uniform(uniformLocation, name, type, size);
    }
}

void GLHelper::attachUBOs(const GLuint program) const {//Attach the light block to our UBO
    GLuint lightAttachPoint = 0, playerAttachPoint = 1;

    int uniformIndex = glGetUniformBlockIndex(program, "LightSourceBlock");
    if (uniformIndex >= 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
        glUniformBlockBinding(program, uniformIndex, lightAttachPoint);
        glBindBufferRange(GL_UNIFORM_BUFFER, lightAttachPoint, lightUBOLocation, 0,
                          lightUniformSize * NR_POINT_LIGHTS);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    int uniformIndex2 = glGetUniformBlockIndex(program, "PlayerTransformBlock");
    if (uniformIndex2 >= 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, playerUBOLocation);
        glUniformBlockBinding(program, uniformIndex2, playerAttachPoint);
        glBindBufferRange(GL_UNIFORM_BUFFER, playerAttachPoint, playerUBOLocation, 0,
                          3 * sizeof(glm::mat4) + sizeof(glm::vec4));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }


}


GLHelper::GLHelper(Options *options): options(options) {
    GLenum rev;
    error = GL_NO_ERROR;
    glewExperimental = GL_TRUE;
    rev = glewInit();

    if (GLEW_OK != rev) {
        std::cout << "GLEW init Error: " << glewGetErrorString(rev) << std::endl;
        exit(1);
    } else {
        std::cout << "GLEW Init: Success!" << std::endl;
    }
    checkErrors("after Context creation");

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureImageUnits);

    std::cout << "Maximum number of texture image units is " << maxTextureImageUnits << std::endl;
    state = new OpenglState(maxTextureImageUnits);

    lightProjectionMatrixDirectional = glm::ortho(options->getLightOrthogonalProjectionValues().x,
                                                  options->getLightOrthogonalProjectionValues().y,
                                                  options->getLightOrthogonalProjectionValues().z,
                                                  options->getLightOrthogonalProjectionValues().w,
                                                  options->getLightOrthogonalProjectionNearPlane(),
                                                  options->getLightOrthogonalProjectionFarPlane());

    lightProjectionMatrixPoint = glm::perspective(glm::radians(90.0f),
                                                  options->getLightPerspectiveProjectionValues().x,
                                                  options->getLightPerspectiveProjectionValues().y,
                                                  options->getLightPerspectiveProjectionValues().z);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    // Setup
    //glDisable(GL_CULL_FACE);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDepthRange(0.0f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);



    std::cout << "Rendererer: " << glGetString(GL_RENDERER) << std::endl  // e.g. Intel HD Graphics 3000 OpenGL Engine
              << "GL version: " << glGetString(GL_VERSION) << std::endl;    // e.g. 3.2 INTEL-8.0.61

    std::cout << "Supported GLSL version is "<< (char *) glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    GLint n, i;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    std::cout << "found " << n << " extensions." << std::endl;
    bool isCubeMapArraySupported = false;
    char extensionNameBuffer[100];
    for (i = 0; i < n; i++) {
        glGetStringi(GL_EXTENSIONS, i);
        sprintf(extensionNameBuffer, "%s", glGetStringi(GL_EXTENSIONS, i));
        if(std::strcmp(extensionNameBuffer, "GL_ARB_texture_cube_map_array") == 0) {
            isCubeMapArraySupported = true;
            break;
        }
    }
    if(!isCubeMapArraySupported) {
        std::cerr << "Cubemap array support is mandatory, exiting.. " << std::endl;
        exit(-1);
    }

    std::cout << "Cubemap array support is present. " << std::endl;

    //create the Light Uniform Buffer Object for later usage
    glGenBuffers(1, &lightUBOLocation);

    glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
    glBufferData(GL_UNIFORM_BUFFER, lightUniformSize * NR_POINT_LIGHTS, nullptr,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //create player transforms uniform buffer object
    glGenBuffers(1, &playerUBOLocation);
    glBindBuffer(GL_UNIFORM_BUFFER, playerUBOLocation);
    //FIXME the value below should be a constant at header
    glBufferData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4) + sizeof(glm::vec4), nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //create depth buffer and texture for directional shadow map
    glGenFramebuffers(1, &depthOnlyFrameBufferDirectional);
    glGenTextures(1, &depthMapDirectional);
    glBindTexture(GL_TEXTURE_2D_ARRAY, depthMapDirectional);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, options->getShadowWidth(), options->getShadowHeight(), NR_POINT_LIGHTS, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[] = {1.0, 1.0, 1.0, 1.0};
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBufferDirectional);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMapDirectional, 0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //create depth buffer and texture for point shadow map
    glGenFramebuffers(1, &depthOnlyFrameBufferPoint);
    // create depth cubemap texture
    glGenTextures(1, &depthCubemapPoint);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, depthCubemapPoint);
    glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, 0, GL_DEPTH_COMPONENT, options->getShadowWidth(), options->getShadowHeight(), NR_POINT_LIGHTS*6, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //if we clamp to border, then the edges become visible. it should be clamped to edge
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameterfv(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBufferPoint);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemapPoint, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkErrors("Constructor");
}

GLuint GLHelper::generateBuffer(const GLuint number) {
    GLuint bufferID;
    glGenBuffers(number, &bufferID);
    bufferObjects.push_back(bufferID);

    checkErrors("generateBuffer");
    return bufferID;
}

bool GLHelper::deleteBuffer(const GLuint number, const GLuint bufferID) {
    if (glIsBuffer(bufferID)) {
        glDeleteBuffers(number, &bufferID);
        checkErrors("deleteBuffer");
        return true;
    }
    checkErrors("deleteBuffer");
    return false;
}

bool GLHelper::freeBuffer(const GLuint bufferID) {
    for (int i = 0; i < bufferObjects.size(); ++i) {
        if (bufferObjects[i] == bufferID) {
            deleteBuffer(1, bufferObjects[i]);
            bufferObjects[i] = bufferObjects[bufferObjects.size() - 1];
            bufferObjects.pop_back();
            checkErrors("freeBuffer");
            return true;
        }
    }
    checkErrors("freeBuffer");
    return false;
}

GLuint GLHelper::generateVAO(const GLuint number) {
    GLuint bufferID;
    glGenVertexArrays(number, &bufferID);
    vertexArrays.push_back(bufferID);
    checkErrors("generateVAO");
    return bufferID;
}

bool GLHelper::deleteVAO(const GLuint number, const GLuint bufferID) {
    if (glIsBuffer(bufferID)) {
        glDeleteVertexArrays(number, &bufferID);
        checkErrors("deleteVAO");
        return true;
    }
    checkErrors("deleteVAO");
    return false;
}

bool GLHelper::freeVAO(const GLuint bufferID) {
    for (int i = 0; i < vertexArrays.size(); ++i) {
        if (vertexArrays[i] == bufferID) {
            deleteBuffer(1, vertexArrays[i]);
            vertexArrays[i] = vertexArrays[vertexArrays.size() - 1];
            vertexArrays.pop_back();
            checkErrors("freeVAO");
            return true;
        }
    }
    checkErrors("freeVAO");
    return false;
}

void GLHelper::bufferVertexData(const std::vector<glm::vec3> &vertices,
                                const std::vector<glm::mediump_uvec3> &faces,
                                uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer,
                                uint_fast32_t &ebo) {

    // Set up the element array buffer
    ebo = generateBuffer(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(glm::mediump_uvec3), faces.data(), GL_STATIC_DRAW);

    // Set up the vertex attributes
    //FIXME this temp should not be needed, but uint_fast32_t requires a cast. re evaluate using uint32_t
    uint32_t temp;
    glGenVertexArrays(1, &temp);
    glBindVertexArray(temp);
    vao = temp;
    vbo = generateBuffer(1);
    bufferObjects.push_back(vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    //glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize, vertexData);
    glVertexAttribPointer(attachPointer, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(attachPointer);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    checkErrors("bufferVertexData");
}

void GLHelper::bufferNormalData(const std::vector<glm::vec3> &normals,
                                uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) {
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);

    glBindVertexArray(vao);
    glVertexAttribPointer(attachPointer, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);
    checkErrors("bufferVertexColor");
}

void GLHelper::bufferExtraVertexData(const std::vector<glm::vec4> &extraData,
                                     uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) {
    bufferExtraVertexData(4, GL_FLOAT, extraData.size() * sizeof(glm::vec4), extraData.data(), vao, vbo, attachPointer);
    checkErrors("bufferVertexDataVec4");
}

void GLHelper::bufferExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData,
                                     uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) {
    bufferExtraVertexData(4, GL_UNSIGNED_INT, extraData.size() * sizeof(glm::lowp_uvec4), extraData.data(), vao, vbo,
                          attachPointer);
    checkErrors("bufferVertexDataIVec4");
}

void GLHelper::bufferExtraVertexData(uint_fast32_t elementPerVertexCount, GLenum elementType, uint_fast32_t dataSize,
                                     const void *extraData, uint_fast32_t &vao, uint_fast32_t &vbo,
                                     const uint_fast32_t attachPointer) {
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, dataSize, extraData, GL_STATIC_DRAW);

    glBindVertexArray(vao);
    switch (elementType) {
        case GL_UNSIGNED_INT:
        case GL_INT:
            glVertexAttribIPointer(attachPointer, elementPerVertexCount, elementType, 0, 0);
            break;
        default:
            glVertexAttribPointer(attachPointer, elementPerVertexCount, elementType, GL_FALSE, 0, 0);
    }

    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);
    checkErrors("bufferExtraVertexDataInternal");
}

void GLHelper::bufferVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates,
                                              uint_fast32_t &vao, uint_fast32_t &vbo, const uint_fast32_t attachPointer) {
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, textureCoordinates.size() * sizeof(glm::vec2), textureCoordinates.data(),
                 GL_STATIC_DRAW);

    glBindVertexArray(vao);
    glVertexAttribPointer(attachPointer, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);
    checkErrors("bufferVertexTextureCoordinates");
}

void GLHelper::switchRenderToShadowMapDirectional(const unsigned int index) {
    glViewport(0, 0, options->getShadowWidth(), options->getShadowHeight());
    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBufferDirectional);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMapDirectional, 0, index);

    glClear(GL_DEPTH_BUFFER_BIT);

    glCullFace(GL_FRONT);
    checkErrors("switchRenderToShadowMapDirectional");

}

void GLHelper::switchRenderToShadowMapPoint(const glm::vec3 &lightPosition) {
    checkErrors("switchRenderToShadowMapPointBefore");
    glViewport(0, 0, options->getShadowWidth(), options->getShadowHeight());
    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBufferPoint);

    glCullFace(GL_FRONT);

    checkErrors("switchRenderToShadowMapPoint");
}


void GLHelper::switchRenderToDefault() {
    glViewport(0, 0, screenWidth, screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //we bind shadow map to last texture unit
    state->attach2DTextureArray(depthMapDirectional, maxTextureImageUnits - 1);
    state->attachCubemapArray(depthCubemapPoint, maxTextureImageUnits - 2);
    glCullFace(GL_BACK);
    checkErrors("switchRenderToDefault");
}

void GLHelper::render(const GLuint program, const GLuint vao, const GLuint ebo, const GLuint elementCount) {
    if (program == 0) {
        std::cerr << "No program render requested." << std::endl;
        return;
    }
    state->setProgram(program);

    // Set up for a glDrawElements call
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    //state->setProgram(0);

    checkErrors("render");
}

bool GLHelper::setUniform(const GLuint programID, const GLuint uniformID, const glm::mat4 &matrix) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniformMatrix4fv(uniformID, 1, GL_FALSE, glm::value_ptr(matrix));
        //state->setProgram(0);
        checkErrors("setUniformMatrix");
        return true;
    }

}


bool
GLHelper::setUniformArray(const GLuint programID, const GLuint uniformID, const std::vector<glm::mat4> &matrixArray) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        int elementCount = matrixArray.size();
        glUniformMatrix4fv(uniformID, elementCount, GL_FALSE, glm::value_ptr(matrixArray.at(0)));
        //state->setProgram(0);
        checkErrors("setUniformMatrixArray");
        return true;
    }

}

bool GLHelper::setUniform(const GLuint programID, const GLuint uniformID, const glm::vec3 &vector) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform3fv(uniformID, 1, glm::value_ptr(vector));
        //state->setProgram(0);
        checkErrors("setUniformVector");
        return true;
    }
}

bool GLHelper::setUniform(const GLuint programID, const GLuint uniformID, const float value) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform1f(uniformID, value);
        //state->setProgram(0);
        checkErrors("setUniformFloat");
        return true;
    }
}

bool GLHelper::setUniform(const GLuint programID, const GLuint uniformID, const int value) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform1i(uniformID, value);
        //state->setProgram(0);
        checkErrors("setUniformInt");
        return true;
    }
}

bool GLHelper::checkErrors(std::string callerFunc) {

    GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "FB status is " << fbStatus << std::endl;
    }
    bool hasError = false;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cerr << "error found on GL context while " << callerFunc << ":" << error << ":" << gluErrorString(error)
                  << std::endl;
        hasError = true;
    }
    return hasError;
}


GLHelper::~GLHelper() {
    for (int i = 0; i < bufferObjects.size(); ++i) {
        deleteBuffer(1, bufferObjects[i]);
    }

    deleteBuffer(1, lightUBOLocation);
    deleteBuffer(1, playerUBOLocation);
    deleteBuffer(1, depthMapDirectional);
    glDeleteFramebuffers(1, &depthOnlyFrameBufferDirectional); //maybe we should wrap this up too
    //state->setProgram(0);
}

void GLHelper::reshape() {
    //reshape actually checks for changes on options->
    this->screenHeight = options->getScreenHeight();
    this->screenWidth = options->getScreenWidth();
    glViewport(0, 0, options->getScreenWidth(), options->getScreenHeight());
    aspect = float(options->getScreenHeight()) / float(options->getScreenWidth());
    perspectiveProjectionMatrix = glm::perspective(45.0f, 1.0f / aspect, 0.1f, 100.0f);
    orthogonalProjectionMatrix = glm::ortho(0.0f, (float) options->getScreenWidth(), 0.0f, (float) options->getScreenHeight());
    checkErrors("reshape");
}

GLuint GLHelper::loadTexture(int height, int width, GLenum format, void *data) {
    GLuint texture;
    glGenTextures(1, &texture);
    state->activateTextureUnit(0);//this is the default working texture
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    checkErrors("loadTexture");
    return texture;
}

void GLHelper::attachTexture(unsigned int textureID, unsigned int attachPoint) {
    state->attachTexture(textureID, attachPoint);
    checkErrors("attachTexture");
}

void GLHelper::attachCubeMap(unsigned int cubeMapID, unsigned int attachPoint) {
    state->attachCubemap(cubeMapID, attachPoint);
    checkErrors("attachCubeMap");
}

bool GLHelper::deleteTexture(GLuint textureID) {
    if (glIsTexture(textureID)) {
        glDeleteTextures(1, &textureID);
        checkErrors("deleteTexture");
        return true;
    } else {
        checkErrors("deleteTexture");
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
    checkErrors("loadCubeMap");
    return cubeMap;
}

bool GLHelper::getUniformLocation(const GLuint programID, const std::string &uniformName, GLuint &location) {
    GLint rawLocation = glGetUniformLocation(programID, uniformName.c_str());
    if (!checkErrors("getUniformLocation")) {
        if (rawLocation >= 0) {
            location = rawLocation;
            return true;
        } else {
            std::cerr << "No error found, but uniform[" << uniformName << "] can not be located " << std::endl;

        }
    }
    return false;
}

void GLHelper::createDebugVAOVBO(uint32_t &vao, uint32_t &vbo, uint32_t bufferSize) {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, bufferSize * sizeof(Line), nullptr, GL_DYNAMIC_DRAW);
    //glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize, vertexData);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 28, nullptr); //position
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 28, (void*)12); //color
    glVertexAttribPointer(2, 1, GL_INT,  GL_FALSE, 28, (void*)24); //needsCameraTransform
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    checkErrors("createDebugVAOVBO");
}

/**
 * This method draws lines, but is refreshes whole buffer. In a better world, these values should be
 * loaded with other values, and rendered without the heavy load. but for now, it is faster then the older version.
 *
 * PLEASE NOTE, IF LINE SIZE IS BIGGER THEN ASSIGNED, UNDEFINED BEHAVIOUR
 *
 * @param program - glsl program used to render
 * @param vao     - vao
 * @param vbo     - vbo that the lines will be buffered. It should be
 * @param lines   - line vector
 */
void GLHelper::drawLines(GLSLProgram &program, uint32_t vao, uint32_t vbo, const std::vector<Line> &lines) {
    state->setProgram(program.getID());
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, lines.size() * sizeof(Line), lines.data());
    program.setUniform("cameraTransformMatrix", perspectiveProjectionMatrix * cameraMatrix);

    glDrawArrays(GL_LINES, 0, lines.size()*2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    checkErrors("drawLines");
}

void GLHelper::setLight(const Light &light, const int i) {
    glm::mat4 lightView = glm::lookAt(light.getPosition(),
                                      glm::vec3(0.0f, 0.0f, 0.0f),
                                      glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 lightSpaceMatrix = lightProjectionMatrixDirectional * lightView;
    GLint lightType;
    switch (light.getLightType()) {
        case Light::DIRECTIONAL:
            lightType = 0;
            break;
        case Light::POINT:
            lightType = 1;
            break;
    }

    //FIXME this calculation should be a part of Light class, and should not be updated at every frame
    glm::mat4 shadowTransforms[6];
    shadowTransforms[0] =lightProjectionMatrixPoint *
                               glm::lookAt(light.getPosition(), light.getPosition() + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0));
    shadowTransforms[1] =lightProjectionMatrixPoint *
                               glm::lookAt(light.getPosition(), light.getPosition() + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0));
    shadowTransforms[2] =lightProjectionMatrixPoint *
                               glm::lookAt(light.getPosition(), light.getPosition() + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
    shadowTransforms[3] =lightProjectionMatrixPoint *
                               glm::lookAt(light.getPosition(), light.getPosition() + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0));
    shadowTransforms[4] =lightProjectionMatrixPoint *
                               glm::lookAt(light.getPosition(), light.getPosition() + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0));
    shadowTransforms[5] =lightProjectionMatrixPoint *
                               glm::lookAt(light.getPosition(), light.getPosition() + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0));


    //std::cout << "light type is " << lightType << std::endl;
    //std::cout << "size is " << sizeof(GLint) << std::endl;

    glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
    glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize,
                    sizeof(glm::mat4) * 6, shadowTransforms);
    glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 6,
                    sizeof(glm::mat4), glm::value_ptr(lightSpaceMatrix));
    glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 7,
                    sizeof(glm::vec3), &light.getPosition());
    glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 7 + sizeof(glm::vec4),
                    sizeof(glm::vec3), &light.getColor());
    glBufferSubData(GL_UNIFORM_BUFFER, i * lightUniformSize + sizeof(glm::mat4) * 7 + sizeof(glm::vec4) + sizeof(glm::vec3),
                    sizeof(GLint), &lightType);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkErrors("setLight");
}

void GLHelper::setPlayerMatrices(const glm::vec3 &cameraPosition, const glm::mat4 &cameraTransform) {
    this->cameraMatrix = cameraTransform;
    glBindBuffer(GL_UNIFORM_BUFFER, playerUBOLocation);
    glBufferSubData(GL_UNIFORM_BUFFER, 0 * sizeof(glm::mat4), sizeof(glm::mat4), &cameraMatrix);//changes with camera
    glBufferSubData(GL_UNIFORM_BUFFER, 1 * sizeof(glm::mat4), sizeof(glm::mat4), &perspectiveProjectionMatrix);//never changes
    glm::mat4 viewMatrix = perspectiveProjectionMatrix * cameraMatrix;
    glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4), &viewMatrix);//changes with camera
    glBufferSubData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), sizeof(glm::vec3), &cameraPosition);//changes with camera
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkErrors("setPlayerMatrices");
}


