//
// This file is used by the template to render a basic scene using GL.
//

//#include "pch.h"

#include "MathHelper.h"

// These are used by the shader compilation methods.
#include <vector>
#include <iostream>
#include <fstream>

// Vertigo
//#include <VrFoundation/VrKernelConfig.h>
#include "VrFoundation/VrMesh.h"
#include "VrFoundation/VrSequence.h"
#include "VrFoundation/VrKeyFrameController.h"
#include "VrFoundation/VrAnimationManager.h"
#include "VrFoundation/VrEvaluator.h"
#include "VrFoundation/VrMainTaskManager.h"
#include "VrFoundation/VrPluginManager.h"
#include "VrFoundation/VrIOManager.h"

#include "VrGraphics/VrGraphicsViewMultiPass.h"
#include "VrGraphics/VrGeometry.h"
#include "VrGraphics/VrGeometryUtils.h"
#include "VrGraphics/VrShader.h"
#include "VrGraphics/VrMaterial.h"
#include "VrGraphics/VrEGLWindow.h"
#include <VrGraphics/VrGraphicsView.h>
#include <VrGraphics/VrPrimitive.h>
#include <VrFoundation/VrStringUtils.h>

#include "VrScene/VrSceneCore.h"
#include "VrScene/VrCameraPass.h"
#include "VrScene/VrNodeAction.h"
#include "VrScene/VrShaderVisitor.h"
#include "VrScene/VrSceneNode.h"
#include "VrScene/VrShapeNode.h"
#include "VrScene/VrLightNode.h"
#include "VrScene/VrTransformNode.h"
#include "VrScene/VrSceneUpdater.h"
#include "VrScene/VrSceneManager.h"
#include "VrScene/VrSceneEngine.h"
#include "VrScene/VrAnimationDecorator.h"
#include "VrScene/VrCameraNode.h"

#include "VrApplication/VrDeviceManager.h"

#include "VrHololensDeviceDriver.h"
#include "VrHololensDevice.h"

// EGL
#include <VrPackPush.h>
#	include <EGL/egl.h>
#	include <GLES2/gl2.h>
#include <VrPackPop.h>

using namespace VrFoundation;
using namespace VrGraphics;
using namespace VrScene;
using namespace VrApplication;
using namespace VrHololens;

#include "SimpleRenderer.h"


using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Holographic;
using namespace Windows::Perception::Spatial;

using namespace Platform;
using namespace HolographicAppForOpenGLES1;

/**
 * The window containing the 3D scene
 */
VrGraphics::VrEGLWindow *gRenderWindow = NULL;
VrScene::VrAnimationDecorator*	gAnimationDecorator = NULL;
VrGraphics::VrShader *gBoxShader = NULL;

/*const VrString lDefaultPluginsDir = "..\\..\\InteroperabilityPlugins\\Bin";*/
const VrString lDefaultPluginsDir = ".";

Windows::Perception::Spatial::SpatialStationaryFrameOfReference^ gStationaryReferenceFrame = nullptr;

/******************************************************************************
 * Load the plugins
 ******************************************************************************/
void loadPlugins(const VrString pPluginsDir)
{
	char buf[512];
	GetCurrentDirectory(512, (LPWSTR)buf);

	std::vector<VrString> lPluginsDirs;
	VrFoundation::splitString(pPluginsDir, ",", lPluginsDirs, false);

	std::vector<VrString>::const_iterator lCurDirIt = lPluginsDirs.begin();
	std::vector<VrString>::const_iterator lEndDirIt = lPluginsDirs.end();

	for
		(; lCurDirIt NEQ lEndDirIt; ++lCurDirIt)
	{
		std::cout << "Loading plugins from " << *lCurDirIt << std::endl;
#if defined(_DEBUG)
		VrFoundation::VrPluginManager::get().load(VrURL(*lCurDirIt, VrURL::eLocalPath), "*.d.vip");
#elif defined(VR_RELWITHDEBINFO)
		VrFoundation::VrPluginManager::get().load(VrURL(*lCurDirIt, VrURL::eLocalPath), "*.rd.vip");
#else
		VrFoundation::VrPluginManager::get().load(VrURL(*lCurDirIt, VrURL::eLocalPath), "*.vip", "*.d.vip;*.rd.vip");
#endif
	}
}


/******************************************************************************
 * Find the first child node with the given name
 ******************************************************************************/
static VrCameraNode *findNodeByName(VrSceneNode *pRootNode, VrString pName)
{
	VrCameraNode *lCamera = NULL;
	std::vector<VrNodeInterface*> lNodes;
	VrScene::findNodesByName(pRootNode, pName, lNodes);

	if
		(NOT lNodes.empty())
	{
		VrAnimationDecorator *lAnimation = dynamic_cast<VrAnimationDecorator *>(lNodes[0]);
		if
			(lAnimation)
		{
			lCamera = dynamic_cast<VrCameraNode *>(lAnimation->editReference());
		}
	}

	return lCamera;
}

/******************************************************************************
* Create a simple scene.
******************************************************************************/
//#define USE_V3B
void createScene(Windows::Graphics::Holographic::HolographicSpace^ pHolographicSpace)
{
#ifdef USE_V3B
	// Load a V3B file.
	VrNode *lBoxShape = NULL;
	VrObject *lObject = NULL;
	std::vector<VrObject *> lObjects;
	//VrString lV3BFilename = "/data/projects/v3b/Tree/tree.v3b";
	//VrString lV3BFilename = "/data/projects/saurous/Delivery/Bin/old/data/Models/Planes/A320-200/D11110391.V3B";
	//VrString lV3BFilename = "/data/projects/v3b/bench.v3b";
	VrString lV3BFilename = "Assets/cube.v3b";
	//VrString lV3BFilename = "/data/projects/v3b/Sponza/sponza03.v3b";
	std::cout << "Loading '" << lV3BFilename << "'." << std::endl;
	VrURL lObjectURL = VrURL(lV3BFilename, VrURL::eFile);
	VrIOManager::get().loadObjects(lObjectURL, lObjects);
	if
		(NOT lObjects.empty() AND lObjects[0] NEQ NULL)
	{
		lObject = lObjects[0];
		if
			(lObject->isA(VrNode::getType()))
		{
			lBoxShape = assert_cast<VrNode *>(lObject);
		}
	}
	if
		(NOT lBoxShape)
	{
		std::cout << "Loading failed." << std::endl;
		return;
	}
#else
	// Create a box.
	VrShapeNode *lBoxShape = VrShapeNode::create();
	VrMesh lBoxMesh = VrFoundation::createBox(0.3, 0.3, 0.3);
	VrPrimitive *lBoxPrimitive = VrGraphics::createPrimitive(lBoxMesh);
	lBoxPrimitive->setCullMode(VrPrimitive::VrCullMode::eDisabled);
	VrGeometry *lBoxGeometry = VrGeometry::create();
	lBoxGeometry->insertPrimitive(lBoxPrimitive);
	lBoxShape->setGeometry(lBoxGeometry);

	// Create the shader (i.e. the material) associated to the box shape.
	gBoxShader = VrShader::create();
	VrMaterial *lBoxMaterial = VrMaterial::create();
	lBoxMaterial->setAmbientRGB(VrColor(0.1f, 0.2f, 0.8f));
	lBoxMaterial->setSpecularRGB(VrColor(1.0f, 1.0f, 1.0f));
	lBoxMaterial->setDiffuseRGB(VrColor(0.8f, 0.0f, 0.1f));
	gBoxShader->setMaterial(lBoxMaterial);
	lBoxShape->setShader(gBoxShader);
#endif

	// Create a light.
	VrLightNode *lLight = VrLightNode::create();
	lLight->setAmbient(VrColor(0.5f, 0.5f, 0.5f));
	lLight->setDiffuse(VrColor(0.8f, 0.8f, 0.8f));
	lLight->setLightType(VrLight::eDirectional);
	lLight->setIntensity(1.0f);

	// Create a camera.
	VrCameraNode *lCameraNode = VrCameraNode::create();
	lCameraNode->setPosition(VrVec3t(0, 0, 0));
	/*lCameraNode->setOrientation(VrQuatt(VrVec3t(1, 0, 0), cPi));*/
	lCameraNode->setBackgroundColor(VrColor(1.0f, 1.0f, 1.0f, 1.0f));

	// Create the scene graph.
	VrSceneNode *lSceneGraph = VrSceneNode::create();
	lSceneGraph->insertChild(lLight);
	lSceneGraph->insertChild(lCameraNode);
#ifdef USE_V3B
	lSceneGraph->insertChild(lBoxShape);
#endif

	VrDeviceManager::VrDeviceParametersVector lDeviceParameters;
	VrHololensDevice *lDevice = static_cast<VrHololensDevice *>(VrDeviceManager::get().editDeviceDriverByName(VrString("HololensDeviceDriver"))->editDevice(VrString(""), lDeviceParameters));
	lDevice->setHolographicSpace(pHolographicSpace);
	lDevice->setCameraNode(lCameraNode);

	/*VrCameraNode *lCameraNode = findNodeByName(lSceneGraph, "Camera01");*/

	// Initialize the animation engine.
	/*VrAnimationManager::get().start();
	VrSceneManager* lSceneManager = VrSceneEngine::get().editSceneManager(lSceneGraph);
	lSceneManager->editSceneUpdater().resume();
	lSceneManager->editSceneUpdater().setEnabled(true);*/

	// Create a CameraPass.
	VrCameraPass *lCameraPass = VrCameraPass::create();
	lCameraPass->setRootNode(lSceneGraph);
	lCameraPass->setCameraNode(lCameraNode);

	// Create a GraphicsViewMultiPass.
	VrGraphicsViewMultiPass *lView = VrGraphicsViewMultiPass::create();
	lView->setMainPass(lCameraPass);
	gRenderWindow->editCanvas()->insertGraphicsViewMultiPass(lView);

#ifndef USE_V3B
	// Animation : box translation
	// -->Transform node
	VrTransformNode* lTransformNode = VrTransformNode::create();
	lTransformNode->insertChild(lBoxShape);
	lTransformNode->setPosition(VrVec3t(0, 0, -2));
	lTransformNode->setOrientation(VrQuatf(VrVec3f(1.0f, 0.0f, 1.0f), cPi / 4));
	lSceneGraph->insertChild(lTransformNode);

	// Create the evaluator and insert some key frames.
	VrLinearEvaluator<VrVec3f, VrFloat> *lEvaluator1 = new VrLinearEvaluator<VrVec3f, VrFloat>;
	VrLinearEvaluator<VrVec3f, VrFloat>::VrKeyFrame lLinearKF;
	lLinearKF.mTime = 0.0;
	lLinearKF.mData = VrVec3f(0.0f, 0.0f, 0.0f);
	lEvaluator1->addKeyFrame(lLinearKF);
	lLinearKF.mTime = 2.5;
	lLinearKF.mData = VrVec3f(5.0f, 5.0f, 5.0f);
	lEvaluator1->addKeyFrame(lLinearKF);
	lLinearKF.mTime = 7.5;
	lLinearKF.mData = VrVec3f(-5.0f, -5.0f, -5.0f);
	lEvaluator1->addKeyFrame(lLinearKF);
	lLinearKF.mTime = 10.0;
	lLinearKF.mData = VrVec3f(0.0f, 0.0f, 0.0f);
	lEvaluator1->addKeyFrame(lLinearKF);

	// Create the controller.
	// Set the interpolation type.
	VrKeyFrameController<VrLinearEvaluator<VrVec3f, VrFloat> > *lController1 = new VrKeyFrameController<VrLinearEvaluator<VrVec3f, VrFloat> >;
	lController1->setTarget(lTransformNode);
	lController1->setAttributeName(VrString("Position"));
	lController1->setEvaluator(lEvaluator1);

	// -->Sequence
	VrSequence *lSequence = new VrSequence;
	lSequence->setDuration(10.0f);
	lSequence->setLoop();
	lSequence->insertController(lController1);

	// Animation : box rotation.

	// Create the evaluator and insert some key frames.
	VrQuatfSlerpEvaluator* lEvaluator2 = new VrQuatfSlerpEvaluator();
	VrQuatfSlerpEvaluator::VrKeyFrame lSlerpKF;
	lSlerpKF.mTime = 0.0;
	lSlerpKF.mData = VrQuatf(0.f, 0.f, 0.f);
	lEvaluator2->addKeyFrame(lSlerpKF);
	lSlerpKF.mTime = 10.0;
	lSlerpKF.mData = VrQuatf(-3.14f, 3.14f, 0.f);
	lEvaluator2->addKeyFrame(lSlerpKF);

	// Create the controller.
	// Set the interpolation type.
	VrQuatfSlerpController* lController2 = new VrQuatfSlerpController();
	lController2->setTarget(lTransformNode);
	lController2->setAttributeName(VrString("Orientation"));
	lController2->setEvaluator(lEvaluator2);

	// -->Sequence
	lSequence->insertController(lController2);

	gAnimationDecorator = dynamic_cast<VrAnimationDecorator*>(VrAnimationDecorator::create(VrAnimationDecorator::getType(), lTransformNode));
	if
		(gAnimationDecorator)
	{
		gAnimationDecorator->insertSequence(lSequence);
	}
#endif
}

void SimpleRenderer::buildWindow()
{
	// Get the default SpatialLocator.
	Windows::Perception::Spatial::SpatialLocator^ locator = Windows::Perception::Spatial::SpatialLocator::GetDefault();

	// Create a stationary frame of reference.
	gStationaryReferenceFrame = locator->CreateStationaryFrameOfReferenceAtCurrentLocation();

	const EGLint configAttributes[] =
	{
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 8,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};

	const EGLint contextAttributes[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	const EGLint surfaceAttributes[] =
	{
		// EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER is part of the same optimization as EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER (see above).
		// If you have compilation issues with it then please update your Visual Studio templates.
		EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER, EGL_TRUE,
		EGL_NONE
	};

	const EGLint defaultDisplayAttributes[] =
	{
		// These are the default display attributes, used to request ANGLE's D3D11 renderer.
		// eglInitialize will only succeed with these attributes if the hardware supports D3D11 Feature Level 10_0+.
		EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,

		// EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER is an optimization that can have large performance benefits on mobile devices.
		// Its syntax is subject to change, though. Please update your Visual Studio templates if you experience compilation issues with it.
		EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,

		// EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE is an option that enables ANGLE to automatically call 
		// the IDXGIDevice3::Trim method on behalf of the application when it gets suspended. 
		// Calling IDXGIDevice3::Trim when an application is suspended is a Windows Store application certification requirement.
		EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
		EGL_NONE,
	};

	const EGLint fl9_3DisplayAttributes[] =
	{
		// These can be used to request ANGLE's D3D11 renderer, with D3D11 Feature Level 9_3.
		// These attributes are used if the call to eglInitialize fails with the default display attributes.
		EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
		EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE, 9,
		EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE, 3,
		EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
		EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
		EGL_NONE,
	};

	const EGLint warpDisplayAttributes[] =
	{
		// These attributes can be used to request D3D11 WARP.
		// They are used if eglInitialize fails with both the default display attributes and the 9_3 display attributes.
		EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
		EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE,
		EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
		EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
		EGL_NONE,
	};

	EGLConfig config = NULL;

	// eglGetPlatformDisplayEXT is an alternative to eglGetDisplay. It allows us to pass in display attributes, used to configure D3D11.
	PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
	if (!eglGetPlatformDisplayEXT)
	{
		throw Exception::CreateException(E_FAIL, L"Failed to get function eglGetPlatformDisplayEXT");
	}

	//
	// To initialize the display, we make three sets of calls to eglGetPlatformDisplayEXT and eglInitialize, with varying 
	// parameters passed to eglGetPlatformDisplayEXT:
	// 1) The first calls uses "defaultDisplayAttributes" as a parameter. This corresponds to D3D11 Feature Level 10_0+.
	// 2) If eglInitialize fails for step 1 (e.g. because 10_0+ isn't supported by the default GPU), then we try again 
	//    using "fl9_3DisplayAttributes". This corresponds to D3D11 Feature Level 9_3.
	// 3) If eglInitialize fails for step 2 (e.g. because 9_3+ isn't supported by the default GPU), then we try again 
	//    using "warpDisplayAttributes".  This corresponds to D3D11 Feature Level 11_0 on WARP, a D3D11 software rasterizer.
	//

	// This tries to initialize EGL to D3D11 Feature Level 10_0+. See above comment for details.
	mEGLDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, defaultDisplayAttributes);
	if (mEGLDisplay == EGL_NO_DISPLAY)
	{
		throw Exception::CreateException(E_FAIL, L"Failed to get EGL display");
	}

	if (eglInitialize(mEGLDisplay, NULL, NULL) == EGL_FALSE)
	{
		// This tries to initialize EGL to D3D11 Feature Level 9_3, if 10_0+ is unavailable (e.g. on some mobile devices).
		mEGLDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, fl9_3DisplayAttributes);
		if (mEGLDisplay == EGL_NO_DISPLAY)
		{
			throw Exception::CreateException(E_FAIL, L"Failed to get EGL display");
		}

		if (eglInitialize(mEGLDisplay, NULL, NULL) == EGL_FALSE)
		{
			// This initializes EGL to D3D11 Feature Level 11_0 on WARP, if 9_3+ is unavailable on the default GPU.
			mEGLDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, warpDisplayAttributes);
			if (mEGLDisplay == EGL_NO_DISPLAY)
			{
				throw Exception::CreateException(E_FAIL, L"Failed to get EGL display");
			}

			if (eglInitialize(mEGLDisplay, NULL, NULL) == EGL_FALSE)
			{
				// If all of the calls to eglInitialize returned EGL_FALSE then an error has occurred.
				throw Exception::CreateException(E_FAIL, L"Failed to initialize EGL");
			}
		}
	}

	EGLint numConfigs = 0;
	if ((eglChooseConfig(mEGLDisplay, configAttributes, &config, 1, &numConfigs) == EGL_FALSE) || (numConfigs == 0))
	{
		throw Exception::CreateException(E_FAIL, L"Failed to choose first EGLConfig");
	}

	// Create a PropertySet and initialize with the EGLNativeWindowType.
	PropertySet^ surfaceCreationProperties = ref new PropertySet();
	surfaceCreationProperties->Insert(ref new String(EGLNativeWindowTypeProperty), mHolographicSpace);
	if (gStationaryReferenceFrame != nullptr)
	{
		surfaceCreationProperties->Insert(ref new String(EGLBaseCoordinateSystemProperty), gStationaryReferenceFrame);
	}

	// You can configure the surface to render at a lower resolution and be scaled up to
	// the full window size. This scaling is often free on mobile hardware.
	//
	// One way to configure the SwapChainPanel is to specify precisely which resolution it should render at.
	// Size customRenderSurfaceSize = Size(800, 600);
	// surfaceCreationProperties->Insert(ref new String(EGLRenderSurfaceSizeProperty), PropertyValue::CreateSize(customRenderSurfaceSize));
	//
	// Another way is to tell the SwapChainPanel to render at a certain scale factor compared to its size.
	// e.g. if the SwapChainPanel is 1920x1280 then setting a factor of 0.5f will make the app render at 960x640
	// float customResolutionScale = 0.5f;
	// surfaceCreationProperties->Insert(ref new String(EGLRenderResolutionScaleProperty), PropertyValue::CreateSingle(customResolutionScale));

	mEGLSurface = eglCreateWindowSurface(mEGLDisplay, config, reinterpret_cast<IInspectable*>(surfaceCreationProperties), surfaceAttributes);
	if (mEGLSurface == EGL_NO_SURFACE)
	{
		throw Exception::CreateException(E_FAIL, L"Failed to create EGL fullscreen surface");
	}

	mEGLContext = eglCreateContext(mEGLDisplay, config, EGL_NO_CONTEXT, contextAttributes);
	if (mEGLContext == EGL_NO_CONTEXT)
	{
		throw Exception::CreateException(E_FAIL, L"Failed to create EGL context");
	}

	if (eglMakeCurrent(mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext) == EGL_FALSE)
	{
		throw Exception::CreateException(E_FAIL, L"Failed to make fullscreen EGLSurface current");
	}
}

void SimpleRenderer::setEGL(EGLDisplay pEGLDisplay, EGLSurface pEGLSurface, EGLContext pEGLContext)
{
	mEGLDisplay = pEGLDisplay;
	mEGLSurface = pEGLSurface;
	mEGLContext = pEGLContext;
}

void SimpleRenderer::initialize()
{
	gRenderWindow->initialize();

	createScene(mHolographicSpace);

	VrMainTaskManager::get().insertTask(gRenderWindow->editCanvas()->editRendererTask());
	gRenderWindow->editCanvas()->editRendererTask().enable();

	VrMainTaskManager::get().insertTask(VrDeviceManager::get(), 0);
}

SimpleRenderer::SimpleRenderer(bool isHolographic, Windows::Graphics::Holographic::HolographicSpace ^pHolographicSpace) :
    mWindowWidth(1440/*1268*/),
    mWindowHeight(936/*720*/),
    mDrawCount(0)
{
	mHolographicSpace = pHolographicSpace;

	Vr::initialize();

	// Load all the plugins.
	loadPlugins(lDefaultPluginsDir);

	gRenderWindow = new VrEGLWindow(mWindowWidth, mWindowHeight, mHolographicSpace);
}

SimpleRenderer::~SimpleRenderer()
{
    if (mProgram != 0)
    {
        glDeleteProgram(mProgram);
        mProgram = 0;
    }

    if (mVertexPositionBuffer != 0)
    {
        glDeleteBuffers(1, &mVertexPositionBuffer);
        mVertexPositionBuffer = 0;
    }

    if (mVertexColorBuffer != 0)
    {
        glDeleteBuffers(1, &mVertexColorBuffer);
        mVertexColorBuffer = 0;
    }

    if (mIndexBuffer != 0)
    {
        glDeleteBuffers(1, &mIndexBuffer);
        mIndexBuffer = 0;
    }
}

void SimpleRenderer::Draw()
{
	VrMainTaskManager::get().update();
}

void SimpleRenderer::UpdateWindowSize(GLsizei width, GLsizei height)
{
}
