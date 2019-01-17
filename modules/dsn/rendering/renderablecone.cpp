/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2018                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#include <modules/dsn/rendering/renderablecone.h>

#include <modules/base/basemodule.h>
#include <openspace/documentation/documentation.h>
#include <openspace/documentation/verifier.h>
#include <openspace/engine/globals.h>
#include <openspace/rendering/renderengine.h>
#include <openspace/util/updatestructures.h>
#include <ghoul/opengl/programobject.h>

namespace {
    constexpr const char* ProgramName = "ConeProgram";
    constexpr const char* _loggerCat = "RenderableCone";


    constexpr const std::array <const char*, openspace::RenderableCone::uniformCacheSize> UniformNames = {
        "modelView", "projectionTransform"};

    constexpr openspace::properties::Property::PropertyInfo ApexPositionInfo = {
        "ApexPosition",
        "Apex Position",
        "This value specifies the position of the cone apex. If this value"
        "is a string, it is interpreted as the identifier of another "
        "scenegraph node. If this value is a 3-vector, it is interpreted "
        "as a 3D world position."
    };

    constexpr openspace::properties::Property::PropertyInfo BaseCenterDirectionInfo = {
        "BaseCenterDirection",
        "Base Center Direction",
        "This value specifies the direction from the apex to the base center of "
        "the cone. If this value is a string, it is interpreted as the identifier " 
        "of another scenegraph node. If this value is a 3-vector, it is interpreted "
        "as a 3D direction vector."
    };

    constexpr openspace::properties::Property::PropertyInfo ReverseDirectionInfo = {
        "ReverseDirection",
        "Reverse Direction",
        "Reverses the BaseCenterDirection"
    };

    constexpr openspace::properties::Property::PropertyInfo ColorInfo = {
        "Color",
        "Color",
        "Color of the cone"
    };

    constexpr openspace::properties::Property::PropertyInfo HeightInfo = {
        "Height",
        "Height",
        "Height of the cone"
    };
    constexpr openspace::properties::Property::PropertyInfo AngleInfo = {
        "Angle",
        "Angle",
        "Angle of the cone base"
    };
    constexpr openspace::properties::Property::PropertyInfo ResolutionInfo = {
        "Resolution",
        "Resolution",
        "Resolution of the cone, i.e number of vertices around the base"
    };
} // namespace

namespace openspace {



documentation::Documentation RenderableCone::Documentation() {
    using namespace documentation;
    return {
        "Renderable Cone",
        "dsn_renderable_renderablecone",
        {
            {
                "Type",
                new StringEqualVerifier("RenderableCone"),
                Optional::No
            },
            {
                ApexPositionInfo.identifier,
                new OrVerifier({ new StringVerifier, new DoubleVector3Verifier, }),
                Optional::No,
                ApexPositionInfo.description
            },
            {
                BaseCenterDirectionInfo.identifier,
                new OrVerifier({ new StringVerifier, new DoubleVector3Verifier, }),
                Optional::No,
                BaseCenterDirectionInfo.description
            },
            {
                ReverseDirectionInfo.identifier,
                new BoolVerifier,
                Optional::Yes,
                ReverseDirectionInfo.description
            },
            {
                ColorInfo.identifier,
                new DoubleVector4Verifier,
                Optional::Yes,
                ColorInfo.description
            },
            {
                HeightInfo.identifier,
                new DoubleVerifier,
                Optional::Yes,
                HeightInfo.description
            },
            {
                AngleInfo.identifier,
                new DoubleVerifier,
                Optional::Yes,
                AngleInfo.description
            },
            {
                ResolutionInfo.identifier,
                new DoubleVerifier,
                Optional::Yes,
                ResolutionInfo.description
            }
        }
    };
}

RenderableCone::RenderableCone(const ghoul::Dictionary& dictionary)
    : Renderable(dictionary)
    , _height(HeightInfo, 0.8, 0.0, 1.0)
    , _angle(AngleInfo, 50, 0.0, 180)
    , _resolution(ResolutionInfo, 8, 4, 100)
    , _color(
        ColorInfo,
        _defaultColor,
        glm::vec4(0.0),
        glm::vec4(1.0)
    )
{

    documentation::testSpecificationAndThrow(
        Documentation(),
        dictionary,
        "RenderableCone"
    );

    if (dictionary.hasKey(ApexPositionInfo.identifier)) {

        if (dictionary.hasKeyAndValue<std::string>(ApexPositionInfo.identifier)) {
            _apexNodeId = dictionary.value<std::string>(ApexPositionInfo.identifier);
        }
        else {
            // We know it has to be a vector now
            _apexPosition = dictionary.value<glm::dvec3>(ApexPositionInfo.identifier);
            _apexIsNodeAttached = false;
        }
    }

    if (dictionary.hasKey(BaseCenterDirectionInfo.identifier)) {

        if (dictionary.hasKeyAndValue<std::string>(BaseCenterDirectionInfo.identifier)) {
            _baseDirNodeId = dictionary.value<std::string>(BaseCenterDirectionInfo.identifier);
        }
        else {
            // We know it has to be a vector now
            _baseCenterDirection = dictionary.value<glm::dvec3>(ApexPositionInfo.identifier);
            _baseCenterIsNodeAttached = false;
        }

        if (dictionary.hasKeyAndValue<bool>(ReverseDirectionInfo.identifier)) {
            _directionIsReversed = dictionary.value<bool>(ReverseDirectionInfo.identifier);
        }
    }

    if (dictionary.hasKeyAndValue<glm::vec4>(ColorInfo.identifier)) {
        _color = dictionary.value<glm::vec4>(ColorInfo.identifier);
        _color.setViewOption(properties::Property::ViewOptions::Color);
        addProperty(_color);
    }

    addProperty(_height);
    addProperty(_angle);
    addProperty(_resolution);
}
void RenderableCone::initializeGL() {
    _programObject = BaseModule::ProgramObjectManager.request(
        ProgramName,
        []() -> std::unique_ptr<ghoul::opengl::ProgramObject> {
            return global::renderEngine.buildRenderProgram(
                ProgramName,
                absPath("${MODULE_DSN}/shaders/renderablecone_vs.glsl"),
                absPath("${MODULE_DSN}/shaders/renderablecone_fs.glsl")
            );
        }
    );

    ghoul::opengl::updateUniformLocations(*_programObject, _uniformCache, UniformNames);
    setRenderBin(Renderable::RenderBin::Overlay);

    // We don't need an index buffer, so we keep it at the default value of 0
    glGenVertexArrays(1, &_lateralSurfaceInfo._vaoID);
    glGenBuffers(1, &_lateralSurfaceInfo._vBufferID);

    glGenVertexArrays(1, &_baseInfo._vaoID);
    glGenBuffers(1, &_baseInfo._vBufferID);

    updateVertexAttributes();
}

void RenderableCone::deinitializeGL() {
    glDeleteVertexArrays(1, &_lateralSurfaceInfo._vaoID);
    glDeleteBuffers(1, &_lateralSurfaceInfo._vBufferID);

    glDeleteVertexArrays(1, &_baseInfo._vaoID);
    glDeleteBuffers(1, &_baseInfo._vBufferID);

    BaseModule::ProgramObjectManager.release(
        ProgramName,
        [](ghoul::opengl::ProgramObject* p) {
            global::renderEngine.removeRenderProgram(p);
        }
    );
    _programObject = nullptr;
}

bool RenderableCone::isReady() const {
    return _programObject != nullptr;
}

// Unbind buffers and arrays
inline void unbindGL() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void RenderableCone::updateVertexAttributes() {
    // position attributes
    glVertexAttribPointer(_vaLocVer, _sizeThreeVal, GL_FLOAT, GL_FALSE, 
                        sizeof(ColorVBOLayout) + sizeof(PositionVBOLayout),
                        (void*)0);
    glEnableVertexAttribArray(_vaLocVer);
    // color attributes
    glVertexAttribPointer(_vaLocCol, _sizeFourVal, GL_FLOAT, GL_FALSE,
                        sizeof(ColorVBOLayout) + sizeof(PositionVBOLayout),
                        (void*)(sizeof(PositionVBOLayout)));
    glEnableVertexAttribArray(_vaLocCol);
};

void RenderableCone::render(const RenderData& data, RendererTasks&) {
    _programObject->activate();
    updateUniforms(data);

    const bool usingFramebufferRenderer =
        global::renderEngine.rendererImplementation() ==
        RenderEngine::RendererImplementation::Framebuffer;

    if (usingFramebufferRenderer) {
        glDepthMask(false);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    }
    //Lateral surface of the cone
    glBindVertexArray(_lateralSurfaceInfo._vaoID);
    glBindBuffer(GL_ARRAY_BUFFER, _lateralSurfaceInfo._vBufferID);
    glBufferData(
        GL_ARRAY_BUFFER,
        _vertexLateralSurfaceArray.size() * sizeof(float),
        _vertexLateralSurfaceArray.data(),
        GL_STATIC_DRAW
    );
    updateVertexAttributes();
    glFrontFace(GL_CW);
    glDrawArrays(
        GL_TRIANGLE_FAN,
        0,
        _count
    );
    glFrontFace(GL_CCW);

    //Base part of the cone
    glBindVertexArray(_baseInfo._vaoID);
    glBindBuffer(GL_ARRAY_BUFFER, _baseInfo._vBufferID);
    glBufferData(
        GL_ARRAY_BUFFER,
        _vertexBaseArray.size() * sizeof(float),
        _vertexBaseArray.data(),
        GL_STATIC_DRAW
    );
    updateVertexAttributes();
    glDrawArrays(
        GL_TRIANGLE_FAN,
        0,
        _count
    );

    unbindGL();

    if (usingFramebufferRenderer) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(true);
    }
    _programObject->deactivate();
}

void RenderableCone::update(const UpdateData& data) {
    _vertexLateralSurfaceArray.clear();
    _vertexBaseArray.clear();
    
    if (_apexIsNodeAttached) {

        if (!global::renderEngine.scene()->sceneGraphNode(_apexNodeId)) {
            LERROR(fmt::format("No scenegraphnode found with id {}", _apexNodeId));
            return;
        }
        _apexPosition = global::renderEngine.scene()->sceneGraphNode(_apexNodeId)->worldPosition();
    }

    if (_baseCenterIsNodeAttached) {

        if (!global::renderEngine.scene()->sceneGraphNode(_baseDirNodeId)) {
            LERROR(fmt::format("No scenegraphnode found with id {}", _baseDirNodeId));
            return;
        }

        glm::dvec3 nodePos = global::renderEngine.scene()->sceneGraphNode(_baseDirNodeId)->worldPosition();

        _baseCenterDirection = glm::normalize(_apexPosition - nodePos);
    }

    std::vector<glm::dvec3> baseVertices;
    glm::dvec3 baseCenterPosition;
    int numBaseVertices = _resolution;
    double height = _height * _unit;

    float angle = (_angle * pi) / 180; //Convert from degrees to radians
    angle = angle / 2; //Half of the full cone angle to get a right -angled triangle
    double radius = height * tan(angle);

    float angleIncrement = glm::radians(360.0 / numBaseVertices);
    glm::dvec3 e0 = glm::cross(_baseCenterDirection, glm::dvec3(1.0, 0.0, 0.0));
    glm::dvec3 e1 = glm::cross(_baseCenterDirection, e0);

    if (_directionIsReversed) {
        baseCenterPosition = _apexPosition + _baseCenterDirection * height;
    }
    else {
        baseCenterPosition = _apexPosition - _baseCenterDirection * height;
    }

    for (int i = 0; i < numBaseVertices; ++i) {
        double rad = angleIncrement * i;
        glm::dvec3 p = baseCenterPosition + (((e0 * glm::cos(rad)) + (e1 * glm::sin(rad))) * radius);
        baseVertices.push_back(p);
    }
    
    fillVertexArray(_vertexBaseArray, baseCenterPosition, baseVertices);
    fillVertexArray(_vertexLateralSurfaceArray, _apexPosition, baseVertices);

    // Update the number of lines to render, same for both vertex arrays
    _count = static_cast<GLsizei>(_vertexLateralSurfaceArray.size() / (_sizeThreeVal + _sizeFourVal));

    unbindGL();
}

void RenderableCone::updateUniforms(const RenderData& data) {
    _programObject->setUniform(_uniformCache.modelView,
        data.camera.combinedViewMatrix() * _localTransform);
    _programObject->setUniform(_uniformCache.projection, data.camera.sgctInternal.projectionMatrix());
}

void RenderableCone::fillVertexArray(std::vector<float> &vertexArray, glm::dvec3 centerPoint, std::vector<glm::dvec3> points) {
    
    addVertexToVertexArray(vertexArray, centerPoint, _color);
    for (int i = 0; i < points.size(); ++i) {
        addVertexToVertexArray(vertexArray,points[i], _color);
    }
    addVertexToVertexArray(vertexArray,points[0], _color);
}

void RenderableCone::addVertexToVertexArray(std::vector<float> &vertexArray,glm::dvec3 position, glm::vec4 color)
{
    vertexArray.push_back(position.x);
    vertexArray.push_back(position.y);
    vertexArray.push_back(position.z);
    vertexArray.push_back(color.r);
    vertexArray.push_back(color.g);
    vertexArray.push_back(color.b);
    vertexArray.push_back(color.a);
}

} // namespace openspace
