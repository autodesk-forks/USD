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

//
// Reference:
//      stageView.py
//      freeCamera.py
//

#include "freeCameraGL.h"

#include <pxr/base/gf/vec2d.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/quaternion.h>
#include <pxr/base/gf/frustum.h>

#include <GLFW/glfw3.h>


const pxr::GfMatrix4d FreeCameraGL::_ZUpRotMatrix = pxr::GfMatrix4d().SetRotate(pxr::GfRotation(pxr::GfVec3d::XAxis(), -90));
const pxr::GfMatrix4d FreeCameraGL::_ZUpRotInvMatrix = FreeCameraGL::_ZUpRotMatrix.GetInverse();


FreeCameraGL::FreeCameraGL(
    bool  isZUp,
    float aspectRatio,
    float fov, 
    float overrideNear, 
    float overrideFar) 
{
    _isZUp = isZUp;
    _overrideNear = overrideNear;
    _overrideFar = overrideFar;
    _gfCamera.SetPerspectiveFromAspectRatioAndFieldOfView(aspectRatio, fov, pxr::GfCamera::FOVVertical);
    ResetClippingPlanes();
    _gfCamera.SetFocusDistance(_dist);
}


pxr::GfMatrix4d FreeCameraGL::RotMatrix(const pxr::GfVec3d &vec, float angle)
{
    return pxr::GfMatrix4d(1.0).SetRotate(pxr::GfRotation(vec, angle));
}

// Updates the camera's transform matrix, that is, the matrix that brings
// the camera to the origin, with the camera view pointing down:
//    +Y if this is a Zup camera, or
//    -Z if this is a Yup camera .
void FreeCameraGL::PushToCameraTransform()
{
    if (!_cameraTransformDirty)
        return;

    pxr::GfMatrix4d cam_xform = pxr::GfMatrix4d().SetTranslate(pxr::GfVec3d::ZAxis() * _dist) *
        RotMatrix(pxr::GfVec3d::ZAxis(), -_rotPsi) *
        RotMatrix(pxr::GfVec3d::XAxis(), -_rotPhi) *
        RotMatrix(pxr::GfVec3d::YAxis(), -_rotTheta);
    if (_isZUp) {
        // _ZUpRotInvMatrix (90 degree rotation around the x-Axis) influences how the FreeCamera will tumble
        cam_xform *= FreeCameraGL::_ZUpRotInvMatrix;
    }
    cam_xform *= pxr::GfMatrix4d().SetTranslate(_center);
    _gfCamera.SetTransform(cam_xform);
    _gfCamera.SetFocusDistance(_dist);
    _cameraTransformDirty = false;
}

// Updates parameters (center, rotTheta, etc.) from the camera transform.
void FreeCameraGL::PullFromCameraTransform()
{
    // # reads the transform set on the camera and updates all the other
    // # parameters.  This is the inverse of PushToCameraTransform
    pxr::GfMatrix4d cam_transform = _gfCamera.GetTransform();
    float dist = _gfCamera.GetFocusDistance();
    pxr::GfFrustum frustum = _gfCamera.GetFrustum();
    pxr::GfVec3d cam_pos = frustum.GetPosition();
    pxr::GfVec3d cam_axis = frustum.ComputeViewDirection();

    //# Compute translational parts
    _dist = dist;
    _selSize = dist / 10.0;
    _center = cam_pos + dist * cam_axis;

    // # self._YZUpMatrix influences the behavior about how the
    // # FreeCamera will tumble. It is the identity or a rotation about the
    // # x-Axis.

    //# Compute rotational part
    pxr::GfMatrix4d transform = cam_transform;
    if (_isZUp) {
        transform *= FreeCameraGL::_ZUpRotMatrix;
    }
    transform.Orthonormalize();
    pxr::GfRotation rotation = transform.ExtractRotation();

    //# Decompose and set angles
    pxr::GfVec3d tmp_decompose = -rotation.Decompose(
        pxr::GfVec3d::YAxis(), 
        pxr::GfVec3d::XAxis(), 
        pxr::GfVec3d::ZAxis());
    _rotTheta = tmp_decompose[0]; 
    _rotPhi   = tmp_decompose[1];
    _rotPsi   = tmp_decompose[2];

    _cameraTransformDirty = true;
}

//def _rangeOfBoxAlongRay(self, camRay, bbox, debugClipping=False):


// Computes and sets automatic clipping plane distances using the
// camera's position and orientation, the bouding box
// surrounding the stage, and the distance to the closest rendered
// object in the central view of the camera (closestVisibleDist).
//
// If either of the "override" clipping attributes are not None,
// we use those instead
void FreeCameraGL::SetClippingPlanes(pxr::GfBBox3d &stageBBox)
{
    //debugClipping = Tf.Debug.IsDebugSymbolNameEnabled(DEBUG_CLIPPING)

    //# If the scene bounding box is empty, or we are fully on manual
    //# override, then just initialize to defaults.
    float computedNear;
    float computedFar;
    if (stageBBox.GetRange().IsEmpty() or \
           (_overrideNear and _overrideFar)) {
        computedNear = FreeCameraGL::defaultNear;
        computedFar  = FreeCameraGL::defaultFar;
    }
    else {
        // @REVISIT: Defer on auto-calc clipping planes for now
        // Requires _rangeOfBoxAlongRay
        computedNear = FreeCameraGL::defaultNear;
        computedFar  = FreeCameraGL::defaultFar;
    }
    float near = (_overrideNear) ? _overrideNear : computedNear;
    float far  = (_overrideFar)  ? _overrideFar  : computedFar;
    //# Make sure far is greater than near
    far = std::fmax(near+1, far);

    //if debugClipping:
    //    print("***Final Near/Far: {}, {}".format(near, far))

    _gfCamera.SetClippingRange(pxr::GfRange1f(near, far));
}


//Set near and far back to their uncomputed defaults.
void FreeCameraGL::ResetClippingPlanes() 
{
    float near = (_overrideNear) ? _overrideNear : FreeCameraGL::defaultNear;
    float far  = (_overrideFar)  ? _overrideFar  : FreeCameraGL::defaultFar;
    _gfCamera.SetClippingRange(pxr::GfRange1f(near, far));
}


void FreeCameraGL::FrameSelection(pxr::GfBBox3d &selBBox, float frameFit)
{
    //# needs to be recomputed
    _closestVisibleDist = 0; // versus None

    _center = selBBox.ComputeCentroid();
    _cameraTransformDirty = true;
    //self.signalFrustumChanged.emit()
    pxr::GfRange3d selRange = selBBox.ComputeAlignedRange();
    pxr::GfVec3d selRangeSize = selRange.GetSize();
    _selSize = std::fmax(std::fmax(selRangeSize[0], selRangeSize[1]), selRangeSize[2]);
    if (_gfCamera.GetProjection() == pxr::GfCamera::Orthographic) {
        SetFOV( _selSize * frameFit );
        _dist = _selSize + FreeCameraGL::defaultNear;
    }
    else {
        float halfFov = (GetFOV()) ? GetFOV()*0.5 : 0.5; //# don't divide by zero
        float lengthToFit = _selSize * frameFit * 0.5;
        _dist = lengthToFit / atan(halfFov*M_PI/180.0);
        // # Very small objects that fill out their bounding boxes (like cubes)
        // # may well pierce our 1 unit default near-clipping plane. Make sure
        // # that doesn't happen.
        if (_dist < FreeCameraGL::defaultNear + _selSize * 0.5) {
            _dist = FreeCameraGL::defaultNear + lengthToFit;
        }
    }
}


//def setClosestVisibleDistFromPoint(self, point):


// Computes the ratio that converts pixel distance into world units.
//
// It treats the pixel distances as if they were projected to a plane going
// through the camera center.
//
float FreeCameraGL::ComputePixelsToWorldFactor(float viewportHeight) 
{

    PushToCameraTransform();
    if (_gfCamera.GetProjection() == pxr::GfCamera::Orthographic) {
        return GetFOV() / viewportHeight;
    }
    else {
        float frustumHeight = _gfCamera.GetFrustum().GetWindow().GetSize()[1];
        return frustumHeight * _dist / viewportHeight;
    }
}


// Tumbles the camera around the center point by (dTheta, dPhi) degrees.
//
void FreeCameraGL::Tumble(float dTheta, float dPhi) 
{
    _rotTheta += dTheta;
    _rotPhi += dPhi;
    _cameraTransformDirty = true;
    //signalFrustumChanged.emit()
}

//Scales the distance of the freeCamera from it's center typically by
//scaleFactor unless it puts the camera into a "stuck" state.
//
void FreeCameraGL::AdjustDistance(float scaleFactor)
{
    //# When dist gets very small, you can get stuck and not be able to
    //# zoom back out, if you just keep multiplying.  Switch to addition
    //# in that case, choosing an incr that works for the scale of the
    //# framed geometry.
    if (scaleFactor > 1 and _dist < 2) {
        float selBasedIncr = _selSize / 25.0;
        scaleFactor -= 1.0;
        _dist += std::fmin(selBasedIncr, scaleFactor);
    }
    else {
        _dist *= scaleFactor;
    }

    // TODO
    /*
    # Make use of our knowledge that we are changing distance to camera
    # to also adjust _closestVisibleDist to keep it useful.  Make sure
    # not to recede farther than the last *computed* closeDist, since that
    # will generally cause unwanted clipping of close objects.
    # XXX:  This heuristic does a good job of preventing undesirable
    # clipping as we zoom in and out, but sacrifices the z-buffer
    # precision we worked hard to get.  If Hd/UsdImaging could cheaply
    # provide us with the closest-point from the last-rendered image,
    # we could use it safely here to update _closestVisibleDist much
    # more accurately than this calculation.
    if self._closestVisibleDist:
        if self.dist > self._lastFramedDist:
            self._closestVisibleDist = self._lastFramedClosestDist
        else:
            self._closestVisibleDist = \
                self._lastFramedClosestDist - \
                self._lastFramedDist + \
                self.dist
    */
}


//Moves the camera by (deltaRight, deltaUp) in worldspace coordinates. 
//This is similar to a camera Truck/Pedestal.
//
void FreeCameraGL::Truck(float deltaRight, float deltaUp) 
{
    //need to update the camera transform before we access the frustum
    PushToCameraTransform();
    pxr::GfFrustum frustum = _gfCamera.GetFrustum();
    pxr::GfVec3d cam_up = frustum.ComputeUpVector();
    pxr::GfVec3d cam_right = pxr::GfCross(frustum.ComputeViewDirection(), cam_up);
    _center += (deltaRight * cam_right + deltaUp * cam_up);
    _cameraTransformDirty = true;
    //self.signalFrustumChanged.emit()
}


// Rotates the camera around the current camera base (approx. the film
// plane).  Both parameters are in degrees.
//
// This moves the center point that we normally tumble around.
//
// This is similar to a camera Pan/Tilt.
//
void FreeCameraGL::PanTilt(float dPan, float dTilt) 
{
    _gfCamera.SetTransform(
            pxr::GfMatrix4d(1.0).SetRotate(pxr::GfRotation(pxr::GfVec3d::XAxis(), dTilt)) *
            pxr::GfMatrix4d(1.0).SetRotate(pxr::GfRotation(pxr::GfVec3d::YAxis(), dPan)) *
            _gfCamera.GetTransform());
    PullFromCameraTransform();

    // # When we Pan/Tilt, we don't want to roll the camera so we just zero it
    // # out here.
    _rotPsi = 0.0;

    _cameraTransformDirty = true;
    //self.signalFrustumChanged.emit()
}

// Specialized camera movement that moves it on the "horizontal" plane
// 
void FreeCameraGL::Walk(float dForward, float dRight)
{
    //# need to update the camera transform before we access the frustum
    PushToCameraTransform();
    pxr::GfFrustum frustum = _gfCamera.GetFrustum();
    pxr::GfVec3d cam_up = frustum.ComputeUpVector().GetNormalized();
    pxr::GfVec3d cam_forward = frustum.ComputeViewDirection().GetNormalized();
    pxr::GfVec3d cam_right = pxr::GfCross(cam_forward, cam_up);
    pxr::GfVec3d delta = dForward * cam_forward + dRight * cam_right;
    _center += delta;
    _cameraTransformDirty = true;
    //self.signalFrustumChanged.emit()
}


// Specialized camera movement that moves it on the "horizontal" plane
// 
void FreeCameraGL::Zoom(float zoomDelta)
{
    if (_gfCamera.GetProjection() == pxr::GfCamera::Orthographic) {
        // orthographic cameras zoom by scaling fov
        // fov is the height of the view frustum in world units
        SetFOV( GetFOV() * (1 + zoomDelta));
    }
    else {
        // perspective cameras dolly forward or back
        AdjustDistance(1 + zoomDelta);
    }
    _cameraTransformDirty = true;
    //self.signalFrustumChanged.emit()
}


// The vertical field of view, in degrees, for perspective cameras. 
// For orthographic cameras fov is the height of the view frustum, in 
// world units.
//
float FreeCameraGL::GetFOV()
{
        if (_gfCamera.GetProjection() == pxr::GfCamera::Perspective) {
            return _gfCamera.GetFieldOfView(pxr::GfCamera::FOVVertical);
        }
        else {
            return (_gfCamera.GetVerticalAperture() * pxr::GfCamera::APERTURE_UNIT);
        }
}

void FreeCameraGL::SetFOV(float value)
{
    if (_gfCamera.GetProjection() == pxr::GfCamera::Perspective) {
        _gfCamera.SetPerspectiveFromAspectRatioAndFieldOfView(
            _gfCamera.GetAspectRatio(), value, pxr::GfCamera::FOVVertical);
    }
    else {
        _gfCamera.SetOrthographicFromAspectRatioAndSize(
            _gfCamera.GetAspectRatio(), value, pxr::GfCamera::FOVVertical);
    }
    //self.signalFrustumChanged.emit()
    //self.signalFrustumSettingsChanged.emit()
}


void FreeCameraGL::update()
{
    PushToCameraTransform();
}


void FreeCameraGL::mouseDown(int button, int action, int mods, int xpos, int ypos)
{
    _dragActive = true;

    if( action == GLFW_PRESS ) {
        if (true) { // (mods & (GLFW_MOD_ALT | GLFW_MOD_CONTROL)) {
            if (button == GLFW_MOUSE_BUTTON_LEFT) {
                //self.switchToFreeCamera()
                _cameraMode = (mods & GLFW_MOD_CONTROL) ? Camera_Mode::TRUCK : Camera_Mode::TUMBLE;
            }
            else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
                //self.switchToFreeCamera()
                _cameraMode = Camera_Mode::TRUCK;
            }
            if (button == GLFW_MOUSE_BUTTON_RIGHT) {
                //self.switchToFreeCamera()
                _cameraMode = Camera_Mode::ZOOM;
            }
        }
        else {
            _cameraMode = Camera_Mode::PICK;
            //self.pickObject(x, y, event.button(), event.modifiers())
        }
    }

    _lastX = xpos;
    _lastY = ypos;
}

void FreeCameraGL::mouseWheel(double dx ,double dy)
{
    // An implicit ZOOM with mouse wheel
    float zoomDelta = -.002 * (dx + dy);
    Zoom(zoomDelta);
}

void FreeCameraGL::mouseMove(int xpos,int ypos)
{
    //if( state == CAM_STATE::ROTATE )
    //    rotEnd = getMouseProjectionOnBall(xpos,ypos);
    //else if( state == CAM_STATE::PAN )
    //    panEnd = getMouseOnScreen(xpos,ypos);

    if (_dragActive) {
        int dx = xpos - _lastX;
        int dy = ypos - _lastY;
        if (dx == 0 && dy == 0) {
            return;
        }

        if (_cameraMode == Camera_Mode::TUMBLE) {
            Tumble(0.25 * dx, 0.25*dy);
        }
        else if (_cameraMode == Camera_Mode::ZOOM) {
            float zoomDelta = -.002 * (dx + dy);
            Zoom(zoomDelta);
        }
        else if (_cameraMode == Camera_Mode::TRUCK) {
            // TODO
            float height = _screenDimensions[1];
            float pixelsToWorld = ComputePixelsToWorldFactor(height);
            Truck(-dx * pixelsToWorld, dy * pixelsToWorld);
        }

        _lastX = xpos;
        _lastY = ypos;
        //self.updateGL()
    }
}

/*
pxr::GfVec2d FreeCameraGL::getMouseOnScreen(int clientX, int clientY)
{
    return pxr::GfVec2d(
            (double)(clientX - screenDimensions[0]) / screenDimensions[2],
            (double)(clientY - screenDimensions[1]) / screenDimensions[3] );
}
*/

void FreeCameraGL::mouseUp()
{
    _cameraMode = Camera_Mode::NONE;
    _dragActive = false;
}


//const pxr::GfMatrix4d FreeCameraGL::GetProjectionMatrix() {
//    pxr::GfFrustum frustum;
//    frustum.SetPerspective(GetFOV() * (180/3.14), screenDimensions[2] / screenDimensions[3], diameter / 100, diameter * 10);
//    return frustum.ComputeProjectionMatrix();
//}
