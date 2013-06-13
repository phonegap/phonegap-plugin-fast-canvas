/*
 Copyright 2013 Adobe Systems Inc.;
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */


#include "FastCanvasJNI.h"
#include "JNIHelper.h"
#include "Canvas.h"
#include <GLES/gl.h>
#include <android/asset_manager.h>
#include <android\asset_manager_jni.h>

// -----------------------------------------------------------
// --                     JNI interface                     --
// -----------------------------------------------------------
JNIEXPORT void JNICALL Java_com_adobe_plugins_FastCanvasJNI_setBackgroundColor
  (JNIEnv *je, jclass jc, jint red, jint green, jint blue) {
    Canvas *theCanvas = Canvas::GetCanvas();
    if (theCanvas) {
        theCanvas->SetBackgroundColor(red, green, blue);
    }
}

JNIEXPORT void JNICALL Java_com_adobe_plugins_FastCanvasJNI_setOrtho
  (JNIEnv *je, jclass jc, jint width, jint height) {
    Canvas *theCanvas = Canvas::GetCanvas();
    if (theCanvas) {
        theCanvas->SetOrtho(width, height);
    }
}

JNIEXPORT void JNICALL Java_com_adobe_plugins_FastCanvasJNI_addTexture
  (JNIEnv *je, jclass jc, jint id, jint glID, jint width, jint height)
{
    Canvas *theCanvas = Canvas::GetCanvas();
    if (theCanvas) {
        theCanvas->AddTexture(id, glID, width, height);
    }
}

JNIEXPORT jboolean JNICALL Java_com_adobe_plugins_FastCanvasJNI_addPngTexture
  (JNIEnv *je, jclass jc, jobject assetManager, jstring path, jint id, jobject dim)
{
	bool success = false;
    Canvas *theCanvas = Canvas::GetCanvas();
    if (theCanvas) {
		AAssetManager* mgr = AAssetManager_fromJava(je, assetManager);
		if (mgr == NULL) return false;

        const char *p = je->GetStringUTFChars(path, 0);
		AAsset* asset = AAssetManager_open(mgr, p, AASSET_MODE_UNKNOWN);
        je->ReleaseStringUTFChars(path, p);
		if (asset == NULL) return false;

		long size = AAsset_getLength(asset);
		unsigned char* buffer = (unsigned char*) malloc (size);
		if (buffer == NULL) return false;

		AAsset_read (asset, buffer, size);
		AAsset_close(asset);

		unsigned int width;
		unsigned int height;
        success = theCanvas->AddPngTexture(buffer, size, id, &width, &height);
		free (buffer);

		if (success) {
			jclass cls = je->GetObjectClass(dim);  
			jfieldID wID = je->GetFieldID(cls, "width", "I");
			je->SetIntField(dim, wID, (int)width);
			jfieldID hID = je->GetFieldID(cls, "height", "I");
			je->SetIntField(dim, hID, (int)height);
		}
    }
	return success;
}

JNIEXPORT void JNICALL Java_com_adobe_plugins_FastCanvasJNI_removeTexture
  (JNIEnv *je, jclass jc, jint id)
{
    Canvas *theCanvas = Canvas::GetCanvas();
    if (theCanvas) {
        theCanvas->RemoveTexture(id);
    }
}

JNIEXPORT void JNICALL Java_com_adobe_plugins_FastCanvasJNI_render
  (JNIEnv *je, jclass jc, jstring renderCommands)
{
    Canvas *theCanvas = Canvas::GetCanvas();
    if (theCanvas) {
        const char *rc = je->GetStringUTFChars(renderCommands, 0);
        int length = je->GetStringUTFLength(renderCommands);
		
        theCanvas->Render(rc, length);
        je->ReleaseStringUTFChars(renderCommands, rc);
		
		//send all callbacks, for now only capture callbacks
		ExecuteCallbacks(je);
    }
}

JNIEXPORT void JNICALL Java_com_adobe_plugins_FastCanvasJNI_surfaceChanged
  (JNIEnv *, jclass, jint width, jint height )
  {
    Canvas *theCanvas = Canvas::GetCanvas();
    if (theCanvas) {
        theCanvas->OnSurfaceChanged( width, height );        
    }
}

JNIEXPORT void JNICALL Java_com_adobe_plugins_FastCanvasJNI_captureGLLayer
  (JNIEnv *je, jclass jc, jstring callbackID, jint x, jint y, jint w, jint h, jstring fileName) 
{
	Canvas *theCanvas = Canvas::GetCanvas();
    if (theCanvas) {
		const char *callback = je->GetStringUTFChars(callbackID, 0);
		const char *fn = je->GetStringUTFChars(fileName, 0);
		theCanvas->QueueCaptureGLLayer(x,y,w,h,callback,fn);
		//release memory for callbackString, might not want to do this here
		je->ReleaseStringUTFChars(callbackID, callback);
		je->ReleaseStringUTFChars(fileName, fn);
    }
}

JNIEXPORT void JNICALL Java_com_adobe_plugins_FastCanvasJNI_contextLost
    (JNIEnv *je, jclass jc) {
    Canvas::ContextLost();
}

JNIEXPORT void JNICALL Java_com_adobe_plugins_FastCanvasJNI_release
    (JNIEnv *je, jclass jc) {
    Canvas::Release();
}

