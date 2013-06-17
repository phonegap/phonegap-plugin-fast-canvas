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


#ifndef _Included_Canvas
#define _Included_Canvas

#define USE_INDEX_BUFFER

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

// Formatted using Artistic Style
// AStyle.exe --style=kr Canvas.h Canvas.cpp

extern bool gErrorFlag;

#ifdef DEBUG
void DLog( const char* format, ... );
#	ifdef __ANDROID__
#		define ASSERT( x ) { if (!(x)) { gErrorFlag = true; DLog( "ASSERT %s:%d %s", __FILE__, __LINE__, #x ); }}
#	else
#		include <assert.h>
#		define ASSERT( x ) { assert(x); }
#	endif
#else
#define ASSERT( x ) {}
#endif

// -----------------------------------------------------------
// --    Minimal Dynamic array template clss
// --    Useful for primitive types and pointers
// --    Memory grows via realloc, but never shrinks
// --    New entries are initialized to 0
// -----------------------------------------------------------
template <class T>
class DynArray
{
public:
    DynArray(unsigned int initialSize = (unsigned int)kInitialMemSize);
    virtual ~DynArray();

    const T&    operator[](int i) const    {
        ASSERT( i<m_size);
        ASSERT(m_size<=m_allocatedSize);
        return m_entries[i];
    }
    T&          operator[](int i)          {
        ASSERT( i<m_size);
        return m_entries[i];
    }

    int     GetSize() const {
        ASSERT(m_size<=m_allocatedSize);
        return m_size;
    }
    void        SetSize(int size);
    bool        IsEmpty() const {
        return (m_size == 0);
    }

    void        Append(const T * entries, int len);
    void        RemoveAt(int index);

    T          *GetData()           {
        return m_entries;
    }
    const T    *GetData() const     {
        return m_entries;
    }

private:
    DynArray(const DynArray & that);                // private, undefined
    DynArray   &operator = (const DynArray &that);  // private, undefined

    enum { kInitialMemSize = 8 };
    T       *m_entries;
    int m_size;
    int m_allocatedSize;
};

template <class T>
DynArray<T>::DynArray(unsigned int initialSize)
{
    m_size = m_allocatedSize = 0;
    m_entries = NULL;
    SetSize(initialSize);
    SetSize(0);
}

template <class T>
DynArray<T>::~DynArray()
{
    if (m_entries)     {
        free (m_entries);
        m_entries = NULL;
        m_size = 0;
        m_allocatedSize = 0;
    }
}

template <class T> void
DynArray<T>::Append (const T *entries, int len)
{
    if (entries && len) {
        unsigned int base = m_size;
        SetSize(m_size + len);
        memcpy (m_entries + base, entries, len * sizeof(T));
    }
}

template <class T> void
DynArray<T>::RemoveAt(int index)
{
    ASSERT(index < m_size);
    if (index < (m_size - 1)) {
        memmove(&m_entries[index], &m_entries[index + 1], (m_size - (index + 1)) * sizeof(T));
    }
    m_size--;
}

template <class T> void
DynArray<T>::SetSize(int size)
{
    ASSERT( size >= 0 );
    if (size > m_allocatedSize) {
        // Really need to guarantee power of 2 for VBOs.
        int newSize = 16;
        while ( newSize < size ) {
            newSize *= 2;
        }
        m_entries = (T *) realloc (m_entries, newSize * sizeof(T));
        ASSERT(m_entries);
        memset (m_entries + m_allocatedSize, 0, (newSize - m_allocatedSize) * sizeof(T));
        m_allocatedSize = newSize;
    }

    m_size = size;
    ASSERT( m_size < 10*1000);    // sanity check.
}

// -----------------------------------------------------------
// --    Clip utility class
// --    Used to process drawImage
// --    Populates the textureArr and vertexArr of a Quad
// -----------------------------------------------------------
struct Clip {
    float cx, cy, cw, ch, px, py, pw, ph;
    int textureID;
};

// -----------------------------------------------------------
// --    Transform utility class
// --    Used by Canvas.setTransform
// --    Populates the vertexArr of a Quad
// -----------------------------------------------------------
struct Transform {
    Transform() : a(1), b(0), c(0), d(1), tx(0), ty(0) {}
    float a, b, c, d, tx, ty;
};

// -----------------------------------------------------------
// --    Texture utility class
// --    Used by loadTexture
// -----------------------------------------------------------
class Texture
{
public:
    Texture (int textureID, int glID, int w, int h) {
        m_textureID = textureID;
        m_glID = glID;
        m_Width = w;
        m_Height = h;
    }

    int GetTextureID () const {
        return m_textureID;
    }
    int GetGlID () const {
        return m_glID;
    }
    int GetWidth () const {
        return m_Width;
    }
    int GetHeight () const {
        return m_Height;
    }

private:
    int m_textureID;
    int m_glID;
    int m_Width;
    int m_Height;
};



// -----------------------------------------------------------
// --    Basic data type for opengl
// -----------------------------------------------------------
struct Vector2 {
    float x;
    float y;
};

struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;

    void SetWhite() {
        r = g = b = a = 0xff;
    }
    bool isWhite() {
        return (r == 0xff && g == 0xff && b == 0xff && a == 0xff);
    }
};

struct Vertex2 {
    Vector2 pos;
    Vector2 tex;
    Color   color;
};

// -----------------------------------------------------------
// --    Quad utility class
// --    Passed to Stream::pushQuad
// -----------------------------------------------------------
struct Quad {
    enum { kQuadArrSize = 4 };
    Vertex2 vertexArr[kQuadArrSize];
};

// -----------------------------------------------------------
// --    Stream utility class
//
//  A stream is:
//      - a reference to an Texture
//      - a reference to vertex data on the GPU
//
// As the render message is decoded, the streams are built up.
// The render message is always decoded into the same working
// memory buffer - vertexBuffer - and then transfered
// to the VBO. No local copy is kept.
// -----------------------------------------------------------

class Stream
{
public:

    const Texture *texture; // We don't own this, we're just using it.

    Stream( const Texture* img=0 ) {
        texture = img;
        vboVertexID = 0;
        nVBOAllocated = 0;
        nVertex = 0;
        usesColor=false;
    }

    void Reset() {
        texture = NULL;
        usesColor=false;
    }

    void VBOUpload( const DynArray<Vertex2>& buffer );

    unsigned int vboVertexID;
    int  nVBOAllocated;
    int	nVertex;
    bool		usesColor;
};
// -----------------------------------------------------------
// --    CaptureParams struct
//
//  Contains the information needed to take a capture of the
//  GL context.
// -----------------------------------------------------------
struct CaptureParams {
    enum {
        ALLOCATED = 512
    };

    CaptureParams();
    CaptureParams(int x, int y, int w, int h, const char * callbackID, const char * fileName);

    int x;
    int y;
    int width;
    int height;
    char callbackID[ALLOCATED];
    char fileName[ALLOCATED];
};

// -----------------------------------------------------------
// --    Callback struct
//
//  Contains the information needed to execute a success or error
//  callback on the cordova side
// -----------------------------------------------------------
struct Callback {
    enum {
        ALLOCATED = 512
    };

    Callback(const char * id, const char * res, bool error);

    char callbackID[ALLOCATED];
    char result[ALLOCATED];
    bool isError;
};


// -----------------------------------------------------------
// --                 Canvas class                      --
// -----------------------------------------------------------

class Canvas
{
public:
    static Canvas *GetCanvas(); // Call any time you need a renderer
    static void ContextLost(); // Call on device loss (Android onPause)
    static void Release(); // Call at shutdown to free memory (calls ContextLost)

    void SetBackgroundColor(float red, float green, float blue);
    void SetOrtho(int width, int height);
    void AddTexture(int id, int glID, int width, int height);
    bool AddPngTexture(const unsigned char *buffer, long size, int id, unsigned int *pWidth, unsigned int *pHeight);
    void RemoveTexture(int id);
    void Render(const char *renderCommands, int length);
    void QueueCaptureGLLayer(int x, int y, int w, int h, const char * callbackID, const char * fn);

    //callback helper functions
    Callback * GetNextCallback(); //return front of callback queue
    void PopCallbacks(); //delete front of callback queue
    void AddCallback(const char * callbackID, const char * result, bool isError);

    // Currently in either platform on C++
    void OnSurfaceChanged( int width, int height );

private:
    Canvas(); // Called by GetCanvas()
    ~Canvas(); // Called by Release()

    void    BuildStreams(const char *renderCommands, int length);
    void	DoSetOrtho(int width, int height);
    void	DoContextLost();

    bool    IsCmd( const char* in, const char* match ) {
        return in[0] == match[0];
//        if (( in[0] == match[0] ) && ( in[1] == match[1] ) && (in[2] == ',')) {
//            return true;
//        }
//        return false;
    }

    void    EnsureIndex( int index );
    bool CaptureGLLayer(CaptureParams * params);
    enum {
        IDENTITY,           // rt
        SET_XFORM,          // st
        SCALE,              // sc
        ROTATE,             // ro
        TRANSLATE,          // tr
        NUM_PARSE_MODES
    };

    const char* ParseSetTransform( const char *renderCommands,
                                   int parseMode,               // what to read: IDENTITY, FULL_XFORM, etc.
                                   bool concat,                 // if true, concatenate, else replace.
                                   Transform transIn,           // the current xform
                                   Transform *transOut);        // where to write the new xform

    const char* ParseDrawImage( const char *renderCommands, Clip *clipOut);
    const char* ParseUnknown( const char *renderCommands );
    void    DoPushQuad( Stream* stream, const Transform &transform, const Clip &clip);
    void    RenderText( const char* format, ... );

    float   FastFloat( const char *str )    {
        return (float)atof( str );
    }
    int FastInt( const char *str )      {
        return atoi( str );
    }
    void UpdateFrameRate();

    // Members
    static Canvas *theCanvas;

    bool m_contextLost;

    float m_backgroundRed;
    float m_backgroundGreen;
    float m_backgroundBlue;

    bool m_orthoSet;
    int m_orthoWidth;
    int m_orthoHeight;

    clock_t m_lastTime;
    int     m_frames;
    int		m_messages;
    float   m_fps;
    float	m_mps;
    int     m_msgLen;
    float   m_bytesPS;
#ifdef USE_INDEX_BUFFER
    unsigned int m_indexVBO;
#endif

    Stream m_textStream;

    // For supporting world alpha, although any color works.
    Color m_worldColor;

    Transform m_transform;

    // For the save/restore behavior.
    DynArray<Transform> m_transformStack;

    // Local scratch buffer for building streams.
    DynArray<Vertex2> m_vertexBuffer;

    DynArray<Stream *> m_streams;
    DynArray<Texture *> m_textures;
    DynArray<CaptureParams *> m_capParams;
    DynArray<Callback *> m_callbacks;

#ifdef USE_INDEX_BUFFER
    // The indices are the same for every call.
    // BASCC only renders quads, so these can be re-used.
    DynArray< unsigned short > m_indices;
#endif
};
#endif
