FastCanvas
==========

FastCanvas is a Cordova/PhoneGap plugin which implements a very fast, 2D, 
mostly-canvas-compatible rendering surface for Android. It focuses on moving 
graphics around on the screen quickly and efficiently using hardware 
acceleration.

Unlike the HTML5 canvas, FastCanvas will encompass your entire screen and cannot be 
integrated with other elements in the DOM.  It lives outside of the DOM in a separate 
rendering surface that fully covers all HTML content.  More on how FastCanvas is displayed 
to the screen is available in the [Architecture](#architecture) section. 
If you already have an application which uses a full screen DOM canvas, switching over 
to FastCanvas could be an easy way to provide a boost in performance, and consistency within
that performance, to your mobile application or game.

While FastCanvas attempts to look and behave very similar to the HTML5 canvas, it
only supports a subset of the HTML5 canvas API, focusing on what benefits most from
hardware acceleration.  More information about API support is described in the 
[FastCanvas API](#fastCanvas-api) section.


Getting Started
===============

FastCanvas is supported on Android devices.  Additionally, your application must 
be designed so that:

1. Your application runs a full screen canvas
2. Use of the 2D canvas API is limited to transforms and drawing images (see: 
[FastCanvas API](#fastcanvas-api))

For devices that don't support FastCanvas, the API will fallback to using the standard
HTML canvas.  This is handled for you seamlessly making it easy to write cross-platform 
Canvas applications the use FastCanvas on Android.

Prerequisites:
* [ADT for Eclipse](http://developer.android.com/sdk/index.html)
* [Cordova](http://cordova.apache.org/#download) or [PhoneGap](http://phonegap.com/download/) for Android (Getting started guides: [Cordova](http://cordova.apache.org/docs/en/2.7.0/guide_getting-started_android_index.md.html#Getting%20Started%20with%20Android), [PhoneGap](http://docs.phonegap.com/en/2.7.0/guide_getting-started_android_index.md.html#Getting%20Started%20with%20Android))
* [FastCanvas](https://github.com/phonegap/phonegap-fast-canvas-plugin)


## Adding FastCanvas to Your Application ##

1. Clone the FastCanvas tree down from github (if you haven't already).
5. Copy `FastCanvas\FastCanvas.js` to your Java project's `assets\www` folder and include a reference to it in your `index.html`.
3. Copy the `FastCanvas\Android\src\com` folder to your Java project's `src` folder.
4. Copy the `FastCanvas\Android\libs\armeabi` folder to your Java project's `libs` folder.
5. In your `res/xml/config.xml` file add the following line to the end of the <plugins> section: 
   
```
<plugin name="FastCanvas" value="com.adobe.plugins.FastCanvas"/>
```

### Adding FastCanvas using the PlugMan script ###

Additional prerequisites:
* [nodejs](http://nodejs.org/)
* [plugman](https://github.com/apache/cordova-plugman)

```
	npm install -g plugman
```

This is an alternate method, as used by the PhoneGap Build system.
After creating your Cordova app with `create`, run the PlugMan script as follows to install the FastCanvas plugin:

```
	plugman --platform android --project <your project folder> --plugin https://github.com/FastCanvas/FastCanvas.git
```

#### If you have scale issues with your content ####
Adding the following to your `index.html` will often help:

```
	<meta name="viewport" content="user-scalable=no, initial-scale=1, maximum-scale=1, minimum-scale=1, width=device-width, height=device-height, target-densitydpi=device-dpi" />
```

#### Changing the Java interface ####

Additional prerequisites:
* Android NDK - the [Android NDK](http://developer.android.com/tools/sdk/ndk/index.html) is a separate download.
The native interface used by FastCanvas is defined in `FastCanvasJNI.java`. 
If at any point in time you change `FastCanvasJNI.java`, you'll also need to regenerate 
`FastCanvasJNI.h` and then recompile the native code.

These instructions are for Windows. If working on a Mac, use / instead of \, mv instead of move, etc.

1. Compile FastCanvasJNI.java:
```
	\FastCanvas\Android>javac -d . src\com\adobe\plugins\FastCanvasJNI.java src\com\adobe\plugins\FastCanvasTextureDimension.java
```

2. Create FastCanvasJNI.h
```
	\FastCanvas\Android>javah -jni com.adobe.plugins.FastCanvasJNI
```
	
3. Move it to the correct location
```
	\FastCanvas\Android>move com_adobe_plugins_FastCanvasJNI.h jni\FastCanvasJNI.h
```
	
4. Clean up
```
	\FastCanvas\Android>rmdir /s com
```
	
5. Build the JNI library from the command line (Command Prompt or Terminal window): 
```
	\FastCanvas\Android>[path to NDK]\ndk-build
```
	
	Should produce output similar to: 
```
		Compile++ thumb  : FastCanvasJNI <= FastCanvasJNI.cpp
		Compile++ thumb  : FastCanvasJNI <= Canvas.cpp
		StaticLibrary    : libstdc++.a
		SharedLibrary    : libFastCanvasJNI.so
		Install          : libFastCanvasJNI.so => libs/armeabi/libFastCanvasJNI.so
```


Using FastCanvas in Your Project
================================

FastCanvas was designed to mimic the standard HTML 2D Canvas to help make it easy to use 
or implement in your existing canvas-based applications.  Like the HTML canvas, FastCanvas
consists of both a canvas object and a context object.  The API is very similar with a few
exceptions:

* Canvas objects are created with `FastCanvas.create()`
* Image objects are created with `FastCanvas.createImage()`
* At the end of each frame a call to `FastCanvas.render()` is required

These commands work with both FastCanvas and HTML canvas applications.  Once you have your 
application working with FastCanvas, it will also work with an HTML canvas.

The following is a basic usage example of FastCanvas shifting and rotating an
image drawn to the screen:

```javascript
var canvas = FastCanvas.create(); // specific to FastCanvas
var context = canvas.getContext("2d");
var myImage = FastCanvas.createImage(); // specific to FastCanvas
myImage.onload = function(){
   context.translate(100, 100);
   context.rotate(Math.PI);
   context.drawImage(myImage, 0, 0);
   FastCanvas.render(); // specific to FastCanvas
}
myImage.src = "images/myImage.jpg";
```

You can see much of the code matches usage of the HTML canvas.  `FastCanvas.create()` will even create 
an HTML canvas is FastCanvas is not supported.  This is also the case 
with `FastCanvas.createImage()` which, depending on the result from `FastCanvas.create()` will create 
a standard HTML Image if an HTML canvas was created and a FastCanvasImage object (images used by FastCanvas) 
when FastCanvas is being used.  Usage of these image objects are largely the same - at least as far as 
canvases are concerned - as you can see from the code used to load the image above.  Similarly, the API of 
the context returned from `FastCanvas.getContext("2d")` matches that of HTML's CanvasRenderingContext2D object, 
at least as far as FastCanvas supports it. `FastCanvas.render()` is an extra step specific 
to FastCanvas but will only do the necessary render step when a FastCanvas is being used and will be a 
no op (doing nothing) when an HTML canvas has been created.

In addition to the code changes, because FastCanvas applications are full screen, your HTML should also include 
the following meta tag to be assured that window metrics are reported accurately and consistently:

```html
   <meta name="viewport" content="user-scalable=no, initial-scale=1.0, maximum-scale=1.0, minimum-scale=1.0, 
		width=device-width, height=device-height, target-densitydpi=device-dpi" />
```



Example Using FastCanvas
========================

The game "Hungry Hero" has been re-implemented in JavaScript (from
AS3). It is a simple touch based flying game based on the Starling API.
We are currently using an approximation of Starling - a simple 
display list API - to wrap calls to the FastCanvas plugin to do rendering.

https://github.com/phonegap/phonegap-fast-canvas-example


FastCanvas API
==============

The bulk of the FastCanvas API consists a subset of the methods within the 
standard HTML5 canvas implementation of 
[CanvasRenderingContext2D](http://www.whatwg.org/specs/web-apps/current-work/multipage/the-canvas-element.html#canvasrenderingcontext2d) - 
the 2D context obtained when using `canvas.getContext("2d");`.  A goal in
designing FastCanvas was to make it as close as possible to the existing
HTML5 canvas to make its easy and intuitive to use, and to make the 
process of porting an existing application to FastCanvas as painless as possible.

The subset supported in FastCanvas for its context includes: 

| Member | Notes |
| ------ | ----- |
| context.clearRect(); | Not supported, but available (NOOP) |
| context.drawImage(image, sx, sy, sw, sh, dx, dy, dw, dh); | Supported |
| context.globalAlpha; | Supported |
| context.resetTransform(); | Supported |
| context.restore(); | Supported |
| context.rotate(angle); | Supported |
| context.save(); | Supported |
| context.scale(x, y); | Supported |
| context.setTransform(a, b, c, d, e, f); | Supported |
| context.transform(a, b, c, d, e, f); | Supported |
| context.translate(x, y); | Supported |

Additionally, FastCanvas includes the following:

| Member | Notes |
| ------ | ----- |
| FastCanvas.create(); | Creates a canvas object for you, FastCanvas if available, otherwise a standard HTML canvas. |
| FastCanvas.createImage(); | Creates an image object for you, FastCanvasImage if a FastCanvas was created in FastCanvas.create(), otherwise a standard HTML Image. |
| FastCanvas.render(); | To be called after all context calls are finished to commit the drawing to the screen. |
| FastCanvas.setBackgroundColor(color); | Sets the canvas background (automatic for first time calling getContext()) |
| FastContext2D.capture(x,y,w,h,fileName, successCallback, errorCallback); | Saves the current state of the canvas as an image |


Architecture
============

FastCanvas runs a Canvas(-like) implementation. 

### On A Top Surface

FastCanvas output will cover any HTML output - so you should generally avoid 
HTML rendering as it will only consume performance. The FastCanvas plugin 
creates an OpenGL surface that sits on top of the browser.

### OpenGL renderer in C++

The renderer itself is OpenGL ES1.1 command streams, and the code
is written in C++. The advantage of C++ is both portability and
control of memory management. 

### Separate Thread

Your JS code runs in the browser thread, while most of the work
FastCanvas does in in the UI thread. A tight stream of rendering commands
is sent between the threads. This allows some load balancing between
the threads, separation of the game from the renderer, and (in the 
future) downclocking the render thread.


Using FastCanvas Efficiently
----------------------------

For best performance, minimize the number of draw calls per fram in the GL layer.

What that means at the JavaScript level is:
* Use sprite sheets
* Use as few textures as possible
* Avoid swapping textures in and out, and preload if possible.
* Try to batch drawImage calls that use the same texture. It is vastly more efficient to make ten drawImage calls in a row using one texture, and then make ten more using a second texture, than to switch back and forth twenty times.

