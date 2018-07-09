//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com
#pragma once

#include "Settings.h"	// todo: is this needed?
#include "Light.h"
#include "Mesh.h"
#include "Skybox.h"
#include "GameObject.h"
#include "GameObjectPool.h"

#include "Utilities/Camera.h"

#include <memory>
#include <mutex>
#include <future>

struct SerializedScene;
class SceneManager;
class Renderer;
class TextRenderer;
struct SceneView;
struct ShadowView;
class MaterialPool;

namespace VQEngine { class ThreadPool; }

#define DO_NOT_LOAD_SCENES 0

//----------------------------------------------------------------------------------------------------------------
// https://en.wikipedia.org/wiki/Template_method_pattern
// https://stackoverflow.com/questions/9724371/force-calling-base-class-virtual-function
// https://isocpp.org/wiki/faq/strange-inheritance#two-strategies-for-virtuals
// template method seems like a good idea here.
// the idea is that base class takes care of the common tasks among all scenes and calls the 
// customized functions of the derived classes through pure virtual functions
//----------------------------------------------------------------------------------------------------------------

struct ModelLoadQueue 
{
	std::mutex mutex;
	std::unordered_map<GameObject*, std::string> objectModelMap;
	std::unordered_map<std::string, std::future<Model>> asyncModelResults;
};

class Scene
{
protected:
	//----------------------------------------------------------------------------------------------------------------
	// SCENE INTERFACE FOR DERIVED SCENES
	//----------------------------------------------------------------------------------------------------------------

	// Update() is called each frame
	//
	virtual void Update(float dt) = 0;

	// Scene-specific loading logic
	//
	virtual void Load(SerializedScene& scene) = 0;
	
	// Scene-specific unloading logic
	//
	virtual void Unload() = 0;

	// each scene has to implement scene-specific RenderUI() function. RenderUI() is called
	// after post processing is finished and is the last rendering workload before presenting the frame.
	//
	virtual void RenderUI() const = 0;

	//	Use this function to programmatically create new objects in the scene.
	//
	GameObject*		CreateNewGameObject();
	
	//	Loads an assimp model - blocks the thread until the model loads
	//
	Model			LoadModel(const std::string& modelPath);

	// Queues a task for loading an assimp model for the GameObject* pObject
	// - ModelData will be assigned when the models finish loading which is sometime 
	//   after Load() and before Render(), it won't be immediately available.
	//
	void LoadModel_Async(GameObject* pObject, const std::string& modelPath);

protected:
	friend class SceneResourceView; // using attorney method, alternatively can use friend function
	friend class ModelLoader;

	std::vector<Mesh>			mMeshes;
	std::vector<Camera>			mCameras;
	std::vector<Light>			mLights;
	std::vector<GameObject*>	mpObjects;

	DirectionalLight			mDirectionalLight;
	Skybox						mSkybox;
	EEnvironmentMapPresets		mActiveSkyboxPreset;
	int mSelectedCamera;

	Settings::SceneRender		mSceneRenderSettings;
	Renderer*					mpRenderer;
	TextRenderer*				mpTextRenderer;
	VQEngine::ThreadPool*		mpThreadPool;	// initialized by the Engine



	//----------------------------------------------------------------------------------------------------------------
	// ENGINE INTERFACE
	//----------------------------------------------------------------------------------------------------------------

	friend class Engine;
public:
	Scene(Renderer* pRenderer, TextRenderer* pTextRenderer);
	~Scene() = default;


	// Moves objects from serializedScene into objects vector and sets the scene pointer in objects
	//
	void LoadScene(SerializedScene& scene, const Settings::Window& windowSettings, const std::vector<Mesh>& builtinMeshes);

	// clears object/light containers and camera settings
	//
	void UnloadScene();

	// updates selected camera and calls overridden Update from derived scene class
	//
	void UpdateScene(float dt);

	// Prepares the mDrawLists for various processing (culling, render pass object lists etc.)
	//
	void PreRender();
	void GatherLightData(SceneLightingData& outLightingData, ShadowView& outShadowView) const;
	void GatherShadowCasters(std::vector<const GameObject*>& casters) const;


	// Renders the meshes in the scene which have materials with alpha=1.0f
	//
	int RenderOpaque(const SceneView& sceneView) const;

	// Renders the transparent meshes in the scene, on a separate draw pass
	//
	int RenderAlpha(const SceneView& sceneView) const;

	// Renders debugging information such as bounding boxes, wireframe meshes, etc.
	//
	int RenderDebug(const XMMATRIX& viewProj) const;

	//	Use these functions to programmatically create material instances which you can add to game objects in the scene. 
	//
	Material* CreateNewMaterial(EMaterialType type); // <Thread safe>
	Material* CreateRandomMaterialOfType(EMaterialType type); // <Thread safe>

	inline const EnvironmentMap&		GetEnvironmentMap() const { return mSkybox.GetEnvironmentMap(); }
	inline const Camera&				GetActiveCamera() const { return mCameras[mSelectedCamera]; }
	inline const Settings::SceneRender& GetSceneRenderSettings() const { return mSceneRenderSettings; }
	inline bool							HasSkybox() const { return mSkybox.GetSkyboxTexture() != -1; }
	inline void							RenderSkybox(const XMMATRIX& viewProj) const { mSkybox.Render(viewProj); }

	EEnvironmentMapPresets GetActiveEnvironmentMapPreset() const { return mActiveSkyboxPreset; }
	void SetEnvironmentMap(EEnvironmentMapPresets preset);
	void ResetActiveCamera();

	
	MeshID AddMesh_Async(Mesh m);

private:
	GameObjectPool			mObjectPool;
	MaterialPool			mMaterials;
	ModelLoader				mModelLoader;
	ModelLoadQueue			mModelLoadQueue;

	std::mutex mSceneMeshMutex;

	struct DrawLists
	{
		std::vector<const GameObject*>	opaqueList;
		std::vector<const GameObject*>	alphaList;
	};

	DrawLists				mDrawLists;

	BoundingBox		mBoundingBox;

private:
	void StartLoadingModels();
	void EndLoadingModels();
	void CalculateSceneBoundingBox();
};







struct SerializedScene
{
	GameObject*		CreateNewGameObject();

	std::vector<Settings::Camera>	cameras;
	std::vector<Light>				lights;
	DirectionalLight				directionalLight;
	MaterialPool					materials;
	std::vector<GameObject>			objects;
	Settings::SceneRender			settings;
	char loadSuccess = '0';
};

constexpr size_t SIZE_BRDF = sizeof(BRDF_Material);			// 72 Bytes
constexpr size_t SIZE_PHNG = sizeof(BlinnPhong_Material);	// 72 Bytes
// if we use 64 bits for the MaterialID, we can use 32-bit to index each array
