//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#ifndef PXR_USDWEB_FREECAMERAGL_H
#define PXR_USDWEB_FREECAMERAGL_H

#include <pxr/base/gf/vec2d.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec4d.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/camera.h>
#include <pxr/base/gf/frustum.h> // @REVISIT: can remove if GfFrustum removed from header
#include <pxr/usd/usd/prim.h>

class FreeCameraGL
{
public:
	FreeCameraGL(
	    bool  isZUp=false,
		float aspectRatio=1.0, 
		float fov=30.0, 
		float overrideNear = FreeCameraGL::defaultNear, 
		float overrideFar = FreeCameraGL::defaultFar
        );
	virtual ~FreeCameraGL(){}

    // Static Const
    static constexpr float defaultNear = 1.0f;
    static constexpr float defaultFar = 2000000.0f;
    // Experimentally on Nvidia M6000, if Far/Near is greater than this,
    // then geometry in the back half of the volume will disappear
    static constexpr float maxSafeZResolution = 1e6f;
    // Experimentally on Nvidia M6000, if Far/Near is greater than this,
    // then we will often see Z-fighting artifacts even for geometry that
    // is close to camera, when rendering for picking
    static constexpr float maxGoodZResolution = 5e4f;

    static const pxr::GfMatrix4d _ZUpRotMatrix;
    static const pxr::GfMatrix4d _ZUpRotInvMatrix;

    // =====
    // Functions incorporated from usdweb camera.h 
    //
	void update();
	void mouseUp();
	void mouseDown(int button, int action, int mods,int xpos,int ypos);
	void mouseMove(int xpos, int ypos);
	void mouseWheel(double xoffset ,double yoffset);

	// @REVISIT: Cache instead of computing each time ComputeViewMatrix(), ComputeProjectionMatrix()
	// Referenced from python code
    // 		renderer.SetCameraState(frustum.ComputeViewMatrix(),
    //                              frustum.ComputeProjectionMatrix())

    // 		viewProjectionMatrix = Gf.Matrix4f(frustum.ComputeViewMatrix()
    //                                         * frustum.ComputeProjectionMatrix())
	const pxr::GfMatrix4d getViewMatrix()       { return _gfCamera.GetFrustum().ComputeViewMatrix(); }
	const pxr::GfMatrix4d getViewInverse()      { return _gfCamera.GetFrustum().ComputeViewInverse(); }
    const pxr::GfMatrix4d getProjectionMatrix() { return _gfCamera.GetFrustum().ComputeProjectionMatrix();}
	const pxr::GfVec3d    getPosition()         { return _gfCamera.GetFrustum().GetPosition();}

	void setGfCamera(pxr::GfCamera &gfCam) { _gfCamera = gfCam; }

	pxr::GfVec2f _screenDimensions = pxr::GfVec2f(1.0, 1.0);
	void setViewportDimensions(const pxr::GfVec2f &screenDims) { _screenDimensions = screenDims; }
	// ======

public:
	enum Camera_Mode {
		NONE=0,
		TUMBLE,
		TRUCK,
		PAN,
		ZOOM,
		PICK
	};

    static pxr::GfMatrix4d RotMatrix(const pxr::GfVec3d &vec, float angle);
    void PushToCameraTransform();
    void PullFromCameraTransform();
	void LookThroughUsdCamera(const pxr::UsdPrim &cameraPrim, const pxr::UsdTimeCode &atTime);
    //def _rangeOfBoxAlongRay(self, camRay, bbox, debugClipping=False):
    void SetClippingPlanes(pxr::GfBBox3d &stageBBox);
    void ResetClippingPlanes();
    void FrameSelection(pxr::GfBBox3d &selBBox, float frameFit);
	//void setClosestVisibleDistFromPoint(self, point);
	float ComputePixelsToWorldFactor(float viewportHeight);
	void Tumble(float dTheta, float dPhi);
	void AdjustDistance(float scaleFactor);
	void Truck(float deltaRight, float deltaUp);
	void PanTilt(float dPan, float dTilt);
	void Walk(float dForward, float dRight);
	void Zoom(float zoomDelta); // added
	float GetFOV();
	void SetFOV(float fov);
	void SetIsZUp(bool isZUp) { _isZUp = isZUp; }
	bool IsZUp() const { return _isZUp ; }

protected:

private:
	pxr::GfCamera _gfCamera;
    float _overrideNear = 0.0f;
    float _overrideFar = 0.0f;
    int _lastX = 0;
    int _lastY = 0;
    bool _dragActive = false;
    Camera_Mode _cameraMode = Camera_Mode::NONE;
    bool _isZUp = false;

    bool _cameraTransformDirty = false;
    float _rotTheta = 0;
    float _rotPhi = 0;
    float _rotPsi = 0;
    pxr::GfVec3d _center = pxr::GfVec3d(0.0, 0.0, 0.0);
    float _dist = 100;
    float _closestVisibleDist=0.0f;// = None;
    //float _lastFramedDist=0.0f;// = None; // used in SetClippingPlanes()
    //float _lastFramedClosestDist=0.0f;// = None;  // used in SetClippingPlanes()
    float _selSize = 10;


};

#endif