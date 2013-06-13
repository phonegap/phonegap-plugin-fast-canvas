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


#include "JNIHelper.h"
#include "Canvas.h"

void ExecuteCallbacks(JNIEnv * je) {
	Canvas *theCanvas = Canvas::GetCanvas();
	Callback *callback = theCanvas ? theCanvas->GetNextCallback() : NULL;
	if(callback) {
		jclass cls = je->FindClass("com/adobe/plugins/FastCanvas");
		if (je->ExceptionCheck()) {
		   return;
		}

		jmethodID mid = je->GetStaticMethodID(cls, "executeCallback", "(Ljava/lang/String;ZLjava/lang/String;)V");
		if (je->ExceptionCheck()) {
		   return;
		}

		while(callback) {
			jstring methodID = je->NewStringUTF(callback->callbackID);
			jstring result = je->NewStringUTF(callback->result);
			je->CallStaticVoidMethod( cls, mid, methodID, callback->isError, result);
			//delete the callback we just sent
			theCanvas->PopCallbacks();
			//get the next callback
			callback = theCanvas->GetNextCallback();
		}
	}
}