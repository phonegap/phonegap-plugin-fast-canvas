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


package com.adobe.plugins;

public class FastCanvasJNI {
	// Native methods
	public static native void setBackgroundColor(int red, int green, int blue);
	public static native void setOrtho(int width, int height);
	public static native void addTexture(int id, int glID, int width, int height); // id's must be from 0 to numTextures-1
	public static native boolean addPngTexture(Object mgr, String path, int id, FastCanvasTextureDimension dim); // id's must be from 0 to numTextures-1
	public static native void removeTexture(int id); // id must have been passed to addTexture in the past
    public static native void render(String renderCommands);
	public static native void surfaceChanged( int width, int height );
	public static native void captureGLLayer(String callbackID, int x, int y, int width, int height, String fileName); //captures the current contents of the GL layer and writes to a temporary file
	public static native void contextLost(); // Deletes native memory associated with lost GL context
	public static native void release(); // Deletes native canvas
	
	// Load library
	static {
		System.loadLibrary("FastCanvasJNI");
	}
}
