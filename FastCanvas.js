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


/**
 * Creates a new FastCanvasImage. For portability, its suggested you use
 * {@link FastCanvas.createImage} instead of ceating FastCanvasImages through 
 * use of the <code>new</code> operator to help ensure you get the correct image
 * type for the context you're targeting.
 * @classdesc Image replacement for images used in FastCanvas. When drawing images
 * in a FastContext2D, images must be of the type FastCanvasImage otherwise
 * the operation will fail. For making portable code, you should use
 * {@link FastCanvas.createImage} to create image instances.
 * @class
 * @example
 * // (Recommended) create a new image of the appropriate
 * // type depending on what kind of context
 * // myContext is (HTML or FastCanvas)
 * var myImage = FastCanvas.createImage();
 * 
 * // Alternatively creating a new FastCanvasImage manually 
 * // using the class constructor function
 * var myFastImage = new FastCanvasImage();
 */

function FastCanvasImage(){
	/**
	 * The width of the image after it is loaded.
	 * @type {number}
	 */
	this.width = 0;

	/**
	 * The height of the image after it is loaded.
	 * @type {number}
	 */
	this.height = 0;

	/**
	 * A unique id assigned to each image upon creation.
	 * @type {number}
	 */
	this.id = (++FastCanvasImage.idCounter);
	
	this._id = this.id; // public facing "id" but _id used to internally track image 
	this._src = ""; // image source path
	this._complete = true; // "is loading" identifier for complete property
}

/**
 * Iterator for generating id values for FastCanvasImage instances internally.
 * @private
 */
FastCanvasImage.idCounter = 0;

/**
 * Callback for when an image has successfully loaded a 
 * file path assigned to {@link FastCanvasImage#src}.
 * @type {function}
 * @name FastCanvasImage#onload
 */

/**
 * Callback for when an image has failed to load a 
 * file path assigned to {@link FastCanvasImage#src}.
 * @type {function}
 * @name FastCanvasImage#onerror
 */

/**
 * The source property, identifying a URL path to the image's
 * file location. Upon setting this value to a file path value,
 * the file is loaded into the FastCanvas plugin. For FastCanvas
 * images can be unloaded by setting the src to null or "".
 * @type {string}
 * @name FastCanvasImage#src
 * @example
 * var myImage = FastCanvas.createImage();
 * myImage.onload = function(){
 *      // ...
 *      myContext.drawImage(myImage, 0,0,100,100, 0,0,100,100);
 *      FastCanvas.render();
 * }
 * myImage.onerror = function(){
 *      console.log("Could not load image!");
 * }
 * myImage.src = "images/spritesheet.jpg";
 */
Object.defineProperty(FastCanvasImage.prototype, "src", 
	{
		get: function(){
			return this._src;
		},
		set: function(value){
			this._src = value;

			var fastCanvas = FastCanvas._instance;
			var context = fastCanvas.getContext("2d");

			// Unloading
			if (!this._src){
				context.unloadTexture(this);
				return;
			}

			// Loading
			this.complete = false;

			// callback wrappers
			var me = this;
			function onerror(err){
				me.complete = true;
				if (typeof me.onerror === 'function'){
					me.onerror(err);
				}
			}

			function onload(metrics) {
				me.complete = true;
				me.width = Math.floor( metrics[0] );
				me.height = Math.floor( metrics[1] );

				if (typeof me.onload === 'function'){
					me.onload();
				}
			}

			context.loadTexture(this, onload, onerror);
		}
	}
);

/**
 * False when the image is in the process of loading an 
 * image after the src property has been set. True when
 * loading is complete or if src is never set. If an error
 * occurred when attempting to load the image, once the
 * process of loading is complete, despite the error, this
 * value will still become true.
 * @type {boolean}
 * @name FastCanvasImage#complete
 */
Object.defineProperty(FastCanvasImage.prototype, "complete", 
	{
		get: function(){
			return this._complete;
		},
		set: function(value){
			this._complete = value;
		}
	}
);

/**
 * <b>Invalid constructor</b>: Obtain a FastContext2D instance 
 * through <code>FastCanvas.getContext("2d")</code>,
 * not through use of the <code>new</code> operator.
 * @classdesc Class used to define the context object used
 * by FastCanvas.  A FastContext2D a subset of the API
 * of the 2D context from an HTML canvas along with a couple
 * additional members, most of which are abstracted through
 * other classes and utility methods.
 * @class
 * @example
 * // get context from FastCanvas
 * var myCanvas = FastCanvas.create();
 * var myContext = myCanvas.getContext("2d");
 */
function FastContext2D(){
	this._drawCommands = "";
	this._globalAlpha = 1.0;
}

/**
 * Represents the alpha value to be used with drawing commands
 * where 1 is completely visible and 0 is fully transparent.
 * @type {number}
 * @name FastContext2D#globalAlpha
 */
Object.defineProperty(FastContext2D.prototype, "globalAlpha", {
	get: function() {
		return this._globalAlpha;
	},
	set: function( value ) {
		this._globalAlpha = value;
		this._drawCommands = this._drawCommands.concat("a" + value.toFixed(6) + ";" );
	}
});

/**
 * Loads an image into the plugin to be used as a texture in the
 * FastCanvas. Generally this method is never called directly.
 * Instead, it is called indirectly through FastCanvasImage instances
 * upon setting their {@link FastCanvasImage#src|FastCanvasImage.src}
 * property.
 * @param {FastCanvasImage} image The image to be loaded into the
 * FastCanvas plugin.
 * @param {function} [successCallback] A callback that is fired when
 * the image has been successfully loaded.
 * @param {function} [errorCallback] A callback that is fired when
 * there was an error in loading the image.
 * @example
 * // create a new image and load 
 * // it from a relative URL path
 * var myImage = FastCanvas.createImage();
 * myImage.src = "images/spritesheet.jpg"; // calls loadTexture for you
 * @private
 */
FastContext2D.prototype.loadTexture = function (image, successCallback, errorCallback) {
	if (successCallback && typeof successCallback !== 'function') {
		throw new Error('FastContext2D.loadTexture failure: successCallback parameter not a function');
	}
	if (errorCallback && typeof errorCallback !== 'function') {
		throw new Error('FastContext2D.loadTexture failure: errorCallback parameter not a function');
	}

	FastCanvas._toNative( successCallback, errorCallback, 'FastCanvas', 'loadTexture', [image.src, image._id]);
};

/**
 * Unloads an image from the FastCanvas plugin. Generally this method
 * is never called directly. Instead, it is called indirectly through
 * FastCanvasImage instances upon setting their 
 * {@link FastCanvasImage#src|FastCanvasImage.src}
 * property to a false value such as <code>null</code> or an empty
 * string (<code>""</code>).
 * @param {FastCanvasImage} image The image to be unloaded from the
 * FastCanvas plugin.
 * @example
 * // unload an image from memory
 * myImage.src = null; // calls unloadTexture for you
 * @private
 */
FastContext2D.prototype.unloadTexture = function (image) {
	FastCanvas._toNative(null, null, 'FastCanvas', 'unloadTexture', [image._id]);
};

/** 
 * Defines the 2D matrix transform applied to drawings within
 * the context.
 * @param {number} a The value that affects the positioning of pixels along the x 
 * axis when scaling or rotating the context.
 * @param {number} b The value that affects the positioning of pixels along the y 
 * axis when rotating or skewing the context.
 * @param {number} c The value that affects the positioning of pixels along the x 
 * axis when rotating or skewing the context.
 * @param {number} d The value that affects the positioning of pixels along the y 
 * axis when scaling or rotating the context.
 * @param {number} tx The distance by which to translate the context along the x axis.
 * @param {number} ty The distance by which to translate the context along the y axis.
 */
FastContext2D.prototype.setTransform = function(a, b, c, d, tx, ty) {
	this._drawCommands = this._drawCommands.concat("t" + (a===1 ? "1" : a.toFixed(6)) + "," + (b===0 ? "0" : b.toFixed(6)) + "," + (c===0 ? "0" : c.toFixed(6)) + "," + (d===1 ? "1" : d.toFixed(6)) + "," + tx + "," + ty + ";");
};

/** 
 * Defines an added 2D matrix transform applied to drawings within
 * the context.
 * @param {number} a The value added to the value that affects the positioning of 
 * pixels along the x axis when scaling or rotating the context.
 * @param {number} b The value added to the value that affects the positioning of 
 * pixels along the y axis when rotating or skewing the context.
 * @param {number} c The value added to the value that affects the positioning of 
 * pixels along the x axis when rotating or skewing the context.
 * @param {number} d The value added to the value that affects the positioning of 
 * pixels along the y axis when scaling or rotating the context.
 * @param {number} tx The value added to the distance by which to translate the 
 * context along the x axis.
 * @param {number} ty The value added to the distance by which to translate the 
 * context along the y axis.
 */
FastContext2D.prototype.transform = function(a, b, c, d, tx, ty) {
	this._drawCommands = this._drawCommands.concat("f" + (a===1 ? "1" : a.toFixed(6)) + "," + (b===0 ? "0" : b.toFixed(6)) + "," + (c===0 ? "0" : c.toFixed(6)) + "," + (d===1 ? "1" : d.toFixed(6)) + "," + tx + "," + ty + ";");
};

/** 
 * Restores the 2D matrix transform to the identity matrix. This is
 * equivalent to calling <code>context.setTransform(1,0,0,1,0,0)</code>.
 */
FastContext2D.prototype.resetTransform = function() {
	this._drawCommands = this._drawCommands.concat("m;");
};

/** 
 * Scales the 2D matrix transform along the x and y axes.
 * @param {number} a The value added to the value that affects the positioning of 
 * pixels along the x axis when scaling or rotating the context.
 * @param {number} d The value added to the value that affects the positioning of 
 * pixels along the y axis when scaling or rotating the context.
 */
FastContext2D.prototype.scale = function( a, d ) {
	this._drawCommands = this._drawCommands.concat("k" + a.toFixed(6) + "," + d.toFixed(6) + ";");
};

/** 
 * Rotates the 2D matrix transform by a specified number of radians.
 * @param {number} angle The value in radians to rotate the context.
 */
FastContext2D.prototype.rotate = function( angle ) {
	this._drawCommands = this._drawCommands.concat("r" + angle.toFixed(6) + ";");
};

/** 
 * Moves the 2D matrix transform along the x and y axes.
 * @param {number} tx The value added to the distance by which to translate the 
 * context along the x axis.
 * @param {number} ty The value added to the distance by which to translate the 
 * context along the y axis.
 */
FastContext2D.prototype.translate = function( tx, ty ) {
	this._drawCommands = this._drawCommands.concat("l" + tx + "," + ty + ";");
};

/** 
 * Sets a save point for the current context transform. This allows
 * you to arbitrarily modify the transform and restore it back to its 
 * to its original state at the time save() was called by using restore().
 * @see FastContext2D#restore
 */
FastContext2D.prototype.save = function() {
	this._drawCommands = this._drawCommands.concat("v;");
};

/** 
 * Restores the state of the context transform to the state at
 * the point in time when save() was last called.
 * @see FastContext2D#save
 */
FastContext2D.prototype.restore = function() {
	this._drawCommands = this._drawCommands.concat("e;");
};

/** 
 * For FastContext2D, clearRect does nothing (no op) but is provided
 * as a convenience function to make working between FastCanvas
 * and an HTML canvas easier.  Regardless of which type of
 * canvas/context you are using, clearRect can be called without
 * error.
 */
FastContext2D.prototype.clearRect = function(x,y,width,height){
	// noop
};

/** 
 * Draws a {@link FastCanvasImage} into the FastCanvas using the 
 * current 2D matrix transform.  This method has 3 different signatures:
 * <code>drawImage(image, dx, dy)</code>, 
 * <code>drawImage(image, dx, dy, dw, dh)</code> and 
 * <code>drawImage(image, sx, sy, sw, sh, dx, dy, dw, dh)</code>. The usage
 * is determined by the number of parameters passed.
 * @param {FastCanvasImage} image The source image to be used when drawing to the canvas.
 * For instances of FastCanvas this must be of the type FastCanvasImage.
 * Using <code>FastCanvas.create()</code> and <code>FastCanvas.createImage()</code>
 * will help you maintain parity between the images being used and the canvas into
 * which they are drawn.
 * @param {number} sx The x location of the source clipping rectangle if only a portion
 * of the image is to be drawn in the canvas.
 * @param {number} sy The y location of the source clipping rectangle if only a portion
 * of the image is to be drawn in the canvas.
 * @param {number} sw The width of the source clipping rectangle if only a portion
 * of the image is to be drawn in the canvas.
 * @param {number} sh The height of the source clipping rectangle if only a portion
 * of the image is to be drawn in the canvas.
 * @param {number} dx The x location within the canvas to place the image.
 * @param {number} dy The y location within the canvas to place the image.
 * @param {number} dw The width of the image as it is placed in the canvas.
 * @param {number} dh The height of the image as it is placed in the canvas.
 */
FastContext2D.prototype.drawImage = function(
	image,			// image
	sx, sy, sw, sh,	// source (or destination if fewer args)
	dx, dy, dw, dh) {	// destination
	
	var numArgs = arguments.length;
	if (numArgs <= 3){
		// drawImage(image, dx,dy); position only (s becomes d)
		this._drawCommands = this._drawCommands.concat("d" + image._id + ",0,0," + image.width + "," + image.height + "," + sx + "," + sy + "," + image.width + "," + image.height + ";");
			
	}else if (numArgs <= 5){
		// drawImage(image, dx,dy,dw,dh); position and size (s becomes d)
		this._drawCommands = this._drawCommands.concat("d" + image._id + ",0,0," + image.width + "," + image.height + "," + sx + "," + sy + "," + sw + "," + sh + ";");
	}else{
		// [full]; all arguments, source and destination
		this._drawCommands = this._drawCommands.concat("d" + image._id + "," + sx + "," + sy + "," + sw + "," + sh + "," + dx + "," + dy + "," + dw + "," + dh + ";");
	}
};
	
/**
 * Informs the drawing context that drawing commands have
 * completed for the current frame and the should be sent
 * to the FastCanvas plugin for drawing to the screen.
 * <p>This method is unique to FastContext2D and does not
 * exist within the HTML 2D context, so the utility method
 * {@link FastCanvas.render} should be used to make it easy
 * to call or not call this method depending on the context
 * you are currently working with.</p>
 * @example
 * // makes necessary FastCanvas render call
 * // if canvas being used is FastCanvas
 * var myCanvas = FastCanvas.create();
 * var myContext = myCanvas.getContext("2d");
 * 
 * // ...
 * myContext.translate(10,10);
 * myContext.rotate(Math.PI);
 * // ... 
 * 
 * // after all context calls are complete
 * // for the current frame:
 * FastCanvas.render(); // calls FastContext2D.render()
 */
FastContext2D.prototype.render = function () {
	var commands = this._drawCommands;
	this._drawCommands = "";
	FastCanvas._toNative(null, null, 'FastCanvas', 'render', [commands]);
};

/**
 * Implementation of FastCanvas.capture.
 * @private
 */
FastContext2D.prototype.capture = function(x,y,w,h,fileName, successCallback, errorCallback) {
	if (successCallback && typeof successCallback !== 'function') {
		throw new Error('successCallback parameter not a function');
	}
	if (errorCallback && typeof errorCallback !== 'function') {
		throw new Error('errorCallback parameter not a function');
	}
	
	FastCanvas._toNative(successCallback, errorCallback, 'FastCanvas', 'capture', [x,y,w,h,fileName]);
};

/**
 * <b>Invalid constructor</b>: Obtain the FastCanvas instance through {@link FastCanvas.create},
 * not through use of the <code>new</code> operator.
 * @classdesc FastCanvas is an alternative to a standard HTML canvas.
 * It provides fast, GPU-accelerated graphics in a full screen window
 * replacing other content on the screen.
 * <p>To use FastCanvas, you reference create a FastCanvas object
 * from {@link FastCanvas.create}.  Like the standard
 * HTML Canvas, it is also a DOM element (DIV) to help with compatibility
 * when working between the two.  However, FastCanvas is not rendered in the
 * DOM like other DOM elements, instead being managed on a much lower level
 * on your device through a hardware surface that covers the DOM.<p>
 * <p><b>Note:</b> Though there is a FastCanvas class available in code;
 * it is not used to construct the FastCanvas instance
 * as it is, iteself, a DIV element. This means operators like <code>instanceof</code>
 * will fail if tested against FastCanvas with an instance returned by 
 * {@link FastCanvas.create}.</p>
 * @class
 * @example
 * // get a reference to the FastCanvas
 * // and FastContext2D for drawing
 * var myCanvas = FastCanvas.create();
 * var myContext = myCanvas.getContext("2d");
 * 
 * // load an image to draw on screen
 * var myImage = FastCanvas.createImage();
 * myImage.onload = function(){
 *      // draw image to screen
 *      myContext.drawImage(myImage, 0,0,100,100, 0,0,100,100);
 *      FastCanvas.render();
 * }
 * myImage.src = "images/spritesheet.jpg";
 */ 
function FastCanvas(){
	// default constructor
}

/**
 * Singleton-esque instance reference to the canvas
 * being used.  This may be a FastCanvas object or an
 * HTML Canvas instance if not supported or the user
 * requested to force the fallback.
 * @private
 */
FastCanvas._instance = null;

/**
 * Identifies whether or not the FastCanvas implementation of
 * canvas is being used.  This will be true if so, false if
 * <code>FastCanvas.create()</code> returned an instance of
 * an HTML canvas as a fallback.  This value is only correctly
 * defined after a call to <code>FastCanvas.create()</code> is
 * made. Otherwise it will always report <code>false</code>.
 * @type {boolean}
 */
FastCanvas.isFast = false;

/**
 * Creates and initializes the FastCanvas instance. There can be
 * only one FastCanvas instance; calling this method multiple times
 * will return the same instance.  Instances of FastCanvas are HTML
 * DIV elements.  If FastCanvas is not supported on the current 
 * device, an on-list HTML Canvas will be returned.
 * @param {boolean} [forceFallback] If true, the return value will
 * always be an HTML canvas rather than a FastCanvas.
 * @return {Canvas|FastCanvas} An instance of the FastCanvas object
 * or an HTML Canvas element if FastCanvas is not supported or the
 * <code>forceFallback</code> parameter is true.
 */
FastCanvas.create = function(forceFallback){

	if (FastCanvas._instance === null){
	
		if (!forceFallback && FastCanvas._isAvailable()){
			FastCanvas._instance = FastCanvas._createFastCanvas();
			FastCanvas.isFast = true;
		}else{
			FastCanvas._instance = FastCanvas._createHTMLCanvas();
			FastCanvas.isFast = false;
		}
	}
	
	return FastCanvas._instance;
}

/**
 * Creates and returns a FastCanvas DIV element
 * placed on the display list as the last element
 * of the body tag.
 * @private
 */
FastCanvas._createFastCanvas = function(){
	/**
	 * @alias FastCanvas.prototype
	 */
	var fastCanvas = document.createElement("div");
	fastCanvas.id = "fastCanvas"; // to help identify in DOM
	fastCanvas.className = "fastCanvas"; // for styling (bgcolor)
	
    // adding FastCanvas element to display list, not to
	// be seen, but so styles can be picked up and
	// inherited from the DOM to help with assigning
	// background color
	document.body.appendChild(fastCanvas);

	/**
	 * Internal reference of the context instance
	 * which should be retrieved via getContext()
	 * @private
	 */
	fastCanvas._context = null;

    // get the FastCanvas size from the available window dimenions
	// we attempt to use window.inner* values but will fallback
	// to using screen.avail* if inner values return 0. For 
	// non-windowed mobile applications, these should be equivalent
	fastCanvas._width = (window.innerWidth > 0) ? window.innerWidth : screen.availWidth;
	fastCanvas._height = (window.innerHeight > 0) ? window.innerHeight : screen.availHeight;

	/**
	 * Defines the width of the FastCanvas. The actual width
	 * of the FastCanvas will always be the width of the screen.
	 * If you're working in dimensions that are, for example
	 * larger than the available screen size, setting the
	 * FastCanvas dimensions will help account for that.
	 * @type {number}
	 * @name FastCanvas#width
	 */
	Object.defineProperty(fastCanvas, "width", {
		get: function(){
			return this._width;
		},
		set: function(value){
			this._width = value;
			FastCanvas._toNative(null, null, 'FastCanvas', 'setOrtho', [this._width, this._height]);
		}
	});

	/**
	 * Defines the height of the FastCanvas. The actual height
	 * of the FastCanvas will always be the height of the screen.
	 * If you're working in dimensions that are, for example
	 * larger than the available screen size, setting the
	 * FastCanvas dimensions will help account for that.
	 * @type {number}
	 * @name FastCanvas#height
	 */
	Object.defineProperty(fastCanvas, "height", {
		get: function(){
			return this._height;
		},
		set: function(value){
			this._height = value;
			FastCanvas._toNative(null, null, 'FastCanvas', 'setOrtho', [this._width, this._height]);
		}
	});

	// copy methods over from FastCanvas into DIV object
	fastCanvas.getContext = FastCanvas.prototype.getContext;

	return fastCanvas;
};
 
/**
 * Creates and returns an HTML Canvas element that has
 * been placed on the display list within the body element.
 * @private
 */
FastCanvas._createHTMLCanvas = function(){
	
	// HTML canvas fallback for fast canvas 
	var canvas = document.createElement("canvas");
	canvas.id = "fastCanvas"; // to help identify in DOM
	canvas.className = "fastCanvas"; // for styling (bgcolor)
	
	// canvas is full screen matching fast canvas 
	canvas.width = (window.innerWidth > 0) ? window.innerWidth : screen.width;
	canvas.height = (window.innerHeight > 0) ? window.innerHeight : screen.height;
	
	// try to keep out of any layout flow
	canvas.style.position = "absolute";
	
	// add to display list
	var body = document.body;
	body.insertBefore(canvas, body.firstChild);
	
	return canvas;
}


/**
 * Creates and returns an image to be used by FastCanvas. FastCanvas
 * uses a different kind of image from the standard HTML canvas.  If
 * an HTML canvas is being used as a fallback, a standard HTML Image
 * object is returned instead of a FastCanvas image.
 * @return {Image|FastCanvasImage} A new image object of either
 * the type Image (HTMLImageElement) or FastCanvasImage
 * depending on which is correct for your canvas.
 * @example
 * // create a new image for the canvas
 * var myImage = FastCanvas.createImage();
 * // ...
 * myContext.drawImage(myImage, ...); 
 */
FastCanvas.createImage = function(){
	if (FastCanvas._instance === null){
		throw new Error("A new canvas must be created with FastCanvas.create() before FastCanvas.createImage() can be used");
	}

	if (FastCanvas.isFast){
		return new FastCanvasImage();
	}

	return new Image();
};

/**
 * Executes the native render method only if the active context 
 * is of the type FastContext2D.
 * HTML 2D contexts do not require the render call so passing
 * @example
 * function renderFrame() {
 *      myContext.setTransform(1, 0, 0, 1, 0, 0);
 *      myContext.clearRect(0,0,myCanvas.width,myCanvas.height);
 *      myDrawing(); // performs draw calls...
 *      FastCanvas.render();
 * }
 */
FastCanvas.render = function(){
	if (FastCanvas.isFast){
		FastCanvas._instance.getContext().render();
	}
};

/**
 * Identifies whether or not the FastCanvas plugin is 
 * installed and available for use in this application.
 * @return {boolean} True if FastCanvas is available,
 * false if not.  If false, be prepared to
 * use an HTML canvas in place of FastCanvas. 
 * @private
 */
FastCanvas._isAvailable = function(){
	var isAvailable = false;
	
	function successCallback(availability){
		isAvailable = availability;
	}
	function errorCallback(){
		isAvailable = false;
	}
	// synchronous unless native code can't callback into 
	// JS at which point we assume the default unavailable
	FastCanvas._toNative( successCallback, errorCallback, 'FastCanvas', 'isAvailable');
	return isAvailable;
};

/**
 * Sets the backgound color of the FastCanvas to the color 
 * specified. This is not needed if the canvas element is
 * styled to have a certain background color by the time
 * <code>getContext()</code> is called as it will be set
 * to that color automatically at that time. 
 * @param {number|string} [color] Color to be used for the background
 * of the FastCanvas.  This can be a numeric value or a 3 or 
 * 6-character hexadecimal string (not counting a preceeding, 
 * optional hash ("#")).  For example, red can be expressed as
 * 0xFF0000, "#FF0000", "FF0000", "#F00", or "F00". If no value
 * or a false value (undefined, null or empty string) is passed,
 * an attempt is made to automatically determine the color from 
 * the style of the fastCanvas element or the document.
 * @example
 * // avoid use of setBackgroundColor through css styling
 * var myCanvas = FastCanvas.create();
 * myCanvas.style.backgroundColor = "#F0F";
 * var myContext = myCanvas.getContext("2d"); // bg color auto-set
 * 
 * // ... change bg color later ...
 * FastCanvas.setBackgroundColor("#000");
 */
FastCanvas.setBackgroundColor = function (color) {
	var colorStr = "";

	if (FastCanvas._instance === null){
		throw new Error("A canvas must be created with FastCanvas.create() before a background color can be set");
	}
	
	if (!color){
		// no color so find automatically
		colorStr = FastCanvas._getBackgroundColor();
		if (!colorStr){
			// could not determine the color from the DOM
			return;
		}
	}else{
	
		var colorType = typeof(color);
		
		if (colorType === "number"){
			colorStr = color.toString(16);
			
			// pad small numbers with preceeding 0's
			while (colorStr.length < 6){
				colorStr = "0" + colorStr;
			}
			
		}else if (colorType === "string"){
			colorStr = color;
			
			// strip preceeding hash (#)
			if (colorStr.charAt(0) === "#"){
				colorStr = colorStr.substring(1);
			}
			var numChars = colorStr.length;
			if (numChars === 3){
				// from ABC to AABBCC
				colorStr = colorStr.charAt(0) + colorStr.charAt(0)
					 + colorStr.charAt(1) + colorStr.charAt(1)
					 + colorStr.charAt(2) + colorStr.charAt(2);
					 
			}else if (numChars !== 6){
				throw new TypeError("Hexadecimal color values must be exactly 3 or 6 characters long");
			}
		}else{
			throw new TypeError("Value color must be numeric or hexadecimal string");
		}
	}
	
	if (FastCanvas.isFast){
		FastCanvas._toNative(null, null, 'FastCanvas', 'setBackgroundColor', [color]);
	}else{
		// assigning to HTML fallback requires the hash (#)
		// (fast canvas color filtering stripped it)
		FastCanvas._instance.style.backgroundColor = "#" + color;
	}
};

/**
 * Determines the background color to use for the FastCanvas
 * depending on the background color style the fastCanvas instance
 * was given or is able to inherit from parents.
 * @private
 */
FastCanvas._getBackgroundColor = function(){
	var col = "";
	var style = null;
	var node = FastCanvas._instance;
	// walk up parent tree finding background color
	// ideally it should be placed on the fastCanvas
	// element itself so it will be found in the
	// first iteration of the loop 
	while(node && node !== document){
		style = window.getComputedStyle(node);
		if (style && style.backgroundColor && style.backgroundColor !== "rgba(0, 0, 0, 0)"){
			col = style.backgroundColor;
			break;
		}
		node = node.parentNode;
	}

	if (!col && node !== document){
		// if the current node from the loop is
		// not the document, we may have been searching
		// an off-list node, at which point make one
		// last check in the document body
		style = window.getComputedStyle(document.body);
		if (style && style.backgroundColor && style.backgroundColor !== "rgba(0, 0, 0, 0)"){
			col = style.backgroundColor;
		}
	}

	// if still color not found, return its
	// default, undefined/empty value 
	if (!col){
		return col;
	}
	
	try {
		// convert rgb(a) into tighter hex[6] format;
		// assumes col is always going to be reported in rgb format
		var channels = col.substring(col.indexOf("(")+1, col.indexOf(")")).split(",");
		var r = parseInt(channels[0], 10).toString(16);
		if (r.length < 2){
			r = "0" + r;
		}
		var g = parseInt(channels[1], 10).toString(16);
		if (g.length < 2){
			g = "0" + g;
		}
		var b = parseInt(channels[2], 10).toString(16);
		if (b.length < 2){
			b = "0" + b;
		}
		
		// does not precede with "#"
		return r + g + b;
	}catch(err){
		// some string parsing probably failed
		console.log("Unexpected error reading background image:", err);
	}

	// here we failed to produce a valid color
	return "";
};


/**
 * Writes the current pixels from the FastCanvas into a file specified by fileName. 
 * x,y,width,height specify a rectangle of the canvas from which to read the pixels. 
 * If width or height equal -1 then the entire canvas is captured. On Android the 
 * root directory is /mnt/sdcard/. Any sub folders can be supplied in the fileName 
 * parameter like so: '/MyFolder/testScreenshot.png'.
 * Calls to capture() must be made prior to a render call, otherwise the behavior is 
 * undefined. The successCallback is executed when the file is saved, and provides 
 * the location as the callback parameter, otherwise errorCallback is executed with 
 * an error message.
 * @param {number} x The x location of the capture rectangle.
 * @param {number} y The y location of the capture rectangle.
 * @param {number} w The width of the capture rectangle.
 * @param {number} h The height of the capture rectangle.
 * @param {string} fileName The relative path to which the capture is to be saved within the
 * storage location.
 * @param {function} successCallback Callback for when the capture completed successfully.
 * @param {function} errorCallback Callback for when the capture did not complete successfully.
 */
FastCanvas.capture = function(x,y,w,h,fileName, successCallback, errorCallback) {
	if (FastCanvas.isFast){
		FastCanvas._instance.getContext().capture(x,y,w,h,fileName, successCallback, errorCallback);
	}
}

/**
 * Returns a FastContext2D instance mimicing the context 
 * (CanvasRenderingContext2D) returned from an HTML canvas.
 * The first time this method is called, FastCanvas will also
 * implicitly call {@link FastCanvas.setBackgroundColor} to
 * update the background color of the canvas context based on
 * the current style of the FastCanvas object.
 * @param {string} contextID The type of context returned, 
 * typically "2d", though this value is ignored and always
 * presumed to be "2d" for FastCanvas. For compatibility 
 * between an HTML canvas and FastCanvas, you should always
 * use "2d" for this value.
 * @return {FastContext2D} The FastCanvas context used to
 * render high performance graphics to the screen.
 */
FastCanvas.prototype.getContext = function(contextID){
	// there is only one context in the plugin fastCanvas
	// so contextID argument is ignored (presumed "2d")
	if (!this._context){
		this._context = new FastContext2D();

		// attempt to grab and use the background color
		// currently being used by the canvas/document
		var col = FastCanvas._getBackgroundColor();
		if (col){
			FastCanvas.setBackgroundColor(col);
		}
	}

	return this._context;
};

/**
 * Wrapper for native calls allowing us
 * to some some extra validation.
 * @private
 */
FastCanvas._toNative = function(success, fail, service, action, args){
	// exec may not be defined outside of the 
	// context of a native cordova application
	if (cordova && typeof cordova.exec === 'function'){
		cordova.exec(success, fail, service, action, args || []);

	}else if (typeof fail === 'function'){
		// we're assuming that if you're attempting
		// a native call when native is not available
		// the result is a failure (callback)
		fail();
	}
}
