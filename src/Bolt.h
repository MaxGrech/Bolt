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

#include <Magnum/Primitives/Axis.h>
#include <Magnum/Shaders/Flat.h>

#include "imgui.h"

using namespace Magnum;

class Bolt
{
    using Scene3D = SceneGraph::Scene<SceneGraph::MatrixTransformation3D>;
    using Object3D = SceneGraph::Object<SceneGraph::MatrixTransformation3D>;
    using DrawableGroup3D = SceneGraph::DrawableGroup3D;

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

            _shader.setLightPosition(lightCamera)
                .setAmbientColor(Color3{ 0.05f })
                .setLightColor(Color3{ 0.003f })
                .setDiffuseColor(Color3{ 0.5f })
                .setSpecularColor(Color3{ 0.5f })

                .setTransformationMatrix(transformationMatrix)
                .setNormalMatrix(transformationMatrix.rotationScaling())
                .setProjectionMatrix(camera.projectionMatrix())
                .draw(_mesh);
        }

        GL::Mesh& _mesh;
        Shaders::Phong& _shader;
        Object3D& _lightObject;
    };

public:
    
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

    Corrade::Containers::Pointer<DroneDrawable> _droneDrawable;

    GL::Mesh axisMesh;
    Shaders::Flat3D axisShader;

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



        if (!meshData->hasAttribute(Trade::MeshAttribute::Position))
            Fatal{} << "Mesh has no positions?!";

        auto positions = meshData->positions3DAsArray(); // or attribute<Vector3>(MeshAttribute::Position)

        Vector3 min = positions[0];
        Vector3 max = positions[0];

        for (const Vector3& p : positions) {
            min = Math::min(min, p);
            max = Math::max(max, p);
        }

        Vector3 size = max - min;
        Debug{}
            << "Drone AABB min:" << min
            << "max:" << max
            << "size:" << size;





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

        

        lightObject.resetTransformation()
            .translate({ 3.0f, 5.0f, 8.0f });

        /* _ChatGPT_ Camera setup (projection uses aspect-ratio policy + viewport). */
        camera
            .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
            .setProjectionMatrix(Matrix4::perspectiveProjection(Deg(60.0f), 1.0f, 0.01f, 5000.0f));

        /* _ChatGPT_ Create a drawable that bridges SceneGraph -> Phong shader. */
        _droneDrawable.reset(new DroneDrawable{ droneObject, drawables, droneMesh, droneShader, lightObject });

        axisMesh = MeshTools::compile(Primitives::axis3D());
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
        static float cameraX = 0, cameraY = 0, cameraZ = 10.0f;
        static float cameraRotX = -15, cameraRotY = 90, cameraRotZ = 0;

        ImGui::Begin("Options");

        ImGui::PushID("drone_control");
            ImGui::SeparatorText("Drone Control");
            ImGui::SliderFloat("Rotation Y", &rotY, 0.0f, 360.0f);
            ImGui::SliderFloat("Rotation X", &rotX, 0.0f, 360.0f);
            ImGui::SliderFloat("Scale", &scale, 0.0f, 5.0f);
        ImGui::PopID();

        ImGui::PushID("cam_control");
            ImGui::SeparatorText("Camera Control");
            ImGui::SliderFloat("Position X", &cameraX, -20.0f, 20.0f);
            ImGui::SliderFloat("Position Y", &cameraY, -20.0f, 20.0f);
            ImGui::SliderFloat("Position Z", &cameraZ, -20.0f, 20.0f);
            ImGui::SliderFloat("Rotation X", &cameraRotX, -180.0f, 180.0f);
            ImGui::SliderFloat("Rotation Y", &cameraRotY, -180.0f, 180.0f);
            ImGui::SliderFloat("Rotation Z", &cameraRotZ, -180.0f, 180.0f);
        ImGui::PopID();
        
        ImGui::End();
        
        cameraObject.resetTransformation()
            .translate(Vector3(cameraX, cameraY, cameraZ))
            .rotateX(Deg(cameraRotX))
            .rotateY(Deg(cameraRotY))
            .rotateZ(Deg(cameraRotZ));


        /* _ChatGPT_ Apply UI-controlled model transform. */
        droneObject.setTransformation(
            Matrix4::rotationY(Deg(rotY)) *
            Matrix4::rotationX(Deg(rotX)) *
            Matrix4::scaling(Vector3{ scale })
        );

        /* _ChatGPT_ Draw the whole drawable group with the camera. */
        camera.draw(drawables);
        
        const Matrix4 vp = camera.projectionMatrix() * camera.cameraMatrix();

        axisShader
            .setTransformationProjectionMatrix(vp * Matrix4{}) // model = identity
            .draw(axisMesh);
    }

};
