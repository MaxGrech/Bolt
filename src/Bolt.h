#pragma once

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pointer.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Debug.h>

#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>

#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>

#include <Magnum/MeshTools/Compile.h>

#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>

#include <Magnum/Shaders/Phong.h>

#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Platform/GLContext.h>

#include "imgui.h"

using namespace Magnum;

class Bolt
{
public:
    using Scene3D = SceneGraph::Scene<SceneGraph::MatrixTransformation3D>;
    using Object3D = SceneGraph::Object<SceneGraph::MatrixTransformation3D>;
    using DrawableGroup3D = SceneGraph::DrawableGroup3D;

    Corrade::PluginManager::Manager<Trade::AbstractImporter> manager;
    Corrade::Containers::Pointer<Trade::AbstractImporter> importer;

    GL::Mesh droneMesh;
    Shaders::Phong droneShader;

    Scene3D scene;
    Object3D droneObject{ &scene };
    Object3D cameraObject{ &scene };
    Object3D lightObject{ &scene };

    SceneGraph::Camera3D camera{ cameraObject };
    DrawableGroup3D drawables;

    void init()
    {
        /* _ChatGPT_ Enable basic render state once. */
        GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

        importer = manager.loadAndInstantiate("AssimpImporter");
        if (!importer) Fatal{} << "Failed to load AssimpImporter plugin.";
        if (!importer->openFile("res/Drone.fbx")) Fatal{} << "Cannot open FBX file.";

        Debug{} << "Meshes:" << importer->meshCount();
        Debug{} << "Scenes:" << importer->sceneCount();
        for (UnsignedInt i = 0; i < importer->meshCount(); ++i)
            Debug{} << "Mesh" << i << "name =" << importer->meshName(i).c_str();

        Corrade::Containers::Optional<Trade::MeshData> meshData = importer->mesh(0);
        if (!meshData) Fatal{} << "FBX has no mesh 0";

        /* _ChatGPT_ Phong needs normals; generate if missing. */
        MeshTools::CompileFlags flags;
        if (!meshData->hasAttribute(Trade::MeshAttribute::Normal))
            flags |= MeshTools::CompileFlag::GenerateFlatNormals;

        droneMesh = MeshTools::compile(*meshData, flags);

        droneShader
            .setDiffuseColor(Color4(0x66ccff))
            .setSpecularColor(Color4(0x000000))
            .setShininess(0.0f);

        /* _ChatGPT_ Establish an actual scene: model at origin, camera pulled back, light in world space. */
        droneObject.resetTransformation();

        cameraObject.resetTransformation()
            .translate(Vector3::zAxis(10.0f)); /* _ChatGPT_ Camera looks down -Z, so +Z backs it away from origin. */

        lightObject.resetTransformation()
            .translate({ 3.0f, 5.0f, 8.0f });

        /* _ChatGPT_ Camera setup (projection uses aspect-ratio policy + viewport). */
        camera
            .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
            .setProjectionMatrix(Matrix4::perspectiveProjection(Deg(60.0f), 1.0f, 0.01f, 100.0f));

        /* _ChatGPT_ Create a drawable that bridges SceneGraph -> Phong shader. */
        _droneDrawable.reset(new DroneDrawable{ droneObject, drawables, droneMesh, droneShader, lightObject });
    }

    void resize(const Vector2i& viewport)
    {
        /* _ChatGPT_ This recalculates the projection according to the aspect-ratio policy. */
        camera.setViewport(viewport);
    }

    void drawFrame()
    {
        static float rotY = 0.0f;
        static float rotX = 0.0f;
        static float scale = 1.0f;

        ImGui::Begin("Drone Options");
        ImGui::SliderFloat("Rotation Y", &rotY, 0.0f, 360.0f);
        ImGui::SliderFloat("Rotation X", &rotX, 0.0f, 360.0f);
        ImGui::SliderFloat("Scale", &scale, 0.0f, 5.0f);
        ImGui::End();

        /* _ChatGPT_ Apply UI-controlled model transform. */
        droneObject.setTransformation(
            Matrix4::rotationY(Deg(rotY)) *
            Matrix4::rotationX(Deg(rotX)) *
            Matrix4::scaling(Vector3{ scale })
        );

        /* _ChatGPT_ Draw the whole drawable group with the camera. */
        camera.draw(drawables);
    }

private:
    class DroneDrawable final : public SceneGraph::Drawable3D
    {
    public:
        explicit DroneDrawable(Object3D& object,
            DrawableGroup3D& group,
            GL::Mesh& mesh,
            Shaders::Phong& shader,
            Object3D& lightObject)
            : SceneGraph::Drawable3D{ object, &group }
            , _mesh(mesh)
            , _shader(shader)
            , _lightObject(lightObject)
        {
        }

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override
        {
            /* _ChatGPT_ Light position must be in camera space for the classic Phong uniforms. */
            const Vector3 lightWorld = _lightObject.absoluteTransformation().translation();
            const Vector3 lightCamera = camera.cameraMatrix().transformPoint(lightWorld);

            _shader
                .setLightPosition(lightCamera)
                .setTransformationMatrix(transformationMatrix)
                .setNormalMatrix(transformationMatrix.rotationScaling())
                .setProjectionMatrix(camera.projectionMatrix())
                .draw(_mesh);
        }

        GL::Mesh& _mesh;
        Shaders::Phong& _shader;
        Object3D& _lightObject;
    };

    Corrade::Containers::Pointer<DroneDrawable> _droneDrawable;
};
