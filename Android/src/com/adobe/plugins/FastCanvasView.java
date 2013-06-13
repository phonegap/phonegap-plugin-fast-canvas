/**
 * Copyright 2012 Adobe Systems Incorporated
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
package com.adobe.plugins;

import android.opengl.GLSurfaceView;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.content.Context;

public class FastCanvasView extends GLSurfaceView {

	public FastCanvasView(Context context) {
		super(context);
		this.setEGLConfigChooser( false );// turn off the depth buffer
		mRenderer = new FastCanvasRenderer(this);
		this.setRenderer(mRenderer);
		this.setRenderMode(RENDERMODE_CONTINUOUSLY);
		
		this.setFocusableInTouchMode(true);
		this.requestFocus();
	}

	@Override
	public boolean onKeyDown (int keyCode, KeyEvent event) {
		synchronized (this) {
			if (FastCanvas.dispatchKeyDown(keyCode, event)) {
				return true;
			} else {
				return super.onKeyDown(keyCode, event);
			}
		}
	}

	@Override
	public boolean onKeyUp (int keyCode, KeyEvent event) {
		synchronized (this) {
			if (FastCanvas.dispatchKeyUp(keyCode, event)) {
				return true;
			} else {
				return super.onKeyDown(keyCode, event);
			}
		}
	}

	@Override
	public boolean onTouchEvent (MotionEvent event) {
		synchronized (this) {
			if (!FastCanvas.dispatchTouchEvent(event)) {
				// tear down.
				mRenderer = null;				
			}
		}
		return true;
	}
	
	public void surfaceCreated(SurfaceHolder holder)
	{
        Log.i("CANVAS", "CanvasView surfaceCreated");
	    super.surfaceCreated(holder);
	    if (isPaused) {
	    	isPaused = false;
	    	mRenderer.reloadTextures();
	    	super.onResume();
	    }
			}
	
	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h)
	{
        Log.i("CANVAS", "CanvasView surfaceChanged");
	    super.surfaceChanged(holder, format, w, h);
		}
	
	public void surfaceDestroyed(SurfaceHolder holder)
	{
		// Context is gone, no GL calls after this
        Log.i("CANVAS", "CanvasView surfaceDestroyed");
		isPaused = true;
	    super.surfaceDestroyed(holder);
		super.onPause();
		FastCanvasJNI.contextLost();
		mRenderer.onSurfaceDestroyed();
	}
	
	public void execScripts(Object obj) {
	}

	private FastCanvasRenderer mRenderer;
	public boolean isPaused = false;
}
