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

#include "Canvas.h"
#include <math.h>
#include <stdarg.h>
#include <string.h>

#if defined(__ANDROID__)
#	include <GLES/gl.h>
#   include <android/log.h>
#elif defined(_WIN32)
#	include "../../../glew/include/GL/glew.h"
#elif defined(__APPLE__) // These are the iOS headers - not desktop OSX
#   include <OpenGLES/ES1/gl.h>
#else
#	error Platform not defined.
#endif

#ifdef DEBUG
#define CHECK_GLERROR  { GLenum error = glGetError(); ASSERT( error == GL_NO_ERROR ); }
#else
#define CHECK_GLERROR {}
#endif
extern "C" {
#include "lodepng.h"
}
bool gErrorFlag = false;


// -----------------------------------------------------------
// --                     Debug logging                     --
// -----------------------------------------------------------
void DLog( const char* format, ... )
{
#ifdef DEBUG
    va_list     va;
    char                buffer[1024];

    //
    //  format and output the message..
    //
    va_start( va, format );
    vsnprintf( buffer, 1024, format, va );
    va_end( va );

#if defined(__ANDROID__)
    __android_log_write( gErrorFlag ? ANDROID_LOG_ERROR : ANDROID_LOG_INFO, "CANVAS", buffer );
#else
    printf( "LOG: %s\n", buffer );
#endif

#endif	// DEBUG
}

// -----------------------------------------------------------

// -----------------------------------------------------------
// --               Canvas implementation               --
// -----------------------------------------------------------

/*static*/
Canvas *Canvas::theCanvas = NULL;

/*static*/
Canvas *Canvas::GetCanvas()
{
    //DLog( "Canvas::GetCanvas");
    if (!theCanvas) {
        theCanvas = new Canvas();
    }
    return theCanvas;
}

//---------------------------------------------------------------

Canvas::Canvas()
{
	contextLost = false;
#ifdef WIN32
    glewInit();
#endif
    DLog( "Canvas::Canvas");
    backgroundRed = 0;
    backgroundGreen = 0;
    backgroundBlue = 0;

	orthoSet = false;
	orthoWidth = 0;
	orthoHeight = 0;
    
    lastTime = clock();
    frames = 0;
    messages = 0;
    fps = 0.0f;
    mps = 0.0f;
    bytesPS = 0.0f;
    msgLen = 0;
#ifdef USE_INDEX_BUFFER
    indexVBO = 0;
#endif
    worldColor.SetWhite();
}

/*static*/
void Canvas::ContextLost()
{
    if (theCanvas) {
        theCanvas->DoContextLost();
    }
}

/*static*/
void Canvas::Release()
{
    if (theCanvas) {
        delete theCanvas;
        theCanvas = NULL;
    }
}

Canvas::~Canvas()
{
    DLog( "Canvas::~Canvas start." );
	DoContextLost();
    DLog( "Canvas::~Canvas end." );
}

void Canvas::DoContextLost() {
    DLog( "Canvas::DoContextLost start." );
	// No need to clean up GL memory with glDeleteBuffers or glDeleteTextures.
	// It all gets blown away automatically when the context is lost.

	contextLost = true;

    int i;
    int size = m_streams.GetSize();
    for (i = size-1; i >= 0; i--) {
        Stream *stream = m_streams[i];
		m_streams.RemoveAt(i);
        if (stream) {
            delete stream;
        }
    }

	size = m_textures.GetSize();
	for (i = size-1; i >= 0; i--) {
		Texture *texture = m_textures[i];
		m_textures.RemoveAt(i);
		if (texture) {
			delete texture;
		}
	}

    DLog( "Canvas::DoContextLost end." );
}

void Canvas::SetBackgroundColor(float red, float green, float blue)
{
    backgroundRed = red;
    backgroundGreen = green;
    backgroundBlue = blue;
}

void Canvas::SetOrtho(int width, int height) {
    if (width <= 0) width = 800;
    if (height <= 0) height = 600;

    DoSetOrtho(width, height);

	orthoSet = true;
	orthoWidth = width;
	orthoHeight = height;
}

void Canvas::DoSetOrtho(int width, int height) {
    if (width <= 0) width = 800;
    if (height <= 0) height = 600;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
#if defined(__ANDROID__) || defined(__APPLE__)
    glOrthof(0, width, height, 0, -1, 1);
#else
    glOrtho(0, width, height, 0, -1, 1);
#endif
    glMatrixMode(GL_MODELVIEW);
	orthoWidth = width;
	orthoHeight = height;
}

// There is an assumption here that stride == width * 4. Works on Android, need to confirm on iOS.
void Canvas::AddTexture(int id, int glID, int width, int height)
{
    DLog( "Entering AddTexture" );
	Texture *img = new Texture (id, glID, width, height);
    if (img) {
        DLog( "Canvas::AddTexture id=%d glID=%d width=%d height=%d", id, glID, width, height );
        m_textures.Append(&img, 1);
    }
    if ( id == -1 ) {
        textStream.texture = img;
    }
    DLog( "Leaving AddTexture" );
}

bool Canvas::AddPngTexture(const unsigned char *buffer, long size, int id, unsigned int *pWidth, unsigned int *pHeight)
{
	bool success = false;
	unsigned char *textureDataRGBA = NULL;
	unsigned int error = lodepng_decode32(&textureDataRGBA, pWidth, pHeight, buffer, (size_t)size);
    if(error) {
        DLog( "Canvas::AddPngTexture Error %d: %s", error, lodepng_error_text(error));
	} else {
		GLuint glID;
		glGenTextures(1, &glID);
		glBindTexture(GL_TEXTURE_2D, glID);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		unsigned int p2Width = 2;
		while (p2Width < *pWidth) {
			p2Width *= 2;
		}

		unsigned int p2Height = 2;
		while (p2Height < *pHeight) {
			p2Height *= 2;
		}

	    if (*pWidth == p2Width && *pHeight == p2Height) {
		    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *pWidth, *pHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureDataRGBA);
	    } else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p2Width, p2Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, *pWidth, *pHeight, GL_RGBA, GL_UNSIGNED_BYTE, textureDataRGBA);
			*pWidth = p2Width;
			*pHeight = p2Height;
	    }

		AddTexture(id, glID, (int)(*pWidth), (int)(*pHeight));
		success = true;
	}

	if (textureDataRGBA) {
		free(textureDataRGBA);
	}

	return success;
}

void Canvas::RemoveTexture(int id)
{
    DLog( "Entering Canvas::RemoveTexture" );
    for( int i=0; i<m_textures.GetSize(); ++i ) {
        Texture *img = m_textures[i];
        if ( img->GetTextureID() == id ) {
            int glID = img->GetGlID();
            DLog( "Canvas::RemoveTexture id=%d glID=%d width=%d height=%d", id, glID, m_textures[i]->GetWidth(), m_textures[i]->GetHeight() );
            m_textures.RemoveAt(i);
            // Reset up any streams using this texture
            for ( int j = 0; j < m_streams.GetSize(); j++) {
                Stream *stream = m_streams[j];
                if (stream && stream->texture == img) {
                    stream->Reset();
                }
            }
            // Delete the texture off the card
            glDeleteTextures(1, (const GLuint *)(&glID));

            delete img;
            break;
        }
    }
    DLog( "Leaving Canvas::RemoveTexture" );
}

void Canvas::EnsureIndex( int nIndex )
{
#ifdef USE_INDEX_BUFFER
    ASSERT( nIndex % 6 == 0 );
    if ( m_indices.GetSize() < nIndex ) {
        m_indices.SetSize( nIndex );

        int c = 0;
        int base = 0;
        static const unsigned short offset[6] = { 0, 1, 2, 0, 3, 2 };
        for( int i=0; i<nIndex; ++i ) {
            m_indices[i] = offset[c++] + base;
            if ( c == 6) {
                c = 0;
                base += 4;
            }
        }
        if ( indexVBO == 0 ) {
            glGenBuffers( 1, &indexVBO );
        }
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexVBO );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_indices.GetSize()*sizeof(uint16_t), m_indices.GetData(), GL_DYNAMIC_DRAW );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    }
#endif
}


void Stream::VBOUpload( const DynArray<Vertex2>& vertexBuffer )
{
    if ( vboVertexID == 0 ) {
        glGenBuffers( 1, &vboVertexID );
    }

#ifdef USE_INDEX_BUFFER
    ASSERT( vertexBuffer.GetSize() % 4 == 0 );
#else
    ASSERT( vertexBuffer.GetSize() % 6 == 0 );
#endif

    if ( nVBOAllocated < vertexBuffer.GetSize() ) {
        nVBOAllocated = vertexBuffer.GetSize();
        nVertex = vertexBuffer.GetSize();
        glBindBuffer( GL_ARRAY_BUFFER, vboVertexID );
        glBufferData( GL_ARRAY_BUFFER, nVBOAllocated*sizeof(Vertex2), vertexBuffer.GetData(), GL_DYNAMIC_DRAW );
    } else {
        nVertex = vertexBuffer.GetSize();
        glBindBuffer( GL_ARRAY_BUFFER, vboVertexID );
        glBufferSubData ( GL_ARRAY_BUFFER, 0, nVertex*sizeof(Vertex2), vertexBuffer.GetData() );
    }
}


void Canvas::UpdateFrameRate()
{
    ++frames;
    if ( frames >= 20 ) {
        clock_t now = clock();
        clock_t delta = now - lastTime;
        double dSeconds = (double)delta / (double)CLOCKS_PER_SEC;
        fps = (float)( (double)frames / dSeconds );
        mps = (float)( (double)messages / dSeconds );
        bytesPS = (float)((double)msgLen / dSeconds );

        //DLog( "delta=%d dSeconds=%f fps=%f", (int)delta, dSeconds, fps );

        frames = 0;
        messages = 0;
        lastTime = now;
        msgLen = 0;
    }
}

void Canvas::RenderText( const char* format, ... )
{
    va_list     va;
    char        buffer[128];
    va_start( va, format );
    vsnprintf( buffer, 127, format, va );
    va_end( va );
    buffer[127] = 0;

    vertexBuffer.SetSize(0);
    if ( textStream.texture ) {
        int len = (int)strlen( buffer );

        for( int j=0; j<len; ++j ) {
            Vertex2 vbuf[4];

            static const float TEXT_SCALE = 1.5f;

            float x = 10.0f + TEXT_SCALE*16.0f*(float)j;
            float y = 10.0f;
            float w = 20.0f*TEXT_SCALE;
            float h = 20.0f*TEXT_SCALE;

            vbuf[0].pos.x = x;
            vbuf[0].pos.y = y;
            vbuf[1].pos.x = x+w;
            vbuf[1].pos.y = y;
            vbuf[2].pos.x = x+w;
            vbuf[2].pos.y = y+h;
            vbuf[3].pos.x = x;
            vbuf[3].pos.y = y+h;

            unsigned char c = (uint8_t)buffer[j] - 32;
            int tx = c % 16;
            int ty = c / 16;
            float u = (float)tx / 16.0f;
            float v = (float)ty / 8.0f;
            vbuf[0].tex.x = u;
            vbuf[0].tex.y = v;
            vbuf[1].tex.x = u+1.0f/16.0f;
            vbuf[1].tex.y = v;
            vbuf[2].tex.x = u+1.0f/16.0f;
            vbuf[2].tex.y = v+1.0f/8.0f;
            vbuf[3].tex.x = u;
            vbuf[3].tex.y = v+1.0f/8.0f;

            vbuf[0].color.SetWhite();
            vbuf[1].color.SetWhite();
            vbuf[2].color.SetWhite();
            vbuf[3].color.SetWhite();

            vertexBuffer.Append( vbuf, 4 );
#ifndef USE_INDEX_BUFFER
            vertexBuffer.Append( &vbuf[0], 1 );
            vertexBuffer.Append( &vbuf[2], 1 );
#endif
        }
        EnsureIndex( len*6 );
        textStream.VBOUpload( vertexBuffer );
        return;
    }
}


void Canvas::Render(const char *renderCommands, int length)
{
	// Render thread can hit this during destruction
	if (contextLost) return;

    worldColor.SetWhite();
    if (length > 0) {
        messages++;
        BuildStreams(renderCommands, length);
    }
#ifdef DEBUG
    UpdateFrameRate();
#endif
    glClearColor(backgroundRed, backgroundGreen, backgroundBlue, 1.0f);
    glClear(  GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glColor4f(1, 1, 1, 1);

    const int size = m_streams.GetSize();
    int quads=0;
    for( int i=0; i<size; ++i ) {
        // How many indices do we need? 6 indices per quad.
        int nVertex = m_streams[i]->nVertex;
        int nIndex = nVertex * 6 / 4;
        quads += nIndex * 4 / 6;
        //if ( nIndex < 600 ) nIndex = 600;
        EnsureIndex( nIndex );
    }
#ifdef DEBUG
    RenderText( "%d [%d] dc=%d kbps=%d quads=%d", (int)(fps+0.5f), (int)(mps+0.5f), size, (int)bytesPS/1024, quads );
#endif
    for ( int i = 0; i <= size; ++i) {
        Stream *stream = (i==size) ? &textStream : m_streams[i];
        if ( !stream || !stream->texture ) {
            continue;
        }
        glBindBuffer(GL_ARRAY_BUFFER, stream->vboVertexID );
#ifdef USE_INDEX_BUFFER
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
#endif
        glBindTexture( GL_TEXTURE_2D, stream->texture->GetGlID() );

        glVertexPointer(2, GL_FLOAT, sizeof(Vertex2), (const void*)(0) );               // position
        glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex2), (const void*)(sizeof(Vector2)) ); // texture
        // This actually makes a difference on some mobile devices. Changes performance from 36 to 51 FPS.
        if (stream->usesColor) {
            glEnableClientState(GL_COLOR_ARRAY);
            glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof(Vertex2), (const void*)(sizeof(Vector2)+sizeof(Vector2)) );
        }

        int nVertex = stream->nVertex;
#ifdef USE_INDEX_BUFFER
        int nIndex = nVertex * 6 / 4;
        ASSERT( nIndex <= m_indices.GetSize() );
        glDrawElements( GL_TRIANGLES, nIndex, GL_UNSIGNED_SHORT, 0 );
#else
        ASSERT( nVertex % 6 == 0 );
        glDrawArrays( GL_TRIANGLES, 0, nVertex );
#endif
        if (stream->usesColor) {
            glDisableClientState(GL_COLOR_ARRAY);
        }
    }

    //process any capture requests
    int i = 0;
    while(!m_capParams.IsEmpty()) {
        bool isError = true;
        DLog("CANVAS::Render, about to capture");
        isError = CaptureGLLayer(m_capParams[i]);

        //create callback if we have one
        AddCallback(m_capParams[i]->callbackID, m_capParams[i]->fileName, isError);
        //Callback *cb = new Callback(m_capParams[i]->callbackID, m_capParams[i]->fileName, isError);
        //m_callbacks.Append(&cb, 1);

        //delete our capture command
        m_capParams.RemoveAt(i++);
        DLog("CANVAS::Render, capture success, left in queue: %d",m_capParams.GetSize());
    }

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    CHECK_GLERROR;
}


void Canvas::BuildStreams( const char *renderCommands, int length )
{
    int size = m_streams.GetSize();
    for ( int i = 0; i < size; i++) {
        Stream *stream = m_streams[i];
        if (stream) {
            stream->Reset();
        }
    }
    
    Clip clip;
    int n = -1;
    vertexBuffer.SetSize(0);
    msgLen += length;
    const char* p = renderCommands;
    const char* end = renderCommands + length;

    while ( *p ) {
        ASSERT( p < end );
        if ( p >= end ) {
            break;
        }
        if ( IsCmd( p, "t" )) {
            // setTransform
            p ++;
            p = ParseSetTransform( p, SET_XFORM, false, transform, &transform);
        } else  if ( IsCmd( p, "f" )) {
            // transform
            p ++;
            p = ParseSetTransform( p, SET_XFORM, true, transform, &transform);
        } else  if ( IsCmd( p, "m" )) {
            // resetTransform
            p ++;
            p = ParseSetTransform( p, IDENTITY, false, transform, &transform );
        } else  if ( IsCmd( p, "k" )) {
            // scale
            p ++;
            p = ParseSetTransform( p, SCALE, true, transform, &transform );
        } else  if ( IsCmd( p, "r" )) {
            // rotate
            p ++;
            p = ParseSetTransform( p, ROTATE, true, transform, &transform );
        } else  if ( IsCmd( p, "l" )) {
            // translate
            p ++;
            p = ParseSetTransform( p, TRANSLATE, true, transform, &transform );
        } else if ( IsCmd( p, "v" )) {
            // save
            p ++;
            transformStack.Append( &transform, 1 );
        } else if ( IsCmd( p, "e" )) {
            // restore
            p ++;
            if ( transformStack.GetSize() > 0 ) {
                transform = transformStack[transformStack.GetSize()-1];
                transformStack.SetSize( transformStack.GetSize()-1 );
            }
        } else if ( IsCmd( p, "a" )) {
            // global alpha
            p++;
            float alpha = FastFloat( p );
            while ( *p && *p != ';' ) ++p;
            if ( *p == ';' ) ++p;
            worldColor.a = (int)(255.0*alpha+0.5f);
        } else if ( IsCmd( p, "d" )) {
            p++;
            // Load the clip
            p = ParseDrawImage(p, &clip );

            // Find the texture with ID == clip.textureID
            const Texture *img = NULL;
            size = m_textures.GetSize();
            for ( int j = 0; j < size; j++) {
                if ( m_textures[j]->GetTextureID() == clip.textureID) {
                    img = m_textures[j];
                    break;
                }
            }

            // Use the current stream or advance to the next if dealing with a different textureID
            // Create a new stream if necessary
            if (img) {
                // Can we continue with the current stream?
                if (    n >= 0
                        && n < m_streams.GetSize()
                        && m_streams[n]->texture == img ) {
                } else {
                    // Switching streams. Flush the current one if needed:
                    if ( n >= 0 && n < m_streams.GetSize() ) {
                        m_streams[n]->VBOUpload( vertexBuffer );
                    }

                    ++n;
                    if ( n == m_streams.GetSize() ) {
                        Stream* s = new Stream( img );
                        m_streams.Append( &s, 1 );
                    } else {
                        ASSERT( n < m_streams.GetSize() );
                        m_streams[n]->texture = img;
                    }
#ifdef DEBUG
                    Stream* stream = m_streams[n];
                    ASSERT( stream );
                    ASSERT( stream->texture );
                    ASSERT( stream->texture->GetTextureID() == clip.textureID );
#endif
                    vertexBuffer.SetSize(0);
                }
                DoPushQuad( m_streams[n], transform, clip);
            }
        } else {
            p = ParseUnknown(p);
        }
    }
    // Flush the last stream.
    if ( n >= 0 ) {
        ASSERT( n < m_streams.GetSize() );
        Stream* stream = m_streams[n];
        ASSERT( stream );
        ASSERT( stream->texture );
        ASSERT( stream->texture->GetTextureID() == clip.textureID );
        stream->VBOUpload( vertexBuffer );
    }
    //__android_log_write(ANDROID_LOG_ERROR, "Canvas::BuildStreams", "End");
}

// From the current position, past semicolon or to end
// set the GL matrix if we have all the right data
const char* Canvas::ParseSetTransform( const char *p,
                                       int parseMode,               // what to read: IDENTITY, FULL_XFORM, etc.
                                       bool concat,                 // if true, concatenate, else replace.
                                       Transform transIn,           // the current xform
                                       Transform *transOut )
{
    float tokens[6] = { 0, 0, 0, 0, 0, 0 };
    int iToken = 0;

    while ( *p && *p != ';' && iToken < 6 ) {
        tokens[iToken++] = FastFloat( p );
        while ( *p && (*p != ',' && *p != ';') ) {
            ++p;
        }
        if ( *p == ',' ) ++p;
    }

    Transform t;
    switch( parseMode ) {
    case IDENTITY:
        break;
    case SET_XFORM:
        t.a = tokens[0];
        t.b = tokens[1];
        t.c = tokens[2];
        t.d = tokens[3];
        t.tx = tokens[4];
        t.ty = tokens[5];
        break;
    case SCALE:
        t.a = tokens[0];
        t.d = tokens[1];
        break;
    case ROTATE: {
        double angle = tokens[0];

        double ca = cos( angle );
        double sa = sin( angle );

        t.a = (float)ca;
        t.b = (float)(sa);
        t.c = (float)(-sa);
        t.d = (float)ca;
    }
    break;
    case TRANSLATE:
        t.tx = tokens[0];
        t.ty = tokens[1];
        break;
    default:
        ASSERT( 0 );
        break;
    }

    if ( concat ) {
        transOut->a  = transIn.a*t.a  + transIn.b*t.c;
        transOut->b  = transIn.a*t.b  + transIn.b*t.d;
        transOut->c  = transIn.c*t.a  + transIn.d*t.c;
        transOut->d  = transIn.c*t.b  + transIn.d*t.d;
        transOut->tx = transIn.a*t.tx + transIn.b*t.ty + transIn.tx;
        transOut->ty = transIn.c*t.tx + transIn.d*t.ty + transIn.ty;
    } else {
        *transOut = t;
    }
    if ( *p == ';' ) ++p;
    return p;
}

// From the current position, past semicolon or to end
// draw the GL texture if we have all the right data
const char* Canvas::ParseDrawImage( const char* p, Clip *clipOut)
{
    float tokens[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    int iToken = 0;

    while ( *p && *p != ';' && iToken < 9 ) {
        if ( iToken == 0 ) {
            // First token is the texture ID
            clipOut->textureID = FastInt( p );
        } else {
            tokens[iToken-1] = FastFloat( p );
        }
        iToken++;
        while ( *p && (*p != ',' && *p != ';') ) {
            ++p;
        }
        if ( *p == ',' ) ++p;
    }

    clipOut->cx = tokens[0]; // cx
    clipOut->cy = tokens[1]; // cy
    clipOut->cw = tokens[2]; // cw
    clipOut->ch = tokens[3]; // ch
    clipOut->px = tokens[4]; // px
    clipOut->py = tokens[5]; // py
    clipOut->pw = tokens[6]; // pw
    clipOut->ph = tokens[7]; // ph

    if ( *p == ';' ) ++p;
    return p;
}

// From the current position, past semicolon or to end
const char* Canvas::ParseUnknown( const char* p )
{
    while( *p && *p != ';' ) {
        ++p;
    }
    if ( *p == ';' ) ++p;
    return p;
}

void Canvas::DoPushQuad (Stream *stream, const Transform &transform, const Clip &clip)
{
    ASSERT( stream );
    Quad q;

    // Vertex
    q.vertexArr[0].pos.x = floor(transform.a*clip.px            + transform.c*clip.py           + transform.tx);
    q.vertexArr[0].pos.y = floor(transform.b*clip.px            + transform.d*clip.py           + transform.ty);

    q.vertexArr[1].pos.x = floor(transform.a*(clip.px+clip.pw)  + transform.c*clip.py           + transform.tx);
    q.vertexArr[1].pos.y = floor(transform.b*(clip.px+clip.pw)  + transform.d*clip.py           + transform.ty);

    q.vertexArr[2].pos.x = floor(transform.a*(clip.px+clip.pw)  + transform.c*(clip.py+clip.ph) + transform.tx);
    q.vertexArr[2].pos.y = floor(transform.b*(clip.px+clip.pw)  + transform.d*(clip.py+clip.ph) + transform.ty);

    q.vertexArr[3].pos.x = floor(transform.a*clip.px            + transform.c*(clip.py+clip.ph) + transform.tx);
    q.vertexArr[3].pos.y = floor(transform.b*clip.px            + transform.d*(clip.py+clip.ph) + transform.ty);

    // Texture
    float width  = (float)stream->texture->GetWidth();
    float height = (float)stream->texture->GetHeight();

    q.vertexArr[0].tex.x = clip.cx           / width;
    q.vertexArr[0].tex.y = clip.cy           / height;

    q.vertexArr[1].tex.x = (clip.cx+clip.cw) / width;
    q.vertexArr[1].tex.y = clip.cy           / height;

    q.vertexArr[2].tex.x = (clip.cx+clip.cw) / width;
    q.vertexArr[2].tex.y = (clip.cy+clip.ch) / height;

    q.vertexArr[3].tex.x = clip.cx           / width;
    q.vertexArr[3].tex.y = (clip.cy+clip.ch) / height;

    q.vertexArr[0].color = worldColor;
    q.vertexArr[1].color = worldColor;
    q.vertexArr[2].color = worldColor;
    q.vertexArr[3].color = worldColor;

    if (!worldColor.isWhite()) {
        stream->usesColor = true;
    }

    // Write
    vertexBuffer.Append( q.vertexArr, Quad::kQuadArrSize );
#ifndef USE_INDEX_BUFFER
    vertexBuffer.Append( &q.vertexArr[0], 1 );
    vertexBuffer.Append( &q.vertexArr[2], 1 );
#endif
}


// Used in C++ framework.
void Canvas::OnSurfaceChanged( int width, int height )
{
    glClearColor(backgroundRed, backgroundGreen, backgroundBlue, 1.0f);
    glShadeModel(GL_SMOOTH);
#if defined(__ANDROID__) || defined(__APPLE__)
    glClearDepthf(1.0f);
#else
    glClearDepth(1.0f);
#endif
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LEQUAL);

    int viewport[4] = { 0, 0, width, height };
    // Sets the current view port to the new size.
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

	if (orthoSet) {
		DoSetOrtho (orthoWidth, orthoHeight);
	} else {
		DoSetOrtho(width, height);
	}

	glLoadIdentity();
	glClear( GL_COLOR_BUFFER_BIT );

	contextLost = false;
}

//called from JNI or Obj-C to indicate we want to readback the GL layer into a file on the next render
void Canvas::QueueCaptureGLLayer(int x, int y, int w, int h, const char * callbackID, const char * fn)
{
    CaptureParams *params = new CaptureParams(x,y,w,h,callbackID,fn);
    m_capParams.Append(&params, 1);
    DLog("Canvas.cpp::QueueCaptureGLLayer - queued");
}

//called from within render when QueueCaptureGLLayer has been called
bool Canvas::CaptureGLLayer(CaptureParams * params)
{
    //get the dimensions of the current viewport
    int results[4];
    glGetIntegerv(GL_VIEWPORT, results);

    //bounds check the parameters
    int x = (params->x < 0) ? 0 : params->x;
    int y = (params->y < 0) ? 0 : params->y;
    int width = (params->width == -1) ? results[2] : params-> width;
    int height = (params->height == -1) ? results[3] : params->height;
    if((x+width) > results[2]) {
        x = 0;
        width = results[2];
    }
    if((y+height) > results[3]) {
        y = 0;
        height = results[3];
    }
    //flip y axis to be in openGL lower left origin
    y = results[3] - y - height;

    //use glGetPixels to get the bits from the current frame buffer
    // Make the BYTE array, factor of 3 because it's RBG.
    GLubyte *pixels = new GLubyte [4 * width * height];
	if (!pixels) {
        DLog( "Canvas::CaptureGLLayer Unable to allocate buffer");
		return true;
	}
    glFinish();
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // taken from example here: http://www.codesampler.com/2010/11/02/introduction-to-opengl-es-2-0/
    // Flip and invert the PNG image since OpenGL likes to load everything
    // backwards from what is considered normal!
    int halfTheHeightInPixels = height / 2;
    int heightInPixels = height;

    // Assuming RGBA for 4 components per pixel.
    int numColorComponents = 4;

    // Assuming each color component is an unsigned char.
    int widthInChars = width * numColorComponents;

    unsigned char *top = NULL;
    unsigned char *bottom = NULL;
    unsigned char temp = 0;

    for( int h = 0; h < halfTheHeightInPixels; ++h ) {
        top = pixels + h * widthInChars;
        bottom = pixels + (heightInPixels - h - 1) * widthInChars;

        for( int w = 0; w < widthInChars; ++w ) {
            // Swap the chars around.
            temp = *top;
            *top = *bottom;
            *bottom = temp;

            ++top;
            ++bottom;
        }
    }

    //use loadpng library to write the rawbits to png
    //Encode the image
    unsigned error = lodepng_encode32_file(params->fileName, pixels, width, height);

    // Free memory
    delete pixels;

    //if there's an error, display it
    if(error) {
        DLog( "Canvas::CaptureGLLayer Error %d: %s", error, lodepng_error_text(error));
        strncpy( params->fileName, lodepng_error_text(error), CaptureParams::ALLOCATED-1 );
        params->fileName[CaptureParams::ALLOCATED-1] = 0;
        //strcpy(params->fileName, lodepng_error_text(error));
        return true;
    } else {
        DLog( "Canvas::CaptureGLLayer png written: %s",params->fileName);
        return false;
    }
}

//Get the front of the callback queue
Callback * Canvas::GetNextCallback()
{
    if(!m_callbacks.IsEmpty())
        return m_callbacks[0];
    else
        return NULL;
}

//delete the front of the callback queue
void Canvas::PopCallbacks()
{
    if(!m_callbacks.IsEmpty())
        m_callbacks.RemoveAt(0);
}

//push to the end of the callback queue
void Canvas::AddCallback(const char * callbackID, const char * result, bool isError)
{
    if(callbackID != NULL && *callbackID != '\0') {
        Callback *cb = new Callback(callbackID, result, isError);
        m_callbacks.Append(&cb, 1);
        DLog("Canvas::AddCallback - Callback created: %s, %s, %d",callbackID, result, isError);
    }
}

CaptureParams::CaptureParams()
{
    DLog("CaptureParams::CaptureParams()");
    int results[4];
    glGetIntegerv(GL_VIEWPORT, results);
    x = results[0];
    y = results[1];
    width = results[2];
    height = results[3];
    const char * defName = "screenshot.png";
    strncpy( fileName, defName, ALLOCATED-1);
    fileName[ALLOCATED-1] = 0;
}

CaptureParams::CaptureParams(int xPos, int yPos, int w, int h, const char * callback, const char * fn)
{
    DLog("CaptureParams::CaptureParams(int,int,int,int const char*, const char *)");
    x = xPos;
    y = yPos;
    width = w;
    height = h;


    strncpy( callbackID, callback, ALLOCATED-1 );
    callbackID[ALLOCATED-1]=0;
    strncpy( fileName, fn, ALLOCATED-1 );
    fileName[ALLOCATED-1] = 0;

    DLog("CaptureParams::CaptureParams(int,int,int,int const char*, const char *) - success");
}

Callback::Callback(const char * id, const char * res, bool error)
{
    strncpy( callbackID, id, ALLOCATED-1 );
    callbackID[ALLOCATED-1] = 0;

    strncpy( result, res, ALLOCATED-1 );
    result[ALLOCATED-1] = 0;
    isError = error;
}
