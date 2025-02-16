//
// Created by engin on 24.09.2019.
//

#include "OpenGLGraphics.h"
#include "API/Graphics/GraphicsProgram.h"

#include "Material.h"
#include "Graphics/Texture.h"

std::shared_ptr<GraphicsInterface> createGraphicsBackend(OptionsUtil::Options* options) {
    return std::make_shared<OpenGLGraphics>(options);
}

GLuint OpenGLGraphics::createShader(GLenum eShaderType, const std::string &strShaderContent) {
    GLuint shader = glCreateShader(eShaderType);

    const char *shaderCodePtr = strShaderContent.c_str();
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

        std::cerr << strShaderType << " type shader " << strShaderContent.c_str() << " could not be compiled:\n" <<
                  strInfoLog << std::endl;
        delete[] strInfoLog;

    }
    checkErrors("createShader");
    return shader;
}


GLuint OpenGLGraphics::createProgram(const std::vector<GLuint> &shaderList) {
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
        //std::cout << "Program compiled successfully" << std::endl;
    }

    for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++) {
        glDetachShader(program, shaderList[iLoop]);
    }

    checkErrors("createProgram");
    return program;
}

uint32_t OpenGLGraphics::createGraphicsProgram(const std::string &vertexShaderContent, const std::string &geometryShaderContent, const std::string &fragmentShaderContent) {
    GLuint program;
    std::vector<GLuint> shaderList;
    checkErrors("before create shaders");
    shaderList.push_back(createShader(GL_VERTEX_SHADER, vertexShaderContent));
    if(!geometryShaderContent.empty()){
        shaderList.push_back(createShader(GL_GEOMETRY_SHADER, geometryShaderContent));
    }
    shaderList.push_back(createShader(GL_FRAGMENT_SHADER, fragmentShaderContent));

    program = createProgram(shaderList);
    std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);
    attachGeneralUBOs(program);
    checkErrors("createGraphicsProgram");
    return program;
}


void OpenGLGraphics::initializeProgramAsset(const uint32_t programId,
                                            std::unordered_map<std::string, std::shared_ptr<Uniform>> &uniformMap, std::unordered_map<std::string, uint32_t> &attributesMap,
                                            std::unordered_map<std::string, std::pair<Uniform::VariableTypes, FrameBufferAttachPoints>> &outputMap) {

    fillUniformAndOutputMaps(programId, uniformMap, attributesMap, outputMap);
    checkErrors("initializeProgramAsset");
}

void OpenGLGraphics::destroyProgram(uint32_t programID) {
    glDeleteProgram(programID);
    checkErrors("destroyProgram");
}

void OpenGLGraphics::fillUniformAndOutputMaps(const GLuint program,
        std::unordered_map<std::string, std::shared_ptr<Uniform>> &uniformMap,
        std::unordered_map<std::string, uint32_t> &attributesMap,
        std::unordered_map<std::string, std::pair<Uniform::VariableTypes, FrameBufferAttachPoints>> &outputMap) {
    GLint i;
    GLint count;

    GLint size; // size of the variable
    GLenum type; // type of the variable (float, vec3 or mat4, etc)

    GLint maxLength;

    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

    GLchar* name = new GLchar[maxLength]; // variable name in GLSL
    GLsizei length; // name length

    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
    //std::cout << "Active Uniforms:" << count << std::endl;

    uint32_t uniformLocation;
    for (i = 0; i < count; i++) {
        glGetActiveUniform(program, (GLuint)i, maxLength, &length, &size, &type, name);
        uniformLocation = glGetUniformLocation(program, name);

        //std::cout << "Uniform " << i << " Location: " << uniformLocation << " Type: " << type << " Name: " << name << std::endl;
        uniformMap[name] = std::make_shared<Uniform>(uniformLocation, name, getVariableType(type), size);
    }


    delete[] name;

    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);
    name = new GLchar[maxLength];

    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &count);
    //std::cout << "Active Uniforms:" << count << std::endl;

    uint32_t attributeLocation;
    for (i = 0; i < count; i++) {
        glGetActiveAttrib(program, (GLuint)i, maxLength, &length, &size, &type, name);
        attributeLocation = glGetAttribLocation(program, name);

        attributesMap[name] = attributeLocation;
    }

    delete[] name;

    if (isProgramInterfaceQuerySupported) {
        glGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_MAX_NAME_LENGTH, &maxLength);
        name = new GLchar[maxLength];

        glGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &count);
        bool depthAdded = false;
        for(i = 0; i < count; i++) {
            glGetProgramResourceName(program, GL_PROGRAM_OUTPUT, i, maxLength, &size, name);
            const GLenum properties[2] = {GL_TYPE, GL_LOCATION};
            GLint queryResults[2];
            Uniform::VariableTypes variableType;
            glGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, i, 2, properties, 2, nullptr, queryResults);
            variableType = getSamplerVariableType(queryResults);
            FrameBufferAttachPoints attachPoint;
            switch (queryResults[1]) {
                case 0:     attachPoint=FrameBufferAttachPoints::COLOR0;  break;
                case 1:     attachPoint=FrameBufferAttachPoints::COLOR1;  break;
                case 2:     attachPoint=FrameBufferAttachPoints::COLOR2;  break;
                case 3:     attachPoint=FrameBufferAttachPoints::COLOR3;  break;
                case 4:     attachPoint=FrameBufferAttachPoints::COLOR4;  break;
                case 5:     attachPoint=FrameBufferAttachPoints::COLOR5;  break;
                case 6:     attachPoint=FrameBufferAttachPoints::COLOR6;  break;
                default:    attachPoint=FrameBufferAttachPoints::NONE;
            }

            if(strcmp(name, "gl_FragDepth") == 0) {
                depthAdded = true;
                outputMap["Depth"] = std::make_pair(variableType, FrameBufferAttachPoints::DEPTH);
            } else {
                outputMap[name] = std::make_pair(variableType, attachPoint);
            }

        }
        if(!depthAdded) {
            /**
             * FIXME this is causing a lot of headache. We need a better way to detect if a program writes to depth or not.
             */
            outputMap["Depth"] =std::make_pair(Uniform::VariableTypes::TEXTURE_2D, FrameBufferAttachPoints::DEPTH);//Depth is always written
        }
        delete[] name;
    }

    checkErrors("fillUniformAndOutputMaps");
}


Uniform::VariableTypes OpenGLGraphics::getVariableType(const GLenum typeEnum) const {
    switch (typeEnum) {
        case GL_SAMPLER_CUBE:               return Uniform::VariableTypes::CUBEMAP;
        case GL_SAMPLER_CUBE_MAP_ARRAY_ARB: return Uniform::VariableTypes::CUBEMAP_ARRAY;
        case GL_SAMPLER_2D:                 return Uniform::VariableTypes::TEXTURE_2D;
        case GL_SAMPLER_2D_ARRAY:           return Uniform::VariableTypes::TEXTURE_2D_ARRAY;
        case GL_BOOL:                       return Uniform::VariableTypes::BOOL;
        case GL_INT:                        return Uniform::VariableTypes::INT;
        case GL_FLOAT:                      return Uniform::VariableTypes::FLOAT;
        case GL_FLOAT_VEC2:                 return Uniform::VariableTypes::FLOAT_VEC2;
        case GL_FLOAT_VEC3:                 return Uniform::VariableTypes::FLOAT_VEC3;
        case GL_FLOAT_VEC4:                 return Uniform::VariableTypes::FLOAT_VEC4;
        case GL_FLOAT_MAT4:                 return Uniform::VariableTypes::FLOAT_MAT4;
        default:                            return Uniform::VariableTypes::UNDEFINED;
    }
}

Uniform::VariableTypes OpenGLGraphics::getSamplerVariableType(const GLint *queryResults) const {
    Uniform::VariableTypes variableType;
    switch (queryResults[0]) {
        case GL_SAMPLER_CUBE:
            variableType = Uniform::VariableTypes::CUBEMAP;
            break;
        case GL_SAMPLER_CUBE_MAP_ARRAY_ARB:
            variableType = Uniform::VariableTypes::CUBEMAP_ARRAY;
            break;
        case GL_SAMPLER_2D:
            variableType = Uniform::VariableTypes::TEXTURE_2D;
            break;
        case GL_SAMPLER_2D_ARRAY:
            variableType = Uniform::VariableTypes::TEXTURE_2D_ARRAY;
            break;
        case GL_INT:
        case GL_FLOAT:
        case GL_FLOAT_VEC2:
        case GL_FLOAT_VEC3:
        case GL_FLOAT_VEC4:
            variableType = Uniform::VariableTypes::TEXTURE_2D;
            break;
        default:
            variableType = Uniform::VariableTypes::UNDEFINED;
    }
    return variableType;
}

void OpenGLGraphics::attachModelTexture(const uint32_t program) {
    GLint allModelsAttachPoint = glGetUniformLocation(program, "allModelTransformsTexture");
    this->setUniform(program, allModelsAttachPoint, maxTextureImageUnits-3);
    state->attachTexture(allModelTransformsTexture, maxTextureImageUnits-3);
    checkErrors("attachModelTexture");
}

void OpenGLGraphics::attachRigTexture(const uint32_t program) {
    GLint allBonesAttachPoint = glGetUniformLocation(program, "allBoneTransformsTexture");
    this->setUniform(program, allBonesAttachPoint, maxTextureImageUnits-4);
    state->attachTexture(allBoneTransformsTexture, maxTextureImageUnits-4);
    checkErrors("attachRigTexture");
}

void OpenGLGraphics::attachModelIndicesUBO(const uint32_t programID) {
    GLuint allModelIndexesAttachPoint = 8;

    int uniformIndex = glGetUniformBlockIndex(programID, "ModelIndexBlock");
    if (uniformIndex >= 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, allModelIndexesUBOLocation);
        glUniformBlockBinding(programID, uniformIndex, allModelIndexesAttachPoint);
        glBindBufferRange(GL_UNIFORM_BUFFER, allModelIndexesAttachPoint, allModelIndexesUBOLocation, 0,
                          sizeof(uint32_t) * NR_MAX_MODELS);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
    checkErrors("attachModelIndicesUBO");
}

void OpenGLGraphics::attachMaterialUBO(const uint32_t program){
    GLuint allMaterialsAttachPoint = 9;

    int uniformIndex = glGetUniformBlockIndex(program, "MaterialInformationBlock");
    if (uniformIndex >= 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, allMaterialsUBOLocation);
        glUniformBlockBinding(program, uniformIndex, allMaterialsAttachPoint);
        glBindBufferBase(GL_UNIFORM_BUFFER, allMaterialsAttachPoint, allMaterialsUBOLocation);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    checkErrors("attachMaterialUBO");
}

void OpenGLGraphics::attachGeneralUBOs(const GLuint program){//Attach the light block to our UBO

    GLuint lightAttachPoint = 0, playerAttachPoint = 1;

    int uniformIndex = glGetUniformBlockIndex(program, "LightSourceBlock");
    if (uniformIndex >= 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
        glUniformBlockBinding(program, uniformIndex, lightAttachPoint);
        glBindBufferRange(GL_UNIFORM_BUFFER, lightAttachPoint, lightUBOLocation, 0,
                          lightUniformSize * NR_TOTAL_LIGHTS);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    int uniformIndex2 = glGetUniformBlockIndex(program, "PlayerTransformBlock");
    if (uniformIndex2 >= 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, playerUBOLocation);
        glUniformBlockBinding(program, uniformIndex2, playerAttachPoint);
        glBindBufferRange(GL_UNIFORM_BUFFER, playerAttachPoint, playerUBOLocation, 0,
                          playerUniformSize);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}


OpenGLGraphics::OpenGLGraphics(OptionsUtil::Options *options): GraphicsInterface(options), options(options) {}

OpenGLGraphics::ContextInformation OpenGLGraphics::getContextInformation() {
    GraphicsInterface::ContextInformation contextInformation;
    contextInformation.SDL_GL_ACCELERATED_VISUAL = 1;
    contextInformation.SDL_GL_CONTEXT_MAJOR_VERSION = 3;
    contextInformation.SDL_GL_CONTEXT_MINOR_VERSION = 3;
    contextInformation.SDL_GL_CONTEXT_PROFILE_MASK = 1;
    contextInformation.SDL_GL_CONTEXT_FLAGS = 1;
    contextInformation.shaderHeader = "#version 330";
    return contextInformation;
}

bool OpenGLGraphics::createGraphicsBackend() {

    this->screenHeight = options->getScreenHeight();
    this->screenWidth = options->getScreenWidth();
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
    int foundExtensionCount = 0;
    for (i = 0; i < n; i++) {
        sprintf(extensionNameBuffer, "%s", glGetStringi(GL_EXTENSIONS, i));
        if(std::strcmp(extensionNameBuffer, "GL_ARB_texture_cube_map_array") == 0) {
            isCubeMapArraySupported = true;
            ++foundExtensionCount;
        }
        if(std::strcmp(extensionNameBuffer, "GL_ARB_program_interface_query") == 0) {
            isProgramInterfaceQuerySupported = true;
            ++foundExtensionCount;
        }

        if(std::strcmp(extensionNameBuffer, "ARB_framebuffer_no_attachments") == 0) {
            isFrameBufferParameterSupported = true;
            ++foundExtensionCount;
        }
        if(std::strcmp(extensionNameBuffer, "GL_ARB_debug_output") == 0) {
            isDebugOutputSupported = true;
            ++foundExtensionCount;
        }

        if (foundExtensionCount == 4) {
            break;
        }
    }
    if(!isCubeMapArraySupported) {
        std::cerr << "Cubemap array support is mandatory, exiting.. " << std::endl;
        exit(-1);
    }

    if(!isProgramInterfaceQuerySupported) {
        std::cerr << "Program Interface Query support is missing, auto discovery of shaders will not work... " << std::endl;
    }

    std::cout << "Cubemap array support is present. " << std::endl;

    GLint uniformBufferAlignSize = 0;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformBufferAlignSize);

    /**
     * When you are using the buffer to keep multiple items, but bind single item, that item has to be aligned to this value
     * If actually put all of them in an array and use the array for binding, it will be aligned to the definition you have (std140 aligns to 16)
     * Don't need this after the material changes, but keeping it here incase I need it again.
     *
    if(uniformBufferAlignSize > materialUniformSize) {
        materialUniformSize = uniformBufferAlignSize;
    } else {
        //it is possible that they are not aligning, align
        if(materialUniformSize % uniformBufferAlignSize == 0 ) {
            //aligned
        } else {
            //MU size 18, alignment 16 -> we need 32
            materialUniformSize = ((materialUniformSize / uniformBufferAlignSize)+1) * uniformBufferAlignSize;
        }
    }
    */


    std::cout << "Uniform alignment size is " << uniformBufferAlignSize << std::endl;

    GLint maxVertexUniformBlockCount = 0;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &maxVertexUniformBlockCount);

    std::cout << "Uniform maxVertexUniformBlockCount size is " << maxVertexUniformBlockCount << std::endl;

    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

    std::cout << "Uniform maxTextureSize size is " << maxTextureSize << std::endl;

    if(maxTextureSize < NR_MAX_MODELS * 4) { // Each model has its transform in a texture, 4 channels, 4 elements
        std::cerr << "Maximum number of models is set higher than supported texture size. This will cause errors, black screens or crashing." << std::endl;
    }

    //create the Light Uniform Buffer Object for later usage
    glGenBuffers(1, &lightUBOLocation);
    glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
    std::vector<GLubyte> emptyData(lightUniformSize * NR_TOTAL_LIGHTS, 0);
    glBufferData(GL_UNIFORM_BUFFER, lightUniformSize * NR_TOTAL_LIGHTS, &emptyData[0], GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //create player transforms uniform buffer object
    glGenBuffers(1, &playerUBOLocation);
    glBindBuffer(GL_UNIFORM_BUFFER, playerUBOLocation);
    glBufferData(GL_UNIFORM_BUFFER, playerUniformSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //create material uniform buffer object
    glGenBuffers(1, &allMaterialsUBOLocation);
    glBindBuffer(GL_UNIFORM_BUFFER, allMaterialsUBOLocation);
    glBufferData(GL_UNIFORM_BUFFER, materialUniformSize * NR_MAX_MATERIALS, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenTextures(1, &allBoneTransformsTexture);
    state->activateTextureUnit(maxTextureImageUnits-4);
    glBindTexture(GL_TEXTURE_2D, allBoneTransformsTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4 * NR_BONE, NR_MAX_MODELS, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    state->activateTextureUnit(0);

    glGenTextures(1, &allModelTransformsTexture);
    state->activateTextureUnit(maxTextureImageUnits-3);
    glBindTexture(GL_TEXTURE_2D, allModelTransformsTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4 * NR_MAX_MODELS, 2, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    state->activateTextureUnit(0);

    //create model index uniform buffer object
    glGenBuffers(1, &allModelIndexesUBOLocation);
    glBindBuffer(GL_UNIFORM_BUFFER, allModelIndexesUBOLocation);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(uint32_t) * NR_MAX_MODELS, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    frustumPlanes.resize(6);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);//clear everything before we start

    checkErrors("Constructor");
    return true;
}

GLuint OpenGLGraphics::generateBuffer(const GLuint number) {
    GLuint bufferID;
    glGenBuffers(number, &bufferID);
    bufferObjects.push_back(bufferID);

    checkErrors("generateBuffer");
    return bufferID;
}

bool OpenGLGraphics::deleteBuffer(const GLuint number, const GLuint bufferID) {
    if (glIsBuffer(bufferID)) {
        glDeleteBuffers(number, &bufferID);
        checkErrors("deleteBuffer");
        return true;
    }
    checkErrors("deleteBuffer");
    return false;
}

bool OpenGLGraphics::freeBuffer(const GLuint bufferID) {
    for (unsigned int i = 0; i < bufferObjects.size(); ++i) {
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

GLuint OpenGLGraphics::generateVAO(const GLuint number) {
    GLuint bufferID;
    glGenVertexArrays(number, &bufferID);
    vertexArrays.push_back(bufferID);
    checkErrors("generateVAO");
    return bufferID;
}

bool OpenGLGraphics::deleteVAO(const GLuint number, const GLuint bufferID) {
    if (glIsBuffer(bufferID)) {
        glDeleteVertexArrays(number, &bufferID);
        checkErrors("deleteVAO");
        return true;
    }
    checkErrors("deleteVAO");
    return false;
}

bool OpenGLGraphics::freeVAO(const GLuint bufferID) {
    for (unsigned int i = 0; i < vertexArrays.size(); ++i) {
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

void OpenGLGraphics::bufferVertexData(const std::vector<glm::vec3> &vertices,
                                      const std::vector<glm::mediump_uvec3> &faces,
                                      uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer,
                                      uint32_t &ebo) {

    //FIXME this temp should not be needed, but uint_fast32_t requires a cast. re evaluate using uint32_t
    uint32_t temp;
    glGenVertexArrays(1, &temp);
    glBindVertexArray(temp);
    vao = temp;

    // Set up the element array buffer
    ebo = generateBuffer(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(glm::mediump_uvec3), faces.data(), GL_STATIC_DRAW);

    // Set up the vertex attributes
    vbo = generateBuffer(1);
    bufferObjects.push_back(vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(attachPointer, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);
    checkErrors("bufferVertexData");
}

void OpenGLGraphics::bufferNormalData(const std::vector<glm::vec3> &normals,
                                      uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) {
    glBindVertexArray(vao);
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(attachPointer, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);
    checkErrors("bufferNormalData");
}

void OpenGLGraphics::bufferExtraVertexData(const std::vector<glm::vec4> &extraData,
                                           uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) {
    bufferExtraVertexData(4, GL_FLOAT, extraData.size() * sizeof(glm::vec4), extraData.data(), vao, vbo, attachPointer);
    checkErrors("bufferVertexDataVec4");
}

void OpenGLGraphics::bufferExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData,
                                           uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) {
    bufferExtraVertexData(4, GL_UNSIGNED_INT, extraData.size() * sizeof(glm::lowp_uvec4), extraData.data(), vao, vbo,
                          attachPointer);
    checkErrors("bufferVertexDataIVec4");
}

void OpenGLGraphics::bufferExtraVertexData(uint32_t elementPerVertexCount, GLenum elementType, uint32_t dataSize,
                                              const void *extraData, uint32_t &vao, uint32_t &vbo,
                                              const uint32_t attachPointer) {
    glBindVertexArray(vao);
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, dataSize, extraData, GL_STATIC_DRAW);

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

void OpenGLGraphics::bufferVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates,
                                                    uint32_t &vao, uint32_t &vbo, const uint32_t attachPointer) {
    glBindVertexArray(vao);
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, textureCoordinates.size() * sizeof(glm::vec2), textureCoordinates.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(attachPointer, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);
    checkErrors("bufferVertexTextureCoordinates");
}

void
OpenGLGraphics::updateVertexData(const std::vector<glm::vec3> &vertices, const std::vector<glm::mediump_uvec3> &faces,
                                 uint32_t &vbo, uint32_t &ebo) {
    // Set up the element array buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(glm::mediump_uvec3), faces.data(), GL_STATIC_DRAW);

    // Set up the vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    checkErrors("updateVertexData");
}
void OpenGLGraphics::updateNormalData(const std::vector<glm::vec3> &normals, uint32_t &vbo) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    checkErrors("updateNormalData");
}
void OpenGLGraphics::updateExtraVertexData(const std::vector<glm::vec4> &extraData, uint32_t &vbo) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, extraData.size() * sizeof(glm::vec4), extraData.data(), GL_STATIC_DRAW);
    checkErrors("updateExtraVertexDataV4");
}
void OpenGLGraphics::updateExtraVertexData(const std::vector<glm::lowp_uvec4> &extraData, uint32_t &vbo) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, extraData.size() * sizeof(glm::lowp_uvec4), extraData.data(), GL_STATIC_DRAW);
    checkErrors("updateExtraVertexDataIV4");
}
void OpenGLGraphics::updateVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates, uint32_t &vbo) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, textureCoordinates.size() * sizeof(glm::vec2), textureCoordinates.data(),
                 GL_STATIC_DRAW);
    checkErrors("updateVertexTextureCoordinates");
}

void
OpenGLGraphics::switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool depthTestEnabled, bool depthWriteEnabled, bool scissorEnabled,
                                  bool clearColor, bool clearDepth, CullModes cullMode, std::map<uint32_t, std::shared_ptr<Texture>> &inputs, const std::string &name) {
    popDebugGroup();
    pushDebugGroup(name);
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);


    if(depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    if(depthWriteEnabled) {
        glDepthMask(GL_TRUE);
    } else {
        glDepthMask(GL_FALSE);
    }
    if(scissorEnabled) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }

    if(clearColor && clearDepth) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else if(clearColor) {
        glClear(GL_COLOR_BUFFER_BIT);
    } else if(clearDepth) {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    for (auto inputIt = inputs.begin(); inputIt != inputs.end(); ++inputIt) {
        switch (inputIt->second->getType()) {
            case OpenGLGraphics::TextureTypes::T2D: state->attachTexture(inputIt->second->getTextureID(), inputIt->first); break;
            case OpenGLGraphics::TextureTypes::T2D_ARRAY: state->attach2DTextureArray(inputIt->second->getTextureID(), inputIt->first); break;
            case OpenGLGraphics::TextureTypes::TCUBE_MAP: state->attachCubemap(inputIt->second->getTextureID(), inputIt->first); break;
            case OpenGLGraphics::TextureTypes::TCUBE_MAP_ARRAY: state->attachCubemapArray(inputIt->second->getTextureID(), inputIt->first); break;
        }
    }
    switch (cullMode) {
        case OpenGLGraphics::CullModes::FRONT: glEnable(GL_CULL_FACE);glCullFace(GL_FRONT); break;
        case OpenGLGraphics::CullModes::BACK: glEnable(GL_CULL_FACE);glCullFace(GL_BACK); break;
        case OpenGLGraphics::CullModes::NONE: glDisable(GL_CULL_FACE); break;
        case OpenGLGraphics::CullModes::NO_CHANGE: break;
    }
    if(blendEnabled) {
        glEnablei(GL_BLEND, 0);
    } else {
        glDisablei(GL_BLEND, 0);
    }
    checkErrors("switchRenderStage");
}


void
OpenGLGraphics::switchRenderStage(uint32_t width, uint32_t height, uint32_t frameBufferID, bool blendEnabled, bool depthTestEnabled, bool depthWriteEnabled, bool scissorEnabled,
                                  bool clearColor, bool clearDepth, CullModes cullMode, const std::map<uint32_t, std::shared_ptr<Texture>> &inputs,
                                  const std::map<std::shared_ptr<Texture>,
                                          std::pair<FrameBufferAttachPoints, int>> &attachmentLayerMap, const std::string &name) {
    popDebugGroup();
    pushDebugGroup(name);
    //now we should change attachments based on the layer information we got
    for (auto attachmentLayerIt = attachmentLayerMap.begin(); attachmentLayerIt != attachmentLayerMap.end(); ++attachmentLayerIt) {
        attachDrawTextureToFrameBuffer(frameBufferID, attachmentLayerIt->first->getType(),
                                       attachmentLayerIt->first->getTextureID(), attachmentLayerIt->second.first,
                                       attachmentLayerIt->second.second, false);//no clear because clear will run afterwards
    }
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);
    if(clearColor && clearDepth) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else if(clearColor) {
        glClear(GL_COLOR_BUFFER_BIT);
    } else if(clearDepth) {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    if(depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    if(depthWriteEnabled) {
        glDepthMask(GL_TRUE);
    } else {
        glDepthMask(GL_FALSE);
    }
    if(scissorEnabled) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }

    //we combine diffuse+specular lighted with ambient / SSAO
    for (auto inputIt = inputs.begin(); inputIt != inputs.end(); ++inputIt) {
        switch (inputIt->second->getType()) {
            case OpenGLGraphics::TextureTypes::T2D: state->attachTexture(inputIt->second->getTextureID(), inputIt->first); break;
            case OpenGLGraphics::TextureTypes::T2D_ARRAY: state->attach2DTextureArray(inputIt->second->getTextureID(), inputIt->first); break;
            case OpenGLGraphics::TextureTypes::TCUBE_MAP: state->attachCubemap(inputIt->second->getTextureID(), inputIt->first); break;
            case OpenGLGraphics::TextureTypes::TCUBE_MAP_ARRAY: state->attachCubemapArray(inputIt->second->getTextureID(), inputIt->first); break;
        }
    }
    switch (cullMode) {
        case OpenGLGraphics::CullModes::FRONT: glEnable(GL_CULL_FACE);glCullFace(GL_FRONT); break;
        case OpenGLGraphics::CullModes::BACK: glEnable(GL_CULL_FACE);glCullFace(GL_BACK); break;
        case OpenGLGraphics::CullModes::NONE: glDisable(GL_CULL_FACE); break;
        case OpenGLGraphics::CullModes::NO_CHANGE: break;
    }
    if(blendEnabled) {
        glEnablei(GL_BLEND, 0);
    } else {
        glDisablei(GL_BLEND, 0);
    }
    checkErrors("switchRenderStageLayer");
}

void OpenGLGraphics::render(const uint32_t program, const uint32_t vao, const uint32_t ebo, const uint32_t elementCount, const uint32_t* startIndex) {
    if (program == 0) {
        std::cerr << "No program render requested." << std::endl;
        return;
    }
    state->setProgram(program);

    // Set up for a glDrawElements call
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    renderTriangleCount = renderTriangleCount + elementCount;
    glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, startIndex);
    glBindVertexArray(0);

    checkErrors("render");
}

void OpenGLGraphics::renderInstanced(uint32_t program, uint32_t VAO, uint32_t EBO, uint32_t triangleCount,
                                        uint32_t instanceCount) {
    if (program == 0) {
        std::cerr << "No program render requested." << std::endl;
        return;
    }
    state->setProgram(program);

    // Set up for a glDrawElements call
    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    renderTriangleCount = renderTriangleCount + (triangleCount * instanceCount);
    glDrawElementsInstanced(GL_TRIANGLES, triangleCount, GL_UNSIGNED_INT, nullptr, instanceCount);
    glBindVertexArray(0);
    //state->setProgram(0);
    checkErrors("renderInstanced");
}

void OpenGLGraphics::renderInstanced(uint32_t program, uint32_t VAO, uint32_t EBO, uint32_t triangleCount, uint32_t startOffset,
                                     uint32_t instanceCount) {
    if (program == 0) {
        std::cerr << "No program render requested." << std::endl;
        return;
    }
    state->setProgram(program);

    // Set up for a glDrawElements call
    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    renderTriangleCount = renderTriangleCount + (triangleCount * instanceCount);
    glDrawElementsInstanced(GL_TRIANGLES, triangleCount, GL_UNSIGNED_INT, (void*)(startOffset*sizeof(GLuint)), instanceCount);
    glBindVertexArray(0);
    //state->setProgram(0);
    checkErrors("renderInstancedOffset");
}

bool OpenGLGraphics::setUniform(const uint32_t programID, const uint32_t uniformID, const glm::mat4 &matrix) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniformMatrix4fv(uniformID, 1, GL_FALSE, glm::value_ptr(matrix));
        //state->setProgram(0);
        uniformSetCount++;
        checkErrors("setUniformMatrix");
        return true;
    }
}


bool
OpenGLGraphics::setUniformArray(const uint32_t programID, const uint32_t uniformID, const std::vector<glm::mat4> &matrixArray) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        int elementCount = matrixArray.size();
        glUniformMatrix4fv(uniformID, elementCount, GL_FALSE, glm::value_ptr(matrixArray.at(0)));
        //state->setProgram(0);
        uniformSetCount++;
        checkErrors("setUniformMatrixArray");
        return true;
    }
}

bool OpenGLGraphics::setUniform(const uint32_t programID, const uint32_t uniformID, const glm::vec3 &vector) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform3fv(uniformID, 1, glm::value_ptr(vector));
        //state->setProgram(0);
        uniformSetCount++;
        checkErrors("setUniformVector");
        return true;
    }
}

bool OpenGLGraphics::setUniform(const uint32_t programID, const uint32_t uniformID, const std::vector<glm::vec3> &vectorArray) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform3fv(uniformID, vectorArray.size(), glm::value_ptr(*vectorArray.data()));
        //state->setProgram(0);
        uniformSetCount++;
        checkErrors("setUniformVector");
        return true;
    }
}

bool OpenGLGraphics::setUniform(const uint32_t programID, const uint32_t uniformID, const float value) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform1f(uniformID, value);
        //state->setProgram(0);
        uniformSetCount++;
        checkErrors("setUniformFloat");
        return true;
    }
}

bool OpenGLGraphics::setUniform(const uint32_t programID, const uint32_t uniformID, const int value) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform1i(uniformID, value);
        //state->setProgram(0);
        uniformSetCount++;
        checkErrors("setUniformInt");
        return true;
    }
}


OpenGLGraphics::~OpenGLGraphics() {
    for (unsigned int i = 0; i < bufferObjects.size(); ++i) {
        deleteBuffer(1, bufferObjects[i]);
    }

    deleteBuffer(1, lightUBOLocation);
    deleteBuffer(1, playerUBOLocation);
    deleteBuffer(1, allMaterialsUBOLocation);
    glDeleteFramebuffers(1, &combineFrameBuffer);
}

void OpenGLGraphics::reshape() {
    //reshape actually checks for changes on options->
    this->screenHeight = options->getScreenHeight();
    this->screenWidth = options->getScreenWidth();
    glViewport(0, 0, options->getScreenWidth(), options->getScreenHeight());
    aspect = float(options->getScreenHeight()) / float(options->getScreenWidth());
    perspectiveProjectionMatrix = glm::perspective(options->PI/3.0f, 1.0f / aspect, 0.01f, 10000.0f);
    inverseProjection = glm::inverse(perspectiveProjectionMatrix);
    orthogonalProjectionMatrix = glm::ortho(0.0f, (float) options->getScreenWidth(), 0.0f, (float) options->getScreenHeight());
    checkErrors("reshape");
}

void OpenGLGraphics::setWrapMode(uint32_t textureID, TextureTypes textureType, TextureWrapModes wrapModeS,
                                 TextureWrapModes wrapModeT, TextureWrapModes wrapModeR) {
    GLenum glTextureType;
    switch (textureType) {
        case TextureTypes::T2D: {
            glTextureType = GL_TEXTURE_2D;
        }
            break;
        case TextureTypes::T2D_ARRAY: {
            glTextureType = GL_TEXTURE_2D_ARRAY;

        }
            break;
        case TextureTypes::TCUBE_MAP: {
            glTextureType = GL_TEXTURE_CUBE_MAP;

        }
            break;
        case TextureTypes::TCUBE_MAP_ARRAY: {
            glTextureType = GL_TEXTURE_CUBE_MAP_ARRAY_ARB;

        }
            break;
    }

    glBindTexture(glTextureType, textureID);
    switch(wrapModeS) {
        case TextureWrapModes::NONE: std::cerr << "Wrap mode S not set, this is unexpected" << std::endl; break;
        case TextureWrapModes::BORDER: glTexParameteri(glTextureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); break;
        case TextureWrapModes::REPEAT: glTexParameteri(glTextureType, GL_TEXTURE_WRAP_S, GL_REPEAT); break;
        case TextureWrapModes::EDGE: glTexParameteri(glTextureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); break;
    }

    switch(wrapModeT) {
        case TextureWrapModes::NONE: std::cerr << "Wrap mode T not set, this is unexpected" << std::endl; break;
        case TextureWrapModes::BORDER: glTexParameteri(glTextureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); break;
        case TextureWrapModes::REPEAT: glTexParameteri(glTextureType, GL_TEXTURE_WRAP_T, GL_REPEAT); break;
        case TextureWrapModes::EDGE: glTexParameteri(glTextureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); break;
    }

    switch(wrapModeR) {
        case TextureWrapModes::NONE: break;
        case TextureWrapModes::BORDER: glTexParameteri(glTextureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); break;
        case TextureWrapModes::REPEAT: glTexParameteri(glTextureType, GL_TEXTURE_WRAP_T, GL_REPEAT); break;
        case TextureWrapModes::EDGE: glTexParameteri(glTextureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); break;
    }

    checkErrors("setWrapMode");
}

void OpenGLGraphics::setFilterMode(uint32_t textureID, TextureTypes textureType, FilterModes filterMode) {
    GLenum glTextureType;
    switch (textureType) {
        case TextureTypes::T2D: {
            glTextureType = GL_TEXTURE_2D;
        }
            break;
        case TextureTypes::T2D_ARRAY: {
            glTextureType = GL_TEXTURE_2D_ARRAY;

        }
            break;
        case TextureTypes::TCUBE_MAP: {
            glTextureType = GL_TEXTURE_CUBE_MAP;

        }
            break;
        case TextureTypes::TCUBE_MAP_ARRAY: {
            glTextureType = GL_TEXTURE_CUBE_MAP_ARRAY_ARB;

        }
            break;
    }

    glBindTexture(glTextureType, textureID);

    switch (filterMode) {
        case FilterModes::NEAREST:
            glTexParameteri(glTextureType, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(glTextureType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;
        case FilterModes::LINEAR:
            glTexParameteri(glTextureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(glTextureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        case FilterModes::TRILINEAR:
            glTexParameteri(glTextureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(glTextureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        default:
            std::cerr << "Unknown filter mode set, this should never happen. " << std::endl;
    }
    checkErrors("setFilterMode");
}


void OpenGLGraphics::setTextureBorder(uint32_t textureID, TextureTypes textureType, bool isBorderColorSet,
                                      const std::vector<float> &borderColors) {
    GLenum glTextureType;
    switch (textureType) {
        case TextureTypes::T2D: {
            glTextureType = GL_TEXTURE_2D;
        }
            break;
        case TextureTypes::T2D_ARRAY: {
            glTextureType = GL_TEXTURE_2D_ARRAY;

        }
            break;
        case TextureTypes::TCUBE_MAP: {
            glTextureType = GL_TEXTURE_CUBE_MAP;

        }
            break;
        case TextureTypes::TCUBE_MAP_ARRAY: {
            glTextureType = GL_TEXTURE_CUBE_MAP_ARRAY_ARB;

        }
            break;
        default:
            std::cerr << "Unknown texture type set, this should never happen. " << std::endl;
            exit(-1);
    }

    glBindTexture(glTextureType, textureID);

    if(isBorderColorSet) {

        glTexParameteri(glTextureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(glTextureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        if(borderColors.size() < 4) {
            std::cerr << "Border color don't have 4 elements, this should never happen!" << std::endl;
            std::exit(-1);
        }
        GLfloat borderColor[] = {borderColors[0], borderColors[1], borderColors[2], borderColors[3]};
        glTexParameterfv(glTextureType, GL_TEXTURE_BORDER_COLOR, borderColor);
    } else {
        glTexParameteri(glTextureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(glTextureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    checkErrors("setTextureBorder");
}

uint32_t OpenGLGraphics::createFrameBuffer(uint32_t width, uint32_t height) {
    GLuint newFrameBufferLocation;
    glGenFramebuffers(1, &newFrameBufferLocation);
    glBindFramebuffer(GL_FRAMEBUFFER, newFrameBufferLocation);
    if(getFrameBufferParameterSupported()) {
        glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, width);
        glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, height);
    }
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "created frame buffer is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkErrors("createFrameBuffer");
    return newFrameBufferLocation;
}

void OpenGLGraphics::deleteFrameBuffer(uint32_t frameBufferID) {
    glDeleteFramebuffers(1, &frameBufferID);
    checkErrors("deleteFrameBuffer");
}

void OpenGLGraphics::attachDrawTextureToFrameBuffer(uint32_t frameBufferID, TextureTypes textureType, uint32_t textureID,
                                                       FrameBufferAttachPoints attachPoint, int32_t layer, bool clear) {

    int32_t maxDrawBuffers;
    glGetIntegerv( GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID);

    GLenum glAttachment;
    uint32_t index = 0;
    switch(attachPoint) {
        case FrameBufferAttachPoints::NONE: glAttachment = GL_NONE; break;
        case FrameBufferAttachPoints::COLOR0: glAttachment = GL_COLOR_ATTACHMENT0; break;
        case FrameBufferAttachPoints::COLOR1: glAttachment = GL_COLOR_ATTACHMENT1; index = 1;break;
        case FrameBufferAttachPoints::COLOR2: glAttachment = GL_COLOR_ATTACHMENT2; index = 2;break;
        case FrameBufferAttachPoints::COLOR3: glAttachment = GL_COLOR_ATTACHMENT3; index = 3;break;
        case FrameBufferAttachPoints::COLOR4: glAttachment = GL_COLOR_ATTACHMENT4; index = 4;break;
        case FrameBufferAttachPoints::COLOR5: glAttachment = GL_COLOR_ATTACHMENT5; index = 5;break;
        case FrameBufferAttachPoints::COLOR6: glAttachment = GL_COLOR_ATTACHMENT6; index = 6;break;
        case FrameBufferAttachPoints::DEPTH:  glAttachment = GL_DEPTH_ATTACHMENT;   break;
    }

    int32_t attachmentTemp;
    unsigned int drawBufferAttachments[6];
    if(attachPoint == OpenGLGraphics::FrameBufferAttachPoints::DEPTH) {
        if(clear) {
            glClear(GL_DEPTH_BUFFER_BIT);
        }
    } else {
        for (unsigned int i = 0; i < 6; ++i) {
            if (i == index) {
                drawBufferAttachments[i] = glAttachment;
                if(clear) {
                    unsigned int tempAttachmentBuffer[1] = {glAttachment};
                    glDrawBuffers(1, tempAttachmentBuffer);
                }
            } else {
                glGetIntegerv(GL_DRAW_BUFFER0 + i, &attachmentTemp);
                drawBufferAttachments[i] = attachmentTemp;
            }
        }
        glDrawBuffers(6, drawBufferAttachments);
    }

    switch (textureType) {
        case TextureTypes::T2D: {
            glFramebufferTexture2D(GL_FRAMEBUFFER, glAttachment, GL_TEXTURE_2D, textureID, 0);
        }
            break;
        case TextureTypes::T2D_ARRAY: {
            if(layer == -1 ) {
                glFramebufferTexture(GL_FRAMEBUFFER, glAttachment, textureID, 0);
            } else {
                glFramebufferTextureLayer(GL_FRAMEBUFFER, glAttachment, textureID, 0, layer);
            }
        }
            break;
        case TextureTypes::TCUBE_MAP: {
            glFramebufferTexture(GL_FRAMEBUFFER, glAttachment, textureID, 0);
        }
            break;
        case TextureTypes::TCUBE_MAP_ARRAY: {
            glFramebufferTexture(GL_FRAMEBUFFER, glAttachment, textureID, 0);
        }
            break;
    }

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "frame buffer texture to attach is not complete" << std::endl;
    }
    if(clear) {
        if(attachPoint == OpenGLGraphics::FrameBufferAttachPoints::DEPTH) {
            glClear(GL_DEPTH_BUFFER_BIT);
        }
    }
    checkErrors("attachDrawTextureToFrameBuffer");

}

uint32_t OpenGLGraphics::createTexture(int height, int width, TextureTypes type, InternalFormatTypes internalFormat, FormatTypes format, DataTypes dataType, uint32_t textureLayers) {
    GLuint texture;
    glGenTextures(1, &texture);
    state->activateTextureUnit(0);//this is the default working texture

    GLint glInternalDataFormat;
    switch (internalFormat) {
        case InternalFormatTypes::RED: glInternalDataFormat = GL_R8; break;
        case InternalFormatTypes::R32F: glInternalDataFormat = GL_R32F; break;
        case InternalFormatTypes::RGB: glInternalDataFormat = GL_RGB; break;
        case InternalFormatTypes::RGBA: glInternalDataFormat = GL_RGBA; break;
        case InternalFormatTypes::RGB16F: glInternalDataFormat = GL_RGB16F; break;
        case InternalFormatTypes::RGB32F: glInternalDataFormat = GL_RGB32F; break;
        case InternalFormatTypes::RGBA32F: glInternalDataFormat = GL_RGBA32F; break;
        case InternalFormatTypes::DEPTH: glInternalDataFormat = GL_DEPTH_COMPONENT; break;
        case InternalFormatTypes::COMPRESSED_RGB: glInternalDataFormat = GL_COMPRESSED_RGB; break;
        case InternalFormatTypes::COMPRESSED_RGBA: glInternalDataFormat = GL_COMPRESSED_RGBA; break;
    }

    GLenum glFormat;
    switch (format) {
        case FormatTypes::RED: glFormat = GL_RED; break;
        case FormatTypes::RGB: glFormat = GL_RGB; break;
        case FormatTypes::RGBA: glFormat = GL_RGBA; break;
        case FormatTypes::DEPTH: glFormat = GL_DEPTH_COMPONENT; break;
    }

    GLenum glDataType;
    switch (dataType) {
        case DataTypes::FLOAT: glDataType = GL_FLOAT; break;
        case DataTypes::HALF_FLOAT: glDataType = GL_FLOAT; break;
        case DataTypes::UNSIGNED_BYTE: glDataType = GL_UNSIGNED_BYTE; break;
        case DataTypes::UNSIGNED_SHORT: glDataType = GL_UNSIGNED_SHORT; break;
        case DataTypes::UNSIGNED_INT: glDataType = GL_UNSIGNED_INT; break;
    }

    GLenum glTextureType;
    switch (type) {
        case TextureTypes::T2D: {
            glTextureType = GL_TEXTURE_2D;
            glBindTexture(glTextureType, texture);
            glTexImage2D(GL_TEXTURE_2D,       0, glInternalDataFormat, width, height,       0, glFormat, glDataType, nullptr);
        }
            break;
        case TextureTypes::T2D_ARRAY: {
            glTextureType = GL_TEXTURE_2D_ARRAY;
            glBindTexture(glTextureType, texture);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, glInternalDataFormat, width,height, textureLayers, 0, glFormat, glDataType, nullptr);
        }
            break;
        case TextureTypes::TCUBE_MAP: {
            glTextureType = GL_TEXTURE_CUBE_MAP;
            glBindTexture(glTextureType, texture);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, glInternalDataFormat, width, height, 0, glFormat, glDataType, nullptr);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, glInternalDataFormat, width, height, 0, glFormat, glDataType, nullptr);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, glInternalDataFormat, width, height, 0, glFormat, glDataType, nullptr);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, glInternalDataFormat, width, height, 0, glFormat, glDataType, nullptr);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, glInternalDataFormat, width, height, 0, glFormat, glDataType, nullptr);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, glInternalDataFormat, width, height, 0, glFormat, glDataType, nullptr);
        }
            break;
        case TextureTypes::TCUBE_MAP_ARRAY: {
            glTextureType = GL_TEXTURE_CUBE_MAP_ARRAY_ARB;
            glBindTexture(glTextureType, texture);
            glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, 0, glInternalDataFormat, width,height, textureLayers, 0,glFormat, glDataType, nullptr);
            if(height != width) {
                std::cerr << "Cubemaps require square textures, this will fail!" << std::endl;
            }
        }
            break;
    }

    glTexParameteri(glTextureType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(glTextureType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    std::string temp;
    OptionsUtil::Options::Option<std::string> textureFilteringOption = options->getOption<std::string>(HASH("TextureFiltering"));
    temp = textureFilteringOption.getOrDefault("Nearest");
    if (temp == "Nearest") {
        glTexParameteri(glTextureType, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(glTextureType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else if(temp == "Bilinear") {
        glTexParameteri(glTextureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(glTextureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else if(temp == "Trilinear") {
        glTexParameteri(glTextureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(glTextureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    glBindTexture(glTextureType, 0);

    checkErrors("Texture Constructor");
    return texture;
}

void
OpenGLGraphics::loadTextureData(uint32_t textureID, int height, int width, TextureTypes type, InternalFormatTypes internalFormat, FormatTypes format, DataTypes dataType, uint32_t depth,
                                   void *data, void *data2, void *data3, void *data4, void *data5, void *data6) {
    state->activateTextureUnit(0);//this is the default working texture

    GLint glInternalDataFormat;
    switch (internalFormat) {
        case InternalFormatTypes::RED: glInternalDataFormat = GL_R8; break;
        case InternalFormatTypes::R32F: glInternalDataFormat = GL_R32F; break;
        case InternalFormatTypes::RGB: glInternalDataFormat = GL_RGB; break;
        case InternalFormatTypes::RGBA: glInternalDataFormat = GL_RGBA; break;
        case InternalFormatTypes::RGB16F: glInternalDataFormat = GL_RGB16F; break;
        case InternalFormatTypes::RGB32F: glInternalDataFormat = GL_RGB32F; break;
        case InternalFormatTypes::RGBA32F: glInternalDataFormat = GL_RGBA32F; break;
        case InternalFormatTypes::DEPTH: glInternalDataFormat = GL_DEPTH_COMPONENT; break;
        case InternalFormatTypes::COMPRESSED_RGB: glInternalDataFormat = GL_COMPRESSED_RGB; break;
        case InternalFormatTypes::COMPRESSED_RGBA: glInternalDataFormat = GL_COMPRESSED_RGBA; break;
    }

    GLenum glFormat;
    switch (format) {
        case FormatTypes::RED: glFormat = GL_RED; break;
        case FormatTypes::RGB: glFormat = GL_RGB; break;
        case FormatTypes::RGBA: glFormat = GL_RGBA; break;
        case FormatTypes::DEPTH: glFormat = GL_DEPTH_COMPONENT; break;
    }

    GLenum glDataType;
    switch (dataType) {
        case DataTypes::FLOAT: glDataType = GL_FLOAT; break;
        case DataTypes::UNSIGNED_BYTE: glDataType = GL_UNSIGNED_BYTE; break;
        case DataTypes::UNSIGNED_SHORT: glDataType = GL_UNSIGNED_SHORT; break;
        case DataTypes::UNSIGNED_INT: glDataType = GL_UNSIGNED_INT; break;
        case DataTypes::HALF_FLOAT: glDataType = GL_HALF_FLOAT; break;
    }

    GLenum glTextureType;
    switch (type) {
        case TextureTypes::T2D: {
            glTextureType = GL_TEXTURE_2D;
            glBindTexture(glTextureType, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, glInternalDataFormat, width, height, 0, glFormat, glDataType, data);
        }
            break;
        case TextureTypes::T2D_ARRAY: {
            glTextureType = GL_TEXTURE_2D_ARRAY;
            glBindTexture(glTextureType, textureID);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, glInternalDataFormat, width,height, depth, 0, glFormat, glDataType, data);
            std::cerr << "This method of loading texture data is not tested." << std::endl;
        }
            break;
        case TextureTypes::TCUBE_MAP: {
            glTextureType = GL_TEXTURE_CUBE_MAP;
            glBindTexture(glTextureType, textureID);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, glInternalDataFormat, width, height, 0, glFormat, glDataType, data);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, glInternalDataFormat, width, height, 0, glFormat, glDataType, data2);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, glInternalDataFormat, width, height, 0, glFormat, glDataType, data3);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, glInternalDataFormat, width, height, 0, glFormat, glDataType, data4);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, glInternalDataFormat, width, height, 0, glFormat, glDataType, data5);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, glInternalDataFormat, width, height, 0, glFormat, glDataType, data6);

        }
            break;
        case TextureTypes::TCUBE_MAP_ARRAY: {
            glTextureType = GL_TEXTURE_CUBE_MAP_ARRAY_ARB;
            glBindTexture(glTextureType, textureID);
            glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, 0, glInternalDataFormat, width,height, depth, 0,glFormat, glDataType, data);
            std::cerr << "This method of loading texture data is not tested." << std::endl;
        }
            break;
    }

    glGenerateMipmap(glTextureType);
    glBindTexture(glTextureType, 0);

    checkErrors("loadTextureData");
}


void OpenGLGraphics::attachTexture(unsigned int textureID, unsigned int attachPoint) {
    state->attachTexture(textureID, attachPoint);
    checkErrors("attachTexture");
}

void OpenGLGraphics::attach2DArrayTexture(unsigned int textureID, unsigned int attachPoint) {
    state->attach2DTextureArray(textureID, attachPoint);
    checkErrors("attachTexture");
}

void OpenGLGraphics::attachCubeMap(unsigned int cubeMapID, unsigned int attachPoint) {
    state->attachCubemap(cubeMapID, attachPoint);
    checkErrors("attachCubeMap");
}

void OpenGLGraphics::attachCubeMapArrayTexture(unsigned int cubeMapID, unsigned int attachPoint) {
    state->attachCubemapArray(cubeMapID, attachPoint);
    checkErrors("attachCubeMap");
}



bool OpenGLGraphics::deleteTexture(GLuint textureID) {
    bool result = state->deleteTexture(textureID);
    checkErrors("deleteTexture");
    return result;
}

bool OpenGLGraphics::getUniformLocation(const uint32_t programID, const std::string &uniformName, uint32_t &location) {
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

void OpenGLGraphics::createDebugVAOVBO(uint32_t &vao, uint32_t &vbo, uint32_t bufferSize) {
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
void OpenGLGraphics::drawLines(GraphicsProgram &program, uint32_t vao, uint32_t vbo, const std::vector<Line> &lines) {
    state->setProgram(program.getID());
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);//FIXME something is broking the vao so we need to attach again
    glBufferSubData(GL_ARRAY_BUFFER, 0, lines.size() * sizeof(Line), lines.data());
    program.setUniform("cameraTransformMatrix", perspectiveProjectionMatrix * cameraMatrix);

    renderLineCount = renderLineCount + lines.size();
    glDrawArrays(GL_LINES, 0, lines.size()*2);

    glBindVertexArray(0);
    checkErrors("drawLines");
}

void OpenGLGraphics::setLight(const int lightIndex,
                              const glm::vec3& attenuation,
                              const std::vector<glm::mat4>& shadowMatrices,
                              const glm::vec3& position,
                              const glm::vec3& color,
                              const glm::vec3& ambientColor,
                              const int32_t lightType,
                              const float farPlane) {

    //std::cout << "light type is " << lightType << std::endl;
    glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
    assert(shadowMatrices.size() <= 6);
    glBufferSubData(GL_UNIFORM_BUFFER, lightIndex * lightUniformSize,
                    sizeof(glm::mat4)*shadowMatrices.size(), shadowMatrices.data());
    glBufferSubData(GL_UNIFORM_BUFFER, lightIndex * lightUniformSize + sizeof(glm::mat4) * 6,
                    sizeof(glm::vec3), &position);
    glBufferSubData(GL_UNIFORM_BUFFER, lightIndex * lightUniformSize + sizeof(glm::mat4) * 6 + sizeof(glm::vec3),
                    sizeof(GLfloat), &farPlane);
    glBufferSubData(GL_UNIFORM_BUFFER, lightIndex * lightUniformSize + sizeof(glm::mat4) * 6 + sizeof(glm::vec4),
                    sizeof(glm::vec3), &color);
    glBufferSubData(GL_UNIFORM_BUFFER, lightIndex * lightUniformSize + sizeof(glm::mat4) * 6 + sizeof(glm::vec4) + sizeof(glm::vec3),
                    sizeof(GLint), &lightType);
    glBufferSubData(GL_UNIFORM_BUFFER, lightIndex * lightUniformSize + sizeof(glm::mat4) * 6 + 2 * sizeof(glm::vec4),
                    sizeof(glm::vec3), glm::value_ptr(attenuation));
    glBufferSubData(GL_UNIFORM_BUFFER, lightIndex * lightUniformSize + sizeof(glm::mat4) * 6 + 3 * sizeof(glm::vec4),
                    sizeof(glm::vec3), glm::value_ptr(ambientColor));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkErrors("setLight");
}

void OpenGLGraphics::setMaterial(const Material& material) {
    /*
     * this buffer has 2 objects, model has mat4 and then the material below:
     *
    layout (std140) uniform MaterialInformationBlock {
            vec3 ambient;
            float shininess;
            vec3 diffuse;
            int isMap;
    };
    */
    float shininess = material.getSpecularExponent();
    uint32_t maps = material.getMaps();

    glBindBuffer(GL_UNIFORM_BUFFER, allMaterialsUBOLocation);
    glBufferSubData(GL_UNIFORM_BUFFER, material.getMaterialIndex() * materialUniformSize,
                    sizeof(glm::vec3), glm::value_ptr(material.getAmbientColor()));
    glBufferSubData(GL_UNIFORM_BUFFER, material.getMaterialIndex() * materialUniformSize + sizeof(glm::vec3),
                    sizeof(GLfloat), &shininess);
    glBufferSubData(GL_UNIFORM_BUFFER, material.getMaterialIndex() * materialUniformSize + sizeof(glm::vec3) + sizeof(GLfloat),
                    sizeof(glm::vec3), glm::value_ptr(material.getDiffuseColor()));
    glBufferSubData(GL_UNIFORM_BUFFER, material.getMaterialIndex() * materialUniformSize + 2 *sizeof(glm::vec3) + sizeof(GLfloat),
                    sizeof(GLint), &maps);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkErrors("setMaterial");
}

void OpenGLGraphics::setBoneTransforms(uint32_t index, const std::vector<glm::mat4>& boneTransforms) {
    if (boneTransforms.size() > NR_BONE) {
        std::cerr << "Too many bones, can't upload more than " << NR_BONE << "ignoring." << std::endl;
    }
    if (boneTransforms.size() < NR_BONE) {
        std::cerr << "too little bones, possible garbage upload " << std::endl;
    }
    state->activateTextureUnit(maxTextureImageUnits-4);
    state->attachTexture(allBoneTransformsTexture, maxTextureImageUnits-4);
    glTexSubImage2D(GL_TEXTURE_2D,0,0, index, 4*NR_BONE, 1, GL_RGBA, GL_FLOAT, boneTransforms.data());
    checkErrors("setBoneTransform");
}

void OpenGLGraphics::setModel(const uint32_t modelID, const glm::mat4& worldTransform) {
    state->activateTextureUnit(maxTextureImageUnits-3);
    state->attachTexture(allModelTransformsTexture, maxTextureImageUnits-3);
    glm::mat4 transposeInverse = glm::transpose(glm::inverse(worldTransform));
    float data[32];
    memcpy(data, glm::value_ptr(worldTransform), sizeof(float)*16);
    memcpy((data+16), glm::value_ptr(transposeInverse), sizeof(float)*12);
    glTexSubImage2D(GL_TEXTURE_2D,0,4*modelID, 0, 4, 2, GL_RGBA, GL_FLOAT, data);
    //std::cout << "setting for model id " << modelID << std::endl;
    checkErrors("setModel");
}

void OpenGLGraphics::setModelIndexesUBO(const std::vector<glm::uvec4> &modelIndicesList) {
    /**
     * POSSIBLE FIXME
     * std140 layout requires arrays to be padded to 16 bytes. std430 is not supported for uniform buffers.
     * we can upload the array as is and calculate the vector component in shader, but since we are GPU bound I am
     * choosing to pad it in CPU instead.
     */

    glBindBuffer(GL_UNIFORM_BUFFER, allModelIndexesUBOLocation);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::uvec4) * modelIndicesList.size(), modelIndicesList.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkErrors("setModelIndexesUBO");
}

void OpenGLGraphics::setPlayerMatrices(const glm::vec3 &cameraPosition, const glm::mat4 &cameraTransform, long currentTime) {
    this->cameraMatrix = cameraTransform;
    this->cameraPosition= cameraPosition;
    glm::vec3 cameraSpacePosition = glm::vec3(cameraMatrix * glm::vec4(cameraPosition, 1.0));
    glm::mat4 inverseCameraMatrix = glm::inverse(cameraTransform);
    glBindBuffer(GL_UNIFORM_BUFFER, playerUBOLocation);
    glBufferSubData(GL_UNIFORM_BUFFER, 0 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(cameraMatrix));//changes with camera
    glBufferSubData(GL_UNIFORM_BUFFER, 1 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(perspectiveProjectionMatrix));//never changes
    glm::mat4 viewMatrix = perspectiveProjectionMatrix * cameraMatrix;
    glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(viewMatrix));//changes with camera
    glBufferSubData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(inverseProjection));//never changes

    glBufferSubData(GL_UNIFORM_BUFFER, 4 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(inverseCameraMatrix));//changes with camera
    glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(glm::transpose(inverseCameraMatrix)));//changes with camera
    //transpose inverse is used as mat3, but std140 pads it to mat43 so it looks like we are overriding 1 row
    glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::mat4) + 3 * sizeof(glm::vec4), sizeof(glm::vec3), glm::value_ptr(cameraPosition));//changes with camera
    glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::mat4) + 4 * sizeof(glm::vec4), sizeof(glm::vec3), glm::value_ptr(cameraSpacePosition));//changes with camera

    glm::vec2 noiseScale(this->screenWidth / 4, this->screenHeight / 4);
    glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::mat4) + 5 * sizeof(glm::vec4), sizeof(glm::vec2), glm::value_ptr(noiseScale));//never changes
    glBufferSubData(GL_UNIFORM_BUFFER, 5 * sizeof(glm::mat4) + 5 * sizeof(glm::vec4) + sizeof(glm::vec2), sizeof(GLfloat), &currentTime);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    checkErrors("setPlayerMatrices");
}

void OpenGLGraphics::backupCurrentState() {
    this->state->backupState();
}

void OpenGLGraphics::restoreLastState() {
    this->state->restoreState();
}